#!/usr/bin/env python3
"""Check RT-BE90U uImage structure; this does not establish flash safety."""

import argparse
import gzip
import hashlib
import json
import lzma
from pathlib import Path
import struct
import zlib


HEADER = struct.Struct('>7I4B32s')
LINUX_VOLUME_BYTES = 0x0500B000
VPN_KERNEL_OPTIONS = ('CONFIG_IPV6', 'CONFIG_IP_MULTIPLE_TABLES',
                      'CONFIG_IPV6_MULTIPLE_TABLES', 'CONFIG_FIB_RULES')


def check_kernel_config(payload):
    # Inspect the shipped kernel, not the build input or the test host kernel.
    # SquashFS follows the LZMA stream, so decompress only the first stream.
    try:
        decoder = lzma.LZMADecompressor(format=lzma.FORMAT_ALONE)
        kernel = decoder.decompress(payload)
        if not decoder.eof:
            raise ValueError('Truncated LZMA kernel')
        start = kernel.find(b'IKCFG_ST')
        end = kernel.find(b'IKCFG_ED', start + 8)
        if start < 0 or end < 0:
            raise ValueError('Missing embedded kernel configuration')
        config = gzip.decompress(kernel[start + 8:end]).decode('ascii')
    except (lzma.LZMAError, OSError, EOFError, UnicodeError) as error:
        raise ValueError('Invalid embedded kernel configuration: ' + str(error)) from error
    values = dict(line.split('=', 1) for line in config.splitlines() if line.startswith('CONFIG_') and '=' in line)
    missing = [name for name in VPN_KERNEL_OPTIONS if values.get(name) != 'y']
    if missing:
        raise ValueError('Kernel lacks required VPN policy routing: ' + ', '.join(missing))
    return {name: values[name] for name in VPN_KERNEL_OPTIONS}


def check_image(data):
    if len(data) < HEADER.size:
        raise ValueError('Truncated uImage header')
    magic, hcrc, timestamp, size, load, entry, dcrc, os_id, arch, kind, comp, name = HEADER.unpack_from(data)
    if magic != 0x27051956:
        raise ValueError('Unexpected uImage magic')
    if zlib.crc32(data[:4] + bytes(4) + data[8:HEADER.size]) != hcrc:
        raise ValueError('Header CRC32 mismatch')
    if size == 0 or len(data) != HEADER.size + size:
        raise ValueError('Payload length mismatch')
    if len(data) > LINUX_VOLUME_BYTES:
        raise ValueError('Image exceeds the observed linux UBI volume')
    if zlib.crc32(data[HEADER.size:]) != dcrc:
        raise ValueError('Payload CRC32 mismatch')
    if (os_id, arch, kind, comp) != (5, 22, 2, 3):
        raise ValueError('Expected Linux/AArch64/kernel/LZMA image')
    if (load, entry) != (0x40080000, 0x40080000):
        raise ValueError('Unexpected load/entry addresses')
    # ASUS encodes four version bytes before the model in the name field.
    if name[:4] != bytes((3, 0, 0, 6)):
        raise ValueError('Unexpected ASUS firmware version')
    model = name[4:16].rstrip(b'\x00')
    if model != b'TUF-BE9400':
        raise ValueError('Expected the TUF-BE9400 hardware product ID')
    return {
        'bytes': len(data),
        'sha256': hashlib.sha256(data).hexdigest(),
        'model': model.decode('ascii'),
        'timestamp': timestamp,
        'header_crc32': 'valid',
        'payload_crc32': 'valid',
        'kernel_config': check_kernel_config(data[HEADER.size:]),
        'flash_compatibility': 'not established',
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    try:
        result = check_image(args.image.read_bytes())
    except (OSError, ValueError) as error:
        parser.exit(1, f'{error}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
