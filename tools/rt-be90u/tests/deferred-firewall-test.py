#!/usr/bin/env python3
"""Shared-router DNS quarantine, production filter emitter and local services."""
import importlib.util
import json
from pathlib import Path
import select
import socket
import struct

spec = importlib.util.spec_from_file_location('deferred_firewall_network', '/tests/vpn-network-test.py')
net = importlib.util.module_from_spec(spec)
spec.loader.exec_module(net)


def compile_driver(fixture):
    exports = fixture.root / 'harness/deferred-firewall-exports'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; };\n')
    net.run('aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
            '-Wl,--dynamic-list,' + str(exports), '-I/work/release/src/router/libovpn',
            '/tests/deferred-firewall-driver.c', '-L/firmware/usr/lib',
            '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib', '-lovpn', '-lshared', '-lnvram',
            '-o', str(fixture.root / 'harness/deferred-firewall-driver'))


def invoke(fixture, operation):
    return net.run('chroot', str(fixture.root), '/qemu', '/harness/deferred-firewall-driver',
                   operation, env=fixture.env).stdout


def exercise(fixture, compile=True):
    if compile:
        compile_driver(fixture)
    # This last fixture stage owns disposable filter/NAT state; the earlier
    # migration checks already verified the independent VPN DNS forwarding.
    net.run('iptables-nft', '-t', 'nat', '-F')
    fixture.sdn(0)
    fixture.rules('')
    fixture.env.update(qca_merlin_vpn_migrated='1', vpnc_clientlist='',
                       vpnc_dev_policy_list='', vpnc_default_wan='0', wgc5_enable='1')
    invoke(fixture, 'refresh')
    servers = {}
    peer = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    peer.bind(('lanpeer', 0))
    for port, tcp in ((53, False), (53, True), (80, True), (67, False)):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM if tcp else socket.SOCK_DGRAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(('192.0.2.1', port))
        if tcp:
            server.listen(32)
        servers[port, tcp] = server
    checks = 0

    def local(source, port, tcp, allowed, label):
        nonlocal checks
        for sock in (peer, servers[port, tcp]):
            if tcp and sock is servers[port, tcp]:
                continue
            while select.select([sock], [], [], 0)[0]:
                sock.recv(65535)
        net.ip('neigh', 'replace', source, 'lladdr', '02:00:00:00:00:10', 'dev', 'br0')
        fixture.packet(source, '192.0.2.1', None, port=port, tcp=tcp)
        received = False
        if tcp:
            while select.select([peer], [], [], 0)[0]:
                packet = peer.recv(65535)
                if len(packet) >= 54 and packet[12:14] == b'\x08\x00' and packet[23] == 6:
                    received |= (socket.inet_ntoa(packet[30:34]) == source and
                                 struct.unpack('!H', packet[34:36])[0] == port and
                                 struct.unpack('!H', packet[36:38])[0] == 30000 + fixture.sequence and
                                 packet[47] & 0x12 == 0x12)
        elif select.select([servers[port, tcp]], [], [], 0)[0]:
            payload, address = servers[port, tcp].recvfrom(65535)
            received = address[0] == source and payload == f'RTBE90U-PACKET-{fixture.sequence}'.encode()
        checks += 1
        assert received == allowed, (label, source, port, tcp, allowed, received)
        print('PASS', label, source, 'TCP' if tcp else 'UDP', port, flush=True)

    def dns(source, allowed, label):
        for tcp in (False, True):
            local(source, 53, tcp, allowed, label)

    def packet(source, allowed, label):
        nonlocal checks
        fixture.packet(source, '203.0.113.99', 'wan0' if allowed else None)
        checks += 1
        print('PASS', label, source, flush=True)

    try:
        dns('192.0.2.20', True, 'router DNS baseline available')
        fixture.env.update(qca_merlin_vpn_migrated='',
                           vpnc_clientlist='<Stock>OpenVPN>1>>>1>5>>>0>0>Web',
                           vpnc_dev_policy_list='<1>192.0.2.20>203.0.113.99>5')
        invoke(fixture, 'refresh')
        dns('192.0.2.20', False, 'affected router DNS quarantined')
        dns('192.0.2.21', True, 'unaffected router DNS available')
        local('192.0.2.20', 80, True, True, 'affected HTTP port remains available')
        local('192.0.2.20', 67, False, True, 'affected DHCP port remains available')
        for family in (4, 6):
            text = invoke(fixture, 'filter' + str(family))
            net.run('iptables-nft-restore' if family == 4 else 'ip6tables-nft-restore', input=text)
        dns('192.0.2.20', False, 'filter replacement using production emitter retains quarantine')
        dns('192.0.2.21', True, 'filter replacement using production emitter preserves other DNS')
        fixture.env['sdn_rl'] = fixture.env['sdn_rl'].replace('<0>LAN>1>0>0>0>0', '<0>LAN>1>0>0>0>5')
        fixture.rules('<1>Independent>192.0.2.50>>WGC5<1>WAN>192.0.2.51>>WAN'
                      '<1>Narrow destination>192.0.2.21>203.0.113.77>WGC5')
        invoke(fixture, 'refresh')
        dns('192.0.2.20', False, 'deferred main LAN quarantined')
        dns('192.0.2.50', True, 'independent Director client router DNS available')
        dns('192.0.2.51', True, 'independent WAN router DNS available')
        dns('192.0.2.21', False, 'unrelated Director destination is not a DNS exemption')
        # Narrowing must retire the whole-interface guard, including legacy
        # untagged generations, without accidentally deleting retained rules.
        fixture.sdn(0)
        invoke(fixture, 'refresh')
        packet('192.0.2.20', False, 'whole-LAN to source-only keeps intended data blocked')
        packet('192.0.2.21', True, 'whole-LAN to source-only releases unrelated data')
        dns('192.0.2.21', True, 'whole-LAN to source-only releases unrelated DNS')
        # Publish broad legacy0 first so the narrower legacy0 can coexist.
        # Their deletion must follow kernel order even when desired state
        # semantically equals one of the old narrower rules.
        net.ip('rule', 'add', 'from', 'all', 'iif', 'br0', 'priority', '12225', 'prohibit')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'to', '203.0.113.99',
               'iif', 'br0', 'priority', '12225', 'prohibit')
        invoke(fixture, 'refresh')
        packet('192.0.2.20', False, 'legacy wildcard replacement keeps intended data blocked')
        packet('192.0.2.21', True, 'legacy wildcard replacement releases unrelated data')
        before = net.ip('-j', 'rule').stdout
        invoke(fixture, 'refresh')
        assert net.ip('-j', 'rule').stdout == before, 'Repeated refresh changed settled guard set'
        checks += 1
        print('PASS source-only to whole-LAN to source-only refresh is bounded', flush=True)
        fixture.env['vpnc_dev_policy_list'] += '<1>not-an-address>>5'
        states = []
        for _ in range(3):
            invoke(fixture, 'refresh')
            states.append((net.run('iptables-nft', '-S').stdout, net.ip('-j', 'rule').stdout))
        assert len(set(states)) == 1, 'Mixed valid/malformed refresh accumulated DNS rules'
        assert any(rule['priority'] == 90 for rule in json.loads(net.ip('-j', 'rule').stdout))
        checks += 2
        print('PASS repeated mixed valid/malformed refresh preserves bounded network quarantine', flush=True)
        packet('192.0.2.21', False, 'unidentifiable malformed source quarantines otherwise unrelated LAN traffic')
        text = invoke(fixture, 'filter4')
        net.run('iptables-nft-restore', input=text)
        dns('192.0.2.20', False, 'mixed-record production filter replacement retains known quarantine')
        dns('192.0.2.50', False, 'mixed-record production filter replacement blocks Director DNS within quarantine')
        # Execute every normal route command; only this explicit failure skips
        # one legacy deletion. A populated old lookup must not escape while
        # that failure leaves temporary protection installed.
        fixture.env['vpnc_dev_policy_list'] = '<1>192.0.2.20>203.0.113.99>5'
        invoke(fixture, 'refresh')
        net.ip('route', 'replace', 'default', 'via', '10.3.0.2', 'dev', 'wgc1', 'table', '5')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'table', '5', 'priority', '100')
        fixture.packet('192.0.2.20', '203.0.113.99', 'wgc1')
        checks += 1
        fixture.env['vpnc_dev_policy_list'] += '<1>not-an-address>>5'
        executable = fixture.root / 'usr/sbin/ip'
        original = fixture.root / 'harness/deferred-fault/ip'
        original.parent.mkdir()
        executable.rename(original)
        executable.write_text('#!/bin/sh\n'
            'if [ "$*" = "-4 rule del from 192.0.2.20 to all priority 100 protocol 0 table 5" ]; then\n'
            '  printf "injected legacy-delete failure\\n" > /tmp/deferred-fault-hit\n  exit 1\nfi\n'
            'exec /harness/deferred-fault/ip "$@"\n')
        executable.chmod(0o755)
        try:
            failed = net.run('chroot', str(fixture.root), '/qemu',
                             '/harness/deferred-firewall-driver', 'refresh', env=fixture.env, check=False)
            assert failed.returncode and (fixture.root / 'tmp/deferred-fault-hit').is_file(), failed
            packet('192.0.2.20', False, 'failed unsafe lookup cleanup retains packet protection')
            dns('192.0.2.20', False, 'failed unsafe lookup cleanup retains DNS protection')
        finally:
            executable.unlink()
            original.rename(executable)
        invoke(fixture, 'refresh')
        assert not any(rule['priority'] == 100 for rule in json.loads(net.ip('-j', 'rule').stdout))
        assert any(rule['priority'] == 90 for rule in json.loads(net.ip('-j', 'rule').stdout))
        checks += 2
        packet('192.0.2.20', False, 'mixed unresolved retry conservatively retains earlier protection')
        fixture.env['vpnc_dev_policy_list'] = '<1>192.0.2.20>203.0.113.99>5'
        invoke(fixture, 'refresh')
        assert not any(rule['priority'] == 90 for rule in json.loads(net.ip('-j', 'rule').stdout))
        checks += 1
        packet('192.0.2.21', True, 'resolved unknown selector releases conservative transition guard')
        # An unknown extension using our metadata range is foreign. Refuse
        # mutation instead of allowing a less-specific delete to remove it.
        for priority in ('90', '12225'):
            for protocol, extension in [('240', ['fwmark', '0x40']), ('2', [])]:
                selector = ['from', 'all', 'iif', 'br0', *extension,
                            'priority', priority, 'protocol', protocol, 'prohibit']
                net.ip('rule', 'add', *selector)
                before = net.ip('-j', 'rule').stdout
                failed = net.run('chroot', str(fixture.root), '/qemu', '/harness/deferred-firewall-driver',
                                 'refresh', env=fixture.env, check=False)
                assert failed.returncode and net.ip('-j', 'rule').stdout == before, failed
                checks += 1
                print('PASS foreign guard metadata survives failed refresh', priority, protocol, extension, flush=True)
                net.ip('rule', 'del', *selector)
        # Overlapping explicit stock WAN preserves its indistinguishable main
        # lookup; it must not exempt deferred DNS or skip unsafe VPN cleanup.
        fixture.env['vpnc_dev_policy_list'] = '<1>192.0.2.20>>0<1>192.0.2.20>203.0.113.99>5'
        net.ip('rule', 'add', 'from', '192.0.2.20', 'table', 'main', 'priority', '100')
        net.ip('rule', 'add', 'from', '192.0.2.20', 'table', '5', 'priority', '100')
        invoke(fixture, 'refresh')
        policy_rules = [rule for rule in json.loads(net.ip('-j', 'rule').stdout) if rule['priority'] == 100]
        assert len(policy_rules) == 1 and policy_rules[0]['table'] == 'main', policy_rules
        checks += 1
        packet('192.0.2.20', True, 'overlapping explicit stock WAN lookup survives unsafe cleanup')
        dns('192.0.2.20', False, 'overlapping stock WAN does not bypass deferred DNS')
        net.ip('rule', 'del', 'from', '192.0.2.20', 'table', 'main', 'priority', '100')
        # The old Fusion parser truncates this valid /25 to /2. Linux stores
        # the original nonzero host bits and requires them on rule deletion.
        fixture.env['vpnc_dev_policy_list'] = '<1>198.51.100.12/25>>5'
        net.ip('rule', 'add', 'from', '198.51.100.12/2', 'table', '5', 'priority', '100')
        fixture.packet('192.0.2.20', '203.0.113.99', 'wgc1')
        checks += 1
        invoke(fixture, 'refresh')
        fixture.packet('198.51.100.12', '203.0.113.99', None, iface='br1')
        fixture.packet('198.51.100.200', '203.0.113.99', 'wan0', iface='br1')
        checks += 2
        packet('192.0.2.20', True, 'exact stored legacy prefix cleanup releases unrelated LAN')
        assert not any(rule['priority'] in (90, 100) for rule in json.loads(net.ip('-j', 'rule').stdout))
        checks += 1
        print('PASS legacy prefix host bits preserved for deletion while semantic scope stays normalized', flush=True)
        fixture.sdn(0)
        fixture.env.update(qca_merlin_vpn_migrated='1', vpnc_clientlist='', vpnc_dev_policy_list='')
        invoke(fixture, 'refresh')
        dns('192.0.2.20', True, 'resolved state releases router DNS quarantine')
        print(f'Deferred firewall summary: {checks} checks passed', flush=True)
    finally:
        peer.close()
        for server in servers.values():
            server.close()
