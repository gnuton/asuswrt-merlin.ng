#!/usr/bin/env python3
"""Assemble the IPQ53xx build tree from checked-out repository sources only."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import time


def relative(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or not path.parts or str(path) != name:
        raise ValueError('Invalid source path: ' + name)
    return path


def describe(path):
    if path.is_symlink():
        target = os.readlink(path)
        return {'link': target, 'sha256': hashlib.sha256(os.fsencode(target)).hexdigest()}
    if not path.is_file():
        raise ValueError('Missing source: ' + str(path))
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return {'mode': 0o755 if path.stat().st_mode & 0o111 else 0o644, 'sha256': digest.hexdigest()}


def source_plan(repo, platform, manifest):
    plan = {}

    def add(name, source=None, link=None):
        relative(name)
        if name in plan:
            raise ValueError('Duplicate build input: ' + name)
        if source is None:
            record = {'link': link, 'sha256': hashlib.sha256(os.fsencode(link)).hexdigest()}
        else:
            record = describe(source)
            record['source'] = str(source.relative_to(repo))
        plan[name] = record

    for name in manifest['shared_files']:
        relative(name)
        if not name.startswith('release/src/'):
            raise ValueError('Invalid common source: ' + name)
        add(name, repo / name)
    for name in manifest['overlay_files']:
        relative(name)
        if not name.startswith('release/src/'):
            raise ValueError('Invalid platform override: ' + name)
        add(name, platform / 'source-overlay' / name)
    for name in manifest['platform_files']:
        relative(name)
        add('release/src-qca-ipq53xx/' + name, platform / name)
    for name, target in manifest['platform_links'].items():
        add('release/src-qca-ipq53xx/' + name, link=target)
    for path in sorted((platform / 'vendor').rglob('*')):
        if path.is_symlink() or path.is_file():
            add(str(path.relative_to(platform / 'vendor')), path)
    # Never copy through a previously created source symlink. The original SDK
    # uses aliases, but does not put separate input records beneath those aliases.
    links = {name for name, record in plan.items() if 'link' in record}
    for name in plan:
        if any(str(parent) in links for parent in PurePosixPath(name).parents):
            raise ValueError('Build input is below a symlink: ' + name)
    return plan


def materialize(repo, build, plan, empty_directories=()):
    marker = build / 'inputs.json'
    source_root = build / 'source'
    if build.exists() and not marker.exists():
        raise ValueError('Unrecognized build directory; inspect it before removing it')
    old = json.loads(marker.read_text()) if marker.exists() else {}
    if set(old) - set(plan):
        raise ValueError('Source files were removed; run make clean before rebuilding')
    changed = [name for name, record in plan.items() if old.get(name) != record]
    # Git does not retain archive mtimes. Give a batch one timestamp so copying
    # overlays after common files cannot make shipped generated headers older
    # than their stamps (for example Nettle's rotors.h/desdata.stamp).
    input_time = time.time_ns()
    source_root.mkdir(parents=True, exist_ok=True)
    for name in empty_directories:
        relative(name)
        (source_root / name).mkdir(parents=True, exist_ok=True)
    for name in changed:
        record = plan[name]
        destination = source_root / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.is_symlink() or destination.is_file():
            destination.unlink()
        elif destination.exists():
            raise ValueError('Unexpected directory at source-file path: ' + name)
        if 'link' in record:
            destination.symlink_to(record['link'])
        else:
            shutil.copyfile(repo / record['source'], destination)
            destination.chmod(record['mode'])
            os.utime(destination, ns=(input_time, input_time))
    # Build-generated modifications are kept on incremental runs. Repository
    # edits are copied with a current mtime so make sees them as newer inputs.
    temporary = marker.with_suffix('.tmp')
    temporary.write_text(json.dumps(plan, sort_keys=True, indent=2) + '\n')
    temporary.replace(marker)
    return len(changed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check inputs without creating a build tree')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    platform = repo / 'release/src-qca-ipq53xx'
    manifest = json.loads((platform / 'sources.json').read_text())
    plan = source_plan(repo, platform, manifest)
    if args.check:
        print('Validated %d repository inputs; no SDK archive, Git fetch or patch application needed.' % len(plan))
    else:
        count = materialize(repo, platform / '.build', plan, manifest.get('empty_directories', []))
        print('Prepared %d inputs (%d new or updated) in %s' % (len(plan), count, platform / '.build/source'))


if __name__ == '__main__':
    main()
