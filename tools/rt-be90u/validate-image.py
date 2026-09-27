#!/usr/bin/env python3
"""Run offline ARM fixtures against an extracted RT-BE90U candidate image."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rootfs', type=Path)
    parser.add_argument('--source', type=Path,
                        default=Path(__file__).resolve().parents[2] / 'release/src-qca-ipq53xx/.build/source')
    parser.add_argument('--logs', type=Path, required=True)
    parser.add_argument('--group', choices=('all', 'basic', 'network', 'openvpn', 'ipv6'), default='all')
    parser.add_argument('--name', default='rt-be90u-validation', help='Unique Docker container-name prefix')
    parser.add_argument('--runtime-image', default='rt-be90u-test:58138')
    parser.add_argument('--network-image', default='rt-be90u-network-test:58138')
    parser.add_argument('--official-rootfs', type=Path, help='Optional extracted stock firmware validator')
    parser.add_argument('--candidate', type=Path, help='Versioned image for the optional stock validator')
    args = parser.parse_args()
    if not re.fullmatch(r'[a-zA-Z0-9][a-zA-Z0-9_.-]*', args.name):
        parser.error('--name must be a valid Docker container-name prefix')
    if bool(args.official_rootfs) != bool(args.candidate):
        parser.error('--official-rootfs and --candidate must be supplied together')
    for path in (args.rootfs, args.source, args.official_rootfs):
        if path and not path.is_dir():
            parser.error('Missing directory: ' + str(path))
    if args.candidate and not args.candidate.is_file():
        parser.error('Missing candidate: ' + str(args.candidate))
    cases = {
        'basic': [('config', 'run-vpn-config-test.sh', [], 'basic'),
                  ('vpn-reset', 'run-vpn-reset-test.sh', [], 'basic'),
                  ('vpn-migration', 'run-vpn-migration-test.sh', [], 'basic'),
                  ('httpd-policy', 'run-httpd-policy-test.sh', [], 'basic'),
                  ('ovpn-wrappers', 'run-ovpn-wrappers-test.sh', [], 'basic'),
                  ('wg-import', 'run-wg-import-test.sh', [], 'basic')],
        'network': [('network', 'run-vpn-network-test.sh', [], 'net'),
                    ('sdn-refresh', 'run-sdn-refresh-test.sh', [], 'net'),
                    ('sdn-default', 'run-sdn-default-test.sh', [], 'net'),
                    ('sdn-concurrency', 'run-sdn-concurrency-test.sh', [], 'net'),
                    ('deferred-migration', 'run-deferred-migration-test.sh', [], 'net'),
                    ('broad-quarantine', 'run-broad-quarantine-test.sh', [], 'net'),
                    ('firewall-lifecycle', 'run-firewall-lifecycle-test.sh', [], 'net'),
                    ('wg-export', 'run-wg-export-test.sh', [], 'net'),
                    ('wg-tunnel', 'run-wg-tunnel-test.sh', [], 'tunnel')],
        'openvpn': [('tunnel-' + mode, 'run-vpn-tunnel-test.sh', ['--mode', mode], 'tunnel')
                    for mode in ('static', 'tls', 'tls-crypt', 'tls-crypt-v2')],
        'ipv6': [('ipv6', 'run-vpn-ipv6-test.sh', [], 'net')],
    }
    if args.candidate:
        cases['basic'].append(('official-validator', 'run-image-runtime-test.sh', [], 'official'))
    selected = list(cases) if args.group == 'all' else [args.group]
    tests = Path(__file__).resolve().parent / 'tests'
    args.logs.mkdir(parents=True, exist_ok=True)
    results = []
    for group in selected:
        for name, script, extra, kind in cases[group]:
            network = kind in ('net', 'tunnel')
            command = ['docker', 'run', '--rm', '--pull=never', '--name', args.name + '-' + name,
                       '--network', 'none', '--read-only', '--ulimit', 'core=0']
            if network:
                command += ['--cap-add', 'NET_ADMIN', '--sysctl', 'net.ipv4.ip_forward=1',
                            '--sysctl', 'net.ipv4.conf.all.rp_filter=0',
                            '--sysctl', 'net.ipv4.conf.default.rp_filter=0',
                            '--sysctl', 'net.ipv6.conf.all.forwarding=1', '--tmpfs', '/etc/iproute2']
            if kind == 'tunnel':
                command += ['--cap-add', 'SYS_ADMIN', '--device', '/dev/net/tun']
            command += ['--tmpfs', '/tmp:exec,' + ('dev,' if kind == 'tunnel' else '') + 'size=512m']
            firmware = args.official_rootfs if kind == 'official' else args.rootfs
            mounts = [(firmware, '/firmware'), (args.source, '/work'), (tests, '/tests')]
            if kind == 'official':
                mounts.append((args.candidate, '/candidate'))
            for source, target in mounts:
                command += ['--mount', 'type=bind,src=%s,dst=%s,readonly' % (source.resolve(), target)]
            command += [args.network_image if network else args.runtime_image, 'sh', '/tests/' + script] + extra
            log_path = args.logs / (name + '.log')
            print('START ' + name, flush=True)
            # Do not silently overwrite evidence from a previous candidate/run.
            with log_path.open('x') as log:
                status = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT).returncode
            results.append({'test': name, 'exit': status, 'command': command})
            (args.logs / (args.group + '-summary.json')).write_text(json.dumps(results, indent=2) + '\n')
            print('END %s %d (%s)' % (name, status, log_path), flush=True)
            if status:
                return status
    return 0


if __name__ == '__main__':
    sys.exit(main())
