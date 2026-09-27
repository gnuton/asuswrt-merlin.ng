#!/usr/bin/env python3
"""Approved broad deferred-policy quarantine through packaged ARM code.

Native kernel packets, including real router service listeners, distinguish
forwarding quarantine from loss of management or router-origin connectivity.
Each scenario starts without earlier guards; preserved images can run the same
expectations with --expect-regression to collect independent negative evidence.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import select
import signal
import socket
import struct
import subprocess
import time

spec = importlib.util.spec_from_file_location('broad_firewall', '/tests/deferred-firewall-test.py')
firewall = importlib.util.module_from_spec(spec)
spec.loader.exec_module(firewall)
net = firewall.net
spec = importlib.util.spec_from_file_location('broad_default', '/tests/sdn-default-test.py')
default = importlib.util.module_from_spec(spec)
spec.loader.exec_module(default)


def compile_sdn_driver(root):
    # Build the established default driver to provision its explicit inactive
    # service traps, then link this fixture's entry point to the same real rc
    # objects. No routing, locking, NVRAM parsing or IPv6 guard is substituted.
    default.compile_driver(root)
    net.run('aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
            '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
            '-Wl,--dynamic-list,' + str(root / 'harness/default-exports.list'),
            '-I/work/release/src-qca-ipq53xx/include', '-I/work/release/src/router/shared',
            '-I/work/release/src/router/libovpn', '-I/work/release/src/router/rc',
            '/tests/broad-sdn-driver.c', str(root / 'harness/default-traps.c'),
            '/work/release/src/router/rc/sdn.o', '/work/release/src/router/rc/vpnc_fusion.o',
            '/work/release/src/router/rc/common.o', '-L/firmware/usr/lib',
            '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib', '-lovpn', '-lshared', '-lnvram',
            '-o', str(root / 'harness/broad-sdn-driver'))


class Fixture(net.Fixture):
    ADDRESSES = {
        'br0': ('192.0.2.1', '2001:db8:1::1', 'lanpeer'),
        'br1': ('198.51.100.1', '2001:db8:3::1', 'sdnpeer'),
    }

    def __init__(self, root, regression):
        super().__init__(root)
        self.regression = regression
        self.checks = 0
        self.failures = []
        self.boundaries = 0
        self.listeners = {}
        self.peers = {}
        for iface, addresses in self.ADDRESSES.items():
            peer = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
            peer.bind((addresses[2], 0))
            self.peers[iface] = peer
            for family, address in ((socket.AF_INET, addresses[0]), (socket.AF_INET6, addresses[1])):
                for port, tcp in ((53, False), (53, True), (80, True), (67 if family == socket.AF_INET else 547, False)):
                    listener = socket.socket(family, socket.SOCK_STREAM if tcp else socket.SOCK_DGRAM)
                    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    if family == socket.AF_INET6:
                        listener.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
                    listener.bind((address, port))
                    if tcp:
                        listener.listen(128)
                    self.listeners[iface, family, port, tcp] = listener

    def check(self, label, action):
        self.checks += 1
        try:
            action()
        except AssertionError as error:
            self.failures.append(label)
            print('FAIL', label, repr(error), flush=True)
            if not self.regression:
                raise
        else:
            print('PASS', label, flush=True)

    def require(self, label, value, detail=None):
        def assertion():
            assert value, detail
        self.check(label, assertion)

    def reset(self, policy):
        # These tables/interfaces exist only inside the disposable container.
        for family in ('-4', '-6'):
            net.ip(family, 'rule', 'flush')
            if not any(rule['priority'] == 0 for rule in json.loads(net.ip(family, '-j', 'rule').stdout)):
                net.ip(family, 'rule', 'add', 'priority', '0', 'lookup', 'local')
            net.ip(family, 'rule', 'add', 'priority', '32766', 'lookup', 'main')
            net.ip(family, 'rule', 'add', 'priority', '32767', 'lookup', 'default')
        for utility in ('iptables-nft-restore', 'ip6tables-nft-restore'):
            net.run(utility, input='*filter\n:INPUT ACCEPT [0:0]\n:FORWARD ACCEPT [0:0]\n:OUTPUT ACCEPT [0:0]\nCOMMIT\n')
        self.sdn(0)
        self.env['sdn_rl'] = ''.join('<' + row + '>0' * (23 - len(row.split('>')))
                                   for row in self.env['sdn_rl'].split('<') if row)
        self.env.update(qca_merlin_vpn_migrated='',
                        vpnc_clientlist='<Stock>OpenVPN>1>>>1>5>>>0>0>Web',
                        vpnc_dev_policy_list=policy, vpnc_default_wan='0')
        self.rules('')

    def refresh(self, label, failure=False):
        result = net.run('chroot', str(self.root), '/qemu', '/harness/deferred-firewall-driver',
                         'refresh', env=self.env, check=False)
        self.require(label, bool(result.returncode) == failure,
                     (result.returncode, result.stdout, result.stderr))
        return result

    def install_barrier(self):
        if (self.root / 'tmp/broad-events').exists():
            return
        for name in ('broad-events', 'broad-acks'):
            os.mkfifo(self.root / 'tmp' / name)
        originals = self.root / 'harness/broad-native'
        originals.mkdir()
        for name in ('ip', 'iptables-restore', 'ip6tables-restore'):
            executable = self.root / 'usr/sbin' / name
            original = originals / name
            executable.rename(original)
            action = ('case " $* " in *" rule add "*|*" rule del "*|*" rule replace "*|*" rule flush "*) observe=1;; esac\n'
                      if name == 'ip' else 'observe=1\n')
            executable.write_text('#!/bin/sh\n' +
                f'/harness/broad-native/{name} "$@"\nresult=$?\nobserve=0\n' + action +
                'if [ "$BROAD_BARRIER" = 1 ] && [ "$observe" = 1 ]; then\n' +
                f'  printf "%s %s %s\\n" {name} "$result" "$*" > /tmp/broad-events\n' +
                '  IFS= read -r ack < /tmp/broad-acks\nfi\nexit "$result"\n')
            executable.chmod(0o755)

    def observed_refresh(self, label, sdn=False, failure=False):
        events = os.open(self.root / 'tmp/broad-events', os.O_RDWR | os.O_NONBLOCK)
        acks = os.open(self.root / 'tmp/broad-acks', os.O_RDWR | os.O_NONBLOCK)
        command = (['/harness/broad-sdn-driver', '1'] if sdn else ['/harness/deferred-firewall-driver', 'refresh'])
        process = subprocess.Popen(['chroot', str(self.root), '/qemu', *command],
                                   env=dict(self.env, BROAD_BARRIER='1'), stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        pending = b''
        count = 0
        deadline = time.monotonic() + 240
        try:
            while process.poll() is None:
                assert time.monotonic() < deadline, 'Broad quarantine refresh timed out'
                if not select.select([events], [], [], 0.1)[0]:
                    continue
                pending += os.read(events, 8192)
                while b'\n' in pending:
                    event, pending = pending.split(b'\n', 1)
                    count += 1
                    self.boundaries += 1
                    print('BOUNDARY', self.boundaries, event.decode(), flush=True)
                    try:
                        # The guest is already protected before expansion.
                        # Every real mutation must retain that protection.
                        for family in (4, 6):
                            self.data('expansion boundary retains guest ' + str(family), 'br1', family, None)
                        if not sdn or failure:
                            self.dns('protected boundary retains guest DNS', 'br1', 4, False)
                            self.dns('protected boundary retains guest IPv6 DNS', 'br1', 6, False)
                        self.router('expansion boundary preserves router DNS', 6)
                    finally:
                        os.write(acks, b'continue\n')
            output = process.communicate(timeout=2)[0].decode()
            assert count > 0, ('No actual mutations observed; fixture setup failed', output)
            self.require(label, bool(process.returncode) == failure, output)
            self.require('transparent wrapper observed actual mutations', count > 0)
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate(timeout=2)
            os.close(events)
            os.close(acks)

    def data(self, label, iface, family, expected, suffix=20, destination=None):
        source = (('192.0.2.' if iface == 'br0' else '198.51.100.') + str(suffix)
                  if family == 4 else ('2001:db8:1::' if iface == 'br0' else '2001:db8:3::') + str(suffix))
        destination = destination or ('203.0.113.99' if family == 4 else '2001:db8:ffff::99')
        self.check(label, lambda: self.packet(source, destination, expected, iface=iface))

    def service(self, label, iface, family, port, tcp, allowed, suffix=20):
        address = self.ADDRESSES[iface][0 if family == 4 else 1]
        source = (('192.0.2.' if iface == 'br0' else '198.51.100.') + str(suffix)
                  if family == 4 else ('2001:db8:1::' if iface == 'br0' else '2001:db8:3::') + str(suffix))
        af = socket.AF_INET if family == 4 else socket.AF_INET6
        listener = self.listeners[iface, af, port, tcp]
        peer = self.peers[iface]

        def probe():
            for sock in ([peer] if tcp else [peer, listener]):
                while select.select([sock], [], [], 0)[0]:
                    sock.recv(65535)
            net.ip('-4' if family == 4 else '-6', 'neigh', 'replace', source,
                   'lladdr', '02:00:00:00:00:10', 'dev', iface)
            self.packet(source, address, None, iface=iface, port=port, tcp=tcp)
            received = False
            if tcp:
                while select.select([peer], [], [], 0)[0]:
                    frame = peer.recv(65535)
                    offset = 34 if family == 4 else 54
                    if len(frame) < offset + 20 or frame[12:14] != (b'\x08\x00' if family == 4 else b'\x86\xdd'):
                        continue
                    if frame[23 if family == 4 else 20] != 6:
                        continue
                    target = socket.inet_ntop(af, frame[30:34] if family == 4 else frame[38:54])
                    received |= (target == source and struct.unpack('!HH', frame[offset:offset + 4]) ==
                                 (port, 30000 + self.sequence) and frame[offset + 13] & 0x12 == 0x12)
            elif select.select([listener], [], [], 0)[0]:
                payload, sender = listener.recvfrom(65535)
                received = sender[0] == source and payload == f'RTBE90U-PACKET-{self.sequence}'.encode()
            assert received == allowed, (source, address, port, tcp, allowed, received)
        self.check(label, probe)

    def dns(self, label, iface, family, allowed, suffix=20):
        for tcp in (False, True):
            self.service(label + (' TCP' if tcp else ' UDP'), iface, family, 53, tcp, allowed, suffix)

    def external_dns(self, label, iface, family, expected, suffix=20):
        source = (('192.0.2.' if iface == 'br0' else '198.51.100.') + str(suffix)
                  if family == 4 else ('2001:db8:1::' if iface == 'br0' else '2001:db8:3::') + str(suffix))
        destination = '203.0.113.53' if family == 4 else '2001:db8:ffff::53'
        for tcp in (False, True):
            self.check(label + (' TCP' if tcp else ' UDP'),
                       lambda tcp=tcp: self.packet(source, destination, expected, port=53, tcp=tcp, iface=iface))

    def router(self, label, family, port=53):
        def probe():
            for sock in self.sockets.values():
                while select.select([sock], [], [], 0)[0]:
                    sock.recv(65535)
            self.sequence += 1
            payload = f'BROAD-ROUTER-{self.sequence}'.encode()
            af = socket.AF_INET if family == 4 else socket.AF_INET6
            with socket.socket(af, socket.SOCK_DGRAM) as sender:
                # Use an address from the quarantined guest: an accidental
                # source-wide rule must not silently break router-origin work.
                sender.bind((self.ADDRESSES['br1'][0 if family == 4 else 1], 0))
                sender.sendto(payload, ('203.0.113.53' if family == 4 else '2001:db8:ffff::53', port))
            found = []
            deadline = time.monotonic() + 1
            while not found and time.monotonic() < deadline:
                ready, _, _ = select.select(list(self.sockets.values()), [], [], max(0, deadline - time.monotonic()))
                for sock in ready:
                    if payload in sock.recv(65535):
                        found.append(sock.getsockname()[0])
            assert found == ['wan0'], found
        self.check(label, probe)

    def local_controls(self, label):
        for family in (4, 6):
            self.service(label + f' IPv{family} management', 'br1', family, 80, True, True)
            self.service(label + f' IPv{family} DHCP port', 'br1', family, 67 if family == 4 else 547, False, True)
            self.router(label + f' IPv{family} router DNS', family)

    def filters(self):
        for family in (4, 6):
            text = firewall.invoke(self, 'filter' + str(family))
            net.run('iptables-nft-restore' if family == 4 else 'ip6tables-nft-restore', input=text)

    def close(self):
        for sock in list(self.sockets.values()) + list(self.peers.values()) + list(self.listeners.values()):
            sock.close()


def exercise(f):
    f.reset('<0>bad>>5>br1<1>198.51.100.20>>0')
    f.refresh('disabled malformed and valid WAN policies are inert')
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('initial forwarding control ' + iface + str(family), iface, family, 'wan0')
            f.dns('initial shared DNS control ' + iface + str(family), iface, family, True)

    f.reset('<1>>>bad>br1')
    # Early assigned routing and explicit Director/WAN policy routes must not
    # bypass the newly approved whole-network quarantine on this same guest.
    f.rules('<1>Independent VPN>198.51.100.50>>WGC1<1>Explicit WAN>198.51.100.51>>WAN')
    for family in ('-4', '-6'):
        net.ip(family, 'rule', 'add', 'iif', 'br1', 'table', 'wgc1', 'priority', '1003')
    f.refresh('malformed known interface activates broad quarantine')
    for family in (4, 6):
        f.data('assigned lookup cannot bypass broad guest ' + str(family), 'br1', family, None)
        f.data('outside guest still forwards ' + str(family), 'br0', family, 'wan0')
        f.dns('broad guest shared DNS ' + str(family), 'br1', family, False)
        f.dns('outside guest shared DNS ' + str(family), 'br0', family, True)
        f.external_dns('broad guest external DNS ' + str(family), 'br1', family, None)
        net.ip('-4' if family == 4 else '-6', 'rule', 'del', 'iif', 'br1', 'table', 'wgc1', 'priority', '1003')
    for suffix, table, priority in ((50, 'wgc1', '12010'), (51, 'main', '10010')):
        net.ip('rule', 'add', 'from', '198.51.100.' + str(suffix), 'table', table, 'priority', priority)
        f.data('explicit lookup cannot bypass broad guest ' + str(suffix), 'br1', 4, None, suffix=suffix)
        f.dns('explicit DNS exception cannot bypass broad guest ' + str(suffix), 'br1', 4, False, suffix=suffix)
    f.local_controls('known guest quarantine preserves')
    f.filters()
    for family in (4, 6):
        f.data('production filter replacement retains broad guest ' + str(family), 'br1', family, None)
        f.dns('production filter replacement retains guest DNS ' + str(family), 'br1', family, False)
    f.local_controls('filter replacement preserves')

    f.reset('<1>198.51.100.20>>5')
    f.refresh('IPv4 source activates known-network IPv6 fallback')
    f.data('precise IPv4 source blocked', 'br1', 4, None)
    f.data('other IPv4 source remains usable', 'br1', 4, 'wan0', suffix=21)
    f.data('unknown device IPv6 blocked across known guest', 'br1', 6, None, suffix=77)
    f.data('outside known guest IPv6 remains usable', 'br0', 6, 'wan0')
    f.dns('known guest IPv6 shared DNS blocked', 'br1', 6, False, suffix=77)
    f.dns('outside known guest IPv6 DNS usable', 'br0', 6, True)

    f.reset('<1>192.0.2.20>>5>br1')
    f.refresh('source and interface branches both contribute IPv6 uncertainty')
    for iface in ('br0', 'br1'):
        f.data('source/interface network union ' + iface, iface, 6, None, suffix=77)

    f.reset('<1>10.99.0.20>>5')
    f.refresh('unknown IPv4 source activates all-network IPv6 fallback')
    for iface in ('br0', 'br1'):
        f.data('unknown IPv4 identity blocks possible IPv6 network ' + iface, iface, 6, None)
        f.data('unknown IPv4 identity retains precise IPv4 scope ' + iface, iface, 4, 'wan0')
        f.dns('unknown IPv4 identity shared IPv6 DNS ' + iface, iface, 6, False)

    f.reset('<1>bad>>5>br1')
    f.refresh('invalid nonempty source remains unknown despite separate valid interface')
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('unknown source branch cannot be narrowed by interface ' + iface + str(family),
                   iface, family, None)

    f.reset('<1>bad>>5')
    f.refresh('unidentifiable active policy activates both families on all networks')
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('unidentifiable policy blocks ' + iface + str(family), iface, family, None)
            f.dns('unidentifiable policy shared DNS ' + iface + str(family), iface, family, False)
    f.local_controls('all-network quarantine preserves')
    before = [net.ip(family, '-j', 'rule').stdout for family in ('-4', '-6')]
    for _ in range(2):
        f.refresh('repeated broad refresh succeeds')
    after = [net.ip(family, '-j', 'rule').stdout for family in ('-4', '-6')]
    f.require('repeated settled broad refresh does not accumulate rules', before == after)
    f.env['vpnc_dev_policy_list'] = '<0>bad>>5'
    f.refresh('explicit disable releases previously broad quarantine')
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('disable restores ' + iface + str(family), iface, family, 'wan0')
            f.dns('disable restores shared DNS ' + iface + str(family), iface, family, True)

    f.reset('<1>>203.0.113.99>5')
    f.refresh('destination-only policy quarantines shared DNS without inventing a device')
    for iface in ('br0', 'br1'):
        f.data('destination-only matching data blocked ' + iface, iface, 4, None)
        f.data('destination-only unrelated IPv4 usable ' + iface, iface, 4, 'wan0', destination='203.0.113.100')
        f.data('destination-only does not infer IPv6 traffic scope ' + iface, iface, 6, 'wan0')
        for family in (4, 6):
            f.dns('destination-only shared resolver blocked ' + iface + str(family), iface, family, False)
    f.env['vpnc_dev_policy_list'] = '<1>>203.0.113.99>0'
    f.refresh('valid WAN correction releases destination DNS uncertainty')
    f.dns('valid WAN correction restores DNS', 'br1', 6, True)

    for field, value, all_networks in ((6, 'bad', False), (2, 'bad', False), (0, 'bad', True)):
        f.reset('')
        rows = [row.split('>') for row in f.env['sdn_rl'].split('<') if row]
        rows[1][6] = '5'
        rows[1][field] = value
        f.env['sdn_rl'] = ''.join('<' + '>'.join(row) for row in rows)
        f.rules('<1>Explicit WAN>198.51.100.51>>WAN')
        net.ip('rule', 'add', 'from', '198.51.100.51', 'table', 'main', 'priority', '10010')
        f.refresh('raw malformed SDN field activates approved fallback ' + str(field))
        for family in (4, 6):
            f.data('raw malformed SDN blocks guest ' + str(field) + '/' + str(family), 'br1', family, None)
            f.data('raw malformed SDN outside scope ' + str(field) + '/' + str(family),
                   'br0', family, None if all_networks else 'wan0')
        f.data('raw malformed SDN cannot use Director WAN ' + str(field), 'br1', 4, None, suffix=51)
        rows[1][0], rows[1][2], rows[1][6] = '1', '0', 'bad'
        f.env['sdn_rl'] = ''.join('<' + '>'.join(row) for row in rows)
        f.refresh('valid disabled SDN releases malformed fallback ' + str(field))
        f.data('disabled malformed SDN guest recovers ' + str(field), 'br1', 6, 'wan0')

    f.reset('')
    rows = [row.split('>') for row in f.env['sdn_rl'].split('<') if row]
    rows[1][0], rows[1][2], rows[1][6] = 'bad', 'bad', '5'
    f.env['sdn_rl'] = ''.join('<' + '>'.join(row) for row in rows)
    f.refresh('combined malformed identity and enable protects configured ingresses')
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('combined malformed fields cannot hide configured ingress ' + iface + str(family),
                   iface, family, None)

    f.reset('<1>>>bad>br0<1>>>bad>br1')
    f.refresh('two separately identifiable malformed networks protected')
    f.env['vpnc_dev_policy_list'] = '<1>>>bad>br0<1>198.51.100.20>>0'
    f.refresh('corrected network releases while malformed neighbor remains')
    for family in (4, 6):
        f.data('malformed neighbor stays blocked ' + str(family), 'br0', family, None)
        f.data('corrected guest recovers ' + str(family), 'br1', family, 'wan0')
        f.dns('corrected guest resolver recovers ' + str(family), 'br1', family, True)

    f.reset('<1>>>bad>br1')
    # Fresh activation must protect even if a custom guard makes route cleanup
    # unsafe. The real helper must publish effective filtering before refusing.
    foreign = ['from', 'all', 'iif', 'br1', 'fwmark', '0x40', 'priority', '90', 'protocol', '240', 'prohibit']
    net.ip('rule', 'add', *foreign)
    f.rules('<1>Independent VPN>198.51.100.20>>WGC1')
    net.ip('rule', 'add', 'from', '198.51.100.20', 'table', 'wgc1', 'priority', '12010')
    foreign_before = net.ip('-j', 'rule').stdout
    f.data('fresh failure control has populated tunnel', 'br1', 4, 'wgc1')
    f.refresh('foreign rule causes explicit cleanup refusal', failure=True)
    f.require('refused route cleanup preserves original custom rules', net.ip('-j', 'rule').stdout == foreign_before)
    for family in (4, 6):
        f.data('fresh refusal still blocks broad traffic ' + str(family), 'br1', family, None)
        f.dns('fresh refusal still blocks broad DNS ' + str(family), 'br1', family, False)
    f.local_controls('fresh refusal preserves')
    net.ip('rule', 'del', *foreign)
    f.refresh('retry after foreign conflict removal succeeds')
    f.data('retry keeps guest protection', 'br1', 4, None)
    f.env['vpnc_dev_policy_list'] = '<1>198.51.100.20>>0'
    f.refresh('correction after retry releases quarantine')
    f.data('correction restores independent Director tunnel', 'br1', 4, 'wgc1')
    f.dns('correction restores guest router DNS', 'br1', 4, True)

    f.reset('<1>>>bad>br1')
    f.refresh('prior guest protection established before changed-plan failure')
    net.ip('rule', 'add', *foreign)
    f.env['vpnc_dev_policy_list'] = '<1>>>bad>br0'
    f.refresh('new plan refuses foreign cleanup without releasing old protection', failure=True)
    for iface in ('br0', 'br1'):
        for family in (4, 6):
            f.data('failed plan retains old plus new network ' + iface + str(family), iface, family, None)
            f.dns('failed plan retains old plus new DNS ' + iface + str(family), iface, family, False)
    net.ip('rule', 'del', *foreign)
    f.refresh('changed-plan retry reconciles old and new scopes')
    for family in (4, 6):
        f.data('retry retains current malformed network ' + str(family), 'br0', family, None)
        f.data('retry releases corrected earlier network ' + str(family), 'br1', family, 'wan0')
        f.dns('retry releases corrected earlier DNS ' + str(family), 'br1', family, True)

    orphaned_lookup_case(f)
    invalid_ingress_case(f)
    sdn_correction_case(f)
    absent_default_sdn_case(f)

    f.reset('<1>>>bad>br1')
    f.refresh('guest protection established before broad expansion')
    f.install_barrier()
    f.env['vpnc_dev_policy_list'] = '<1>bad>>5'
    f.observed_refresh('broad expansion completes under packet observations')
    for family in (4, 6):
        f.data('expanded scope blocks newly affected LAN ' + str(family), 'br0', family, None)

    print(f'Broad quarantine summary: {f.checks} checks; {f.boundaries} command boundaries; {len(f.failures)} failures', flush=True)
    if f.regression:
        assert f.failures, 'Preserved baseline unexpectedly passed every new behavior'
        print('Baseline gaps:', json.dumps(f.failures), flush=True)
    else:
        assert not f.failures, f.failures


def orphaned_lookup_case(f):
    f.reset('<1>>>bad>br1')
    net.ip('route', 'replace', 'default', 'via', '10.3.0.2', 'dev', 'wgc1', 'table', '5')
    selector = ['from', '198.51.100.20', 'table', '5', 'priority', '100']
    net.ip('rule', 'add', *selector)
    f.data('orphan control uses populated earlier stock lookup', 'br1', 4, 'wgc1')
    f.refresh('broad guest quarantine covers unmatched earlier stock lookup')
    f.data('broad guest blocks unmatched earlier stock lookup', 'br1', 4, None)
    before = [rule for rule in json.loads(net.ip('-j', 'rule').stdout) if rule['priority'] == 100]
    f.env['vpnc_dev_policy_list'] = '<1>198.51.100.21>>5'
    f.refresh('changed source cannot release broad protection over ambiguous orphan', failure=True)
    after = [rule for rule in json.loads(net.ip('-j', 'rule').stdout) if rule['priority'] == 100]
    f.require('ambiguous orphan lookup is preserved for explicit resolution', before == after and len(after) == 1)
    f.data('orphan source stays blocked after source correction alone', 'br1', 4, None)
    f.dns('orphan source DNS stays blocked until conflict resolved', 'br1', 4, False)
    f.data('new intended source also protected during conflict', 'br1', 4, None, suffix=21)
    net.ip('rule', 'del', *selector)
    f.refresh('explicit orphan removal permits retry to release old broad scope')
    f.data('orphan removal restores corrected old source', 'br1', 4, 'wan0')
    f.dns('orphan removal restores corrected old source DNS', 'br1', 4, True)
    f.data('orphan removal retains current precise source guard', 'br1', 4, None, suffix=21)
    f.data('orphan removal retains current IPv6 network uncertainty', 'br1', 6, None)


def invalid_ingress_case(f):
    f.reset('<1>bad>>5')
    f.refresh('known guest protected before malformed interface replacement')
    original = f.env['subnet_rl']
    f.env['subnet_rl'] = original.replace('>br1>', '>bad name>')
    f.refresh('malformed ingress refuses release after protecting identifiable networks', failure=True)
    for family in (4, 6):
        f.data('malformed ingress retains earlier real guest guard ' + str(family), 'br1', family, None)
        f.dns('malformed ingress retains earlier real guest DNS ' + str(family), 'br1', family, False)
        f.data('malformed ingress still protects known LAN ' + str(family), 'br0', family, None)
    f.env['subnet_rl'] = original
    f.env['vpnc_dev_policy_list'] = '<1>198.51.100.20>>0'
    f.refresh('corrected interface and WAN policy permit quarantine release')
    for family in (4, 6):
        f.data('corrected interface releases guest ' + str(family), 'br1', family, 'wan0')
        f.data('corrected interface releases main LAN ' + str(family), 'br0', family, 'wan0')


def sdn_correction_case(f):
    f.reset('<1>>>bad>br1')
    f.refresh('SDN guest quarantine established before correcting assignment')
    net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'table', '1', 'priority', '1003')
    f.env['qca_merlin_vpn_migrated'] = '1'
    f.sdn(6)
    f.env['vpnc_dev_policy_list'] = ''
    f.env['sdn_rl'] = ''.join('<' + row + '>0' * (23 - len(row.split('>')))
                              for row in f.env['sdn_rl'].split('<') if row)
    # The corrected VPN has no route yet. SDN must publish its IPv4/IPv6
    # protection before releasing the old network quarantine.
    net.ip('route', 'flush', 'table', 'ovpnc1')
    f.install_barrier()
    executable = f.root / 'harness/broad-native/ip'
    original = f.root / 'harness/broad-fault/ip'
    original.parent.mkdir()
    executable.rename(original)
    executable.write_text('#!/bin/sh\n'
        'case " $* " in *" rule add "*"table 6 "*)\n'
        '  printf "injected SDN lookup install failure\\n" > /tmp/broad-sdn-fault-hit\n'
        '  exit 1;; esac\nexec /harness/broad-fault/ip "$@"\n')
    executable.chmod(0o755)
    try:
        f.observed_refresh('failed actual SDN owner retains staged network quarantine', sdn=True, failure=True)
        f.require('SDN failure injection reached actual lookup installation',
                  (f.root / 'tmp/broad-sdn-fault-hit').is_file())
    finally:
        executable.unlink()
        original.rename(executable)
    for family in (4, 6):
        f.data('failed SDN owner retains guest protection ' + str(family), 'br1', family, None)
    f.observed_refresh('actual SDN owner retry safely releases staged quarantine', sdn=True)
    f.data('corrected VPN with empty table remains blocked', 'br1', 4, None)
    f.data('corrected VPN keeps IPv6 interface guard', 'br1', 6, None)
    net.ip('route', 'replace', 'default', 'via', '10.2.0.2', 'dev', 'tun11', 'table', 'ovpnc1')
    f.data('corrected fixed VPN forwards after post-owner quarantine release', 'br1', 4, 'tun11')
    # A separate WAN correction retires an exact SDN-owned old lookup. It does
    # not require deleting an indistinguishable custom priority100 source rule.
    f.reset('<1>>>bad>br1')
    f.refresh('SDN guest quarantine established before WAN correction')
    net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'table', '1', 'priority', '1003')
    f.env.update(qca_merlin_vpn_migrated='1', vpnc_dev_policy_list='')
    result = net.run('chroot', str(f.root), '/qemu', '/harness/broad-sdn-driver', '1', env=f.env, check=False)
    f.require('actual SDN WAN correction retires its exact old lookup', result.returncode == 0,
              (result.stdout, result.stderr))
    for family in (4, 6):
        f.data('actual SDN WAN correction restores guest ' + str(family), 'br1', family, 'wan0')
        f.dns('actual SDN WAN correction restores shared DNS ' + str(family), 'br1', family, True)
    # The new owner may have no old lookup, or already have its new lookup but
    # still lack the kill switch. Both must retain quarantine until the actual
    # owner publishes protection. This is a synthetic transaction boundary,
    # not a simulation of the complete migration service lifecycle.
    for matching_lookup in (False, True):
        f.reset('<1>>>bad>br1')
        f.refresh('establish quarantine before incomplete corrected SDN ' + str(matching_lookup))
        f.env.update(qca_merlin_vpn_migrated='1', vpnc_dev_policy_list='')
        f.sdn(6)
        f.env['sdn_rl'] = ''.join('<' + row + '>0' * (23 - len(row.split('>')))
                                  for row in f.env['sdn_rl'].split('<') if row)
        net.ip('route', 'flush', 'table', 'ovpnc1')
        if matching_lookup:
            net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'table', '6', 'priority', '1018')
        f.observed_refresh('actual owner fills missing lookup or guard before release ' + str(matching_lookup), sdn=True)
        for family in (4, 6):
            f.data('incomplete-owner handoff remains blocked ' + str(matching_lookup) + '/' + str(family),
                   'br1', family, None)
        net.ip('route', 'replace', 'default', 'via', '10.2.0.2', 'dev', 'tun11', 'table', 'ovpnc1')
        f.data('incomplete-owner handoff eventually restores selected VPN ' + str(matching_lookup), 'br1', 4, 'tun11')


def absent_default_sdn_case(f):
    f.reset('')
    f.env.update(sdn_rl='', vpnc_default_wan='bad')
    f.refresh('missing SDN list still quarantines malformed known main LAN')
    f.env.update(qca_merlin_vpn_migrated='1', vpnc_default_wan='6')
    f.refresh('missing SDN list retains main-LAN staging until normal VPN owner is ready')
    # No actual SDN record exists for the normal handler to activate here.
    # A new fixed target alone must not release protection without its owner.
    for family in (4, 6):
        f.data('missing default SDN retains traffic protection ' + str(family), 'br0', family, None)
        f.dns('missing default SDN retains DNS protection ' + str(family), 'br0', family, False)
        f.data('missing default SDN leaves unrelated guest usable ' + str(family), 'br1', family, 'wan0')
    f.env['vpnc_default_wan'] = '0'
    f.refresh('explicit WAN default releases missing-record staging')
    for family in (4, 6):
        f.data('WAN default restores main LAN ' + str(family), 'br0', family, 'wan0')
        f.dns('WAN default restores main-LAN DNS ' + str(family), 'br0', family, True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--expect-regression', action='store_true')
    args = parser.parse_args()
    net.prepare(args.root)
    net.setup_links()
    net.ip('-6', 'addr', 'add', '2001:db8:4::1/64', 'dev', 'wgc1', 'nodad')
    net.ip('-6', 'neigh', 'add', '2001:db8:4::2', 'lladdr', '02:00:00:00:00:02', 'dev', 'wgc1')
    net.ip('-6', 'route', 'add', 'default', 'via', '2001:db8:4::2', 'dev', 'wgc1', 'table', 'wgc1')
    fixture = Fixture(args.root, args.expect_regression)
    firewall.compile_driver(fixture)
    compile_sdn_driver(args.root)
    try:
        exercise(fixture)
    finally:
        fixture.close()


if __name__ == '__main__':
    main()
