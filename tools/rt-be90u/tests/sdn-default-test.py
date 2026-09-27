#!/usr/bin/env python3
"""Default-LAN and multiple-SDN transitions through compiled rc entry points."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import select
import socket
import struct
import time

spec = importlib.util.spec_from_file_location('sdn_refresh', '/tests/sdn-refresh-test.py')
sdn = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdn)
net = sdn.net


def compile_driver(root):
    objects = [Path('/work/release/src/router/rc') / (name + '.o') for name in ('sdn', 'vpnc_fusion', 'common')]
    for obj in objects:
        print(obj.name + ' sha256:', hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = root / 'harness/default-exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; };\n')
    traps = root / 'harness/default-traps.c'
    traps.write_text('extern void fixture_unexpected(const char *);\n' + ''.join(
        f'void {name}(void) {{ fixture_unexpected("{name}"); }}\n' for name in (
        'get_dns_filter',
        'handle_NwServiceFilter_jump_rule', 'handle_SDN_internal_access', 'handle_URLFilter_jump_rule',
        'mkdir_if_none', 'update_ipsec_server_by_sdn',
        'update_wgs_by_sdn', 'write_NwServiceFilter_SDN', 'write_URLFilter_SDN')))
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
               '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--dynamic-list,' + str(exports), '-I/work/release/src-qca-ipq53xx/include',
               '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
               '-I/work/release/src/router/rc', '/tests/sdn-default-driver.c', str(traps)] + [str(obj) for obj in objects] + [
               '-L/firmware/usr/lib', '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
               '-lovpn', '-lshared', '-lnvram', '-o', str(root / 'harness/default-driver')]
    result = net.run(*command, check=False)
    if result.returncode:
        raise RuntimeError(result.stderr)



class Fixture(net.Fixture):
    def __init__(self, root):
        super().__init__(root)
        self.checks = 0
        self.boundaries = 0
        self.failures = []
        self.env.update(ipv6_service='disabled', lan_ipaddr='192.0.2.1',
                        lan_netmask='255.255.255.0', vpn_client1_if='tun',
                        vpnc_default_wan='0', vpnc_clientlist='')
        self.assign((0, 0, 0))

    def assign(self, targets):
        self.env.update(vlan_rl='<1>10>0<2>20>0',
            subnet_rl='<1>br1>198.51.100.1>255.255.255.0>1>198.51.100.10>198.51.100.200>86400>>>>0>0'
                      '<2>br2>198.18.0.1>255.255.255.0>1>198.18.0.10>198.18.0.200>86400>>>>0>0',
            sdn_rl=''.join(f'<{idx}>LAN{idx}>1>{idx}>{idx}>0>{target}' + '>0' * 12 + '>WEB>0>0>0'
                           for idx, target in enumerate(targets)))

    def invoke(self, operation, index, observe=None):
        command = ['chroot', str(self.root), '/qemu', '/harness/default-driver', operation, str(index)]
        old_rows = self.env['sdn_rl'].split('<')[2:]
        if observe is None:
            result = net.run(*command, env=self.env)
            output = result.stdout + result.stderr
        else:
            events = os.open(self.root / 'tmp/sdn-command-events', os.O_RDWR | os.O_NONBLOCK)
            acks = os.open(self.root / 'tmp/sdn-command-acks', os.O_RDWR | os.O_NONBLOCK)
            process = subprocess.Popen(command, env=dict(self.env, SDN_COMMAND_BARRIER='1'),
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True)
            pending = b''
            count = 0
            deadline = time.monotonic() + 240
            try:
                while process.poll() is None:
                    assert time.monotonic() < deadline, 'Default transition timed out'
                    if not select.select([events], [], [], 0.1)[0]:
                        continue
                    pending += os.read(events, 8192)
                    while b'\n' in pending:
                        event, pending = pending.split(b'\n', 1)
                        count += 1
                        self.boundaries += 1
                        try:
                            observe(count, event.decode())
                        finally:
                            os.write(acks, b'continue\n')
                output = process.communicate(timeout=2)[0].decode()
                assert process.returncode == 0, (process.returncode, output)
                assert count, 'No actual IPv4 mutations observed'
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.communicate(timeout=2)
                os.close(events)
                os.close(acks)
        print(output, end='', flush=True)
        for line in output.splitlines():
            if line.startswith('NVRAM '):
                key, value = line[6:].split('=', 1)
                self.env[key] = value
        if operation == 'default':
            self.check(f'default {index} preserves other SDN records', self.env['sdn_rl'].split('<')[2:] == old_rows)

    def check(self, label, condition):
        self.checks += 1
        if not condition:
            self.failures.append(label)
        print(('PASS ' if condition else 'FAIL ') + label, flush=True)

    def probe(self, label, interface, expected, source=None, destination='203.0.113.99'):
        """Inject IPv4 UDP into each real LAN peer and capture actual egress."""
        source = source or {'br0': '192.0.2.10', 'br1': '198.51.100.10', 'br2': '198.18.0.10'}[interface]
        for sock in self.sockets.values():
            while select.select([sock], [], [], 0)[0]:
                sock.recv(65535)
        self.sequence += 1
        payload = f'DEFAULT-SDN-{self.sequence}'.encode()
        src, dst = socket.inet_aton(source), socket.inet_aton(destination)
        body = struct.pack('!HHHH', 30000 + self.sequence, 12345, 8 + len(payload), 0) + payload
        pseudo = src + dst + struct.pack('!BBH', 0, 17, len(body))
        body = body[:6] + struct.pack('!H', net.checksum(pseudo + body) or 65535) + body[8:]
        header = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 20 + len(body), self.sequence, 0, 64, 17, 0, src, dst)
        header = header[:10] + struct.pack('!H', net.checksum(header)) + header[12:]
        mac = bytes.fromhex(Path('/sys/class/net/' + interface + '/address').read_text().strip().replace(':', ''))
        with socket.socket(socket.AF_PACKET, socket.SOCK_RAW) as sender:
            sender.bind(({'br0': 'lanpeer', 'br1': 'sdnpeer', 'br2': 'sdnpeer2'}[interface], 0))
            sender.send(mac + b'\x02\x00\x00\x00\x00\x10\x08\x00' + header + body)
        found = []
        deadline = time.monotonic() + (0.3 if expected is None else 1)
        while time.monotonic() < deadline:
            ready, _, _ = select.select(list(self.sockets.values()), [], [], max(0, deadline - time.monotonic()))
            for sock in ready:
                packet = sock.recv(65535)
                if payload in packet and packet[12:14] == b'\x08\x00' and packet[23] == 17:
                    found.append(sock.getsockname()[0])
            if found:
                break
        self.check(f'{label}: {interface} -> {expected}, observed {found}',
                   found in [([] if value is None else [value]) for value in (expected if isinstance(expected, tuple) else (expected,))])

    def reset(self):
        net.ip('rule', 'flush')
        if not any(rule['priority'] == 0 for rule in json.loads(net.ip('-j', 'rule').stdout)):
            net.ip('rule', 'add', 'priority', '0', 'lookup', 'local')
        net.ip('rule', 'add', 'priority', '32766', 'lookup', 'main')
        net.ip('rule', 'add', 'priority', '32767', 'lookup', 'default')
        for table, iface, address in [('ovpnc1', 'tun11', '10.2.0.2'), ('wgc1', 'wgc1', '10.3.0.2')]:
            net.ip('route', 'flush', 'table', table)
            net.ip('route', 'add', 'default', 'via', address, 'dev', iface, 'onlink', 'table', table)
        self.assign((0, 0, 0))
        self.env['vpnc_default_wan'] = '0'
        self.rules('')


def exercise(fixture):
    for index, proto, table, tunnel in [(1, 'wg', 'wgc1', 'wgc1'), (6, 'ovpn', 'ovpnc1', 'tun11')]:
        fixture.reset()
        fixture.assign((index, 0, 0))
        fixture.invoke('wan', 0)
        fixture.probe(f'{proto} direct SDN0 populated tunnel', 'br0', tunnel)
        fixture.probe(f'{proto} unassigned guest', 'br1', 'wan0')
        net.ip('route', 'flush', 'table', table)
        fixture.probe(f'{proto} direct SDN0 empty tunnel', 'br0', None)
        fixture.invoke('wan', 0)
        fixture.probe(f'{proto} direct SDN0 repeated refresh', 'br0', None)
        fixture.assign((0, 0, 0))
        fixture.invoke('wan', 0)
        fixture.probe(f'{proto} direct SDN0 assignment removed', 'br0', 'wan0')
        fixture.check(f'{proto} direct SDN0 stale guard removed',
                      not any(r.get('iif') == 'br0' and r['priority'] == 12220
                              for r in json.loads(net.ip('-j', 'rule').stdout)))

        fixture.env['vpnc_default_wan'] = str(index)
        fixture.invoke('wan', 0)
        fixture.probe(f'{proto} legacy default field empty tunnel', 'br0', None)
        fixture.call('kill', proto)
        fixture.probe(f'{proto} legacy default field inferred guard', 'br0', None)
        fixture.env['vpnc_default_wan'] = '0'
        fixture.invoke('wan', 0)
        fixture.probe(f'{proto} legacy default field removed', 'br0', 'wan0')

    for index, proto, table, tunnel in [(1, 'wg', 'wgc1', 'wgc1'), (6, 'ovpn', 'ovpnc1', 'tun11')]:
        fixture.reset()
        fixture.assign((0, index, index))
        fixture.invoke('wan', 255)
        fixture.call('kill', proto)
        fixture.probe(f'{proto} shared-client first guest populated tunnel', 'br1', tunnel)
        fixture.probe(f'{proto} shared-client second guest populated tunnel', 'br2', tunnel)
        net.ip('route', 'flush', 'table', table)
        fixture.assign((0, 0, index))
        fixture.invoke('wan', 1)
        fixture.call('kill', proto)
        fixture.probe(f'{proto} shared-client removed guest released', 'br1', 'wan0')
        fixture.probe(f'{proto} shared-client remaining guest protected', 'br2', None)

    fixture.reset()
    fixture.assign((0, 1, 6))
    fixture.invoke('wan', 255)
    fixture.probe('multiple SDNs WG populated tunnel', 'br1', 'wgc1')
    fixture.probe('multiple SDNs OVPN populated tunnel', 'br2', 'tun11')
    fixture.rules('<1>Director host>192.0.2.50>>OVPN1<1>WAN exception>198.51.100.11>>WAN')
    fixture.call('rules', 'ovpn')
    fixture.call('kill', 'ovpn')
    fixture.call('rules', 'wg')
    fixture.call('kill', 'wg')
    net.ip('rule', 'add', 'priority', '900', 'from', '198.18.0.88', 'iif', 'br2', 'lookup', 'main')
    for table in ('wgc1', 'ovpnc1'):
        net.ip('route', 'flush', 'table', table)
    for repeat in range(2):
        fixture.invoke('wan', 255)
        fixture.probe(f'multiple SDNs repeat {repeat} WG protection', 'br1', None)
        fixture.probe(f'multiple SDNs repeat {repeat} OVPN protection', 'br2', None)
        fixture.probe(f'multiple SDNs repeat {repeat} Director host protection', 'br0', None, '192.0.2.50')
        fixture.probe(f'multiple SDNs repeat {repeat} WAN exception', 'br1', 'wan0', '198.51.100.11')
        fixture.probe(f'multiple SDNs repeat {repeat} custom policy', 'br2', 'wan0', '198.18.0.88')
        sdn.router_packet(fixture)
    fixture.assign((0, 0, 6))
    fixture.invoke('wan', 1)
    fixture.probe('guest assignment removed', 'br1', 'wan0')
    fixture.probe('other guest protection retained', 'br2', None)
    fixture.probe('Director host protection retained', 'br0', None, '192.0.2.50')
    fixture.probe('custom other guest policy retained', 'br2', 'wan0', '198.18.0.88')
    fixture.check('removed guest owned guard absent',
                  not any(r.get('iif') == 'br1' and r['priority'] == 12220
                          for r in json.loads(net.ip('-j', 'rule').stdout)))

    fixture.reset()
    fixture.assign((0, 1, 6))
    fixture.invoke('wan', 255)
    for index, tunnel in [(1, 'wgc1'), (6, 'tun11'), (1, 'wgc1')]:
        fixture.invoke('default', index)
        fixture.probe(f'actual default switch to {index}', 'br0', tunnel)
        fixture.probe(f'actual default switch {index} preserves guest WG', 'br1', 'wgc1')
        fixture.probe(f'actual default switch {index} preserves guest OVPN', 'br2', 'tun11')
    net.ip('route', 'flush', 'table', 'wgc1')
    fixture.probe('actual default WG empty tunnel', 'br0', None)
    fixture.invoke('default', 0)
    fixture.probe('actual default switch to WAN releases stale protection', 'br0', 'wan0')
    fixture.probe('actual default switch to WAN preserves guest WG protection', 'br1', None)
    fixture.probe('actual default switch to WAN preserves guest OVPN', 'br2', 'tun11')
    fixture.check('actual default switch to WAN removes owned main-LAN guard',
                  not any(r.get('iif') == 'br0' and r['priority'] == 12220
                          for r in json.loads(net.ip('-j', 'rule').stdout)))

    # Start with both guest VPNs and a separate Director host protected. All
    # routing commands execute before the barrier allows a packet observation.
    fixture.reset()
    fixture.assign((0, 1, 6))
    fixture.invoke('wan', 255)
    fixture.rules('<1>Director host>198.18.0.50>>OVPN1<1>WAN exception>198.51.100.11>>WAN')
    for proto in ('ovpn', 'wg'):
        fixture.call('rules', proto)
        fixture.call('kill', proto)
    for table in ('wgc1', 'ovpnc1'):
        net.ip('route', 'flush', 'table', table)
    for index in (1, 6, 0):
        released = False
        def observe(number, command):
            nonlocal released
            phase = f'default {index} mutation {number}'
            print('OBSERVE', phase, command, flush=True)
            if index == 0:
                # Deliberate removal may retain protection until lookup cleanup
                # completes; after release, later callbacks must not block again.
                rules = json.loads(net.ip('-j', 'rule').stdout)
                released |= not any(r.get('iif') == 'br0' and r['priority'] == 12220 for r in rules)
            fixture.probe(phase + ' main LAN', 'br0', ('wan0' if released else (None, 'wan0')) if index == 0 else None)
            fixture.probe(phase + ' WG guest', 'br1', None)
            fixture.probe(phase + ' OVPN guest', 'br2', None)
            fixture.probe(phase + ' WAN exception', 'br1', 'wan0', '198.51.100.11')
            sdn.router_packet(fixture)
        fixture.invoke('default', index, observe)
        fixture.probe(f'default {index} completed empty-table transition', 'br0', 'wan0' if index == 0 else None)


def install_barrier(root):
    sdn.install_barrier(root)
    # Include explicit IPv4 family commands emitted by actual common.o. IPv6
    # is disabled in this fixture and is outside these IPv4 observations.
    wrapper = root / 'usr/sbin/ip'
    wrapper.write_text("""#!/bin/sh
/harness/native/ip "$@"
status=$?
command="$*"
if [ "$1" = -4 ]; then shift; fi
if [ "$SDN_COMMAND_BARRIER" = 1 ]; then
    case "$1:$2" in
    rule:add|rule:del|rule:delete|rule:replace)
        /harness/timeout 15 /bin/sh -c '
            printf "%s\\t%s\\n" "$1" "$2" > /tmp/sdn-command-events
            IFS= read -r ack < /tmp/sdn-command-acks
            test "$ack" = continue
        ' barrier "$status" "$command" || exit 124
        ;;
    esac
fi
exit "$status"
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    args = parser.parse_args()
    net.prepare(args.root)
    (args.root / 'tmp/var/lock').mkdir(exist_ok=True)
    compile_driver(args.root)
    install_barrier(args.root)
    net.setup_links()
    net.ip('link', 'add', 'br2', 'type', 'veth', 'peer', 'name', 'sdnpeer2')
    net.ip('addr', 'add', '198.18.0.1/24', 'dev', 'br2')
    for iface in ('br2', 'sdnpeer2'):
        net.ip('link', 'set', iface, 'up')
        assert Path('/proc/sys/net/ipv4/conf/' + iface + '/rp_filter').read_text().strip() == '0'
    fixture = Fixture(args.root)
    try:
        exercise(fixture)
        print(f'Default SDN summary: {fixture.checks} checks, {fixture.boundaries} mutation boundaries, {len(fixture.failures)} failures', flush=True)
        assert not fixture.failures, fixture.failures
    finally:
        print('Final IPv4 rules:', net.ip('rule').stdout, flush=True)
        for sock in fixture.sockets.values():
            sock.close()


if __name__ == '__main__':
    main()
