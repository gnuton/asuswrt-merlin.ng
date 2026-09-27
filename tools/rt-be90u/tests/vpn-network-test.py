#!/usr/bin/env python3
"""Real kernel routing/NAT with the final ARM libovpn, synthetic NVRAM and links.

Only run in the dedicated network-disabled Docker container. Native iproute2 and
iptables-nft replace the firmware utilities because QEMU cannot translate all
their netlink/ioctl calls on the host kernel. No command-execution stubs or binfmt.
"""
import json
import hashlib
import argparse
import os
from pathlib import Path
import re
import select
import shutil
import socket
import struct
import subprocess
import time


def run(*args, check=True, **kwargs):
    result = subprocess.run(args, text=True, capture_output=True, timeout=30, **kwargs)
    if check and result.returncode:
        raise RuntimeError((args, result.returncode, result.stdout, result.stderr))
    return result


def ip(*args, **kwargs):
    return run('ip', *args, **kwargs)


def copy_native(root, source, dest=None):
    source = Path(source)
    target = root / (dest or str(source)).lstrip('/')
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.is_symlink():
        target.unlink()
    shutil.copy2(source, target)
    result = run('ldd', str(source), check=False)
    for dep in re.findall(r'(/[^\s()]+)', result.stdout):
        target = root / dep.lstrip('/')
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(dep, target)


def prepare(root):
    if not Path('/.dockerenv').exists():
        raise SystemExit('Dedicated Docker container required')
    if {link['ifname'] for link in json.loads(ip('-j', 'link').stdout)} != {'lo'}:
        raise SystemExit('Refusing existing network interfaces: use --network none')
    if Path('/proc/sys/net/ipv4/ip_forward').read_text().strip() != '1':
        raise SystemExit('Container IPv4 forwarding must be enabled')
    if Path('/proc/sys/net/ipv6/conf/all/forwarding').read_text().strip() != '1':
        raise SystemExit('Container IPv6 forwarding must be enabled')
    print('Library SHA-256:', hashlib.sha256(Path('/firmware/usr/lib/libovpn.so').read_bytes()).hexdigest(), flush=True)
    print('Host kernel:', os.uname().release, ';', run('ip', '-Version').stdout.strip(),
          ';', run('iptables-nft', '--version').stdout.strip(), flush=True)
    shutil.copytree('/firmware', root, symlinks=True)
    # Production init creates /var/lock before routing services acquire locks.
    for name in ('tmp/etc', 'tmp/var/run', 'tmp/var/lock', 'jffs/openvpn', 'dev', 'harness'):
        (root / name).mkdir(parents=True, exist_ok=True)
    shutil.copytree('/firmware/rom/etc', root / 'tmp/etc', symlinks=True, dirs_exist_ok=True)
    # A native shell can launch native tools from QEMU without a global binfmt handler.
    copy_native(root, '/bin/dash', '/bin/sh')
    copy_native(root, '/bin/busybox', '/bin/grep')
    copy_native(root, '/sbin/ip', '/usr/sbin/ip')
    copy_native(root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/iptables')
    copy_native(root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/iptables-restore')
    copy_native(root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/ip6tables')
    copy_native(root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/ip6tables-restore')
    shutil.copytree('/usr/lib/x86_64-linux-gnu/xtables', root / 'usr/lib/x86_64-linux-gnu/xtables')
    copy_native(root, '/usr/bin/qemu-aarch64-static', '/qemu')
    # tmpfs is nodev. These are disposable regular files, never host devices.
    (root / 'dev/null').touch()
    run('/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc',
        '-Wall', '-Wextra', '-Werror', '-Wl,-E',
        '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
        '/tests/vpn-network-driver.c', '-L/firmware/usr/lib',
        '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib', '-lovpn',
        '-o', str(root / 'harness/driver'))
    # iproute2 needs the same table names outside the firmware chroot for assertions.
    Path('/etc/iproute2/rt_tables').write_text((root / 'etc/iproute2/rt_tables').read_text())


def setup_links():
    for bridge, peer, subnet in [('br0', 'lanpeer', '192.0.2'), ('br1', 'sdnpeer', '198.51.100')]:
        ip('link', 'add', bridge, 'type', 'veth', 'peer', 'name', peer)
        ip('addr', 'add', subnet + '.1/24', 'dev', bridge)
        for iface in (bridge, peer):
            ip('link', 'set', iface, 'up')
    for index, iface in enumerate(('wan0', 'tun11', 'wgc1'), 1):
        ip('link', 'add', iface, 'type', 'dummy')
        ip('addr', 'add', f'10.{index}.0.1/24', 'dev', iface)
        ip('link', 'set', iface, 'up')
        ip('neigh', 'add', f'10.{index}.0.2', 'lladdr', '02:00:00:00:00:02', 'dev', iface)
    ip('route', 'add', 'default', 'via', '10.1.0.2', 'dev', 'wan0')
    for table, iface, index in [('ovpnc1', 'tun11', 2), ('wgc1', 'wgc1', 3)]:
        ip('route', 'add', 'default', 'via', f'10.{index}.0.2', 'dev', iface, 'onlink', 'table', table)
    ip('-6', 'addr', 'add', '2001:db8:1::1/64', 'dev', 'br0', 'nodad')
    ip('-6', 'addr', 'add', '2001:db8:3::1/64', 'dev', 'br1', 'nodad')
    ip('-6', 'addr', 'add', '2001:db8:2::1/64', 'dev', 'wan0', 'nodad')
    ip('-6', 'neigh', 'add', '2001:db8:2::2', 'lladdr', '02:00:00:00:00:02', 'dev', 'wan0')
    ip('-6', 'route', 'add', 'default', 'via', '2001:db8:2::2', 'dev', 'wan0')


def checksum(data):
    data += b'\0' * (len(data) % 2)
    total = sum(struct.unpack('!%dH' % (len(data) // 2), data))
    while total >> 16:
        total = (total & 65535) + (total >> 16)
    return (~total) & 65535


class Fixture:
    def __init__(self, root):
        self.root = root
        self.env = dict(os.environ, PATH='/usr/sbin:/usr/bin:/sbin:/bin',
                        lan_ifname='br0', vpn_clientx_eas='1,', vpn_client1_rgw='2',
                        vpn_client1_enforce='1', vpn_client1_verb='3', vpn_client1_state='2',
                        wgc1_enable='1', wgc1_enforce='1', wgc1_dns='10.3.0.53')
        for directory in ('etc/openvpn/client1', 'etc/wg'):
            (root / directory).mkdir(parents=True, exist_ok=True)
        (root / 'etc/openvpn/client1/resolv.dnsmasq').write_text('server=10.2.0.53\n')
        self.sockets = {}
        for iface in ('wan0', 'tun11', 'wgc1'):
            sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
            sock.bind((iface, 0))
            self.sockets[iface] = sock
        self.sequence = 0

    def call(self, operation, proto, *args):
        result = run('chroot', str(self.root), '/qemu', '/harness/driver', operation, proto,
                     *args, env=self.env)
        if result.stdout or result.stderr:
            print(result.stdout + result.stderr, end='', flush=True)

    def rules(self, text):
        (self.root / 'jffs/openvpn/vpndirector_rulelist').write_text(text)

    def sdn(self, target):
        self.env.update(lan_ipaddr='192.0.2.1', lan_netmask='255.255.255.0',
                        vlan_rl='<1>10>0',
                        subnet_rl='<1>br1>198.51.100.1>255.255.255.0>1>198.51.100.10>198.51.100.200>86400>>>>0>0',
                        sdn_rl=f'<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>{target}')

    def route(self, source, dest, expected, iface='br0'):
        result = ip('route', 'get', dest, 'from', source, 'iif', iface, check=False)
        if expected is None:
            assert result.returncode and 'Permission denied' in result.stderr, result
        else:
            assert result.returncode == 0 and 'dev ' + expected in result.stdout, result

    def packet(self, source, dest, expected, port=12345, translated=None, iface='br0', tcp=False):
        # Inject a real Ethernet frame at the LAN boundary; observe actual forwarding.
        for sock in self.sockets.values():
            while select.select([sock], [], [], 0)[0]:
                sock.recv(65535)
        self.sequence += 1
        payload = ('RTBE90U-PACKET-%d' % self.sequence).encode()
        version6 = ':' in source
        transport = 6 if tcp else 17
        if tcp:
            body = struct.pack('!HHLLBBHHH', 30000 + self.sequence, port, self.sequence, 0, 0x50, 2, 4096, 0, 0) + payload
        else:
            body = struct.pack('!HHHH', 30000 + self.sequence, port, 8 + len(payload), 0) + payload
        if version6:
            src = socket.inet_pton(socket.AF_INET6, source)
            dst = socket.inet_pton(socket.AF_INET6, dest)
            pseudo = src + dst + struct.pack('!L3xB', len(body), transport)
        else:
            src, dst = socket.inet_aton(source), socket.inet_aton(dest)
            pseudo = src + dst + struct.pack('!BBH', 0, transport, len(body))
        offset = 16 if tcp else 6
        body = body[:offset] + struct.pack('!H', checksum(pseudo + body) or 65535) + body[offset + 2:]
        if version6:
            header = struct.pack('!LHBB16s16s', 6 << 28, len(body), transport, 64, src, dst)
        else:
            header = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 20 + len(body), self.sequence,
                                 0, 64, transport, 0, src, dst)
            header = header[:10] + struct.pack('!H', checksum(header)) + header[12:]
        mac = bytes.fromhex(Path('/sys/class/net/' + iface + '/address').read_text().strip().replace(':', ''))
        ether_type = b'\x86\xdd' if version6 else b'\x08\x00'
        frame = mac + b'\x02\x00\x00\x00\x00\x10' + ether_type + header + body
        with socket.socket(socket.AF_PACKET, socket.SOCK_RAW) as sender:
            sender.bind(('lanpeer' if iface == 'br0' else 'sdnpeer', 0))
            sender.send(frame)
        found = []
        deadline = time.monotonic() + (0.3 if expected is None else 1.0)
        while time.monotonic() < deadline:
            ready, _, _ = select.select(list(self.sockets.values()), [], [], max(0, deadline - time.monotonic()))
            for sock in ready:
                packet = sock.recv(65535)
                if payload in packet and packet[12:14] == ether_type and packet[20 if version6 else 23] == transport:
                    address = socket.inet_ntop(socket.AF_INET6, packet[38:54]) if version6 else socket.inet_ntoa(packet[30:34])
                    found.append((sock.getsockname()[0], address))
            if found:
                break
        assert found == ([] if expected is None else [(expected, translated or dest)]), (source, dest, expected, found)


def exercise(fixture, protocol):
    fixture.sdn(0)
    for proto, target, interface in [('ovpn', 'OVPN1', 'tun11'), ('wg', 'WGC1', 'wgc1')]:
        if protocol not in ('both', proto):
            continue
        table = 'ovpnc1' if proto == 'ovpn' else 'wgc1'
        index = 2 if proto == 'ovpn' else 3
        fixture.rules(f'<1>protected>192.0.2.10>>{target}<1>wan>192.0.2.11>>WAN'
                      f'<0>disabled>192.0.2.12>>{target}')
        fixture.call('rules', proto)
        fixture.call('kill', proto)
        for source, output in [('192.0.2.10', interface), ('192.0.2.11', 'wan0'), ('192.0.2.12', 'wan0')]:
            fixture.route(source, '203.0.113.99', output)
            fixture.packet(source, '203.0.113.99', output)
        print('PASS', proto, 'source policy, WAN and disabled rule forwarding', flush=True)
        fixture.call('dns', proto)
        for tcp in (False, True):
            fixture.packet('192.0.2.10', '203.0.113.53', interface, port=53, translated=f'10.{index}.0.53', tcp=tcp)
            fixture.packet('192.0.2.11', '203.0.113.53', 'wan0', port=53, tcp=tcp)
        print('PASS', proto, 'UDP/TCP exclusive DNS DNAT and WAN exemption', flush=True)
        fixture.call('undns', proto)
        fixture.call('clear', proto)
        fixture.route('192.0.2.10', '203.0.113.99', None)
        fixture.packet('192.0.2.10', '203.0.113.99', None)
        fixture.packet('192.0.2.10', '203.0.113.53', None, port=53)
        fixture.packet('192.0.2.11', '203.0.113.99', 'wan0')
        fixture.call('rules', proto)
        fixture.packet('192.0.2.10', '203.0.113.99', interface)
        ip('route', 'flush', 'table', table)
        fixture.packet('192.0.2.10', '203.0.113.99', None)
        ip('route', 'add', 'default', 'via', f'10.{index}.0.2', 'dev', interface, 'onlink', 'table', table)
        fixture.packet('192.0.2.10', '203.0.113.99', interface)
        print('PASS', proto, 'tunnel policy removal, kill switch and reconnection', flush=True)
        fixture.call('clear', proto)
        fixture.call('unkill', proto)
        fixture.packet('192.0.2.10', '203.0.113.99', 'wan0')
        # Destination-only rules must be protected when the tunnel is down.
        fixture.rules(f'<1>destination>>203.0.113.99>{target}')
        fixture.call('kill', proto)
        fixture.packet('192.0.2.10', '203.0.113.99', None)
        fixture.packet('192.0.2.10', '203.0.113.100', 'wan0')
        fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1')
        fixture.packet('198.51.100.10', '203.0.113.100', 'wan0', iface='br1')
        print('PASS', proto, 'destination-only kill switch', flush=True)
        fixture.call('unkill', proto)
        fixture.rules(f'<1>narrow>192.0.2.10>203.0.113.99>{target}')
        fixture.call('kill', proto)
        fixture.packet('192.0.2.10', '203.0.113.99', None)
        fixture.packet('192.0.2.10', '203.0.113.100', 'wan0')
        fixture.packet('192.0.2.12', '203.0.113.99', 'wan0')
        fixture.call('unkill', proto)
        for source in ('', '0.0.0.0', '0.0.0.0/0'):
            fixture.rules(f'<1>all>{source}>>{target}')
            fixture.call('kill', proto)
            fixture.packet('192.0.2.10', '203.0.113.99', None)
            fixture.packet('192.0.2.11', '203.0.113.99', 'wan0')
            # Router-originated traffic must still reach the VPN endpoint to reconnect.
            endpoint_route = ip('route', 'get', '203.0.113.99', check=False)
            assert endpoint_route.returncode == 0 and 'dev wan0' in endpoint_route.stdout, endpoint_route
            fixture.call('kill', proto)
            rules = json.loads(ip('-j', 'rule').stdout)
            priority = 12210 if proto == 'ovpn' else 12215
            assert sum(rule['priority'] == priority for rule in rules) == 2, rules
            fixture.call('unkill', proto)
        print('PASS', proto, 'precise destination selectors, catch-all and idempotent kill switch', flush=True)
        fixture.rules('')
        fixture.call('kill', proto, 'br1')
        fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1')
        fixture.packet('192.0.2.10', '203.0.113.99', 'wan0')
        fixture.call('unkill', proto, 'br1')
        fixture.packet('198.51.100.10', '203.0.113.99', 'wan0', iface='br1')
        fixture.sdn(6 if proto == 'ovpn' else 1)
        fixture.call('kill', proto)
        fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1')
        fixture.call('unkill', proto)
        fixture.packet('198.51.100.10', '203.0.113.99', 'wan0', iface='br1')
        fixture.sdn(0)
        print('PASS', proto, 'explicit and NVRAM-associated SDN kill switch and removal', flush=True)
        if proto == 'ovpn':
            fixture.rules('<1>missing gateway>192.0.2.10>>OVPN1')
            fixture.env['dev'] = 'tun11'
            fixture.call('kill', 'ovpn')
            fixture.call('up', 'ovpn')
            fixture.route('192.0.2.10', '203.0.113.99', None)
            fixture.packet('192.0.2.10', '203.0.113.99', None)
            fixture.packet('192.0.2.11', '203.0.113.99', 'wan0')
            fixture.env['route_vpn_gateway'] = '10.2.0.2'
            fixture.call('up', 'ovpn')
            fixture.packet('192.0.2.10', '203.0.113.99', 'tun11')
            for key in ('dev', 'route_vpn_gateway'):
                del fixture.env[key]
            fixture.call('clear', 'ovpn')
            fixture.call('unkill', 'ovpn')
            print('PASS actual OpenVPN up hook blocks missing-gateway WAN fallback and restores tunnel routing', flush=True)
    if protocol != 'both':
        return
    fixture.rules('<1>overlap>192.0.2.10>>OVPN1<1>overlap>192.0.2.10>>WGC1<1>wan>192.0.2.11>>WAN')
    for proto in ('wg', 'ovpn'):
        fixture.call('rules', proto)
        fixture.call('kill', proto)
        fixture.call('dns', proto)
    fixture.packet('192.0.2.10', '203.0.113.99', 'tun11')
    fixture.packet('192.0.2.10', '203.0.113.53', 'tun11', port=53, translated='10.2.0.53')
    fixture.packet('192.0.2.11', '203.0.113.53', 'wan0', port=53)
    fixture.call('clear', 'ovpn')
    fixture.call('undns', 'ovpn')
    fixture.packet('192.0.2.10', '203.0.113.53', 'wgc1', port=53, translated='10.3.0.53')
    fixture.call('clear', 'wg')
    fixture.call('undns', 'wg')
    fixture.packet('192.0.2.10', '203.0.113.53', None, port=53)
    for proto in ('ovpn', 'wg'):
        fixture.call('unkill', proto)
    print('PASS overlapping OpenVPN/WireGuard priorities, DNS order and policy fallback', flush=True)
    fixture.env['vpn_client1_rgw'] = '1'
    fixture.call('rules', 'ovpn')
    fixture.packet('192.0.2.10', '203.0.113.99', 'tun11')
    fixture.call('kill', 'ovpn')
    fixture.call('clear', 'ovpn')
    fixture.packet('192.0.2.10', '203.0.113.99', None)
    fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1')
    fixture.packet('2001:db8:1::10', '2001:db8:ffff::53', None, port=53)
    fixture.packet('2001:db8:3::10', '2001:db8:ffff::53', None, port=53, iface='br1')
    print('PASS ovpn global routing, IPv4 kill switch and LAN/SDN IPv6 blocking', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--protocol', choices=('ovpn', 'wg', 'both'), default='both')
    args = parser.parse_args()
    root = args.root
    prepare(root)
    setup_links()
    fixture = Fixture(root)
    try:
        exercise(fixture, args.protocol)
    finally:
        print('Final rules:', ip('rule').stdout, flush=True)
        print('Final NAT:', run('iptables-nft', '-t', 'nat', '-S').stdout, flush=True)
        for sock in fixture.sockets.values():
            sock.close()
