#!/bin/sh
# Use STUN to find the external IP. Ownership/publication is shared with realip.

servers="default stun.l.google.com:19302 stun.cloudflare.com:3478 stun1.l.google.com:19302 stun2.l.google.com:19302 stun3.l.google.com:19302 stun4.l.google.com:19302"

command -v ministun >/dev/null || exit 1

child=
interrupted=0
token0=
token1=
workdir=
# shellcheck disable=SC2329 # Invoked by the EXIT trap.
cleanup()
{
	trap '' HUP INT TERM
	if [ -n "$child" ]; then
		kill -TERM "$child" 2>/dev/null
		wait "$child" 2>/dev/null
	fi
	# Finish only tokens still owned by this invocation, never newer requests.
	[ -z "$token0" ] || realip --finish 0 "$token0" "" >/dev/null
	[ -z "$token1" ] || realip --finish 1 "$token1" "" >/dev/null
	[ -z "$workdir" ] || rm -rf "$workdir"
}
trap cleanup EXIT
trap 'interrupted=1' HUP INT TERM

# rc reserves a token before spawning us. Direct/inadyn calls reserve both LB
# WANs in a single transaction, before starting either probe.
lb=0
if [ "$#" -ne 0 ]; then
	[ "$#" -eq 2 ] || exit 1
	case "$1" in 0) token0=$2;; 1) token1=$2;; *) exit 1;; esac
	units=$1
	[ "$(nvram get wans_mode)" != lb ] || lb=1
else
	reservation=$(realip --begin-script) || exit 1
	# The helper emits only a mode flag and two numeric tokens (or '-').
	# shellcheck disable=SC2086
	set -- $reservation
	lb=$1
	[ "$2" = - ] || token0=$2
	[ "$3" = - ] || token1=$3
	units="0 1"
fi

# mkdir is available even on targets whose BusyBox does not enable mktemp.
# Do not reuse a directory left behind by SIGKILL and later PID reuse.
attempt=0
while [ "$attempt" -lt 10 ]; do
	candidate=/tmp/getrealip.$$.$attempt
	if mkdir -m 700 "$candidate" 2>/dev/null; then
		workdir=$candidate
		break
	fi
	attempt=$((attempt + 1))
done
[ -n "$workdir" ] || exit 1

for unit in $units; do
	[ "$interrupted" = 0 ] || exit 1
	case "$unit" in 0) token=$token0;; 1) token=$token1;; esac
	[ -n "$token" ] || continue
	realip --current "$unit" "$token" || exit 1
	prefix=wan${unit}_
	proto=$(nvram get "${prefix}proto")
	case "$proto" in
		pppoe|pptp|l2tp) ifname=$(nvram get "${prefix}pppoe_ifname");;
		*) ifname=$(nvram get "${prefix}ifname");;
	esac
	result=
	for server in $servers; do
		[ "$interrupted" = 0 ] || exit 1
		realip --current "$unit" "$token" || exit 1
		[ "$server" = default ] && server=
		# The helper owns/reaps ministun and cancels it when this token expires.
		realip --probe "$unit" "$token" "$ifname" "$server" >"$workdir/result" 2>/dev/null &
		child=$!
		[ "$interrupted" = 0 ] || exit 1
		wait "$child"
		status=$?
		# An interrupted wait has not reaped the helper; cleanup must stop it.
		[ "$interrupted" = 0 ] || exit 1
		child=
		if [ "$status" -eq 0 ]; then
			result=$(cat "$workdir/result")
			realip --valid "$result" && break
		fi
		result=
	done
	[ "$interrupted" = 0 ] || exit 1
	realip --finish "$unit" "$token" "$result" || exit 1
done
# inadyn parses stdout even on failure: emit no partial result from an obsolete
# LB run. Normal STUN failures still print Failed. and return success.
[ "$interrupted" = 0 ] || exit 1
realip --report "$token0" "$token1" "$lb"
