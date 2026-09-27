#!/bin/sh
# Only inside an isolated --network none container; see docs/rt-be90u.md.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/vpn-network-test.py "$test_dir/root" "$@"
