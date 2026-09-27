#!/bin/sh
# Isolated network-none container only; needs NET_ADMIN.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/wg-export-test.py "$test_dir/root"
