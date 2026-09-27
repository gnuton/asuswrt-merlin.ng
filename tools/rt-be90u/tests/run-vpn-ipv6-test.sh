#!/bin/sh
# Same isolated network-none container/mounts as run-vpn-network-test.sh.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/vpn-ipv6-test.py "$test_dir/root" "$@"
