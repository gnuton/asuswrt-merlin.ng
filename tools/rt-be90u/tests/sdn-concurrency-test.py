#!/usr/bin/env python3
"""Deterministic compiled SDN/service interleavings with actual kernel packets."""
import argparse
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import select
import signal
import shutil
import subprocess
import time

spec = importlib.util.spec_from_file_location('sdn', '/tests/sdn-refresh-test.py')
sdn = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdn)
net = sdn.net


def protocol_cli_probe(root):
    net.run('aarch64-openwrt-linux-musl-gcc', '-shared', '-fPIC', '-Wall', '-Wextra', '-Werror',
            '-I/work/release/src/router/iproute2-5.18.0/include/uapi',
            '/tests/rule-protocol-probe.c', '-o', str(root / 'harness/protocol-probe.so'))
    shutil.copy2('/firmware/usr/sbin/ip', root / 'harness/firmware-ip')
    for action, tag, kind in [('add', 200, 32), ('del', 0, 33), ('add', 215, 32), ('del', 215, 33),
                              ('add', 240, 32), ('del', 240, 33), ('add', 249, 32), ('del', 249, 33)]:
        result = net.run('chroot', str(root), '/qemu', '-E', 'LD_PRELOAD=/harness/protocol-probe.so',
                         '/harness/firmware-ip', 'rule', action, 'from', 'all', 'to', 'all',
                         'iif', 'lo', 'priority', '12210', 'prohibit', 'protocol', str(tag), check=False)
        assert result.returncode and f'PACKAGED-FRA-PROTOCOL {kind} {tag}\n' in result.stdout, result
    print('PASS packaged ARM ip serializes add/delete protocol metadata, including legacy zero; '
          'only this parser probe intercepts sendmsg, packet scenarios use real routing commands', flush=True)


def compile_driver(root):
    objects = [Path('/work/release/src/router/rc/' + name) for name in ('sdn.o', 'services.o')]
    for obj in objects:
        print(obj.name + ' sha256:', hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = root / 'harness/concurrent-exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; '
                       'file_lock; file_unlock; run_custom_script; };\n')
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
               '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--dynamic-list,' + str(exports),
               '-I/work/release/src-qca-ipq53xx/include',
               '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
               '/tests/sdn-concurrency-driver.c', *map(str, objects), '-L/firmware/usr/lib',
               '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
               '-lovpn', '-lshared', '-lnvram', '-lcrypto', '-lssl', '-ldl',
               '-o', str(root / 'harness/concurrent-driver')]
    first = net.run(*command, check=False)
    names = sorted(set(re.findall(r"undefined reference to `([^']+)'", first.stderr)))
    protected = re.compile(r'(?:.*(?:amvpn|nvram|file_lock|file_unlock).*|'
                           r'update_sdn_by_vpnc|handle_notifications|'
                           r'ovpn_set_exclusive_dns|wgc_set_exclusive_dns)')
    assert names and all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name) for name in names), first.stderr
    assert not [name for name in names if protected.fullmatch(name)], names
    definitions = []
    for directory in ('rc', 'shared', 'libovpn'):
        definitions += sorted(Path('/work/release/src/router', directory).glob('*.o'))
    symbols = net.run('aarch64-openwrt-linux-musl-readelf', '-sW', *map(str, definitions)).stdout
    functions = set(re.findall(r'^\s*\d+:\s+\w+\s+\d+\s+FUNC\s+\S+\s+\S+\s+(?!UND)\S+\s+(\w+)$', symbols, re.MULTILINE))
    # The linked production rc carries FUNC types for imports resolved by its
    # additional packaged libraries; object-only NOTYPE imports are insufficient.
    linked = net.run('aarch64-openwrt-linux-musl-readelf', '-sW', '/work/release/src/router/rc/rc').stdout
    functions.update(re.findall(r'^\s*\d+:\s+\w+\s+\d+\s+FUNC\s+\S+\s+\S+\s+\S+\s+(\w+)$', linked, re.MULTILINE))
    assert not set(names) - functions, ('Unresolved symbols lack production FUNC types', set(names) - functions)
    traps = root / 'harness/concurrent-traps.c'
    traps.write_text('extern void fixture_unexpected(const char *);\n' + ''.join(
        'void %s(void) { fixture_unexpected("%s"); }\n' % (name, name) for name in names))
    result = net.run(*command, str(traps))
    if result.stderr:
        print(result.stderr, end='', flush=True)
    print('Unrelated entry points guarded by fatal traps:', len(names), flush=True)


def install_barriers(root):
    (root / 'harness/native').mkdir()
    (root / 'tmp/var/lock').mkdir(exist_ok=True)
    (root / 'tmp/concurrent-nvram').mkdir()
    (root / 'usr/sbin/ip').rename(root / 'harness/native/ip')
    net.copy_native(root, '/bin/busybox', '/harness/timeout')
    wrapper = root / 'usr/sbin/ip'
    wrapper.write_text('''#!/bin/sh
/harness/native/ip "$@"
status=$?
if [ -n "$CONCURRENT_ACTOR" ]; then
    command="$1:$2"
    if [ "$1" = -4 ]; then command="$2:$3"; fi
    case "$command" in
    rule:add|rule:del|rule:delete|rule:replace)
        /harness/timeout 60 /bin/sh -c '
            printf "%s\\tRULE\\t%s\\t%s\\n" "$1" "$2" "$3" > /tmp/concurrent-events
            IFS= read -r ack < /tmp/concurrent-ack-"$1"
            test "$ack" = continue
        ' barrier "$CONCURRENT_ACTOR" "$status" "$*" || exit 124
        ;;
    esac
fi
exit "$status"
''')
    wrapper.chmod(0o755)


class Actors:
    def __init__(self, fixture):
        self.fixture = fixture
        self.pending = b''
        self.processes = {}
        self.acks = {}
        self.owner = None
        self.boundaries = 0
        path = fixture.root / 'tmp/concurrent-events'
        os.mkfifo(path)
        self.events = os.open(path, os.O_RDWR | os.O_NONBLOCK)

    def spawn(self, actor, *args):
        path = self.fixture.root / ('tmp/concurrent-ack-' + actor)
        os.mkfifo(path)
        self.acks[actor] = os.open(path, os.O_RDWR | os.O_NONBLOCK)
        self.processes[actor] = subprocess.Popen(
            ['chroot', str(self.fixture.root), '/qemu', '/harness/concurrent-driver', *args],
            env=dict(self.fixture.env, CONCURRENT_ACTOR=actor), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, start_new_session=True)

    def event(self):
        deadline = time.monotonic() + 30
        while b'\n' not in self.pending:
            assert time.monotonic() < deadline, 'Timed out waiting for deterministic actor event'
            if select.select([self.events], [], [], 0.1)[0]:
                self.pending += os.read(self.events, 8192)
        line, self.pending = self.pending.split(b'\n', 1)
        actor, kind, detail = line.decode().split('\t', 2)
        if kind == 'LOCK_ACQUIRED':
            assert self.owner is None, ('Actual routing locks overlapped', self.owner, actor)
            self.owner = actor
        elif kind == 'LOCK_RELEASING':
            assert self.owner == actor
            self.owner = None
        elif kind == 'RULE':
            self.boundaries += 1
        print('OBSERVE', actor, kind, detail, flush=True)
        return actor, kind, detail

    def expect(self, actor, kind):
        observed = self.event()
        assert observed[:2] == (actor, kind), observed
        return observed

    def ack(self, actor):
        os.write(self.acks[actor], b'continue\n')

    def drain(self, actors, probe=None):
        remaining = set(actors)
        deadline = time.monotonic() + 120
        while remaining:
            assert time.monotonic() < deadline, ('Actors did not finish', remaining)
            if b'\n' in self.pending or select.select([self.events], [], [], 0.1)[0]:
                actor, kind, detail = self.event()
                assert actor in remaining, ('Unexpected active actor', actor, remaining)
                if kind == 'RULE' and probe:
                    probe(actor + ' ' + detail)
                self.ack(actor)
            for actor in tuple(remaining):
                process = self.processes[actor]
                if process.poll() is not None:
                    output = process.communicate(timeout=2)[0].decode()
                    print(output, end='', flush=True)
                    assert process.returncode == 0, (actor, process.returncode, output)
                    remaining.remove(actor)

    def close(self):
        for process in self.processes.values():
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate(timeout=2)
        for fd in [self.events, *self.acks.values()]:
            os.close(fd)


def configure(fixture, **values):
    for key, value in values.items():
        path = fixture.root / 'tmp/concurrent-nvram' / key
        temporary = path.with_suffix('.new')
        temporary.write_text(str(value))
        temporary.replace(path)


def assign(fixture, index):
    configure(fixture, sdn_rl=''.join(f'<{idx}>LAN{idx}>1>{idx}>{idx}>0>{target}' + '>0' * 12 + '>WEB>0>0>0'
                                    for idx, target in enumerate((0, index))))


def refresh(fixture, index):
    result = net.run('chroot', str(fixture.root), '/qemu', '/harness/concurrent-driver',
                     'refresh', str(index), env=fixture.env)
    print(result.stdout + result.stderr, end='', flush=True)


def exercise(fixture, regression):
    failures = []
    checks = 0
    actors = Actors(fixture)

    def check(label, action):
        nonlocal checks
        checks += 1
        try:
            action()
        except AssertionError as error:
            print('FAIL', label, repr(error), flush=True)
            failures.append(label)
            if not regression:
                raise

    def clear_rules():
        for rule in json.loads(net.ip('-j', 'rule').stdout):
            if rule['priority'] not in (0, 32766, 32767):
                net.ip('rule', 'del', 'priority', str(rule['priority']))

    try:
        fixture.env.update(ipv6_service='disabled', vpnc_default_wan='0')
        fixture.sdn(1)
        fixture.rules('')
        configure(fixture, wgc1_enforce='0', vpn_client1_enforce='1')
        assign(fixture, 1)
        refresh(fixture, 1)
        for table in ('wgc1', 'ovpnc1'):
            net.ip('route', 'flush', 'table', table)
        check('old unprotected assignment can use WAN',
              lambda: fixture.packet('198.51.100.10', '203.0.113.99', 'wan0', iface='br1'))

        actors.spawn('old', 'refresh', '1')
        actors.expect('old', 'LOCK_BEFORE')
        # The outer caller selected an SDN from earlier MT-LAN data. A newer
        # assignment completes through the same compiled function first.
        assign(fixture, 6)
        actors.spawn('new', 'refresh', '6')
        actors.drain(['new'])
        check('new protected assignment blocks WAN before old refresh resumes',
              lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1'))
        actors.ack('old')
        actors.drain(['old'])
        check('older refresh cannot erase newer protected assignment',
              lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1'))
        print('AFTER stale refresh rules:', net.ip('-j', 'rule').stdout.strip(), flush=True)
        check('stale-refresh control unprotected LAN',
              lambda: fixture.packet('192.0.2.10', '203.0.113.99', 'wan0'))
        check('stale-refresh control router endpoint', lambda: sdn.router_packet(fixture))

        clear_rules()
        assign(fixture, 6)
        configure(fixture, wgc1_enforce='1', vpn_client1_enforce='1')
        fixture.sdn(6)
        fixture.rules('<1>OVPN Director>192.0.2.20>>OVPN1<1>WG Director>192.0.2.30>>WGC1'
                      '<1>WAN exception>198.51.100.11>>WAN')
        fixture.call('rules', 'ovpn')
        fixture.call('rules', 'wg')
        fixture.call('kill', 'ovpn')
        fixture.call('kill', 'wg')
        refresh(fixture, 6)

        def service_probes(phase):
            check(phase + ' assigned SDN remains guarded',
                  lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1'))
            check(phase + ' OVPN Director remains guarded',
                  lambda: fixture.packet('192.0.2.20', '203.0.113.99', None))
            check(phase + ' other WG Director remains guarded',
                  lambda: fixture.packet('192.0.2.30', '203.0.113.99', None))
            check(phase + ' WAN exception with empty VPN table',
                  lambda: fixture.packet('198.51.100.11', '203.0.113.99', 'wan0', iface='br1'))
            check(phase + ' unprotected LAN',
                  lambda: fixture.packet('192.0.2.10', '203.0.113.99', 'wan0'))
            check(phase + ' router endpoint', lambda: sdn.router_packet(fixture))

        service_probes('before concurrent service')
        actors.spawn('service', 'service')
        actors.expect('service', 'LOCK_BEFORE')
        actors.ack('service')
        actors.expect('service', 'LOCK_ACQUIRED')
        # Independently prove this is a real POSIX lock held by another process.
        with (fixture.root / 'tmp/var/lock/vpnrouting-dns.lock').open('a+b') as lock:
            try:
                fcntl.lockf(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                pass
            else:
                raise AssertionError('Service reported a lock without kernel exclusion')
        actors.spawn('waiting', 'refresh', '6')
        actors.expect('waiting', 'LOCK_BEFORE')
        actors.ack('waiting')
        actors.ack('service')
        actors.drain(['service', 'waiting'], service_probes)
        service_probes('after serialized service and SDN refresh')
        print('PASS actual service/SDN lock calls serialize; packet checks expose any helper-local gaps', flush=True)

        controls = '<1>WG Director>192.0.2.30>>WGC1<1>WAN exception>198.51.100.11>>WAN'
        narrow = '<1>OVPN Director>192.0.2.20>203.0.113.99>OVPN1' + controls
        wide = '<1>OVPN Director>192.0.2.20>>OVPN1' + controls
        fixture.rules(narrow)
        fixture.call('rules', 'ovpn')
        fixture.call('kill', 'ovpn')
        # Seed an actual pre-metadata generation, including its routing rule.
        for priority in (10210, 12210):
            for rule in json.loads(net.ip('-j', 'rule').stdout):
                if rule['priority'] == priority:
                    net.ip('rule', 'del', 'priority', str(priority))
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99',
               'priority', '10210', 'table', 'ovpnc1')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99',
               'priority', '12210', 'prohibit')
        check('narrow initial policy leaves other destination usable',
              lambda: fixture.packet('192.0.2.20', '203.0.113.98', 'wan0'))

        def policy_probes(phase, widening):
            service_probes(phase)
            if widening:
                check(phase + ' newly covered destination stays guarded',
                      lambda: fixture.packet('192.0.2.20', '203.0.113.98', None))

        fixture.rules(wide)
        actors.spawn('widen', 'service')
        actors.drain(['widen'], lambda phase: policy_probes(phase, True))
        policy_probes('after legacy-to-tagged widening', True)
        refresh(fixture, 6)
        check('SDN refresh preserves independently reconciled guard',
              lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1'))
        net.ip('route', 'add', 'default', 'via', '10.2.0.2', 'dev', 'tun11', 'onlink', 'table', 'ovpnc1')
        check('widened route forwards new destination through populated VPN table',
              lambda: fixture.packet('192.0.2.20', '203.0.113.98', 'tun11'))
        net.ip('route', 'flush', 'table', 'ovpnc1')
        fixture.rules(narrow)
        actors.spawn('narrow', 'service')
        actors.drain(['narrow'], lambda phase: policy_probes(phase, False))
        check('narrowed policy releases other destination after publication',
              lambda: fixture.packet('192.0.2.20', '203.0.113.98', 'wan0'))
        unchanged = net.ip('-j', 'rule').stdout
        before = actors.boundaries
        actors.spawn('unchanged', 'service')
        actors.drain(['unchanged'], lambda phase: policy_probes(phase, False))
        assert actors.boundaries == before and net.ip('-j', 'rule').stdout == unchanged

        # A foreign extension before a legacy zero-protocol rule must never
        # be consumed by Linux's wildcard delete while replacing that rule.
        for rule in json.loads(net.ip('-j', 'rule').stdout):
            if rule['priority'] == 12210:
                net.ip('rule', 'del', 'priority', '12210')
        net.ip('rule', 'add', 'from', 'all', 'fwmark', '0x55', 'priority', '12210', 'prohibit', 'protocol', '42')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99', 'priority', '12210', 'prohibit')
        net.run('iptables-nft', '-t', 'mangle', '-A', 'PREROUTING', '-s', '192.0.2.44', '-j', 'MARK', '--set-mark', '0x55')
        before_foreign = net.ip('-j', 'rule').stdout
        fixture.rules(wide)
        actors.spawn('foreign', 'service')
        actors.drain(['foreign'])
        assert net.ip('-j', 'rule').stdout == before_foreign, 'Ambiguous foreign rule cleanup must leave the original set intact'
        check('foreign marked rule still protects its independent client',
              lambda: fixture.packet('192.0.2.44', '203.0.113.99', None))
        check('refused cleanup retains original policy protection',
              lambda: fixture.packet('192.0.2.20', '203.0.113.99', None))
        net.ip('rule', 'del', 'from', 'all', 'fwmark', '0x55', 'priority', '12210', 'prohibit', 'protocol', '42')
        actors.spawn('retry', 'service')
        actors.drain(['retry'], lambda phase: policy_probes(phase, True))
        policy_probes('retry after foreign conflict removal', True)

        for rule in json.loads(net.ip('-j', 'rule').stdout):
            if rule['priority'] == 10210:
                net.ip('rule', 'del', 'priority', '10210')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99', 'iif', 'br0',
               'priority', '10210', 'table', 'ovpnc1')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99',
               'priority', '10210', 'table', 'ovpnc1')
        before_foreign = net.ip('-j', 'rule').stdout
        actors.spawn('foreign-route', 'service')
        actors.drain(['foreign-route'])
        assert net.ip('-j', 'rule').stdout == before_foreign, 'Simple unowned iif lookup must survive a legacy wildcard delete'
        check('refused simple-lookup cleanup retains existing source protection',
              lambda: fixture.packet('192.0.2.20', '203.0.113.99', None))
        net.ip('rule', 'del', 'from', '192.0.2.20', 'to', '203.0.113.99', 'iif', 'br0',
               'priority', '10210', 'table', 'ovpnc1')
        actors.spawn('retry-route', 'service')
        actors.drain(['retry-route'], lambda phase: policy_probes(phase, True))
        policy_probes('retry after simple-lookup conflict removal', True)
        fixture.rules(wide + '<1>Noncanonical prefix>198.51.100.20/24>203.0.113.99>OVPN1')
        actors.spawn('prefix-add', 'service')
        actors.drain(['prefix-add'], service_probes)
        assert '198.51.100.20/24' in net.ip('rule', 'show').stdout, 'Kernel must retain the noncanonical selector used for exact deletion'
        check('noncanonical prefix protects matching forwarded source',
              lambda: fixture.packet('198.51.100.25', '203.0.113.99', None))
        fixture.rules(wide)
        actors.spawn('prefix-remove', 'service')
        actors.drain(['prefix-remove'], service_probes)
        assert '198.51.100.20/24' not in net.ip('rule', 'show').stdout, 'Prefix removal must delete the original stored selector'
        check('removed noncanonical prefix releases matching forwarded source',
              lambda: fixture.packet('198.51.100.25', '203.0.113.99', 'wan0'))
        print('PASS policy wildcard widening/narrowing preserves packets, rule priorities and idempotence; '
              'ambiguous legacy cleanup preserves foreign rules and permits explicit retry; '
              'noncanonical prefixes delete by their original selectors', flush=True)

        net.ip('rule', 'del', 'from', 'all', 'iif', 'br1', 'priority', '1018', 'table', 'ovpnc1')
        net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'fwmark', '0x40',
               'priority', '1018', 'table', 'ovpnc1')
        net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'priority', '1018', 'table', 'ovpnc1')
        before_foreign = net.ip('-j', 'rule').stdout
        actors.spawn('foreign-sdn4', 'refresh-fail', '6')
        actors.drain(['foreign-sdn4'], service_probes)
        assert net.ip('-j', 'rule').stdout == before_foreign, 'SDN cleanup must retain earlier foreign marked lookup'
        service_probes('after refused IPv4 SDN cleanup')
        net.ip('rule', 'del', 'from', 'all', 'iif', 'br1', 'fwmark', '0x40',
               'priority', '1018', 'table', 'ovpnc1')
        net.ip('-6', 'rule', 'add', 'from', 'all', 'iif', 'br1', 'priority', '1018', 'table', 'ovpnc1', 'protocol', '2')
        net.ip('-6', 'rule', 'add', 'from', 'all', 'iif', 'br1', 'priority', '1018', 'table', 'ovpnc1')
        before_foreign = net.ip('-6', '-j', 'rule').stdout
        actors.spawn('foreign-sdn6', 'refresh-fail', '6')
        actors.drain(['foreign-sdn6'], service_probes)
        assert net.ip('-6', '-j', 'rule').stdout == before_foreign, 'SDN cleanup must retain explicit foreign kernel protocol'
        check('refused IPv6 SDN cleanup retains bridge protection',
              lambda: fixture.packet('2001:db8:3::10', '2001:db8:2::99', None, iface='br1'))
        check('refused IPv6 SDN cleanup preserves unrelated LAN',
              lambda: fixture.packet('2001:db8:1::10', '2001:db8:2::99', 'wan0'))
        net.ip('-6', 'rule', 'del', 'from', 'all', 'iif', 'br1', 'priority', '1018', 'table', 'ovpnc1', 'protocol', '2')
        actors.spawn('retry-sdn', 'refresh', '6')
        actors.drain(['retry-sdn'], service_probes)
        service_probes('after SDN cleanup retry')
        print('PASS actual IPv4/IPv6 SDN cleanup refuses ambiguous foreign rules, retains protection, '
              'and succeeds after explicit conflict removal', flush=True)
    finally:
        actors.close()
    if failures:
        raise SystemExit('Reproduced SDN concurrency failures: %d/%d packet checks; %d routing command boundaries; 4 scenarios' %
                         (len(failures), checks, actors.boundaries))
    assert not regression, 'Frozen image unexpectedly passed the concurrency regression'
    print('SDN concurrency fixture passed:', checks, 'packet checks;', actors.boundaries,
          'routing command boundaries; 4 scenarios', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--expect-regression', action='store_true')
    args = parser.parse_args()
    net.prepare(args.root)
    compile_driver(args.root)
    protocol_cli_probe(args.root)
    install_barriers(args.root)
    net.setup_links()
    fixture = net.Fixture(args.root)
    try:
        exercise(fixture, args.expect_regression)
    finally:
        print('Final IPv4 rules:', net.ip('rule').stdout, flush=True)
        for sock in fixture.sockets.values():
            sock.close()


if __name__ == '__main__':
    main()
