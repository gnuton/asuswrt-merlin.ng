#!/bin/sh
# Actual extracted libshared parser, synthetic NVRAM, no network or router writes.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu"
mkdir -p "$test_dir/root/tmp" "$test_dir/root/dev"
touch "$test_dir/root/dev/null"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
    -Wall -Wextra -Werror -Wl,-E /tests/wg-import-test.c \
    -L/firmware/usr/lib -Wl,-rpath-link,/firmware/usr/lib:/firmware/lib -lshared -lnvram \
    -o "$test_dir/root/tmp/driver"
python3 - "$test_dir/root" <<'PY'
import subprocess
import sys

failed = []
for case in ('valid', 'whitespace', 'unbracketed-ipv6', 'long-allowedips', 'missing-file', 'empty',
             'missing-equals', 'wrong-key', 'multiple-peers', 'unsupported-option',
             'oversized-allowedips', 'unit-zero', 'unit-six', 'invalid-mtu',
             'invalid-endpoint', 'invalid-prefix', 'duplicate-key', 'embedded-nul'):
    result = subprocess.run(['chroot', sys.argv[1], '/qemu', '/tmp/driver', case], timeout=5)
    if result.returncode:
        failed.append((case, result.returncode))
if failed:
    raise SystemExit('WireGuard import failures: ' + str(failed))
print('All ARM WireGuard parser and failure-preservation checks passed')
PY
