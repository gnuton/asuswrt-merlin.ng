#!/bin/sh
# Run only with the existing isolated network-none/NET_ADMIN fixture mounts.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/sdn-concurrency-test.py "$test_dir/root" "$@"
