#!/bin/sh
# Actual compiled rc object and matching headers at /work, packaged image at
# /firmware. Run in a disposable network-none test container with QEMU.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu"
mkdir -p "$test_dir/root/tmp"
object=/work/release/src/router/rc/openvpn.o
test -s "$object"
sha256sum "$object"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
    -Wall -Wextra -Werror -DTUFBE6500 \
    -I/work/release/src-qca-ipq53xx/include \
    -I/work/release/src/router/shared -I/work/release/src/router/libovpn \
    /tests/ovpn-wrappers-test.c "$object" -o "$test_dir/root/tmp/wrappers-test"
chroot "$test_dir/root" /qemu /tmp/wrappers-test
