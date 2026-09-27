#!/usr/bin/env python3
"""Full firewall/service lifecycle with real native routing and netfilter."""
import argparse
import hashlib
import importlib.util
import os
from pathlib import Path
import re
import select
import signal
import subprocess
import time

spec = importlib.util.spec_from_file_location('lifecycle_broad', '/tests/broad-quarantine-test.py')
broad = importlib.util.module_from_spec(spec)
spec.loader.exec_module(broad)
net = broad.net


def compile_driver(root):
    objects = [Path('/work/release/src/router/rc') / (name + '.o')
               for name in ('firewall', 'firewall_sdn', 'services', 'sdn', 'vpnc_fusion',
                            'common', 'wireguard', 'rc_ipsec', 'wan', 'ppp', 'dnsfilter',
                            'pc', 'pc_reward', 'pc_tmp', 'ic', 'private')]
    for obj in objects:
        print(obj.name + ' sha256:', hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = root / 'harness/lifecycle-exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; '
                       'file_lock; file_unlock; _eval; };\n')
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
               '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--wrap=handle_sdn_feature',
               '-Wl,--dynamic-list,' + str(exports),
               '-I/work/release/src-qca-ipq53xx/include', '-I/work/release/src/router/shared',
               '-I/work/release/src/router/libovpn', '-I/work/release/src/router/rc',
               '/tests/firewall-lifecycle-driver.c', *map(str, objects), '-L/firmware/usr/lib',
               '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
               '-lovpn', '-lshared', '-lnvram', '-lcrypto', '-lssl', '-lletsencrypt', '-ldl',
               '-o', str(root / 'harness/lifecycle-driver')]
    first = net.run(*command, check=False)
    names = sorted(set(re.findall(r"undefined reference to `([^']+)'", first.stderr)))
    protected = re.compile(r'.*(?:amvpn|nvram|file_lock|file_unlock).*|start_firewall|'
                           r'start_default_filter|handle_notifications|handle_sdn_feature')
    assert names and all(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name) for name in names), first.stderr
    assert not [name for name in names if protected.fullmatch(name)], names
    definitions = []
    for directory in ('rc', 'shared', 'libovpn'):
        definitions += sorted(Path('/work/release/src/router', directory).glob('*.o'))
    symbols = net.run('aarch64-openwrt-linux-musl-readelf', '-sW', *map(str, definitions),
                      '/work/release/src/router/rc/rc').stdout
    functions = set(re.findall(r'^\s*\d+:\s+\w+\s+\d+\s+FUNC\s+\S+\s+\S+\s+\S+\s+(\w+)$',
                               symbols, re.MULTILINE))
    assert not set(names) - functions, ('Unresolved symbols lack production FUNC types', set(names) - functions)
    traps = root / 'harness/lifecycle-traps.c'
    traps.write_text('extern void fixture_unexpected(const char *);\n' + ''.join(
        'void %s(void) { fixture_unexpected("%s"); }\n' % (name, name) for name in names))
    net.run(*command, str(traps))
    print('Inactive dependencies guarded by fatal traps:', len(names), flush=True)


class Actors:
    def __init__(self, fixture):
        self.f = fixture
        self.pending = b''
        self.processes = {}
        self.acks = {}
        self.locks = {}
        events = fixture.root / 'tmp/lifecycle-events'
        os.mkfifo(events)
        self.events = os.open(events, os.O_RDWR | os.O_NONBLOCK)

    def start(self, actor, operation):
        path = self.f.root / ('tmp/lifecycle-ack-' + actor)
        os.mkfifo(path)
        self.acks[actor] = os.open(path, os.O_RDWR | os.O_NONBLOCK)
        self.processes[actor] = subprocess.Popen(
            ['chroot', str(self.f.root), '/qemu', '/harness/lifecycle-driver', operation],
            env=dict(self.f.env, LIFECYCLE_ACTOR=actor), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, start_new_session=True)

    def event(self):
        deadline = time.monotonic() + 60
        while b'\n' not in self.pending:
            assert time.monotonic() < deadline, 'Timed out waiting for real lifecycle event'
            if select.select([self.events], [], [], .1)[0]:
                self.pending += os.read(self.events, 8192)
        line, self.pending = self.pending.split(b'\n', 1)
        actor, phase, status, detail = line.decode().split('\t', 3)
        status = int(status)
        cleanup = phase == 'CLEANUP_AFTER'
        if cleanup:
            phase = 'COMMAND_AFTER'
        if phase == 'LOCK_ACQUIRED':
            assert status >= 0, (actor, detail, status)
            assert detail not in self.locks, ('Overlapping real locks', self.locks, actor, detail)
            self.locks[detail] = actor, status
        elif phase == 'LOCK_RELEASE_BEFORE':
            owned = [tag for tag, value in self.locks.items() if value == (actor, status)]
            assert len(owned) == 1, (actor, status, self.locks)
            del self.locks[owned[0]]
        elif phase == 'COMMAND_AFTER' and self.mutation(detail):
            self.f.boundaries += 1
            if (not cleanup or status not in (0, 1)) and 'tables-restore' in detail and detail != self.f.env.get('LIFECYCLE_FAIL_RESTORE'):
                self.f.require('active lifecycle restore succeeds: ' + detail, status == 0, status)
        print('OBSERVE', actor, 'CLEANUP_AFTER' if cleanup else phase, status, detail, flush=True)
        return actor, phase, status, detail

    @staticmethod
    def mutation(command):
        return ('tables-restore' in command or
                re.search(r'\bip6?tables (?:.* )?(?:-[ADIFXRPN]|--flush)(?:\s|$)', command) or
                re.search(r'\bip (?:-[46] )?rule (?:add|del|replace|flush)\b', command))

    def ack(self, actor):
        os.write(self.acks[actor], b'continue\n')

    def until(self, predicate):
        while True:
            event = self.event()
            if predicate(event):
                return event
            self.ack(event[0])

    def drain(self, actors, probe=None):
        active = set(actors)
        deadline = time.monotonic() + 900
        while active:
            assert time.monotonic() < deadline, ('Lifecycle processes did not finish', active)
            if b'\n' in self.pending or select.select([self.events], [], [], .1)[0]:
                event = self.event()
                assert event[0] in active, ('Unexpected actor progressed', event, active)
                if probe:
                    probe(event)
                self.ack(event[0])
            for actor in tuple(active):
                process = self.processes[actor]
                if process.poll() is not None:
                    output = process.communicate(timeout=2)[0].decode()
                    print(output, end='', flush=True)
                    assert process.returncode == 0, (actor, process.returncode, output)
                    active.remove(actor)

    def close(self):
        for process in self.processes.values():
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate(timeout=2)
        os.close(self.events)
        (self.f.root / 'tmp/lifecycle-events').unlink()
        for actor, fd in self.acks.items():
            os.close(fd)
            (self.f.root / ('tmp/lifecycle-ack-' + actor)).unlink()


class Fixture(broad.Fixture):
    def publish(self, key, value):
        self.env[key] = value
        path = self.root / 'tmp/lifecycle-nvram' / key
        temporary = path.with_name(path.name + '.next')
        temporary.write_text(value)
        temporary.replace(path)

    def setup(self, policy):
        for path in (self.root / 'tmp/lifecycle-nvram').iterdir():
            path.unlink()
        self.reset(policy)
        # The guest's actual IPv6 SDN firewall is conditional on this subnet flag.
        self.env['subnet_rl'] += '>1>0>2001:db8:3::1'
        self.env.update(sw_mode='1', wan0_ifname='wan0', wan0_gw_ifname='wan0',
                        wan0_proto='dhcp', wan0_ipaddr='10.1.0.1', wan0_netmask='255.255.255.0',
                        wan0_state_t='2', wan0_primary='1', wans_mode='fo', wans_dualwan='wan none',
                        ipv6_service='dhcp6', ipv6_ifdev='eth', ipv6_ifname='wan0',
                        ipv6_rtr_addr='2001:db8:1::1',
                        fw_enable_x='0', ipv6_fw_enable='0', x_Setting='1',
                        wgc1_enable='0', vpn_clientx_eas='', vpn_client1_state='0')
        for key, value in self.env.items():
            self.publish(key, value)

    def invoke(self, operation):
        result = net.run('chroot', str(self.root), '/qemu', '/harness/lifecycle-driver',
                         operation, env=self.env, check=False)
        print(result.stdout + result.stderr, end='', flush=True)
        for status, command in re.findall(r'^Restore returned (\d+): (.*)$',
                                          result.stdout + result.stderr, re.MULTILINE):
            self.require('active lifecycle restore succeeds: ' + command, False, int(status))
        assert result.returncode == 0, (operation, result.returncode)


def ddns_stop(f, cached_filters=(True, False)):
    for cached_filter in cached_filters:
        f.setup('')
        # Cache an older filter, then install a newer guest quarantine.
        cache = f.root / 'tmp/filter_rules'
        cache.write_text(net.run('iptables-nft-save', '-t', 'filter').stdout)
        if not cached_filter:
            cache.unlink()
        f.publish('vpnc_dev_policy_list', '<1>>>bad>br1')
        f.invoke('refresh')
        rule = ('INPUT', '-p', 'icmp', '-s', '66.220.2.74', '-j', 'ACCEPT')
        net.run('iptables-nft', '-I', *rule)
        f.publish('ddns_tunbkrnet', '1')
        before = net.run('iptables-nft', '-S').stdout
        expected = before.replace('-A INPUT -s 66.220.2.74/32 -p icmp -j ACCEPT\n', '')
        assert expected != before
        for family in (4, 6):
            f.dns('DDNS setup blocks guest DNS ' + str(family), 'br1', family, False)
            f.dns('DDNS setup allows LAN DNS ' + str(family), 'br0', family, True)
        actors = Actors(f)
        try:
            actors.start('ddns', 'stop_ddns')

            def probe(e):
                if e[1] == 'COMMAND_AFTER' and Actors.mutation(e[3]):
                    for family in (4, 6):
                        f.dns('DDNS stop boundary retains guest DNS ' + str(family), 'br1', family, False)
            actors.drain(('ddns',), probe)
        finally:
            actors.close()
        f.require('DDNS stop preserves all unrelated live filter rules',
                  net.run('iptables-nft', '-S').stdout == expected)
        f.require('DDNS stop removes its ICMP exception',
                  net.run('iptables-nft', '-C', *rule, check=False).returncode != 0)
        assert (f.root / 'tmp/lifecycle-nvram/ddns_tunbkrnet').read_text() == ''
        # A repeated stop, including a flag whose rule was already removed by
        # a full reload, must preserve quarantine too.
        f.invoke('stop_ddns')
        f.publish('ddns_tunbkrnet', '1')
        f.invoke('stop_ddns')
        for family in (4, 6):
            f.dns('DDNS stop retains guest DNS ' + str(family), 'br1', family, False)
            f.dns('DDNS stop preserves LAN DNS ' + str(family), 'br0', family, True)


def stale_restore(f, operation):
    f.setup('<1>>>bad>br1')
    f.invoke('refresh')
    # Prove protection is the broad filter, independent of an earlier route.
    for family in ('-4', '-6'):
        net.ip(family, 'rule', 'add', 'from', 'all', 'iif', 'br0', 'lookup', 'main', 'priority', '50')
        net.ip(family, 'rule', 'add', 'from', 'all', 'iif', 'br1', 'lookup', 'main', 'priority', '50')
    previous = f.regression
    f.regression = False
    try:
        for family in (4, 6):
            f.data('setup protected guest ' + str(family), 'br1', family, None)
            f.data('setup outside LAN ' + str(family), 'br0', family, 'wan0')
            f.dns('setup LAN DNS available ' + str(family), 'br0', family, True)
    finally:
        f.regression = previous
    actors = Actors(f)
    try:
        actors.start('writer', operation)
        filename = '/tmp/filter.default' if operation == 'default' else '/tmp/filter_rules'
        actors.until(lambda e: e[0] == 'writer' and e[1] == 'COMMAND_BEFORE' and
                     e[3] == 'iptables-restore ' + filename)
        writer_holds_vpn = actors.locks.get('vpnrouting-dns', ('', -1))[0] == 'writer'
        f.publish('vpnc_dev_policy_list', '<1>unknown-source>>5')
        actors.start('policy', 'refresh')
        actors.until(lambda e: e[0] == 'policy' and e[1] == 'LOCK_BEFORE' and e[3] == 'vpnrouting-dns')
        actors.ack('policy')

        def protected_boundary(e):
            if e[1] != 'COMMAND_AFTER' or not Actors.mutation(e[3]):
                return
            # Every observed kernel mutation must retain the already-active
            # guest scope, independent of when the new LAN scope is published.
            for family in (4, 6):
                f.data(operation + ' boundary retains guest data ' + str(family), 'br1', family, None)
                f.service(operation + ' boundary retains guest DNS ' + str(family),
                          'br1', family, 53, False, False)
            if actors.processes['policy'].poll() == 0:
                f.service(operation + ' post-policy boundary retains new LAN DNS', 'br0', 4, 53, False, False)

        if writer_holds_vpn:
            print('Actual writer lock serializes the policy refresh until restore finishes', flush=True)
            actors.ack('writer')
            actors.drain(('writer', 'policy'), protected_boundary)
        else:
            print('Baseline writer has no VPN transaction lock; policy publishes before stale restore', flush=True)
            actors.drain(('policy',))
            for family in (4, 6):
                f.dns('new scope published before stale restore ' + str(family), 'br0', family, False)
            actors.ack('writer')

            def after_restore(e):
                protected_boundary(e)
                if e[1] == 'COMMAND_AFTER' and e[3] == 'iptables-restore ' + filename:
                    assert e[2] == 0, ('Generated filter did not apply', e)
                    f.dns(operation + ' stale restore preserves newly blocked LAN DNS', 'br0', 4, False)
                    if operation != 'default':
                        f.data(operation + ' stale restore preserves newly blocked LAN data', 'br0', 4, None)
            actors.drain(('writer',), after_restore)
        for family in (4, 6):
            f.dns(operation + ' completed lifecycle retains current DNS scope ' + str(family), 'br0', family, False)
            f.router(operation + ' router-origin control ' + str(family), family)
            f.service(operation + ' keeps own-SDN router management usable IPv' + str(family),
                      'br1', family, 80, True, True)
            f.service(operation + ' keeps affected guest DHCP port usable IPv' + str(family),
                      'br1', family, 67 if family == 4 else 547, False, True)
    finally:
        actors.close()


def disabled_ipv6(f):
    f.setup('<1>>>bad>br1')
    f.invoke('normal')  # Establish the real enabled filter before disabling IPv6.
    f.publish('ipv6_service', 'disabled')
    f.invoke('refresh')
    net.ip('-6', 'rule', 'add', 'from', 'all', 'iif', 'br1', 'lookup', 'main', 'priority', '50')
    previous = f.regression
    f.regression = False
    try:
        f.data('disabled-config setup IPv6 still reaches native WAN outside scope', 'br0', 6, 'wan0')
        f.data('disabled-config setup broad guest guarded despite earlier lookup', 'br1', 6, None)
        f.dns('disabled-config setup guest DNS guarded', 'br1', 6, False)
    finally:
        f.regression = previous
    actors = Actors(f)
    try:
        actors.start('disabled', 'normal')

        def probe(e):
            if e[1] == 'COMMAND_AFTER' and (e[3] == 'ip6tables -F' or
                    e[3].startswith('ip6tables-restore')):
                f.data('disabled-IPv6 firewall boundary retains broad guest', 'br1', 6, None)
                f.dns('disabled-IPv6 firewall boundary retains guest DNS', 'br1', 6, False)
        actors.drain(('disabled',), probe)
        f.data('disabled-IPv6 reload retains guest data guard', 'br1', 6, None)
        f.dns('disabled-IPv6 reload retains guest DNS guard', 'br1', 6, False)
        f.data('disabled-IPv6 reload preserves outside LAN native connectivity', 'br0', 6, 'wan0')
        f.router('disabled-IPv6 reload preserves router-origin traffic', 6)
    finally:
        actors.close()


def default_restore_fault(f, family):
    f.setup('<1>>>bad>br1')
    f.invoke('refresh')
    utility = 'iptables' if family == 4 else 'ip6tables'
    command = utility + '-restore /tmp/' + ('filter.default' if family == 4 else 'filter_ipv6.default')
    before = net.run(utility + '-nft', '-S').stdout
    f.env['LIFECYCLE_FAIL_RESTORE'] = command
    hit = f.root / 'tmp/lifecycle-fault-hit'
    hit.unlink(missing_ok=True)
    actors = Actors(f)
    callbacks = []
    try:
        actors.start('fault', 'default')

        def probe(e):
            if e[1] == 'SDN_CALLBACK':
                callbacks.append(e[3])
        actors.drain(('fault',), probe)
        assert hit.read_text() == command, 'Requested fault was not exercised'
        f.require('default restore failure skips SDN callbacks IPv' + str(family), not callbacks, callbacks)
        f.require('failed family filter remains unchanged IPv' + str(family),
                  net.run(utility + '-nft', '-S').stdout == before)
        f.dns('failed default retains protected DNS IPv' + str(family), 'br1', family, False)
    finally:
        actors.close()
        del f.env['LIFECYCLE_FAIL_RESTORE']
    # A fresh helper and full writer must acquire the released locks and recover.
    f.invoke('refresh')
    f.invoke('default')
    f.dns('default failure retry retains DNS IPv' + str(family), 'br1', family, False)


def service_restart(f):
    f.setup('<1>>>bad>br1')
    f.invoke('refresh')
    for operation in ('restart_firewall', 'start_vpnrouting1'):
        f.invoke(operation)
        for family in (4, 6):
            f.data(operation + ' keeps guest forwarding blocked IPv' + str(family), 'br1', family, None)
            f.dns(operation + ' keeps guest DNS blocked IPv' + str(family), 'br1', family, False)
            f.data(operation + ' preserves outside LAN IPv' + str(family), 'br0', family, 'wan0')
            f.service(operation + ' keeps main management usable IPv' + str(family), 'br0', family, 80, True, True)
            f.service(operation + ' keeps main DHCP port usable IPv' + str(family),
                      'br0', family, 67 if family == 4 else 547, False, True)
            f.service(operation + ' keeps own-SDN router management usable IPv' + str(family),
                      'br1', family, 80, True, True)
            f.service(operation + ' keeps affected guest DHCP port usable IPv' + str(family),
                      'br1', family, 67 if family == 4 else 547, False, True)
            f.router(operation + ' preserves router-origin IPv' + str(family), family)
    before = [net.run(tool, '-S').stdout for tool in ('iptables-nft', 'ip6tables-nft')]
    f.invoke('stop_firewall')
    f.require('actual stop notification retains both filter tables', before ==
              [net.run(tool, '-S').stdout for tool in ('iptables-nft', 'ip6tables-nft')])
    for family in (4, 6):
        f.data('actual stop notification retains guest quarantine IPv' + str(family), 'br1', family, None)


def filter_open_fault(f, operation, family):
    f.setup('<1>>>bad>br1')
    f.invoke('refresh')
    utility = 'iptables-nft' if family == 4 else 'ip6tables-nft'
    before = net.run(utility, '-S').stdout
    names = ('filter.default', 'filter_ipv6.default') if operation == 'default' else (
        'filter_rules', 'filter_rules_ipv6')
    path = f.root / 'tmp' / names[family == 6]
    path.unlink(missing_ok=True)
    path.mkdir()  # The real fopen must fail with EISDIR, without a substituted result.
    actors = Actors(f)
    callbacks = []
    try:
        actors.start('openfault', operation)

        def probe(e):
            if e[1] == 'SDN_CALLBACK':
                callbacks.append(e[3])
        actors.drain(('openfault',), probe)
        label = operation + ' IPv' + str(family) + ' open failure'
        f.require(label + ' releases transaction locks', not actors.locks, actors.locks)
        f.require(label + ' preserves family filter', net.run(utility, '-S').stdout == before)
        if operation == 'default':
            f.require(label + ' skips SDN callbacks', not callbacks, callbacks)
        f.dns(label + ' retains protected DNS', 'br1', family, False)
    finally:
        actors.close()
        path.rmdir()
    f.invoke('refresh')
    f.invoke(operation)
    f.dns(label + ' retry retains protected DNS', 'br1', family, False)


def parallel_writers(f):
    f.setup('<1>>>bad>br1')
    f.invoke('refresh')
    actors = Actors(f)
    try:
        actors.start('first', 'normal')
        actors.until(lambda e: e[:2] == ('first', 'COMMAND_BEFORE') and
                     e[3] == 'iptables-restore /tmp/filter_rules')
        assert actors.locks.get('firewall', ('', -1))[0] == 'first'
        actors.start('second', 'normal')
        actors.until(lambda e: e[:2] == ('second', 'LOCK_BEFORE') and e[3] == 'firewall')
        actors.ack('second')
        actors.ack('first')

        def probe(e):
            if e[1] == 'COMMAND_AFTER' and e[3] in (
                    'iptables-restore /tmp/filter_rules', 'ip6tables-restore /tmp/filter_rules_ipv6'):
                assert e[2] == 0, e
                for family in (4, 6):
                    f.data('overlapping full reloads retain data IPv' + str(family), 'br1', family, None)
                    f.dns('overlapping full reloads retain DNS IPv' + str(family), 'br1', family, False)
        actors.drain(('first', 'second'), probe)
        f.require('both full reloads finish with all actual locks released', not actors.locks, actors.locks)
    finally:
        actors.close()


def normal_ipv6_guard(f):
    f.setup('')
    f.publish('qca_merlin_vpn_migrated', '1')
    f.publish('vpnc_clientlist', '')
    f.publish('vpn_clientx_eas', '1,')
    f.publish('vpn_client1_rgw', '2')
    f.publish('vpn_client1_enforce', '1')
    rows = f.env['sdn_rl'].split('<')
    for n, row in enumerate(rows):
        if row.startswith('1>'):
            fields = row.split('>')
            fields[6] = '6'
            rows[n] = '>'.join(fields)
    f.publish('sdn_rl', '<'.join(rows))
    net.ip('route', 'flush', 'table', 'ovpnc1')
    f.invoke('normal')
    f.publish('ipv6_service', 'disabled')
    f.invoke('ipv6guard')
    previous = f.regression
    f.regression = False
    try:
        f.data('normal enforced IPv6 setup guards assigned guest', 'br1', 6, None)
        f.data('normal enforced IPv6 setup preserves outside LAN', 'br0', 6, 'wan0')
    finally:
        f.regression = previous
    actors = Actors(f)
    try:
        actors.start('normalguard', 'normal')

        def probe(e):
            if e[1] == 'COMMAND_AFTER' and (e[3] == 'ip6tables -F' or
                    e[3].startswith('ip6tables-restore')):
                f.data('disabled-IPv6 mutation retains normal VPN6KS', 'br1', 6, None)
        actors.drain(('normalguard',), probe)
        f.data('disabled-IPv6 full reload retains normal VPN6KS', 'br1', 6, None)
        f.data('normal VPN6KS reload preserves outside LAN', 'br0', 6, 'wan0')
        f.router('normal VPN6KS reload preserves router-origin traffic', 6)
    finally:
        actors.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--expect-regression', action='store_true')
    parser.add_argument('--operation', default='race')
    args = parser.parse_args()
    net.prepare(args.root)
    (args.root / 'tmp/lifecycle-nvram').mkdir()
    for name in ('iptables-save', 'ip6tables-save'):
        net.copy_native(args.root, '/usr/sbin/xtables-nft-multi', '/usr/sbin/' + name)
    # Native glibc loads this module with dlopen; ldd cannot discover it.
    # Full vendor filters use names such as ipv6-nonxt from /etc/protocols.
    net.copy_native(args.root, '/lib/x86_64-linux-gnu/libnss_files.so.2')
    net.copy_native(args.root, '/bin/busybox', '/bin/sed')
    (args.root / 'etc/nsswitch.conf').write_text('protocols: files\nservices: files\nhosts: files\n')
    compile_driver(args.root)
    net.setup_links()
    fixture = Fixture(args.root, args.expect_regression)
    try:
        if args.operation == 'race':
            ddns_stop(fixture)
            for operation in ('default', 'normal', 'filter2'):
                stale_restore(fixture, operation)
            disabled_ipv6(fixture)
            for family in (4, 6):
                default_restore_fault(fixture, family)
                for operation in ('default', 'filter', 'filter2'):
                    filter_open_fault(fixture, operation, family)
            service_restart(fixture)
            parallel_writers(fixture)
            normal_ipv6_guard(fixture)
        elif args.operation == 'ddns-case':
            ddns_stop(fixture)
        elif args.operation == 'ddns-cached-case':
            ddns_stop(fixture, (True,))
        elif args.operation == 'normalguard-case':
            normal_ipv6_guard(fixture)
        elif args.operation == 'filter2-case':
            stale_restore(fixture, 'filter2')
        elif args.operation == 'parallel-case':
            parallel_writers(fixture)
        elif args.operation == 'service-case':
            service_restart(fixture)
        elif args.operation == 'disabled-case':
            disabled_ipv6(fixture)
        elif args.operation in ('fault4-case', 'fault6-case'):
            default_restore_fault(fixture, int(args.operation[5]))
        else:
            fixture.setup('<1>>>bad>br1')
            fixture.invoke(args.operation)
        print(f'Firewall lifecycle summary: {fixture.checks} checks; '
              f'{fixture.boundaries} command boundaries; {len(fixture.failures)} failures', flush=True)
        assert bool(fixture.failures) == args.expect_regression, fixture.failures
    except Exception:
        print('IPv6 filter failure diagnostic:\n' + net.run('ip6tables-nft', '-S').stdout, flush=True)
        print('IPv6 routing failure diagnostic:\n' + net.ip('-6', 'rule', 'show').stdout, flush=True)
        for name in ('filter_rules_ipv6', 'filter_ipv6.default'):
            path = args.root / 'tmp' / name
            if path.is_file():
                print('Generated ' + name + ':\n' + path.read_text(), flush=True)
        for name in ('ipv6_service', 'ipv6_fw_enable', 'wan0_ifname', 'ipv6_ifname', 'ipv6_ifdev'):
            path = args.root / 'tmp/lifecycle-nvram' / name
            print('NVRAM', name, path.read_text() if path.exists() else '<unset>', flush=True)
        raise
    finally:
        fixture.close()


if __name__ == '__main__':
    main()
