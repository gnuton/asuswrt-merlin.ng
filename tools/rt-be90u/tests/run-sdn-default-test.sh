#!/bin/sh
# Disposable network-none container; same mounts as the SDN refresh fixture.
set -eu
test -f /.dockerenv
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
python3 /tests/sdn-default-test.py "$test_dir/root" "$@"
