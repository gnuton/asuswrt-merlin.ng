#!/bin/sh
# Isolated container only; needs NET_ADMIN, SYS_ADMIN and /dev/net/tun.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/vpn-tunnel-test.py "$test_dir/root" "$@"
