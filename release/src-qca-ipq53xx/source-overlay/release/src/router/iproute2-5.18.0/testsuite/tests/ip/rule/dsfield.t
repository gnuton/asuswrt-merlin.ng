#!/bin/sh

. lib/generic.sh

ts_log "[Testing rule with option dsfield/tos]"

ts_ip "$0" "Add IPv4 rule with dsfield 0x10" -4 rule add dsfield 0x10
ts_ip "$0" "Show IPv4 rule with dsfield 0x10" -4 rule show dsfield 0x10
test_on "tos 0x10"
test_lines_count 1
ts_ip "$0" "Delete IPv4 rule with dsfield 0x10" -4 rule del dsfield 0x10

ts_ip "$0" "Add IPv4 rule with tos 0x10" -4 rule add tos 0x10
ts_ip "$0" "Show IPv4 rule with tos 0x10" -4 rule show tos 0x10
test_on "tos 0x10"
test_lines_count 1
ts_ip "$0" "Delete IPv4 rule with tos 0x10" -4 rule del tos 0x10

ts_ip "$0" "Add IPv6 rule with dsfield 0x10" -6 rule add dsfield 0x10
ts_ip "$0" "Show IPv6 rule with dsfield 0x10" -6 rule show dsfield 0x10
test_on "tos 0x10"
test_lines_count 1
ts_ip "$0" "Delete IPv6 rule with dsfield 0x10" -6 rule del dsfield 0x10

ts_ip "$0" "Add IPv6 rule with tos 0x10" -6 rule add tos 0x10
ts_ip "$0" "Show IPv6 rule with tos 0x10" -6 rule show tos 0x10
test_on "tos 0x10"
test_lines_count 1
ts_ip "$0" "Delete IPv6 rule with tos 0x10" -6 rule del tos 0x10
