#!/bin/sh
# Same isolated network-none container and mounts as run-vpn-network-test.sh.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/sdn-refresh-test.py "$test_dir/root" "$@"
