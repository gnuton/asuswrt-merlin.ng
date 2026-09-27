#!/bin/sh
# Usage: inside the build container with tests at /tests and source at /work.
set -eu
test -d /jffs
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cp /work/release/src/router/shared/scripts.c "$test_dir/scripts.c"
cc -DRTCONFIG_SOC_IPQ53XX -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Werror -I/tests/include \
	"$test_dir/scripts.c" /tests/scripts-test.c -o "$test_dir/scripts-test"
"$test_dir/scripts-test"
cp /work/release/src/router/rc/ovpn.c "$test_dir/ovpn.c"
cc -DRTCONFIG_SOC_IPQ53XX -Wall -Wextra -Wno-unused-parameter -Werror -I/tests/include \
	"$test_dir/ovpn.c" /tests/vpn-event-test.c -o "$test_dir/vpn-event-test"
"$test_dir/vpn-event-test"
