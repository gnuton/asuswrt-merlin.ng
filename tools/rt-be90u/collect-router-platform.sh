#!/bin/sh
# Run on the router: ssh user@router sh -s < tools/rt-be90u/collect-router-platform.sh
# Read hardware metadata only; do not dump NVRAM, logs, or flash contents.

set -eu
LC_ALL=C
export LC_ALL

if ! type nvram >/dev/null 2>&1; then
	printf '%s\n' 'nvram not found; run on an ASUS router' >&2
	exit 1
fi

read_text()
{
	for path in "$@"; do
		[ -r "$path" ] || continue
		printf '\n%s\n' "$path"
		tr '\000' '\n' < "$path"
		printf '\n'
	done
}

printf 'Collected: '
date -u '+%Y-%m-%dT%H:%M:%SZ'

printf '\nModel and firmware\n'
for key in productid odmpid firmver buildno extendno; do
	printf '%s=%s\n' "$key" "$(nvram get "$key")"
done

printf '\nKernel\n'
uname -srvm
read_text /proc/version

printf '\nCPU\n'
awk '/^(processor|model name|CPU implementer|CPU architecture|CPU variant|CPU part|CPU revision|Hardware)[[:space:]]*:/' /proc/cpuinfo

printf '\nMemory\n'
awk '/^MemTotal:/' /proc/meminfo

read_text /sys/firmware/devicetree/base/model \
	/sys/firmware/devicetree/base/compatible \
	/sys/devices/soc0/machine /sys/devices/soc0/family \
	/sys/devices/soc0/soc_id /proc/mtd

printf '\nUBI volume names\n'
read_text /sys/class/ubi/ubi[0-9]_[0-9]*/name

printf '\nUBI health metadata\n'
read_text /sys/class/ubi/ubi[0-9]/ro_mode \
	/sys/class/ubi/ubi[0-9]/bad_peb_count \
	/sys/class/ubi/ubi[0-9]/avail_eraseblocks \
	/sys/class/ubi/ubi[0-9]/max_ec

printf '\nLoaded modules\n'
if [ -r /proc/modules ]; then
	awk '{print $1}' /proc/modules | sort
fi
