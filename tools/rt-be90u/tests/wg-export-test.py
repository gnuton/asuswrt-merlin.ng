#!/usr/bin/env python3
"""Check exact source-generated WG exports through image import and native setconf."""
import importlib.util
import os
from pathlib import Path
import shutil
import stat
import sys

spec = importlib.util.spec_from_file_location('wgtest', '/tests/wg-tunnel-test.py')
wgtest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wgtest)
run, ip, arm = wgtest.run, wgtest.ip, wgtest.arm


def main(root):
    wgtest.network.prepare(root)
    ip('link', 'add', 'wgc1', 'type', 'wireguard')
    for name, minor in [('urandom', 9), ('null', 3)]:
        path = root / 'dev' / name
        path.unlink(missing_ok=True)
        os.mknod(path, stat.S_IFCHR | 0o600, os.makedev(1, minor))
    wg = wgtest.compile_config(root)
    keys = {}
    for side in ('server', 'client'):
        keys[side] = arm(root, '/usr/sbin/wg', 'genkey')
        keys[side + '_pub'] = arm(root, '/usr/sbin/wg', 'pubkey', input=keys[side] + '\n')
    env = dict(os.environ, wgs1_priv=keys['server'], wgs1_pub=keys['server_pub'],
               wgs1_port='51820', wgs1_psk='0', wgs1_dns='1', wgs1_alive='25',
               wgs1_c1_priv=keys['client'], wgs1_c1_addr='10.9.0.2/32',
               wgs1_c1_caips='0.0.0.0/0', wan0_ipaddr='10.250.0.2',
               lan_ipaddr='10.250.0.2', sw_mode='1')
    cases = [('explicit', '10.9.0.1/24', '0.0.0.0/0', '25', '0.0.0.0/0', '25'),
             ('keepalive-off', '10.9.0.1/24', '0.0.0.0/0', '0', '0.0.0.0/0', 'off'),
             ('bare-ip-fallback', '10.9.0.1', '', '25', '10.9.0.1/32', '25'),
             ('cidr-fallback', '10.9.0.1/24', '', '25', '10.9.0.1/32', '25'),
             ('dual-stack-fallback', '10.9.0.1/24,fd00:9::1/64', '', '25',
              '10.9.0.1/32,fd00:9::1/128', '25')]
    failed = []
    for name, address, allowed, alive, expected_allowed, expected_alive in cases:
        exported = root / 'tmp/export.conf'
        exported.unlink(missing_ok=True)
        env.update(wgs1_addr=address, wgs1_c1_caips=allowed, wgs1_alive=alive)
        try:
            arm(root, '/harness/wg-config', 'export', env=env)
            assert exported.exists(), 'No profile exported'
            text = exported.read_text()
            actual_allowed = next(line for line in text.splitlines() if line.startswith('AllowedIPs = '))
            assert actual_allowed == 'AllowedIPs = ' + expected_allowed, actual_allowed
            # wg setconf consumes crypto/connection options; wg-quick consumes Address/DNS.
            stripped = '\n'.join(line for line in text.splitlines()
                                 if not line.startswith(('Address =', 'DNS =')))
            (root / 'tmp/export-stripped.conf').write_text(stripped + '\n')
            run(wg, 'setconf', 'wgc1', str(root / 'tmp/export-stripped.conf'))
            assert run(wg, 'show', 'wgc1', 'persistent-keepalive').stdout.split()[1] == expected_alive
            shutil.copy2(exported, root / 'tmp/provider.conf')
            arm(root, '/harness/wg-config', 'client')
            run(wg, 'setconf', 'wgc1', str(root / 'tmp/client.conf'))
            actual_alive = run(wg, 'show', 'wgc1', 'persistent-keepalive').stdout.split()[1]
            assert actual_alive == expected_alive, 'Import changed keepalive to ' + actual_alive
            print('PASS server export/import/native setconf: ' + name, flush=True)
        except (AssertionError, RuntimeError) as error:
            failed.append((name, str(error)))
    assert not failed, failed


if __name__ == '__main__':
    main(Path(sys.argv[1]))
