#!/usr/bin/env python3
"""Compiled boot conversion, real packaged libraries and disposable persistence."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    work = Path(sys.argv[1])
    root = work / 'root'
    exports = work / 'exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; '
                       'logmessage; logmessage_normal; mkstemp; write; fsync; close; rename; };\n')
    objects = [Path('/work/release/src/router/rc') / (s + '.o') for s in ('init', 'format', 'vpn_migrate')]
    for obj in objects:
        print(obj.name, hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror', '-DTUFBE6500',
               '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--dynamic-list,' + str(exports), '-I/work/release/src-qca-ipq53xx/include',
               '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
               '/tests/vpn-migration-test.c'] + [str(o) for o in objects]
    for symbol in ('adjust_url_urlelist', 'adjust_ddns_config', 'adjust_access_restrict_config',
                   'sync_nc_conf', 'fsync', 'link'):
        command += ['-Wl,--wrap=' + symbol]
    command += ['-L/firmware/usr/lib', '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
                '-lovpn', '-lshared', '-lnvram', '-ldl', '-o', str(root / 'tmp/migrate')]
    subprocess.run(command, check=True)
    run = ['chroot', str(root), '/qemu', '/tmp/migrate']
    backup = root / 'jffs/openvpn/fusion-migration.backup'
    policy = root / 'jffs/openvpn/vpndirector_rulelist'
    calls = 0

    def write_db(path, values):
        path.write_bytes(b''.join(k.encode() + b'\0' + v.encode() + b'\0' for k, v in sorted(values.items())))

    def read_db(path):
        data = path.read_bytes().split(b'\0')
        assert data.pop() == b''
        return {data[i].decode(): data[i + 1].decode() for i in range(0, len(data), 2)}

    def seed(values):
        shutil.rmtree(root / 'jffs/openvpn')
        (root / 'jffs/openvpn').mkdir()
        write_db(root / 'tmp/nvram.db', values)

    def call(operation='migrate', fault='', argument=None, crash=False):
        nonlocal calls
        p = subprocess.run(run + [operation, fault] + ([] if argument is None else [argument]), timeout=30)
        assert p.returncode == (77 if crash else 0), (operation, fault, p.returncode)
        calls += 1
        if crash:
            return None
        return (read_db(root / 'tmp/result.db'), json.loads((root / 'tmp/report.json').read_text()),
                (root / 'tmp/rules').read_text())

    sdn = '<0>DEFAULT>1>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>WEB>0>0>0'
    base = {'vpnc_clientlist': '<OVPN>OpenVPN>1>user>password>1>5>>>0>0>Web'
                             '<WG>WireGuard>1>>>1>6>>>0>0>Web<Other>PPTP>example>>>0>7',
            'vpn_clientx_eas': '1,2,', 'wgc1_enable': '1', 'wgc2_enable': '0',
            'vpn_client1_addr': 'vpn.fixture.example', 'wgc1_priv': 'synthetic-private',
            'wgc1_ppub': 'synthetic-peer', 'wgc1_addr': '192.0.2.2/32',
            'vpnc_pptp_options_x_list': '<<<+mppe-128',
            'vpn_client2_desc': 'Neighbour', 'vpn_server1_port': '1194',
            'vpnc_dev_policy_list': '<1>192.0.2.10>>5<0>192.0.2.20>198.51.100.0/24>6<1>192.0.2.30>>0',
            'sdn_rl': sdn + '<1>Guest>1>1>1>1>6>0>0>0>0>0>0>0>1',
            'vpnc_default_wan': '5', 'vpn_client1_custom': 'remote fixture.example 443\n',
            'unrelated': 'keep'}
    expected = dict(base, vpnc_clientlist='<Other>PPTP>example>>>0>7',
                    vpnc_pptp_options_x_list='<+mppe-128',
                    vpn_client1_username='user', vpn_client1_password='password',
                    vpn_client1_desc='OVPN', vpn_client1_rgw='2', wgc1_desc='WG',
                    vpnc_default_wan='0', vpnc_dev_policy_list='', qca_merlin_vpn_migrated='1',
                    vpndirector_rulelist='<1>Fusion rule 1>192.0.2.10>>OVPN1'
                                        '<0>Fusion rule 2>192.0.2.20>198.51.100.0/24>WGC1'
                                        '<1>Fusion rule 3>192.0.2.30>>WAN')
    # Build the SDN expectation from documented field positions, not the converter.
    row = sdn[1:].split('>'); row[6] = '6'
    expected['sdn_rl'] = '<' + '>'.join(row) + '<1>Guest>1>1>1>1>1>0>0>0>0>0>0>0>1'
    seed(base)
    key = root / 'jffs/openvpn/vpn_crt_client1_key'
    key.write_text('fixture key')
    state, report, rules = call('boot')
    assert state == expected, (state, expected)
    assert report['commits'] == 1 and rules == expected['vpndirector_rulelist']
    assert key.read_text() == 'fixture key' and not policy.exists()
    state, report, _ = call('reader')
    assert state == expected and report['sets'] == 0
    assert backup.stat().st_mode & 0o777 == 0o600
    saved_backup = backup.read_bytes()
    assert b'password\0' in saved_backup and b'vpnc_clientlist\0' in saved_backup
    state, report, _ = call('boot')
    assert state == expected and report['commits'] == 0 and report['sets'] == 0
    state, report, _ = call('reset')
    assert state['vpn_clientx_eas'] == '2,' and 'wgc1_enable' not in state
    after_reset = state
    state, report, _ = call('boot')
    assert state == after_reset and report['commits'] == 0
    assert backup.read_bytes() == saved_backup
    print('PASS actual boot chain, state conversion, fresh-process reload, repeat boot and reset', flush=True)

    # A pure profile read must not reapply stale active flags, even with conflicts.
    conflict = dict(base, vpn_clientx_eas='2,', wgc1_enable='0')
    seed(conflict)
    state, report, _ = call('reader')
    assert state == conflict and report['sets'] == 0 and report['commits'] == 0
    provider = dict(conflict, vpnc_clientlist='Provider>NordVPN>1>>>1>5>us>udp>0>0>Web',
                    wgc1_tp='nordvpn', wgc1_tp_region='us', vpnc_pptp_options_x_list='<')
    seed(provider)
    state, report, _ = call('reader')
    assert state == provider and report['sets'] == 0 and report['commits'] == 0
    print('PASS retained Fusion reader has no enable-state writes', flush=True)

    invalid = [dict(base, vpn_clientx_eas='2,'), dict(base, wgc1_enable='0'),
               dict(base, qca_merlin_vpn_migrated='2'), dict(base, restart_wifi='1'),
               dict(base, asus_stop_commit=''),
               dict(base, vpn_client1_addr=''), dict(base, wgc1_priv=''),
               dict(base, wgc1_ppub=''), dict(base, wgc1_addr=''),
               dict(base, vpnc_pptp_options_x_list='<+mppe-128'),
               dict(base, vpnc_pptp_options_x_list='<<<+mppe-128<extra'),
               dict(base, vpn_client1_desc='Edited'), dict(base, vpn_client1_password='Edited'),
               dict(base, vpn_client1_rgw='1'), dict(base, vpndirector_rulelist='Existing policy'),
               dict(base, vpnc_default_wan='7'), dict(base, vpnc_default_wan_tmp='6'),
               dict(base, vpnc_dev_policy_list='<1>192.0.2.10>>7'),
               dict(base, vpnc_dev_policy_list='<1>192.0.2.10>>5>br1'),
               dict(base, vpnc_dev_policy_list='<1>192.0.2.10;bad>>5'),
               dict(base, vpnc_dev_policy_list='<1>192.0.2.10/33>>5'),
               dict(base, vpnc_dev_policy_list='<1>2001:db8::1>>5'),
               dict(base, sdn_rl='<0>DEFAULT>1>0>0>0>7'),
               dict(base, sdn_rl=''), dict(base, sdn_rl='<0>DEFAULT>1>0>0>0>6')]
    for profile in ['Bad>OpenVPN>0>>>1>5', 'Bad>OpenVPN>6>>>1>5',
                    'Bad>WireGuard>-1>>>1>5', 'Bad>WireGuard>1junk>>>1>5',
                    'Bad>OpenVPN>1>>>2>5', 'Bad>OpenVPN>1>>>1>21',
                    'Bad>OpenVPN>1>>>1>99999999999999999999999999999',
                    'Old>OpenVPN>1>>', 'Provider>OpenVPN>1>>>1>5>region',
                    'Duplicate>OpenVPN>1>>>1>5<Duplicate>WireGuard>1>>>1>5',
                    'WG>WireGuard>1>>>1>5<Provider>NordVPN>1>>>1>6>us>udp',
                    'Provider>NordVPN>1>>>1>6>us>udp<WG>WireGuard>1>>>1>5',
                    'Duplicate>OpenVPN>1>>>1>5<Duplicate>OpenVPN>1>>>1>6']:
        invalid.append(dict(base, vpnc_clientlist=profile))
    for source in invalid:
        seed(source)
        state, report, _ = call()
        assert report['result'] == -1 and report['sets'] == report['commits'] == 0, source
        assert state == source and not backup.exists(), source
    print('PASS', len(invalid), 'conflicting/unsupported/malformed inputs leave all settings untouched', flush=True)

    # Faults at every NVRAM set, commit, backup sync/link, and interruption before
    # commit must retain old durable state and allow an idempotent retry.
    seed(base)
    _, good, _ = call()
    for fault in [str(n) for n in range(1, good['sets'] + 1)] + ['commit', 'sync1', 'sync2', 'sync3', 'link']:
        seed(base)
        state, report, _ = call(fault=fault)
        assert state == base and read_db(root / 'tmp/nvram.db') == base, fault
        assert report['result'] == -1 and report['injected'] > 0, fault
        state, report, rules = call()
        assert state == expected and report['commits'] == 1 and rules == expected['vpndirector_rulelist'], fault
    seed(base)
    call(fault='crash', crash=True)
    assert read_db(root / 'tmp/nvram.db') == base
    state, report, _ = call('boot')
    assert state == expected and report['commits'] == 1
    print('PASS backup/write/commit fault rollback and fresh-process interrupted-commit retry', flush=True)

    for path in (backup, policy):
        seed(base); path.write_text('Existing user data')
        state, report, _ = call()
        assert state == base and report['commits'] == 0 and report['result'] == -1
        assert path.read_text() == 'Existing user data'
    seed(base)
    backup.symlink_to('/tmp/do-not-follow')
    state, report, _ = call()
    assert state == base and report['result'] == -1 and not (root / 'tmp/do-not-follow').exists()
    seed(base); os.mkfifo(backup)
    state, report, _ = call()
    assert state == base and report['result'] == -1 and report['commits'] == 0
    print('PASS existing backup/policy and symlink conflicts are not overwritten', flush=True)

    seed(base); call(fault='commit'); backup.chmod(0o644)
    state, report, _ = call()
    assert state == base and report['commits'] == 0 and report['result'] == -1
    seed(base); policy.write_text(expected['vpndirector_rulelist'])
    state, report, rules = call()
    assert state == expected and rules == expected['vpndirector_rulelist'] and report['commits'] == 1

    seed(base); call()
    policy.mkdir()
    state, report, rules = call('policy', argument='')
    assert report['result'] == -1 and report['commits'] == 0
    assert state['vpndirector_rulelist'] == expected['vpndirector_rulelist'] and rules == ''
    policy.rmdir()
    state, report, rules = call('policy', argument='')
    assert not rules and report['commits'] == 1 and 'vpndirector_rulelist' not in state
    assert policy.is_file() and policy.read_text() == ''
    assert call('read')[2] == ''
    print('PASS policy fallback, filesystem errors, explicit empty UI save and fallback retirement', flush=True)

    replacement = '<1>Replacement>192.0.2.99>>WAN'
    original = '<1>Existing>192.0.2.88>>OVPN2'
    for existing in (None, original):
        for fault in ('policy-parent-sync', 'policy-write', 'policy-partial', 'policy-zero', 'policy-fsync',
                      'policy-close', 'policy-rename'):
            seed(expected)
            if existing is not None:
                policy.write_text(existing)
            state, report, rules = call('policy', fault=fault, argument=replacement)
            assert report['result'] == -1 and report['commits'] == 0 and report['injected'] == 1, fault
            assert state == expected and read_db(root / 'tmp/nvram.db') == expected, fault
            assert rules == (expected['vpndirector_rulelist'] if existing is None else existing), fault
            assert policy.exists() == (existing is not None), fault
            if existing is not None:
                assert policy.read_text() == existing, fault
            assert not list(policy.parent.glob('.vpndirector_rulelist.*')), fault
            assert call('read')[2] == rules, fault
    # A directory-sync failure occurs after atomic publication: the complete
    # new file may be visible, but fallback retirement must remain uncommitted.
    seed(expected)
    state, report, rules = call('policy', fault='policy-dir-sync', argument=replacement)
    assert report['result'] == -1 and report['commits'] == 0 and report['injected'] == 1
    assert state == expected and read_db(root / 'tmp/nvram.db') == expected
    assert rules == replacement and policy.read_text() == replacement
    assert not list(policy.parent.glob('.vpndirector_rulelist.*'))
    state, report, rules = call('policy', argument=replacement)
    assert report['result'] == 0 and report['commits'] == 1 and rules == replacement
    assert 'vpndirector_rulelist' not in state and policy.stat().st_mode & 0o777 == 0o600
    state, report, rules = call('policy', argument='')
    assert report['result'] == 0 and report['commits'] == 1 and rules == '' and policy.read_text() == ''
    assert call('read')[2] == ''
    print('PASS atomic policy publication preserves fallback and existing policy on write failures', flush=True)

    for unit in range(1, 6):
        source = {'vpnc_clientlist': 'C>OpenVPN>%d>>>1>5<W>WireGuard>%d>>>0>6' % (unit, unit),
                  'vpn_client%d_addr' % unit: 'vpn.fixture.example',
                  'sdn_rl': sdn, 'vpnc_dev_policy_list': '', 'unrelated': 'keep'}
        seed(source)
        initialized, _, _ = call('defaults')
        assert 'vpn_clientx_eas' not in initialized
        assert initialized['lan_ipaddr'] and len(initialized) > len(source)
        state, report, _ = call('boot')
        assert state['vpn_clientx_eas'] == '%d,' % unit and state['wgc%d_enable' % unit] == '0'
        assert state['vpn_client%d_desc' % unit] == 'C' and state['wgc%d_desc' % unit] == 'W'
        assert report['commits'] == 1 and state['vpnc_clientlist'] == ''

    # Actual router-default initialization must preserve explicit disable edits
    # while leaving a missing enable list available for stock migration.
    for eas in ('', '2,'):
        source = dict(base, vpn_clientx_eas=eas)
        seed(source)
        initialized, _, _ = call('defaults')
        assert initialized['vpn_clientx_eas'] == eas
        state, report, _ = call()
        assert state == initialized and report['result'] == -1 and report['commits'] == 0
    source = dict(base, vpnc_dev_policy_list='<1>192.0.2.10>>5>br1')
    del source['vpn_clientx_eas']
    seed(source)
    for _ in range(2):
        initialized, _, _ = call('defaults')
        assert 'vpn_clientx_eas' not in initialized
        state, report, _ = call()
        assert state == initialized and report['result'] == -1 and report['commits'] == 0
    corrected = dict(state, vpnc_dev_policy_list=base['vpnc_dev_policy_list'])
    seed(corrected)
    call('defaults')
    state, report, _ = call('boot')
    assert state['vpn_clientx_eas'] == '1,' and report['commits'] == 1
    for source, operation in (({}, 'defaults'),
                              ({'qca_merlin_vpn_migrated': '1'}, 'defaults'),
                              (base, 'factory-defaults')):
        seed(source)
        state, _, _ = call(operation)
        assert state['vpn_clientx_eas'] == ''
    print('PASS initialized defaults, explicit disable edits, deferred reboot/retry and factory defaults', flush=True)
    for source in ({}, {'vpnc_clientlist': ''}, {'vpnc_clientlist': 'Keep>PPTP>example>>>0>5'}):
        seed(source); state, report, _ = call('boot')
        assert state == source and report['commits'] == 0 and not backup.exists()
    print('PASS all client slots, older seven-field records and empty/unrelated configurations', flush=True)
    print('VPN migration fixture passed:', calls, 'ARM process invocations', flush=True)


if __name__ == '__main__':
    main()
