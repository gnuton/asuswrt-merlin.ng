#!/bin/sh
# Packaged HTTP server, synthetic NVRAM, loopback only in --network none.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu"
mkdir -p "$test_dir/root/tmp/etc" "$test_dir/root/tmp/var/run" \
    "$test_dir/root/tmp/var/log" "$test_dir/root/jffs/openvpn" \
    "$test_dir/root/dev" "$test_dir/root/proc"
cp -a "$test_dir/root/rom/etc/." "$test_dir/root/tmp/etc/"
touch "$test_dir/root/dev/null"
printf 'admin:%s:0:0:99999:7:::\n' "$(openssl passwd -6 -salt rtbe90utest rtbe90u-fixture)" \
    > "$test_dir/root/tmp/etc/shadow"
printf '%s\n' 'admin:x:0:0:Test:/tmp/home/root:/bin/sh' > "$test_dir/root/tmp/etc/passwd"
aarch64-openwrt-linux-musl-gcc -Wall -Wextra -Werror -shared -fPIC \
    /tests/httpd-policy-nvram.c -o "$test_dir/root/tmp/httpd-policy-nvram.so"
python3 /tests/httpd-policy-test.py "$test_dir/root"
