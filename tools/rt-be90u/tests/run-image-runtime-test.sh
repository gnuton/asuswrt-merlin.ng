#!/bin/sh
# /firmware: vendor or candidate rootfs; /candidate: versioned BE9400 image.
# Offline only. Never runs a flash utility or touches a router.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
mkdir -p "$test_dir/root/tmp/etc" "$test_dir/root/dev"
cp -a /firmware/rom/etc/. "$test_dir/root/tmp/etc/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
    -Wall -Wextra -Werror -Wl,-E /tests/image-runtime-driver.c \
    -L/firmware/usr/lib -Wl,-rpath-link,/firmware/usr/lib:/firmware/lib -lshared -lnvram \
    -o "$test_dir/root/tmp/driver"
python3 - "$test_dir/root" <<'PY'
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import zlib

root = Path(sys.argv[1])
original = Path('/candidate').read_bytes()
results = {}
for name in ('candidate', 'bad-magic', 'bad-payload', 'wrong-model'):
    data = bytearray(original)
    if name == 'bad-magic':
        data[0] ^= 1
    elif name == 'bad-payload':
        data[-8] ^= 1
    elif name == 'wrong-model':
        data[36:48] = b'WRONG-MODEL'.ljust(12, b'\0')
        data[4:8] = bytes(4)
        struct.pack_into('>I', data, 4, zlib.crc32(data[:64]))
    (root / 'tmp/test.trx').write_bytes(data)
    result = subprocess.run(['chroot', str(root), '/qemu', '/tmp/driver', '/tmp/test.trx'],
                            check=True, capture_output=True, text=True, timeout=30)
    fields = next(line.split()[1:] for line in result.stdout.splitlines() if line.startswith('RESULT '))
    header, size, image = map(int, fields)
    results[name] = {'header_accepted': bool(header), 'bytes': size, 'image_result': image}
assert results['candidate'] == {'header_accepted': True, 'bytes': len(original), 'image_result': 0}, results
assert not results['bad-magic']['header_accepted'], results
assert results['bad-payload']['image_result'] != 0, results
print(json.dumps({'candidate_sha256': hashlib.sha256(original).hexdigest(),
                  'validator_sha256': hashlib.sha256(Path('/firmware/usr/lib/libshared.so').read_bytes()).hexdigest(),
                  'checks': results}, indent=2))
if results['wrong-model']['header_accepted'] and results['wrong-model']['image_result'] == 0:
    print('KNOWN LIMIT: these validator calls do not enforce the model under synthetic NVRAM')
print('No conclusion about live upload, bootloader signature policy, boot or recovery')
PY
