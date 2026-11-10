#!/usr/bin/env python3
"""Linux regression tests: production shell/C, file NVRAM, fake STUN/routes."""
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'release/src/router/rom/apps_scripts/getrealip.sh'
SOURCE = ROOT / 'release/src/router/rc/realip.c'
HERE = Path(__file__).resolve().parent
SERVICES = ROOT / 'release/src/router/rc/services.c'

STUN = '''#!/usr/bin/env python3
import os, pathlib, time
marker = os.environ.get('STUN_MARKER')
if marker:
    pathlib.Path(marker).write_text(str(os.getpid()))
iface = __import__('sys').argv[__import__('sys').argv.index('-i') + 1]
gate = os.environ.get('STUN_GATE_' + iface.upper(), os.environ.get('STUN_GATE'))
while gate and not pathlib.Path(gate).exists():
    time.sleep(0.01)
if os.environ.get('STUN_FAIL'):
    raise SystemExit(1)
iface = __import__('sys').argv[__import__('sys').argv.index('-i') + 1]
print(os.environ.get('STUN_IP_' + iface, os.environ.get('STUN_IP', '198.51.100.10')))
'''


class Realip(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix='realip-build-')
        cls.bin = Path(cls.build.name)
        base = ['cc', '-std=gnu89', '-Wall', '-Wextra', '-Werror', '-I' + str(HERE)]
        for name, defines in [('single', []), ('dual', ['RTCONFIG_DUALWAN']),
                              ('multi', ['RTCONFIG_MULTIWAN_IF']),
                              ('both', ['RTCONFIG_DUALWAN', 'RTCONFIG_MULTIWAN_IF'])]:
            subprocess.run(base + ['-DRTCONFIG_GETREALIP'] + ['-D' + d for d in defines] +
                           [str(SOURCE), str(HERE / 'host.c'), '-o', str(cls.bin / name)], check=True)
        subprocess.run(base + ['-c', str(SOURCE), '-o', str(cls.bin / 'disabled.o')], check=True)
        services = SERVICES.read_text()
        block = services[services.index('/* Keep the daemon serving'):services.index('void reload_upnp(void)')]
        (cls.bin / 'upnp_services.c').write_text(block)
        for name, defines in [('upnp', ['RTCONFIG_GETREALIP', 'RTCONFIG_DUALWAN', 'RTCONFIG_IGD2']),
                              ('upnp-single', ['RTCONFIG_GETREALIP']),
                              ('upnp-disabled', []),
                              ('upnp-features', ['RTCONFIG_GETREALIP', 'RTCONFIG_DUALWAN',
                               'RTCONFIG_IGD2', 'RTCONFIG_AUPNPC', 'RTCONFIG_AURASYNC',
                               'RTCONFIG_GEFORCENOW', 'RTCONFIG_HTTPS', 'RTCONFIG_WEBDAV',
                               'RTCONFIG_TUNNEL', 'RTCONFIG_APP_PREINSTALLED'])]:
            subprocess.run(base + ['-Wno-unused-function', '-Wno-unused-variable', '-Wno-misleading-indentation', '-I' + str(cls.bin)] +
                           ['-D' + d for d in defines] + (['-DUPNP_WITHOUT_REALIP'] if name == 'upnp-disabled' else []) +
                           ['-DRTCONFIG_GETREALIP',
                           str(SOURCE), str(HERE / 'upnp_host.c'), '-o', str(cls.bin / name)], check=True)
        (cls.bin / 'realip').symlink_to('both')
        (cls.bin / 'nvram').symlink_to('both')
        (cls.bin / 'getrealip.sh').symlink_to(SCRIPT)
        (cls.bin / 'ministun').write_text(STUN)
        (cls.bin / 'ministun').chmod(0o755)

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='realip-test-')
        self.data = Path(self.tmp.name)
        self.env = dict(os.environ, REALIP_TEST_DIR=str(self.data),
                        PATH=str(self.bin) + ':' + os.environ['PATH'])
        self.processes = []
        self.set('wan0_upnp_enable', '1')
        self.set('wan1_upnp_enable', '1')
        self.set('lan_ipaddr', '192.168.1.1')
        self.set('lan_netmask', '255.255.255.0')
        self.set('lan_ifname', 'br0')
        self.set('wans_mode', 'fo')
        self.set('link_internet', '2')
        self.set('test_mapped_unit', '0')
        for unit in (0, 1):
            for key, value in dict(state_t='2', sbstate_t='0', auxstate_t='0', proto='dhcp',
                                   ifname=f'eth{unit}', pppoe_ifname=f'ppp{unit}',
                                   ipaddr=f'192.0.2.{unit + 2}', gateway='192.0.2.1',
                                   primary=str(1 - unit)).items():
                self.set(f'wan{unit}_{key}', value)

    def tearDown(self):
        for p in self.processes:
            if p.poll() is None:
                p.terminate()
                try:
                    p.communicate(timeout=2)
                except subprocess.TimeoutExpired:
                    p.kill()
                    p.communicate()
            elif p.stdout:
                p.communicate()
        self.tmp.cleanup()

    def set(self, key, value):
        (self.data / key).write_text(str(value))

    def get(self, key):
        return (self.data / key).read_text() if (self.data / key).exists() else ''

    def command(self, *args, check=True, **extra):
        return subprocess.run([str(self.bin / 'realip'), *map(str, args)], env=dict(self.env, **extra),
                              text=True, capture_output=True, check=check)

    def spawn(self, args=None, **extra):
        p = subprocess.Popen(args or ['/bin/sh', str(SCRIPT)], env=dict(self.env, **extra),
                             text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.processes.append(p)
        return p

    def wait_for(self, pred):
        end = time.monotonic() + 5
        while not pred():
            if time.monotonic() >= end:
                self.fail('timed out waiting for controlled process event')
            time.sleep(0.01)

    def held(self, name, args=None, **extra):
        marker, gate = self.data / (name + '.pid'), self.data / (name + '.gate')
        p = self.spawn(args, STUN_MARKER=str(marker), STUN_GATE=str(gate), **extra)
        self.wait_for(marker.exists)
        return p, gate, int(marker.read_text())

    def finish(self, p):
        out, err = p.communicate(timeout=5)
        return p.returncode, out, err

    def assert_result(self, unit, state, ip):
        self.assertEqual(self.get(f'wan{unit}_realip_state'), str(state))
        self.assertEqual(self.get(f'wan{unit}_realip_ip'), ip)

    def upnp(self, action='start', variant='upnp'):
        subprocess.run([str(self.bin / variant), action], env=self.env, check=True, timeout=2)

    def config(self):
        return (self.data / 'upnp/config').read_text()

    def daemon_starts(self):
        return sum(line.startswith('start miniupnpd') for line in self.get('routes').splitlines())

    def address(self, unit=0):
        return self.command('--test-address', f'wan{unit}_').stdout.strip()

    def publish(self, ip='198.51.100.10', unit=0):
        token = self.command('--begin', unit).stdout.strip()
        self.command('--finish', unit, token, ip)
        return token

    def test_upnp_initial_fast_and_unchanged_completion(self):
        token = self.publish()
        self.upnp()
        self.assertIn('ext_ip=198.51.100.10\n', self.config())
        self.assertEqual(self.daemon_starts(), 1)
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 1)
        self.command('--finish', 0, token, '203.0.113.1', check=False)
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 1)

    def test_upnp_slow_boot_public_wan_renew_and_recovery(self):
        self.set('wan0_ipaddr', '198.51.100.10')
        pending, gate, _ = self.held('boot')
        self.upnp()
        self.assertNotIn('ext_ip=', self.config())
        self.assertEqual(self.get('miniupnpd'), '1')
        self.upnp()
        self.assertEqual(self.daemon_starts(), 1)
        gate.touch()
        self.assertEqual(self.finish(pending)[0], 0)
        self.upnp('update')
        self.assertNotIn('ext_ip=', self.config())
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 1)
        self.command('--test-state', 'wan0_', 2)
        self.upnp()
        token = self.command('--begin', 0).stdout.strip()
        self.command('--finish', 0, token, '')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 1)

    def test_upnp_failure_removes_old_ip_and_recovers(self):
        self.publish()
        self.upnp()
        pending = self.command('--begin', 0).stdout.strip()
        self.upnp('update')
        self.assertNotIn('ext_ip=', self.config())
        starts = self.daemon_starts()
        self.command('--finish', 0, pending, '')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), starts)
        self.publish('203.0.113.8')
        self.upnp('update')
        self.assertIn('ext_ip=203.0.113.8', self.config())

    def test_upnp_pending_renew_keeps_daemon_without_previous_ip(self):
        self.upnp()
        self.command('--test-state', 'wan0_', 2)
        pending, gate, _ = self.held('renew')
        self.upnp()
        self.assertEqual(self.daemon_starts(), 1)
        self.set('upnp_secure', '1')
        self.upnp()
        self.assertEqual(self.daemon_starts(), 2)
        self.assertIn('secure_mode=yes', self.config())
        gate.touch()
        self.finish(pending)

    def test_upnp_public_address_change_and_secondary_result(self):
        self.set('wans_mode', 'lb')
        self.publish()
        self.upnp()
        self.publish('203.0.113.99', unit=1)
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 1)
        self.assertIn('ext_ip=198.51.100.10', self.config())
        self.publish('203.0.113.10')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 2)
        self.assertIn('ext_ip=203.0.113.10', self.config())

    def test_upnp_disconnect_failover_and_same_address_reconnect(self):
        self.upnp()
        self.command('--test-down', 'wan0_')
        self.command('--test-state', 'wan0_', 4)
        self.set('wan0_upnp_enable', '0')
        self.publish('203.0.113.11', unit=1)
        self.upnp('update')
        self.assertIn('ext_ifname=eth1', self.config())
        self.assertIn('ext_ip=203.0.113.11', self.config())
        self.assertEqual(self.get('upnp_unit'), '1')
        self.set('wan0_upnp_enable', '1')
        self.command('--test-state', 'wan0_', 2)
        self.upnp()
        self.assertIn('ext_ifname=eth0', self.config())
        self.assertNotIn('ext_ip=', self.config())
        self.assertEqual(self.daemon_starts(), 3)

    def test_upnp_manual_restart_and_stop_during_stun(self):
        self.upnp()
        pending, gate, _ = self.held('manual')
        self.upnp('restart')
        self.assertEqual(self.daemon_starts(), 2)
        self.upnp('stop')
        gate.touch()
        self.finish(pending)
        self.upnp('update')
        self.assertEqual(self.get('miniupnpd'), '0')
        self.assertEqual(self.daemon_starts(), 2)

    def test_upnp_snapshot_rejects_changed_context_and_stale_generation(self):
        old = self.publish()
        self.assertEqual(self.address(), '198.51.100.10')
        self.set('wan0_ifname', 'eth9')
        self.assertEqual(self.address(), '')
        token = self.command('--begin', 0).stdout.strip()
        self.command('--finish', 0, old, '203.0.113.1', check=False)
        self.assertEqual(self.address(), '')
        self.command('--finish', 0, token, '203.0.113.2')
        self.upnp()
        self.assertIn('ext_ifname=eth9', self.config())
        self.assertIn('ext_ip=203.0.113.2', self.config())

    def test_upnp_effective_hooks_alt_and_igd2(self):
        self.upnp()
        self.set('test_postconf', 'changed')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 2)
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 2)
        self.set('upnp_pinhole_enable', '1')
        self.upnp('update')
        self.assertEqual(self.get('miniupnpd-igdv2'), '1')
        (self.data / 'upnp/config.alt').write_text('ext_ifname=eth0\n')
        self.upnp('update')
        self.assertEqual(self.get('miniupnpd'), '1')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 4)

    def test_upnp_completion_after_early_daemon_exit(self):
        self.upnp()
        self.set('miniupnpd', '0')
        self.publish('203.0.113.90')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 2)
        self.assertIn('ext_ip=203.0.113.90', self.config())
        self.upnp('stop')
        self.upnp('update')
        self.assertEqual(self.daemon_starts(), 2)

    def test_upnp_lifecycle_epoch_and_wan_address(self):
        self.upnp()
        self.command('--test-down', 'wan0_')
        self.command('--test-state', 'wan0_', 2)
        self.upnp()
        self.assertEqual(self.daemon_starts(), 2)
        self.set('wan0_ipaddr', '192.168.55.20')
        self.upnp()
        self.assertEqual(self.daemon_starts(), 3)

    def test_upnp_compiled_service_variants_run(self):
        for variant in ['upnp-single', 'upnp-disabled', 'upnp-features']:
            self.upnp('start', variant=variant)
            self.assertEqual(self.get('miniupnpd'), '1')
            self.upnp('stop', variant=variant)
            self.assertEqual(self.get('miniupnpd'), '0')

    def test_upnp_publication_notifies_only_for_current_request(self):
        token = self.command('--begin', 0).stdout.strip()
        self.assertEqual(self.get('routes').count('notify update_upnp'), 1)
        newer = self.command('--begin', 0).stdout.strip()
        self.command('--finish', 0, token, '203.0.113.3', check=False)
        self.assertEqual(self.get('routes').count('notify update_upnp'), 2)
        self.command('--finish', 0, newer, '203.0.113.4')
        self.assertEqual(self.get('routes').count('notify update_upnp'), 3)
        self.command('--finish', 0, newer, '203.0.113.5', check=False)
        self.assertEqual(self.get('routes').count('notify update_upnp'), 3)
        services = SERVICES.read_text()
        self.assertIn('strcmp(script, "update_upnp") == 0', services)
        self.assertNotIn('sleep(', services[services.index('void start_upnp(void)'):
                                           services.index('void stop_upnp(void)')])

    def test_upnp_disabled_and_failed_start(self):
        self.set('test_start_fail', '1')
        self.upnp()
        self.assertFalse((self.data / 'upnp/config.applied').exists())
        self.set('test_start_fail', '0')
        self.upnp()
        self.set('wan0_upnp_enable', '0')
        self.upnp('update')
        self.assertEqual(self.get('miniupnpd'), '0')

    def test_reverse_completion(self):
        old, gate, _ = self.held('old', STUN_IP='198.51.100.1')
        new = self.spawn(STUN_IP='198.51.100.2')
        self.assertEqual(self.finish(new)[:2], (0, 'External IP is 198.51.100.2\n'))
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assert_result(0, 2, '198.51.100.2')

    def test_reconnect_same_interface_and_ip(self):
        old, gate, _ = self.held('old')
        self.command('--test-down', 'wan0_')
        self.command('--test-state', 'wan0_', 4)
        self.assertNotEqual(self.command('--begin', 0, check=False).returncode, 0)
        self.command('--test-state', 'wan0_', 2)
        new = self.spawn(STUN_IP='203.0.113.2')
        self.assertEqual(self.finish(new)[0], 0)
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assert_result(0, 2, '203.0.113.2')

    def test_dhcp_renew_unchanged_ip(self):
        old, gate, _ = self.held('old')
        self.command('--test-state', 'wan0_', 2)
        self.command('--test-start', 0)
        self.wait_for(lambda: self.get('wan0_realip_state') == '2')
        gate.touch()
        self.assertEqual(self.finish(old)[0], 1)
        self.assert_result(0, 2, '198.51.100.10')

    def test_failure_success_failure_contract(self):
        failed = self.spawn(STUN_FAIL='1')
        self.assertEqual(self.finish(failed)[:2], (0, 'Failed.\n'))
        self.assert_result(0, 1, '')
        self.assertEqual(self.finish(self.spawn())[0], 0)
        self.assert_result(0, 2, '198.51.100.10')
        self.assertEqual(self.finish(self.spawn(STUN_FAIL='1'))[:2], (0, 'Failed.\n'))
        self.assert_result(0, 1, '')

    def test_wan_up_and_inadyn_and_delayed_launch(self):
        # Exercise the production start_realip() pre-spawn reservation.
        marker, gate = self.data / 'rc.pid', self.data / 'rc.gate'
        self.command('--test-start', 0, STUN_MARKER=str(marker), STUN_GATE=str(gate),
                     STUN_IP='198.51.100.1')
        self.wait_for(marker.exists)
        outcome = self.finish(self.spawn(STUN_IP='198.51.100.2'))
        self.assertEqual(outcome[0], 0, outcome)
        gate.touch()
        time.sleep(0.15)
        self.assert_result(0, 2, '198.51.100.2')
        token = self.command('--begin', 0).stdout.strip()
        self.command('--test-state', 'wan0_', 2)
        delayed = self.spawn(['/bin/sh', str(SCRIPT), '0', token])
        self.assertEqual(self.finish(delayed)[:2], (1, ''))
        self.assert_result(0, 0, '')

    def test_failover(self):
        old, gate, _ = self.held('old')
        self.command('--test-down', 'wan0_')
        self.set('wan0_primary', '0')
        self.set('wan1_primary', '1')
        self.assertEqual(self.finish(self.spawn(STUN_IP='203.0.113.2'))[:2],
                         (0, 'External IP is 203.0.113.2\n'))
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assert_result(0, 0, '')
        self.assert_result(1, 2, '203.0.113.2')

    def test_lb_reserves_both_wans_before_probe(self):
        self.set('wans_mode', 'lb')
        old, gate, _ = self.held('old', STUN_IP='198.51.100.1')
        self.assertTrue(self.get('wan1_realip_generation'))
        new = self.spawn(STUN_IP_eth0='203.0.113.10', STUN_IP_eth1='203.0.113.11')
        self.assertEqual(self.finish(new)[:2],
                         (0, 'wan0 external IP is 203.0.113.10\nwan1 external IP is 203.0.113.11\n'))
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assert_result(0, 2, '203.0.113.10')
        self.assert_result(1, 2, '203.0.113.11')
        self.command('--test-down', 'wan1_')
        self.assert_result(1, 0, '')
        self.assert_result(0, 2, '203.0.113.10')

    def test_interface_and_gateway_changes(self):
        for key, value in [('ifname', 'eth9'), ('gateway', '192.0.2.9'), ('ipaddr', '192.0.2.19')]:
            old, gate, _ = self.held(key)
            self.set('wan0_' + key, value)
            gate.touch()
            self.assertEqual(self.finish(old)[:2], (1, ''))
            self.assertNotEqual(self.get('wan0_realip_state'), '2')

    def test_ipv4_validation(self):
        for ip in ['0.0.0.0', '0.1.2.3', '127.0.0.1', '224.0.0.1', '255.255.255.255',
                   '999.1.2.3', '198.51.100.1 garbage', '1.2.3', '01.2.3.4', '']:
            self.assertNotEqual(self.command('--valid', ip, check=False).returncode, 0, ip)
        self.assertEqual(self.finish(self.spawn(STUN_IP='0.0.0.0'))[:2], (0, 'Failed.\n'))
        self.assert_result(0, 1, '')

    def test_sigterm_kills_and_reaps_child(self):
        for shell in [['/bin/sh'], ['busybox', 'ash']]:
            p, _, pid = self.held('term' + str(len(self.processes)), shell + [str(SCRIPT)])
            p.send_signal(signal.SIGTERM)
            self.assertEqual(self.finish(p)[:2], (1, ''))
            self.assertFalse(Path(f'/proc/{pid}').exists(), 'orphaned ministun')
            self.assert_result(0, 1, '')

    def test_consumed_token_and_service_stop(self):
        token = self.command('--begin', 0).stdout.strip()
        self.command('--finish', 0, token, '198.51.100.4')
        self.assertNotEqual(self.command('--finish', 0, token, '198.51.100.5', check=False).returncode, 0)
        token = self.command('--begin', 0).stdout.strip()
        self.command('--test-cancel', 'wan0_')
        self.assertNotEqual(self.command('--finish', 0, token, '198.51.100.5', check=False).returncode, 0)
        self.assert_result(0, 0, '')
        self.assertEqual(self.finish(self.spawn())[0], 0)

    def test_lock_covers_check_and_all_writes(self):
        token = self.command('--begin', 0).stdout.strip()
        finisher = self.spawn([str(self.bin / 'realip'), '--finish', '0', token, '198.51.100.4'],
                              REALIP_TEST_WRITE_DELAY='1')
        self.wait_for(lambda: self.get('wan0_realip_ip') == '198.51.100.4')
        # This invalidation must wait for the in-progress publisher, then clear it.
        self.command('--test-state', 'wan0_', 2)
        self.assertEqual(self.finish(finisher)[0], 0)
        self.assert_result(0, 0, '')

    def test_legacy_c_shares_ownership_and_tempfiles(self):
        old, _, pid = self.held('legacy', [str(self.bin / 'realip'), '10'])
        self.assertEqual(self.finish(self.spawn(STUN_IP='203.0.113.4'))[0], 0)
        self.assertNotEqual(self.finish(old)[0], 0)
        self.assertFalse(Path(f'/proc/{pid}').exists())
        self.assert_result(0, 2, '203.0.113.4')
        legacy = self.spawn([str(self.bin / 'realip'), '10'], STUN_IP='203.0.113.5')
        self.assertEqual(self.finish(legacy)[:2], (0, '203.0.113.5\n'))
        self.assert_result(0, 2, '203.0.113.5')
        routes = self.get('routes').splitlines()
        self.assertEqual(sum(' add ' in r for r in routes), sum(' del ' in r for r in routes))

    def test_legacy_signal_and_mapping_change(self):
        old, _, pid = self.held('legacyterm', [str(self.bin / 'realip'), '10'])
        old.terminate()
        self.assertNotEqual(self.finish(old)[0], 0)
        self.assertFalse(Path(f'/proc/{pid}').exists())
        self.assert_result(0, 1, '')
        old, gate, _ = self.held('mapping', [str(self.bin / 'realip'), '10'])
        self.set('test_mapped_unit', '1')
        gate.touch()
        self.assertNotEqual(self.finish(old)[0], 0)
        self.assert_result(0, 1, '')

    def test_lb_registration_is_one_transaction(self):
        self.set('wans_mode', 'lb')
        gate = self.data / 'register.gate'
        old = self.spawn([str(self.bin / 'realip'), '--begin-script'],
                         REALIP_TEST_REGISTER_GATE=str(gate))
        self.wait_for(lambda: (self.data / 'register_entered').exists())
        new = self.spawn([str(self.bin / 'realip'), '--begin-script'])
        time.sleep(0.1)
        self.assertIsNone(new.poll(), 'registration interleaved between WAN0 and WAN1')
        gate.touch()
        old_tokens = self.finish(old)[1].split()[1:]
        new_tokens = self.finish(new)[1].split()[1:]
        for unit in (0, 1):
            self.assertGreater(int(new_tokens[unit]), int(old_tokens[unit]))
            self.assertNotEqual(self.command('--finish', unit, old_tokens[unit],
                                            '198.51.100.1', check=False).returncode, 0)
            self.command('--finish', unit, new_tokens[unit], '203.0.113.2')
            self.assert_result(unit, 2, '203.0.113.2')

    def test_generation_exhaustion_does_not_reuse_tokens(self):
        self.set('wan0_realip_generation', str(2**64 - 1))
        self.assertNotEqual(self.command('--begin', 0, check=False).returncode, 0)
        self.command('--test-state', 'wan0_', 2)
        self.assertEqual(self.get('wan0_realip_generation'), str(2**64 - 1))
        self.set('wan0_realip_generation', str(2**64 - 2))
        token = self.command('--begin', 0).stdout.strip()
        self.command('--test-state', 'wan0_', 2)
        self.assertNotEqual(self.command('--finish', 0, token, '198.51.100.8', check=False).returncode, 0)
        self.assert_result(0, 0, '')

    def test_legacy_failure_and_reserved_multiwan_start(self):
        self.assertNotEqual(self.finish(self.spawn([str(self.bin / 'realip'), '0'], STUN_FAIL='1'))[0], 0)
        self.assert_result(0, 1, '')
        self.command('--test-start', 10)
        self.wait_for(lambda: self.get('wan0_realip_state') == '2')
        self.assert_result(0, 2, '198.51.100.10')

    def test_alias_lifecycle_invalidates_underlying_wan(self):
        old, gate, _ = self.held('alias')
        self.command('--test-down', 'wan10_')
        self.command('--test-state', 'wan10_', 4)
        self.assertNotEqual(self.command('--begin', 0, check=False).returncode, 0)
        self.command('--test-state', 'wan10_', 2)
        new = self.spawn(STUN_IP='203.0.113.20')
        self.assertEqual(self.finish(new)[0], 0)
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assert_result(0, 2, '203.0.113.20')

    def test_delayed_alias_launch_cannot_steal_same_token_on_other_wan(self):
        token0 = self.command('--begin', 0).stdout.strip()
        token1 = self.command('--begin', 1).stdout.strip()
        self.assertEqual(token0, token1)
        self.set('test_mapped_unit', '1')
        delayed = self.spawn([str(self.bin / 'realip'), '10', token0, 'wan0_'])
        self.assertNotEqual(self.finish(delayed)[0], 0)
        self.command('--finish', 1, token1, '203.0.113.21')
        self.assert_result(1, 2, '203.0.113.21')

    def test_lb_cancellation_after_wan0_emits_no_partial_stdout(self):
        self.set('wans_mode', 'lb')
        # A gate only on eth1 lets WAN0 complete before the cancellation.
        marker, gate = self.data / 'partial.pid', self.data / 'partial.gate'
        old = self.spawn(STUN_MARKER=str(marker), STUN_GATE_ETH1=str(gate))
        self.wait_for(lambda: self.get('wan0_realip_state') == '2' and marker.exists())
        self.wait_for(lambda: self.get('wan1_realip_state') == '0')
        self.command('--test-down', 'wan0_')
        gate.touch()
        self.assertEqual(self.finish(old)[:2], (1, ''))

    def test_superseded_script_reaps_probe_without_waiting_for_stun(self):
        old, _, pid = self.held('cancel')
        self.assertEqual(self.finish(self.spawn(STUN_IP='203.0.113.22'))[0], 0)
        self.assertEqual(self.finish(old)[:2], (1, ''))
        self.assertFalse(Path(f'/proc/{pid}').exists())
        self.assert_result(0, 2, '203.0.113.22')

    def test_multiservice_unit_is_not_limited_to_eight_bits(self):
        for key in ('state_t', 'sbstate_t', 'auxstate_t', 'proto', 'ifname',
                    'pppoe_ifname', 'ipaddr', 'gateway', 'primary'):
            self.set('wan1501_' + key, self.get('wan0_' + key))
        self.command('--test-start', 1501)
        self.wait_for(lambda: self.get('wan1501_realip_state') == '2')
        self.assert_result(1501, 2, '198.51.100.10')

    def test_built_configurations_run(self):
        for variant in ['single', 'dual', 'multi', 'both']:
            p = self.spawn([str(self.bin / variant), '0'])
            self.assertEqual(self.finish(p)[:2], (0, '198.51.100.10\n'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
