#!/usr/bin/env python3
"""Validate and extract a newly built CI image, then audit packaged ELF linkage.

Run inside the build container. No router programs are executed by this check.
The destination must not exist, to avoid mixing different candidate images.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('rootfs', type=Path)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    if args.rootfs.exists():
        parser.error('Extraction destination already exists: ' + str(args.rootfs))
    tools = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location('check_image', tools / 'check-image.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    data = args.image.read_bytes()
    result = module.check_image(data)
    offsets = []
    start = 0
    while True:
        offset = data.find(b'hsqs', start)
        if offset < 0:
            break
        # Require SquashFS 4.0 as well as the magic, avoiding incidental matches.
        if data[offset + 28:offset + 32] == b'\x04\x00\x00\x00':
            offsets.append(offset)
        start = offset + 4
    if len(offsets) != 1:
        parser.error('Expected one SquashFS 4.0 filesystem; found ' + str(offsets))
    result['squashfs_offset'] = offsets[0]
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    subprocess.run(['unsquashfs', '-no-progress', '-no-xattrs', '-o', str(offsets[0]),
                    '-d', str(args.rootfs), str(args.image)], check=True)
    audit = subprocess.run(['python3', str(tools / 'check-linkage.py'), str(args.rootfs),
                            '/sbin/rc', '/usr/sbin/httpd', '/usr/sbin/openvpn'],
                           stdout=subprocess.PIPE, text=True)
    args.report.with_name('linkage.json').write_text(audit.stdout)
    audit.check_returncode()
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
