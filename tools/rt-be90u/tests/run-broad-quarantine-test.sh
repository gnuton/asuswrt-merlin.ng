#!/bin/sh
# Only in a disposable network-none container with NET_ADMIN and fixture mounts.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/broad-quarantine-test.py "$test_dir/root" "$@"
