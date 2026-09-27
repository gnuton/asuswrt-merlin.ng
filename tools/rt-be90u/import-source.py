#!/usr/bin/env python3
"""One-time, offline import of a verified SDK into an empty IPQ53xx platform tree.

Builds do not run this importer. It separates identical common files from platform
sources and vendor overrides without changing an existing upstream source file.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import zipfile

BASE = '920b77f5f92db14717a27abd5c8e1b06ae6c8ec1'
ARCHIVE_SHA = '930142d207b1f9dfc11e2ad17c767abc59a5df5777223dfc99f12a69f1d07b9e'


def contents(path):
    return os.fsencode(os.readlink(path)) if path.is_symlink() else path.read_bytes()


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    if source.is_symlink():
        target.symlink_to(os.readlink(source))
    else:
        shutil.copy2(source, target)


def files(root):
    return sorted(p for p in root.rglob('*') if p.is_symlink() or p.is_file())


def verify_sdk(sdk, archive):
    expected = set()
    with zipfile.ZipFile(archive) as outer:
        names = outer.namelist()
        if len(names) != 1 or not names[0].endswith('.tgz'):
            raise SystemExit('Unexpected SDK archive layout')
        with outer.open(names[0]) as compressed, tarfile.open(fileobj=compressed, mode='r|gz') as source:
            for member in source:
                if member.isdir():
                    continue
                name = Path(member.name)
                if name.is_absolute() or '..' in name.parts or name.parts[0] != 'asuswrt':
                    raise SystemExit('Unexpected archive path: ' + member.name)
                relative = name.relative_to('asuswrt')
                path = sdk / relative
                expected.add(relative)
                if member.issym():
                    valid = path.is_symlink() and os.readlink(path) == member.linkname
                elif member.isfile():
                    valid = (path.is_file() and not path.is_symlink() and
                             path.read_bytes() == source.extractfile(member).read() and
                             bool(path.stat().st_mode & 0o111) == bool(member.mode & 0o111))
                else:
                    raise SystemExit('Unexpected SDK member type: ' + member.name)
                if not valid:
                    raise SystemExit('Extracted SDK mismatch: ' + str(relative))
    actual = {p.relative_to(sdk) for p in files(sdk)}
    if actual != expected:
        raise SystemExit('Extracted SDK has missing or additional files')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk', type=Path)
    parser.add_argument('archive', type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    target = repo / 'release/src-qca-ipq53xx'
    if target.exists():
        raise SystemExit('Import destination exists; refusing to overwrite it')
    with args.archive.open('rb') as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
        if digest.hexdigest() != ARCHIVE_SHA:
            raise SystemExit('ASUS archive checksum mismatch')
    verify_sdk(args.sdk, args.archive)
    env = dict(os.environ, GIT_NO_LAZY_FETCH='1')
    data = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-rz', BASE, 'release/src'], env=env)
    base = {}
    for entry in data.split(b'\0'):
        if entry:
            meta, name = entry.split(b'\t', 1)
            mode, kind, oid = meta.decode().split()
            base[name.decode()] = (mode, oid)
    target.mkdir(parents=True)
    shared, overlay, platform, links = [], [], [], {}
    # Hydrate only byte-identical Git blobs from the verified local archive. This
    # lets an offline sparse checkout reuse common sources without lazy fetches.
    importer = subprocess.Popen(['git', '-C', str(repo), 'fast-import', '--quiet'], stdin=subprocess.PIPE, env=env)
    try:
        for path in files(args.sdk / 'release/src'):
            name = str(path.relative_to(args.sdk))
            blob = contents(path)
            mode = '120000' if path.is_symlink() else ('100755' if path.stat().st_mode & 0o111 else '100644')
            oid = hashlib.sha1(b'blob ' + str(len(blob)).encode() + b'\0' + blob).hexdigest()
            if base.get(name) == (mode, oid):
                shared.append(name)
                importer.stdin.write(b'blob\ndata ' + str(len(blob)).encode() + b'\n' + blob + b'\n')
                destination = repo / name
                if not destination.exists() and not destination.is_symlink():
                    copy(path, destination)
                elif contents(destination) != blob:
                    raise SystemExit('Existing common source differs: ' + name)
            else:
                overlay.append(name)
                copy(path, target / 'source-overlay' / name)
        importer.stdin.write(b'done\n')
    finally:
        importer.stdin.close()
    if importer.wait():
        raise SystemExit('Git blob import failed')
    sdk_platform = args.sdk / 'release/src-qca-ipq53xx'
    for path in files(sdk_platform):
        name = str(path.relative_to(sdk_platform))
        # U-Boot is not a firmware build input. No bootloader target is exposed.
        if name.startswith('uboot_v2016.01/'):
            continue
        if '/' not in name and path.is_symlink():
            links[name] = os.readlink(path)
        else:
            platform.append(name)
            copy(path, target / name)
    for name in ('release/src-rt', 'buildtools', 'toolchain', 'tools'):
        shutil.copytree(args.sdk / name, target / 'vendor' / name, symlinks=True)
    for name in ('License', 'README.TXT'):
        shutil.copy2(args.sdk / name, target / 'vendor' / name)
    manifest = {'schema': 1, 'base_revision': BASE, 'asus_archive_sha256': ARCHIVE_SHA,
                'shared_files': shared, 'overlay_files': overlay,
                'platform_files': platform, 'platform_links': links}
    (target / 'sources.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({k: len(manifest[k]) for k in ('shared_files', 'overlay_files', 'platform_files', 'platform_links')}))


if __name__ == '__main__':
    main()
