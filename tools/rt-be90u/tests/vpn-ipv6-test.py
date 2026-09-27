#!/usr/bin/env python3
"""Actual ARM library, IPv6 packets and firewall transactions in isolation.

Interface protection intentionally blocks IPv6 forwarding even while the IPv4
tunnel is up. IPv4 device/destination selectors do not identify IPv6 peers.
"""
import argparse
import importlib.util
from pathlib import Path
import socket

spec = importlib.util.spec_from_file_location('network', '/tests/vpn-network-test.py')
net = importlib.util.module_from_spec(spec)
spec.loader.exec_module(net)


def exercise(fixture, regression):
    failures = []

    def packets(iface, expected):
        source = '2001:db8:1::10' if iface == 'br0' else '2001:db8:3::10'
        for tcp in (False, True):
            for port in (53, 443):
                fixture.packet(source, '2001:db8:ffff::53', expected,
                               iface=iface, port=port, tcp=tcp)

    def scenario(label, action):
        try:
            action()
            print('PASS', label, flush=True)
        except AssertionError as error:
            print('FAIL', label, repr(error), flush=True)
            failures.append(label)
            if not regression:
                raise

    fixture.sdn(0)
    fixture.rules('')
    fixture.env.update(vpn_client1_enforce='0', wgc1_enforce='0')
    fixture.call('kill', 'ovpn')
    packets('br0', 'wan0')
    packets('br1', 'wan0')

    # A pre-existing ACCEPT must never precede the guard after a refresh.
    net.run('ip6tables-nft', '-A', 'FORWARD', '-j', 'ACCEPT')
    fixture.env.update(vpn_client1_enforce='1', vpn_client1_rgw='1')
    fixture.call('kill', 'ovpn')
    scenario('global OpenVPN IPv6 UDP/TCP DNS and data blocking', lambda: packets('br0', None))
    scenario('global OpenVPN covers unassigned SDN', lambda: packets('br1', None))
    fixture.env['vpn_client1_enforce'] = '0'
    fixture.call('kill', 'ovpn')
    packets('br0', 'wan0')
    packets('br1', 'wan0')

    for proto, target, index, key in [('ovpn', 'OVPN1', 6, 'vpn_client1_enforce'),
                                      ('wg', 'WGC1', 1, 'wgc1_enforce')]:
        fixture.env.update(vpn_client1_rgw='2')
        fixture.env[key] = '1'
        fixture.sdn(index)
        fixture.call('kill', proto)
        scenario(proto + ' assigned SDN blocks IPv6', lambda: packets('br1', None))
        packets('br0', 'wan0')
        fixture.call('kill', proto)
        scenario(proto + ' repeated refresh preserves protection', lambda: packets('br1', None))
        fixture.call('kill', proto, 'br1')
        scenario(proto + ' SDN event preserves configured protection', lambda: packets('br1', None))
        fixture.sdn(0)
        fixture.call('kill', proto)
        packets('br1', 'wan0')

        for wildcard in ('', '0.0.0.0', '0.0.0.0/0'):
            fixture.rules(f'<1>all>{wildcard}>{wildcard}>{target}')
            fixture.call('kill', proto)
            scenario(proto + ' catch-all ' + repr(wildcard), lambda: packets('br0', None))
            scenario(proto + ' catch-all SDN ' + repr(wildcard), lambda: packets('br1', None))

        fixture.rules(f'<0>disabled>>>{target}<1>device>192.0.2.10>>{target}')
        fixture.call('kill', proto)
        packets('br0', 'wan0')
        print('KNOWN LIMIT: IPv4 device selectors cannot identify IPv6 addresses', flush=True)
        fixture.rules(f'<1>destination>>203.0.113.99>{target}')
        fixture.call('kill', proto)
        packets('br0', 'wan0')
        fixture.rules('')
        fixture.env[key] = '0'
        fixture.call('kill', proto)

    if regression:
        assert failures, 'Old image unexpectedly passed all regression cases'
        raise SystemExit('Reproduced IPv6 guard failures: ' + ', '.join(failures))

    # Overlapping enabled profiles are recomputed together; disabling one must
    # leave the other's protection intact. A stopped-but-enabled client is kept.
    fixture.rules('<1>all>>>OVPN1<1>all>>>WGC1')
    fixture.env.update(vpn_client1_enforce='1', wgc1_enforce='1', vpn_client1_state='0')
    fixture.call('kill', 'ovpn')
    fixture.env['vpn_client1_enforce'] = '0'
    fixture.call('kill', 'ovpn')
    packets('br0', None)
    fixture.call('clear', 'wg')
    packets('br0', None)
    fixture.env['wgc1_enable'] = '0'
    fixture.call('kill', 'wg')
    packets('br0', 'wan0')
    fixture.env['wgc1_enable'] = '1'
    fixture.call('kill', 'wg')
    packets('br0', None)
    print('PASS overlapping profiles, disabled client and stopped-enabled protection', flush=True)

    # IPv4 WAN exceptions cannot identify their IPv6 counterparts either.
    fixture.rules('<1>all>>>WGC1<1>wan>192.0.2.10>>WAN')
    fixture.call('kill', 'wg')
    packets('br0', None)
    print('PASS catch-all blocks IPv6 even with an IPv4-only WAN exemption', flush=True)

    # A full firewall restore must contain the guard in the same transaction,
    # before an existing ACCEPT; exercise the exact image writer used by rc.
    result = net.run('chroot', str(fixture.root), '/qemu', '/harness/driver',
                     'ipv6-write', 'wg', env=fixture.env)
    for _ in range(2):
        rules = '*filter\n:INPUT ACCEPT [0:0]\n:FORWARD ACCEPT [0:0]\n:OUTPUT ACCEPT [0:0]\n'
        rules += '-A FORWARD -j ACCEPT\n' + result.stdout + 'COMMIT\n'
        net.run('ip6tables-nft-restore', input=rules)
        packets('br0', None)
        fixture.call('kill', 'wg')
        chain = net.run('ip6tables-nft', '-S', 'FORWARD').stdout
        assert chain.count('-j VPN6KS') == 1, chain
        assert chain.splitlines()[1] == '-A FORWARD -j VPN6KS', chain
    print('PASS full firewall replacement and idempotent atomic chain refresh', flush=True)

    # Local router services and outbound VPN endpoint traffic remain reachable.
    with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM) as listener:
        listener.bind(('2001:db8:1::1', 53535))
        listener.settimeout(2)
        fixture.packet('2001:db8:1::10', '2001:db8:1::1', None, port=53535)
        assert listener.recv(512).startswith(b'RTBE90U-PACKET-')
    with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM) as sender:
        sender.sendto(b'RTBE90U-RECONNECT', ('2001:db8:ffff::53', 51820))
        fixture.sockets['wan0'].settimeout(2)
        while b'RTBE90U-RECONNECT' not in fixture.sockets['wan0'].recv(65535):
            pass
    print('PASS router-local IPv6 input and outbound reconnect traffic', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--regression', action='store_true')
    args = parser.parse_args()
    net.prepare(args.root)
    net.setup_links()
    fixture = net.Fixture(args.root)
    try:
        exercise(fixture, args.regression)
    finally:
        print(net.run('ip6tables-nft-save').stdout, flush=True)
        for sock in fixture.sockets.values():
            sock.close()
