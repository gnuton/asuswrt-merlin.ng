#!/usr/bin/env python3
"""Link the actual rc dispatcher, trapping unrelated services, then test resets."""
import hashlib
from pathlib import Path
import re
import subprocess
import sys


def main():
    work = Path(sys.argv[1])
    obj = Path('/work/release/src/router/rc/services.o')
    print('services.o sha256:', hashlib.sha256(obj.read_bytes()).hexdigest(), flush=True)
    exports = work / 'exports.list'
    exports.write_text('{ nvram_get; nvram_set; nvram_unset; nvram_commit; '
                       'logmessage; logmessage_normal; run_custom_script; system; };\n')
    command = ['aarch64-openwrt-linux-musl-gcc', '-Wall', '-Wextra', '-Werror',
               '-DTUFBE6500', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-Wl,--dynamic-list,' + str(exports),
               '-I/work/release/src-qca-ipq53xx/include',
               '-I/work/release/src/router/shared', '-I/work/release/src/router/libovpn',
               '/tests/vpn-reset-test.c', str(obj), '-L/firmware/usr/lib',
               '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
               '-lovpn', '-lshared', '-lnvram', '-lcrypto', '-lssl',
               '-o', str(work / 'root/tmp/vpn-reset-test')]
    # handle_notifications references many unrelated rc services. Resolve those
    # to fatal traps, never silent no-ops. Keep actual reset functions/defaults in
    # the packaged libraries, and explicitly implement observable test doubles.
    first = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    names = sorted(set(re.findall(r"undefined reference to `([^']+)'", first.stderr)))
    protected = re.compile(r'(?:.*(?:ovpn|wgc|nvram).*|router_defaults|run_custom_script)')
    if not names or any(not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', n) for n in names):
        raise RuntimeError('Unexpected initial link result:\n' + first.stderr)
    # VPN lifecycle calls other than the three observed stops also get traps;
    # reset/config/key operations must always resolve from the real libraries.
    allowed_vpn_traps = {'start_ovpn_client', 'start_ovpn_server', 'start_wgc',
                         'start_wgcall', 'stop_wgcall', 'create_ovpn_passwd'}
    forbidden = [n for n in names if protected.fullmatch(n) and n not in allowed_vpn_traps]
    if forbidden:
        raise RuntimeError('Missing required VPN/NVRAM symbols: ' + repr(forbidden))
    stubs = work / 'traps.c'
    stubs.write_text('extern void fixture_unexpected(const char *);\n' + ''.join(
        'void %s(void) { fixture_unexpected("%s"); }\n' % (n, n) for n in names))
    subprocess.run(command + [str(stubs)], check=True)
    print('Unrelated rc entry points guarded by fatal traps:', len(names), flush=True)
    run = ['chroot', str(work / 'root'), '/qemu', '/tmp/vpn-reset-test']
    cases = 0

    def case(kind, unit, stop, command):
        nonlocal cases
        print('CASE', command, flush=True)
        subprocess.run(run + ['seed'], check=True)
        subprocess.run(run + ['run', str(kind), str(unit), str(stop), command], check=True)
        # A new ARM process must see exactly the committed data and files.
        subprocess.run(run + ['verify', str(kind), str(unit)], check=True)
        if unit:
            # Repeating a reset must preserve the same defaults and neighbours.
            subprocess.run(run + ['run', str(kind), str(unit), str(stop), command], check=True)
            subprocess.run(run + ['verify', str(kind), str(unit)], check=True)
        cases += 1

    for kind, reset, stop, maximum in [(0, 'clearovpnserver', 'stop_vpnserver', 2),
                                        (1, 'clearovpnclient', 'stop_vpnclient', 5),
                                        (2, 'clearwgclient', 'stop_wgc ', 5)]:
        for unit in range(1, maximum + 1):
            case(kind, unit, 1, '%s%d;%s %d' % (stop, unit, reset, unit))
        case(kind, 1, 0, reset + ' 01')
        for argument in ['', '0', '-1', str(maximum + 1), '1junk', '+1', '1.0',
                         '999999999999999999999999999999', '1 extra']:
            case(kind, 0, 0, (reset + ' ' + argument).rstrip())
    print('VPN reset dispatcher/persistence cases passed:', cases,
          '(including repeated resets for all 15 valid requests)', flush=True)


if __name__ == '__main__':
    main()
