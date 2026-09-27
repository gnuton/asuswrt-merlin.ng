#!/bin/sh
# Compiled firmware services.o and headers at /work; matching image at /firmware.
# Only run in a disposable network-none container, with QEMU and the toolchain.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu"
mkdir -p "$test_dir/root/tmp" "$test_dir/root/jffs/openvpn"
python3 /tests/vpn-reset-test.py "$test_dir"
