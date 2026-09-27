#!/bin/sh
# Test image at /firmware, pinned downloads at /packages, tests at /tests.
# Run only in the network-disabled rt-be90u-test container.
set -eu
python3 - <<'PY'
import hashlib
import json
from pathlib import Path

manifest = json.loads(Path('/tests/entware-fixture.json').read_text())
expected_packages = {name for name in manifest['files'] if name.endswith('.ipk')}
if {path.name for path in Path('/packages').glob('*.ipk')} != expected_packages:
    raise SystemExit('Unexpected package set')
for name, expected in manifest['files'].items():
    if hashlib.sha256((Path('/packages') / name).read_bytes()).hexdigest() != expected:
        raise SystemExit('Checksum mismatch: ' + name)
PY
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu-aarch64-static"
# Preserve the firmware's /opt -> tmp/opt link in this disposable copy.
mkdir -p "$test_dir/root/tmp/opt/bin" "$test_dir/root/tmp/etc"
cp -a "$test_dir/root/rom/etc/." "$test_dir/root/tmp/etc/"
cp /packages/installer/opkg "$test_dir/root/opt/bin/opkg"
chmod 755 "$test_dir/root/opt/bin/opkg"
for package in /packages/*.ipk; do
    tar -xOf "$package" ./data.tar.gz > "$test_dir/data.tar.gz"
    tar --keep-directory-symlink -xzf "$test_dir/data.tar.gz" -C "$test_dir/root"
done
mkdir -p "$test_dir/root/opt/etc" "$test_dir/root/opt/tmp" "$test_dir/root/opt/var/lock"
printf '%s\n' 'arch aarch64-3.10 10' > "$test_dir/root/opt/etc/opkg.conf"
result=$(chroot "$test_dir/root" /qemu-aarch64-static /opt/bin/opkg print-architecture)
printf '%s\n' "$result"
printf '%s\n' "$result" | grep -qx 'arch aarch64-3.10 10'
chroot "$test_dir/root" /qemu-aarch64-static /opt/libexec/find-gnu --version
mkdir "$test_dir/root/opt/rt-be90u-fixture"
touch "$test_dir/root/opt/rt-be90u-fixture/example"
result=$(chroot "$test_dir/root" /qemu-aarch64-static /opt/libexec/find-gnu \
    /opt/rt-be90u-fixture -type f -name example -print)
test "$result" = /opt/rt-be90u-fixture/example
printf '%s\n' 'Entware binary execution tests passed; installation and service lifecycle untested'
