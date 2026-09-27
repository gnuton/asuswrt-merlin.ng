#!/usr/bin/env python3
"""Actual deferred migration followed by compiled SDN routing/resolver consumers."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import select
import socket
import time

spec = importlib.util.spec_from_file_location('network', '/tests/vpn-network-test.py')
net = importlib.util.module_from_spec(spec)
spec.loader.exec_module(net)
default_spec = importlib.util.spec_from_file_location('sdn_default', '/tests/sdn-default-test.py')
default = importlib.util.module_from_spec(default_spec)
default_spec.loader.exec_module(default)


def compile_driver(root, regression):
    objects = [Path('/work/release/src/router/rc') / (name + '.o')
               for name in ('vpn_migrate', 'sdn', 'vpnc_fusion', 'common')]
    for obj in objects:
        print(obj.name, hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = root / 'harness/deferred-exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; };\n')
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror', '-DTUFBE6500',
               '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--dynamic-list,' + str(exports), '-Wl,--wrap=_eval',
               '-I/work/release/src-qca-ipq53xx/include', '-I/work/release/src/router/shared',
               '-I/work/release/src/router/libovpn', '/tests/deferred-migration-driver.c',
               '/tests/deferred-migration-traps.c'] + [str(obj) for obj in objects]
    command += ['-L/firmware/usr/lib', '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
                '-lovpn', '-lshared', '-lnvram', '-o', str(root / 'harness/deferred-driver')]
    result = net.run(*command)
    if result.stderr:
        print(result.stderr, end='', flush=True)
    if not regression:
        binding = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
                   '-Wl,--dynamic-list,' + str(exports),
                   '-I/work/release/src/router/shared', '/tests/deferred-binding-test.c',
                   '-L/firmware/usr/lib', '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
                   '-lshared', '-lnvram', '-o', str(root / 'harness/deferred-binding-test')]
        net.run(*binding)
        result = net.run('chroot', str(root), '/qemu', '/harness/deferred-binding-test')
        print(result.stdout + result.stderr, end='', flush=True)


def invoke(fixture, operation, index=5):
    result = net.run('chroot', str(fixture.root), '/qemu', '/harness/deferred-driver',
                     operation, str(index), env=fixture.env)
    print(result.stdout + result.stderr, end='', flush=True)
    return result


def change_default_wan(fixture, index):
    # Production SDN records include fields following the VPN assignment.
    fixture.env['sdn_rl'] = ''.join('<' + row + '>0' * (23 - len(row.split('>')))
                                  for row in fixture.env['sdn_rl'].split('<') if row)
    old_other = fixture.env['sdn_rl'].split('<')[2:]
    result = net.run('chroot', str(fixture.root), '/qemu', '/harness/default-driver',
                     'default', str(index), env=fixture.env)
    print(result.stdout + result.stderr, end='', flush=True)
    changed = {}
    for line in result.stdout.splitlines():
        if line.startswith('NVRAM '):
            name, value = line[6:].split('=', 1)
            changed[name] = value
    assert changed.keys() == {'vpnc_default_wan', 'sdn_rl'}, changed
    assert changed['vpnc_default_wan'] == str(index), changed
    assert changed['sdn_rl'].split('<')[2:] == old_other, changed
    fixture.env.update(changed)


def reset_stock_rules():
    # Scenario isolation: keep the independent Merlin Director rule at 12010.
    for priority in (100, 999, 1000, 1015, 12220):
        while net.ip('rule', 'del', 'priority', str(priority), check=False).returncode == 0:
            pass


def router_dns_packet(fixture, address):
    for sock in fixture.sockets.values():
        while select.select([sock], [], [], 0)[0]:
            sock.recv(65535)
    payload = b'RTBE90U-DEFERRED-RESOLVER-PROBE'
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as query:
        query.sendto(payload, (address, 53))
    found = []
    until = time.monotonic() + 1
    while time.monotonic() < until and not found:
        ready, _, _ = select.select(list(fixture.sockets.values()), [], [], max(0, until - time.monotonic()))
        for sock in ready:
            frame = sock.recv(65535)
            if payload in frame:
                found.append((sock.getsockname()[0], socket.inet_ntoa(frame[30:34])))
    assert found == [('wgc5', address)], found


def exercise(fixture, regression):
    failures = []
    checks = 0

    def check(label, action):
        nonlocal checks
        checks += 1
        try:
            action()
        except AssertionError as error:
            failures.append(label)
            print('FAIL', label, repr(error), flush=True)
            if not regression:
                raise

    def packet(label, source, dest, expected, **options):
        check(label, lambda: fixture.packet(source, dest, expected, **options))

    def expect(value, detail):
        assert value, detail

    def resolver_blocked():
        conf = (fixture.root / 'etc/dnsmasq-1.conf').read_text()
        files = [line.split('=', 1)[1] for line in conf.splitlines() if line.startswith('servers-file=')]
        assert files and 'no-resolv\n' in conf, conf
        for name in files:
            content = (fixture.root / name.lstrip('/')).read_text()
            assert not any(line.startswith('server=') for line in content.splitlines()), (name, content)
        assert 'server=' not in conf, conf

    fixture.env.update(ipv6_service='disabled', vpn_client1_addr='stock-vpn.fixture.example',
                       vpn_client1_desc='Existing Merlin edit', vpn_clientx_eas='1,',
                       vpn_client1_enforce='1', vpn_client1_rgw='2', wgc5_enable='1',
                       wgc5_enforce='1', wgc5_desc='Unrelated active WG5',
                       vpnc_clientlist='<Stock OpenVPN>OpenVPN>1>>>1>5>>>0>0>Web',
                       vpnc5_dns='203.0.113.53', vpnc_pptp_options_x_list='<',
                       vpnc_dev_policy_list='', vpnc_default_wan='0')
    fixture.rules('<1>Independent WG5>192.0.2.50>>WGC5')
    fixture.sdn(5)
    invoke(fixture, 'defer')
    invoke(fixture, 'independent')
    invoke(fixture, 'refresh')
    packet('independent WG5 remains usable', '192.0.2.50', '203.0.113.99', 'wgc5')
    packet('assigned stock OVPN1 cannot select WG5 or WAN', '198.51.100.10', '203.0.113.99', None, iface='br1')
    rules = net.ip('-j', 'rule').stdout
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh')
    check('repeated deferral/refresh is stable', lambda: expect(net.ip('-j', 'rule').stdout == rules, net.ip('rule').stdout))
    invoke(fixture, 'resolver')
    check('affected SDN resolver has no upstream', resolver_blocked)
    check('independent WG5 resolver is preserved', lambda: expect(
        (fixture.root / 'tmp/resolv.vpnc5').read_text() == 'server=203.0.113.53\n', 'WG5 resolver changed'))
    # Fixed-slot DNS producer/routing is independent from a retained stock reference.
    invoke(fixture, 'dnsroute')
    check('independent WG5 router DNS remains usable', lambda: router_dns_packet(fixture, '203.0.113.53'))
    reset_stock_rules()
    fixture.env['sdn_rl'] = '<0>LAN>1>0>0>0>5<1>SDN>1>1>1>0>0'
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh')
    packet('default LAN raw stock binding is blocked', '192.0.2.10', '203.0.113.99', None)
    packet('independent WG5 survives affected default LAN', '192.0.2.50', '203.0.113.99', 'wgc5')
    for tcp in (False, True):
        packet('independent WG5 DNS survives affected default LAN ' + str(tcp),
               '192.0.2.50', '203.0.113.53', 'wgc5', port=53, tcp=tcp)
    fixture.env.update(sdn_rl='<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>0', vpnc_default_wan='5')
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh')
    packet('retained stock default-WAN binding is blocked', '192.0.2.10', '203.0.113.99', None)
    reset_stock_rules()
    fixture.env.update(vpnc5_state_t='2', wan0_state_t='2',
                       vpnc_dev_policy_list='<1>192.0.2.20>203.0.113.99>5'
                                            '<0>192.0.2.21>>5<1>192.0.2.22>>0')
    # The real default_wan service refreshes SDN routing and VPN6KS together;
    # changing only NVRAM and invoking the separate policy service skips it.
    change_default_wan(fixture, 0)
    invoke(fixture, 'defer')
    invoke(fixture, 'policies')
    packet('stock client target is blocked', '192.0.2.20', '203.0.113.99', None)
    packet('original destination narrowing is retained', '192.0.2.20', '203.0.113.100', 'wan0')
    packet('disabled stock policy does not block', '192.0.2.21', '203.0.113.99', 'wan0')
    packet('explicit WAN policy remains usable', '192.0.2.22', '203.0.113.99', 'wan0')
    invoke(fixture, 'nat')
    nat = (fixture.root / 'tmp/deferred-nat.rules').read_text()
    check('stock DNS cannot select unrelated WG5 resolver', lambda: expect('--to-destination 203.0.113.53' not in nat, nat))
    net.run('iptables-nft-restore', input=nat)
    for tcp in (False, True):
        packet('affected client router DNS blocked ' + str(tcp), '192.0.2.20', '192.0.2.1', None, port=53, tcp=tcp)
        packet('affected client external DNS blocked ' + str(tcp), '192.0.2.20', '203.0.113.53', None, port=53, tcp=tcp)
        packet('unrelated client external DNS usable ' + str(tcp), '192.0.2.21', '203.0.113.53', 'wan0', port=53, tcp=tcp)
    reset_stock_rules()
    fixture.env['vpnc_dev_policy_list'] = '<1>>203.0.113.99>5'
    invoke(fixture, 'defer')
    invoke(fixture, 'policies')
    packet('destination-only VPN target blocked', '192.0.2.20', '203.0.113.99', None)
    packet('destination-only unrelated control', '192.0.2.20', '203.0.113.100', 'wan0')
    fixture.env['vpnc_dev_policy_list'] = '<1>2001:db8:1::10>>5'
    invoke(fixture, 'defer')
    invoke(fixture, 'policies')
    packet('valid IPv6 source blocked', '2001:db8:1::10', '2001:db8:ffff::99', None)
    packet('other IPv6 source usable', '2001:db8:1::11', '2001:db8:ffff::99', 'wan0')
    net.ip('-6', 'rule', 'add', 'iif', 'br0', 'table', '5', 'priority', '999')
    packet('stale IPv6 SDN lookup seeded', '2001:db8:1::10', '2001:db8:ffff::99', 'wgc5')
    invoke(fixture, 'policies')
    packet('deferred refresh retires earlier stale IPv6 SDN lookup', '2001:db8:1::10', '2001:db8:ffff::99', None)
    invoke(fixture, 'refresh', 0)
    packet('unassigned SDN refresh preserves IPv6 device guard', '2001:db8:1::10', '2001:db8:ffff::99', None)
    fixture.env['vpnc_dev_policy_list'] = '<1>198.51.100.12/25>>5'
    # The frozen parser truncates this to a still-valid /2, which previously
    # selected WG5 for unrelated LAN clients. Seed that exact persisted rule.
    parsed = invoke(fixture, 'parse')
    assert 'from=198.51.100.12/2 to=' in parsed.stdout, parsed.stdout
    net.ip('rule', 'add', 'from', '198.51.100.12/2', 'table', '5', 'priority', '100')
    packet('legacy broad lookup seeded', '192.0.2.10', '203.0.113.99', 'wgc5')
    invoke(fixture, 'policies')
    packet('full intended CIDR remains blocked', '198.51.100.12', '203.0.113.99', None, iface='br1')
    packet('outside intended CIDR recovers', '198.51.100.200', '203.0.113.99', 'wan0', iface='br1')
    packet('unrelated LAN recovers after broad legacy cleanup', '192.0.2.10', '203.0.113.99', 'wan0')
    packet('independent WG5 survives broad legacy cleanup', '192.0.2.50', '203.0.113.99', 'wgc5')
    reset_stock_rules()
    fixture.env.update(vpnc_dev_policy_list='',
                       sdn_rl='<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>bad')
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh', 0)
    packet('malformed guest target blocks known bridge', '198.51.100.10', '203.0.113.99', None, iface='br1')
    packet('malformed guest target preserves unrelated LAN', '192.0.2.10', '203.0.113.99', 'wan0')
    invoke(fixture, 'resolver')
    check('malformed guest resolver has no upstream', resolver_blocked)
    fixture.env.update(sdn_rl='<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>0', vpnc_default_wan='bad')
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh', 0)
    packet('malformed default target blocks known LAN', '192.0.2.10', '203.0.113.99', None)
    # An unparseable default now quarantines its whole known LAN until fixed,
    # including independent VPN clients on that same affected network.
    packet('malformed default quarantines WG5 client on affected LAN', '192.0.2.50', '203.0.113.99', None)
    # Clear prior owned quarantine before testing a first refresh without SDN
    # rows, so existing main-LAN guards cannot make this control pass.
    if not regression:
        fixture.env['qca_merlin_vpn_migrated'] = '1'
        invoke(fixture, 'quarantine')
        del fixture.env['qca_merlin_vpn_migrated']
    reset_stock_rules()
    fixture.env.update(sdn_rl='', vpnc_default_wan='5')
    invoke(fixture, 'defer')
    if not regression:
        invoke(fixture, 'quarantine')
    packet('missing SDN rows still protect known default LAN', '192.0.2.10', '203.0.113.99', None)
    packet('missing SDN rows preserve independent WG5', '192.0.2.50', '203.0.113.99', 'wgc5')
    fixture.sdn(5)
    fixture.env.update(vpn_client1_desc='Stock OpenVPN', vpnc_dev_policy_list='', vpnc_default_wan='0',
                       vpnc_clientlist='<Provider>CyberGhost>1>>>0>8>>>0>0>Web'
                                       '<Stock OpenVPN>OpenVPN>1>>>1>5>>>0>0>Web',
                       vpnc_pptp_options_x_list='<<')
    invoke(fixture, 'defer')
    invoke(fixture, 'refresh')
    packet('provider collision keeps stock SDN blocked', '198.51.100.10', '203.0.113.99', None, iface='br1')
    packet('provider collision preserves independent WG5', '192.0.2.50', '203.0.113.99', 'wgc5')
    if not regression:
        # Resolve the synthetic conflict while retaining the same independent
        # WG5 policy in the migration plan; only its description is normalized.
        fixture.rules('<1>Fusion rule 1>192.0.2.50>>WGC5')
        fixture.env.update(vpnc_clientlist='<Stock OpenVPN>OpenVPN>1>>>1>5>>>0>0>Web'
                                           '<Unrelated active WG5>WireGuard>5>>>1>8>>>0>0>Web',
                           vpnc_dev_policy_list='<1>192.0.2.50>>8', wgc5_priv='fixture-private',
                           wgc5_ppub='fixture-peer', wgc5_addr='10.5.0.1/24')
        invoke(fixture, 'recover')
        packet('successful migration restores intended OVPN1', '198.51.100.10', '203.0.113.99', 'tun11', iface='br1')
        packet('successful migration preserves independent WG5', '192.0.2.50', '203.0.113.99', 'wgc5')
        packet('captured stock callback cannot recreate wrong WG5 route after migration', '192.0.2.20', '203.0.113.99', 'wan0')
        conf = (fixture.root / 'etc/dnsmasq-1.conf').read_text()
        check('successful migration restores intended OVPN1 DNS', lambda: expect('servers-file=/tmp/resolv.vpnc6\n' in conf, conf))
        check('successful migration removed only quarantine', lambda: expect(
            all(rule['priority'] != 12225 for rule in json.loads(net.ip('-j', 'rule').stdout)), net.ip('rule').stdout))
        # Retire the recovered SDN assignment through its real service before
        # the independently seeded firewall scenarios reuse these interfaces.
        fixture.sdn(0)
        fixture.env.update(qca_merlin_vpn_migrated='1', vpnc_clientlist='',
                           vpnc_dev_policy_list='', vpnc_default_wan='0')
        invoke(fixture, 'refresh', 0)
        packet('retired recovered SDN returns to WAN', '198.51.100.200', '203.0.113.99', 'wan0', iface='br1')
        packet('retiring recovered SDN preserves independent WG5', '192.0.2.50', '203.0.113.99', 'wgc5')
    if failures:
        raise SystemExit('Reproduced deferred migration failures: %d/%d checks' % (len(failures), checks))
    assert not regression, 'Frozen image unexpectedly passed deferred migration regression'
    print('Deferred migration routing/DNS fixture passed:', checks, 'checks', flush=True)
    firewall_spec = importlib.util.spec_from_file_location('deferred_firewall', '/tests/deferred-firewall-test.py')
    firewall = importlib.util.module_from_spec(firewall_spec)
    firewall_spec.loader.exec_module(firewall)
    firewall.exercise(fixture)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--expect-regression', action='store_true')
    args = parser.parse_args()
    net.prepare(args.root)
    net.copy_native(args.root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/iptables-restore')
    (args.root / 'tmp/var/lock').mkdir(exist_ok=True)
    compile_driver(args.root, args.expect_regression)
    default.compile_driver(args.root)
    net.setup_links()
    net.ip('link', 'add', 'wgc5', 'type', 'dummy')
    net.ip('addr', 'add', '10.5.0.1/24', 'dev', 'wgc5')
    net.ip('link', 'set', 'wgc5', 'up')
    net.ip('neigh', 'add', '10.5.0.2', 'lladdr', '02:00:00:00:00:05', 'dev', 'wgc5')
    net.ip('route', 'add', 'default', 'via', '10.5.0.2', 'dev', 'wgc5', 'onlink', 'table', 'wgc5')
    net.ip('-6', 'addr', 'add', '2001:db8:5::1/64', 'dev', 'wgc5', 'nodad')
    net.ip('-6', 'neigh', 'add', '2001:db8:5::2', 'lladdr', '02:00:00:00:00:05', 'dev', 'wgc5')
    net.ip('-6', 'route', 'add', 'default', 'via', '2001:db8:5::2', 'dev', 'wgc5', 'table', 'wgc5')
    fixture = net.Fixture(args.root)
    # Normal WAN resolver state makes an unsafe SDN fallback observable.
    (args.root / 'tmp/resolv.dnsmasq').write_text('server=203.0.113.54\n')
    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    sock.bind(('wgc5', 0)); fixture.sockets['wgc5'] = sock
    try:
        exercise(fixture, args.expect_regression)
    finally:
        print('Final IPv4 rules:', net.ip('rule').stdout, flush=True)
        for sock in fixture.sockets.values():
            sock.close()


if __name__ == '__main__':
    main()
