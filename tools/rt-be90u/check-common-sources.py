#!/usr/bin/env python3
"""Check shared port code against the upstream and validated IPQ53xx baselines.

This checks selected C tokens across feature profiles, with includes removed.
It does not type-check headers or replace a complete other-model firmware build.
Checks run without lazy fetching. --fetch-baselines explicitly provisions the
two baseline commits and the selected source blobs first, for shallow CI clones.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import subprocess

FILES = ['release/src/router/libovpn/' + name for name in (
    'amvpn_routing.c', 'amvpn_routing.h', 'openvpn_config.c', 'openvpn_config.h',
    'openvpn_control.c', 'openvpn_control.h', 'openvpn_options.c', 'openvpn_setup.c',
)] + ['release/src/router/shared/scripts.c',
      'release/src/router/rc/openvpn.c', 'release/src/router/rc/wireguard.c',
      'release/src/router/rc/vpnc_fusion.c']
QCA_RUNTIME_FILES = {
    'release/src/router/libovpn/amvpn_routing.c',
    'release/src/router/libovpn/amvpn_routing.h',
    'release/src/router/rc/vpnc_fusion.c',
}
FEATURES = ('RTCONFIG_IPV6', 'RTCONFIG_MULTILAN_CFG', 'RTCONFIG_WIREGUARD', 'RTCONFIG_AUTO_WANPORT')
RC_FEATURES = ('RTCONFIG_VPN_FUSION', 'RTCONFIG_VPN_FUSION_MERLIN')
PLATFORMS = {
    'generic': [],
    'bcm-hnd': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER'],
    'bcm-6756': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER', 'RTCONFIG_HND_ROUTER_AX_6756'],
    'bcm-502': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER', 'RTCONFIG_BCM_502L07P2'],
    'bcm-675x': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER', 'RTCONFIG_HND_ROUTER_AX_675X'],
    'bcm-4916': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER', 'RTCONFIG_HND_ROUTER_BE_4916'],
    'bcm-mfg': ['RTCONFIG_BCMARM', 'HND_ROUTER', 'RTCONFIG_HND_ROUTER', 'RTCONFIG_BCM_MFG'],
    'ipq53xx': ['RTCONFIG_SOC_IPQ53XX'],
}
TOKEN = re.compile(r'''(?:u8|[LuU])?"(?:\\.|[^"\\])*"|(?:[LuU])?'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|\d+(?:\.\d*)?|>>=|<<=|\.\.\.|\+\+|--|->|&&|\|\||<=|>=|==|!=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<|>>|\S''')


def git_source(repo, revision, name, fetch=False):
    return subprocess.check_output(
        ['git', 'show', revision + ':' + name], cwd=repo,
        env=dict(os.environ, GIT_NO_LAZY_FETCH='0' if fetch else '1'), text=True)


def preprocess(source, definitions, compiler):
    source = re.sub(r'^\s*#\s*include\b[^\n]*', '', source, flags=re.M)
    command = [compiler, '-E', '-P', '-undef', '-nostdinc', '-x', 'c', '-', '-D__LINE__=0']
    command += ['-D' + name + '=1' for name in definitions]
    return subprocess.run(command, input=source, text=True, capture_output=True)


def tokens(source, definitions, compiler):
    result = preprocess(source, definitions, compiler)
    result.check_returncode()
    return TOKEN.findall(result.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', default='920b77f5f92db14717a27abd5c8e1b06ae6c8ec1')
    parser.add_argument('--qca-base', default='2db9b885dec71cb4971b54edf7ad31d00063d842')
    parser.add_argument('--compiler', default='cc')
    parser.add_argument('--fetch-baselines', action='store_true',
                        help='Fetch pinned baseline objects from origin before offline comparisons')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    if args.fetch_baselines:
        for revision, prefix in ((args.base, ''),
                                 (args.qca_base, 'release/src-qca-ipq53xx/source-overlay/')):
            if not re.fullmatch(r'[0-9a-f]{40}', revision):
                parser.error('--fetch-baselines requires full commit IDs')
            subprocess.run(['git', 'fetch', '--no-tags', '--depth=1', '--filter=blob:none',
                            'origin', revision], cwd=repo, check=True)
            for name in FILES:
                git_source(repo, revision, ('' if name.endswith('/rc/vpnc_fusion.c') else prefix) + name, fetch=True)
    results = []
    for name in FILES:
        upstream = git_source(repo, args.base, name)
        original_hash = hashlib.sha256(upstream.encode()).hexdigest()
        adjustments = []
        qca_adjustments = []
        if name.endswith('/openvpn_options.c'):
            # The old format has two %d placeholders but only one argument.
            # This existing port fix is valid on all architectures.
            before = '"Options error: Residual parse state (%d) in %d", line_num'
            after = '"Options error: Residual parse state in %d", line_num'
            if upstream.count(before) != 1:
                raise ValueError('Unexpected upstream parser diagnostic; review the baseline')
            upstream = upstream.replace(before, after)
            adjustments.append('Correct the residual-state diagnostic format/argument mismatch')
        qca = git_source(repo, args.qca_base,
                         ('' if name.endswith('/rc/vpnc_fusion.c') else
                          'release/src-qca-ipq53xx/source-overlay/') + name)
        if name in QCA_RUNTIME_FILES:
            qca_adjustments.append('Changed QCA routing/default behavior requires separate compiled runtime validation; QCA token comparison omitted')
        current = (repo / name).read_text()
        checks = 0
        rejected = 0
        runtime_profiles = 0
        features = FEATURES + (RC_FEATURES if '/rc/' in name else ())
        for platform, definitions in PLATFORMS.items():
            for flags in itertools.product((False, True), repeat=len(features)):
                selected = definitions + [name for name, enabled in zip(features, flags) if enabled]
                expected = qca if platform == 'ipq53xx' else upstream
                if platform == 'ipq53xx' and name in QCA_RUNTIME_FILES:
                    runtime_profiles += 1
                    continue
                if (name.endswith('/rc/openvpn.c') and platform == 'ipq53xx'
                        and 'RTCONFIG_VPN_FUSION_MERLIN' not in selected):
                    diagnostic = '#error "RT-BE90U VPN variant requires RTCONFIG_VPN_FUSION_MERLIN"'
                    for source in (current, expected):
                        result = preprocess(source, selected, args.compiler)
                        errors = [line.split('error: ', 1)[1] for line in result.stderr.splitlines()
                                  if 'error: ' in line]
                        if not result.returncode or errors != [diagnostic]:
                            raise ValueError('Expected the IPQ53xx Merlin-VPN configuration guard')
                    rejected += 1
                    continue
                if tokens(current, selected, args.compiler) != tokens(expected, selected, args.compiler):
                    raise ValueError('Conditional code differs: %s (%s)' % (name, ','.join(selected) or 'generic'))
                checks += 1
        results.append({'file': name, 'profiles_checked': checks,
                        'invalid_profiles_rejected': rejected,
                        'qca_profiles_requiring_runtime_validation': runtime_profiles,
                        'upstream_sha256': original_hash, 'upstream_adjustments': adjustments,
                        'ipq53xx_adjustments': qca_adjustments})
    print(json.dumps({'base': args.base, 'qca_base': args.qca_base,
                      'files': results, 'comparisons_passed': sum(r['profiles_checked'] for r in results),
                      'invalid_profiles_rejected': sum(r['invalid_profiles_rejected'] for r in results),
                      'qca_profiles_requiring_runtime_validation': sum(r['qca_profiles_requiring_runtime_validation'] for r in results),
                      'scope': 'Non-QCA and unaffected QCA C tokens with includes removed; changed QCA routing/default files explicitly omitted and require runtime validation; not a full other-model compile'}, indent=2))


if __name__ == '__main__':
    main()
