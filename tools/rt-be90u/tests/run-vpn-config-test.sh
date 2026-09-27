#!/bin/sh
# Test image at /firmware, corresponding source at /work, tests at /tests.
# Run only in the network-disabled rt-be90u-test container.
set -eu
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu-aarch64-static"
mkdir -p "$test_dir/root/tmp" "$test_dir/root/jffs/openvpn"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
	-Wall -Wextra -Werror -Wl,-E \
	-I/work/release/src/router/shared \
	-I/work/release/src/router/libovpn /tests/vpn-config-test.c \
	-L/firmware/usr/lib -Wl,-rpath-link,/firmware/usr/lib:/firmware/lib -lovpn \
	-o "$test_dir/root/tmp/vpn-config-test"
chroot "$test_dir/root" /qemu-aarch64-static /tmp/vpn-config-test
