#!/usr/bin/env python3
"""Compiled SDN refresh, real VPN routing, and isolated IPv4 packet forwarding."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import select
import signal
import socket
import subprocess
import time

spec = importlib.util.spec_from_file_location('network', '/tests/vpn-network-test.py')
net = importlib.util.module_from_spec(spec)
spec.loader.exec_module(net)


def compile_driver(root):
    obj = Path('/work/release/src/router/rc/sdn.o')
    print('sdn.o sha256:', hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = root / 'harness/sdn-exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; };\n')
    result = net.run('aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
                    '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                    '-Wl,--dynamic-list,' + str(exports),
                    '-I/work/release/src-qca-ipq53xx/include',
                    '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
                    '/tests/sdn-refresh-driver.c', str(obj), '-L/firmware/usr/lib',
                    '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib', '-lovpn', '-lshared', '-lnvram',
                    '-o', str(root / 'harness/sdn-driver'))
    if result.stderr:
        print(result.stderr, end='', flush=True)


def router_packet(fixture):
    for sock in fixture.sockets.values():
        while select.select([sock], [], [], 0)[0]:
            sock.recv(65535)
    fixture.sequence += 1
    payload = ('RTBE90U-SDN-ENDPOINT-%d' % fixture.sequence).encode()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
        sender.bind(('10.1.0.1', 0))
        sender.sendto(payload, ('203.0.113.99', 443))
    seen = []
    deadline = time.monotonic() + 1
    while time.monotonic() < deadline:
        ready, _, _ = select.select(list(fixture.sockets.values()), [], [], max(0, deadline - time.monotonic()))
        for sock in ready:
            if payload in sock.recv(65535):
                seen.append(sock.getsockname()[0])
        if seen:
            break
    assert seen == ['wan0'], ('Router endpoint packet lost or redirected', seen)


def install_barrier(root):
    """Execute real ip commands, then pause mutations for parent packet probes."""
    (root / 'harness/native').mkdir()
    (root / 'tmp/var/lock').mkdir(exist_ok=True)
    (root / 'usr/sbin/ip').rename(root / 'harness/native/ip')
    net.copy_native(root, '/bin/busybox', '/harness/timeout')
    wrapper = root / 'usr/sbin/ip'
    wrapper.write_text('''#!/bin/sh
fault=
if [ "$1:$2" = rule:add ]; then
    case " $* " in
    *" prohibit "*) test "$SDN_IP_FAULT" != guard-add || fault=guard-add ;;
    *" table "*) test "$SDN_IP_FAULT" != lookup-add || fault=lookup-add ;;
    esac
fi
if [ -n "$fault" ]; then
    printf '%s\\n' "$fault" >> /tmp/sdn-injected-fault
    status=2
else
    /harness/native/ip "$@"
    status=$?
fi
if [ "$SDN_COMMAND_BARRIER" = 1 ]; then
    case "$1:$2" in
    rule:add|rule:del|rule:delete|rule:replace)
        /harness/timeout 15 /bin/sh -c '
            printf "%s\\t%s\\n" "$1" "$2" > /tmp/sdn-command-events
            IFS= read -r ack < /tmp/sdn-command-acks
            test "$ack" = continue
        ' barrier "$status" "$*" || exit 124
        ;;
    esac
fi
exit "$status"
''')
    wrapper.chmod(0o755)
    for name in ('sdn-command-events', 'sdn-command-acks'):
        os.mkfifo(root / 'tmp' / name)


def refresh(fixture, index, observe=None, fault=None):
    command = ['chroot', str(fixture.root), '/qemu', '/harness/sdn-driver', str(index)]
    if fault:
        command.append('fail')
    if observe is None:
        result = net.run(*command, env=fixture.env)
        print(result.stdout + result.stderr, end='', flush=True)
        return
    events = os.open(fixture.root / 'tmp/sdn-command-events', os.O_RDWR | os.O_NONBLOCK)
    acks = os.open(fixture.root / 'tmp/sdn-command-acks', os.O_RDWR | os.O_NONBLOCK)
    process = subprocess.Popen(command, env=dict(fixture.env, SDN_COMMAND_BARRIER='1', SDN_IP_FAULT=fault or ''),
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True)
    pending = b''
    mutations = 0
    deadline = time.monotonic() + 120
    try:
        while process.poll() is None:
            assert time.monotonic() < deadline, 'SDN refresh timed out'
            if not select.select([events], [], [], 0.1)[0]:
                continue
            pending += os.read(events, 8192)
            while b'\n' in pending:
                event, pending = pending.split(b'\n', 1)
                mutations += 1
                try:
                    observe(mutations, event.decode())
                finally:
                    os.write(acks, b'continue\n')
        output = process.communicate(timeout=2)[0].decode()
        print(output, end='', flush=True)
        assert process.returncode == 0, ('SDN driver failed', process.returncode, output)
        assert mutations > 0, 'No real routing mutations observed'
    finally:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate(timeout=2)
        os.close(events)
        os.close(acks)


def exercise(fixture, regression):
    failures = []
    checks = 0
    boundaries = 0

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

    fixture.env.update(ipv6_service='disabled', vpnc_default_wan='0')
    for proto, index, table, interface, network, enforce, enabled, target, other_table, other_iface in (
            ('ovpn', 6, 'ovpnc1', 'tun11', 2, 'vpn_client1_enforce', 'vpn_clientx_eas', 'OVPN1', 'wgc1', 'wgc1'),
            ('wg', 1, 'wgc1', 'wgc1', 3, 'wgc1_enforce', 'wgc1_enable', 'WGC1', 'ovpnc1', 'tun11')):
        for seed in ('explicit', 'inferred'):
            label = proto + '/' + seed
            # Clear only this isolated container's synthetic non-default rules.
            for rule in json.loads(net.ip('-j', 'rule').stdout):
                if rule['priority'] not in (0, 32766, 32767):
                    net.ip('rule', 'del', 'priority', str(rule['priority']))
            fixture.env.update(vpn_client1_enforce='0', wgc1_enforce='0',
                               vpn_clientx_eas='1,', wgc1_enable='1')
            fixture.env[enforce] = '1'
            fixture.sdn(index)
            fixture.rules('<1>Director>192.0.2.20>>%s<1>WAN exception>198.51.100.11>>WAN' % target)
            fixture.call('rules', proto)
            refresh(fixture, index)
            # Both the SDN-specific and ordinary startup/vpnrouting helper
            # paths must produce protection that later SDN refresh preserves.
            fixture.call('kill', proto, *(['br1'] if seed == 'explicit' else []))
            fixture.packet('198.51.100.10', '203.0.113.99', interface, iface='br1')
            fixture.packet('192.0.2.20', '203.0.113.99', interface)
            # These distinct custom lookups are unrelated to SDN ownership.
            net.ip('rule', 'add', 'iif', 'br1', 'to', '203.0.113.77', 'table', 'main', 'priority', '900')
            net.ip('rule', 'add', 'iif', 'br0', 'to', '203.0.113.88', 'table', other_table, 'priority', '901')
            net.ip('route', 'flush', 'table', table)

            def probes(phase, protect=True):
                if protect:
                    check(phase + ' SDN guard', lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1'))
                check(phase + ' Director guard', lambda: fixture.packet('192.0.2.20', '203.0.113.99', None))
                check(phase + ' WAN exception', lambda: fixture.packet('198.51.100.11', '203.0.113.99', 'wan0', iface='br1'))
                check(phase + ' unprotected LAN', lambda: fixture.packet('192.0.2.10', '203.0.113.99', 'wan0'))
                check(phase + ' custom SDN lookup', lambda: fixture.packet('198.51.100.10', '203.0.113.77', 'wan0', iface='br1'))
                check(phase + ' unrelated interface lookup', lambda: fixture.packet('192.0.2.10', '203.0.113.88', other_iface))
                check(phase + ' router endpoint', lambda: router_packet(fixture))

            def observed_refresh(phase, protect=True, fault=None):
                def observe(number, command):
                    nonlocal boundaries
                    boundaries += 1
                    boundary = '%s mutation %d' % (phase, number)
                    print('OBSERVE', boundary, command, flush=True)
                    probes(boundary, protect)
                refresh(fixture, index, observe, fault)

            probes(label + ' seeded')
            print('PASS', label, 'populated-table forwarding and seeded empty-table controls', flush=True)
            previous = None
            for attempt in (1, 2):
                phase = '%s refresh %d' % (label, attempt)
                observed_refresh(phase)
                probes(phase + ' complete')
                check(phase + ' TCP SDN guard', lambda: fixture.packet('198.51.100.10', '203.0.113.99', None, iface='br1', tcp=True))
                current = json.loads(net.ip('-j', 'rule').stdout)
                if previous is not None:
                    assert current == previous, ('Repeated refresh accumulated rules', previous, current)
                previous = current
                print('COMPLETE', phase, 'rules:', json.dumps(current), flush=True)
            print('CHECKED' if regression else 'PASS', label,
                  'repeated refresh preserves guards and unrelated rules at every mutation', flush=True)

            for setting in (enforce, enabled):
                fixture.env[setting] = '' if setting == 'vpn_clientx_eas' else '0'
                observed_refresh(label + ' disable ' + setting, protect=False)
                probes(label + ' disabled ' + setting, protect=False)
                check(label + ' no stale SDN guard after ' + setting,
                      lambda: fixture.packet('198.51.100.10', '203.0.113.99', 'wan0', iface='br1'))
                fixture.env[setting] = '1,' if setting == 'vpn_clientx_eas' else '1'
                observed_refresh(label + ' restore ' + setting)
                probes(label + ' restored ' + setting)
            print('CHECKED' if regression else 'PASS', label,
                  'enforcement/client toggles release and restore the SDN guard', flush=True)
            if seed == 'inferred' and not regression:
                injected = fixture.root / 'tmp/sdn-injected-fault'
                # Preserve a legacy unit-priority interface guard when the new
                # owned guard cannot be installed. It is intentionally distinct
                # from the source-scoped Director guard at the same priority.
                legacy_priority = '12210' if proto == 'ovpn' else '12215'
                net.ip('rule', 'add', 'from', 'all', 'iif', 'br1', 'priority', legacy_priority, 'prohibit')
                net.ip('rule', 'del', 'from', 'all', 'iif', 'br1', 'priority', '12220', 'prohibit')
                before = json.loads(net.ip('-j', 'rule').stdout)
                observed_refresh(label + ' failed guard add', fault='guard-add')
                assert injected.read_text() == 'guard-add\n'
                assert json.loads(net.ip('-j', 'rule').stdout) == before, 'Guard-add failure changed existing rules'
                injected.unlink()
                observed_refresh(label + ' guard add retry')
                net.ip('rule', 'del', 'from', 'all', 'iif', 'br1', 'priority', legacy_priority, 'prohibit')
                probes(label + ' guard add recovered')
                observed_refresh(label + ' failed lookup add', fault='lookup-add')
                assert injected.read_text() == 'lookup-add\n'
                injected.unlink()
                probes(label + ' failed lookup remains guarded')
                observed_refresh(label + ' lookup add retry')
                probes(label + ' lookup add recovered')
                print('PASS', label, 'guard/lookup command failures retain protection and allow retry', flush=True)
            net.ip('route', 'add', 'default', 'via', '10.%d.0.2' % network,
                   'dev', interface, 'onlink', 'table', table)
            check(label + ' restored tunnel forwarding',
                  lambda: fixture.packet('198.51.100.10', '203.0.113.99', interface, iface='br1'))
    if failures:
        raise SystemExit('Reproduced SDN refresh failures: %d/%d packet checks; %d mutation boundaries' %
                         (len(failures), checks, boundaries))
    assert not regression, 'Frozen image unexpectedly passed the regression'
    print('SDN refresh fixture passed:', checks, 'packet checks;', boundaries, 'mutation boundaries', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--expect-regression', action='store_true')
    args = parser.parse_args()
    net.prepare(args.root)
    compile_driver(args.root)
    install_barrier(args.root)
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
