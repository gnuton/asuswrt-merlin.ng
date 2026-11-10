#!/bin/sh
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d /tmp/gtbe98-wan-vlan.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
"${CC:-cc}" -std=gnu89 -Wall -Wextra -Werror -pedantic \
	-I"$repo/release/src/router/shared" \
	"$repo/release/src/router/shared/gtbe98_wan_vlan.c" \
	"$repo/tests/gtbe98_wan_vlan_test.c" -o "$work/test"
"$work/test"
"${CC:-cc}" -std=gnu89 -Wall -Wextra -Werror -pedantic \
	-I"$repo/release/src/router/shared" \
	"$repo/release/src/router/shared/gtbe98_wan_vlan.c" \
	"$repo/tests/gtbe98_wan_vlan_http_test.c" -o "$work/http-test"
"$work/http-test"
node "$repo/tests/gtbe98_wan_vlan_ui_test.js"
python3 "$repo/tests/gtbe98_wan_vlan_http_integration.py"
"${CC:-cc}" -std=gnu99 -Wall -Wextra -Werror \
	-I"$repo/release/src/router/shared" \
	"$repo/release/src/router/shared/gtbe98_wan_vlan.c" \
	"$repo/release/src/router/shared/gtbe98_vlan_hw.c" \
	"$repo/tests/gtbe98_vlan_runtime_test.c" \
	-Wl,--wrap=gtbe98_vlan_hw_capture -Wl,--wrap=gtbe98_vlan_hw_identity -o "$work/runtime-test"
"$work/runtime-test"
"${CC:-cc}" -std=gnu99 -Wall -Wextra -Werror \
	-I"$repo/release/src/router/shared" \
	"$repo/tests/gtbe98_vlan_hw_test.c" -o "$work/hw-test"
"$work/hw-test"
python3 "$repo/tests/gtbe98_wan_vlan_lifecycle_test.py"
