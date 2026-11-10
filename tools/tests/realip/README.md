# Real-IP / UPnP regression checks

Run on Linux with a C compiler, Python 3, BusyBox and ShellCheck:

```sh
python3 tools/tests/realip/test_realip.py
sh -n release/src/router/rom/apps_scripts/getrealip.sh
busybox ash -n release/src/router/rom/apps_scripts/getrealip.sh
shellcheck release/src/router/rom/apps_scripts/getrealip.sh
```

The suite compiles production `realip.c` in GNU89 mode with warnings as errors,
with GETREALIP alone, DUALWAN, MULTIWAN_IF, both WAN options, and GETREALIP off.
It runs real processes, POSIX locks and the production worker/script against
file-backed NVRAM and fake STUN/routes. The UPnP harness extracts the complete
service functions, remaps `/etc/upnp` into `/tmp`, and mocks daemon launches,
hooks and service notifications. It also compiles single/dual WAN, GETREALIP
off, IGDv2, AUPNPC, AuraSync, GeForceNow, HTTPS, WebDAV, tunnel and app options.
Legacy UPnP unused-variable/indentation warnings and unused host stubs are
suppressed; other warnings are errors. No router or external STUN is contacted.

## Design and review decisions

An incremental fix could keep the shell/C helper protocol and add a separate
last-applied-IP cache. Instead, `getrealip.sh` now execs `realip --script`:
reservation, probing, cancellation, publication and DDNS reporting share one
worker. This removes the shell's child/trap/work-directory lifecycle and its
seven helper commands. Test-only operations live in `host.c`, outside the
firmware CLI. The routed C lookup retains its existing server/routing policy;
DDNS retains its own server order and stdout format.

The short publication lock, per-WAN generation and connection snapshot remain:
a kill-before-start scheme cannot prevent a delayed worker or obsolete publisher
from overwriting a newer lookup. Both selected LB WANs reserve under one lock.
Connection snapshots catch interface/address/gateway/protocol/mode/primary
changes; disconnect generations also catch reconnects with identical addresses.
Multi-WAN aliases invalidate their underlying WAN; reserved routed launches
carry the canonical prefix to reject alias remapping and equal-token collisions.

The former `realip_notify_upnp()` could enter UPnP synchronously inside a WAN
transition. State/invalidation helpers now only change ownership. Settled
`wan_up`, `wan_down`, Multi-WAN and explicit getrealip-service paths reconcile;
workers only queue completion notifications, outside the publication lock.
The DHCP+PPP LB start point follows routing/firewall setup. Notifications carry
no address: late, duplicate and secondary-WAN events reread current ownership.
The actual `notify_rc` implementation is not supplied as C source in this tree;
its firmware queue timing is not established by these host tests.

Only `/etc/upnp/config.applied` tracks UPnP launch intent and applied connection
identity/configuration. The redundant `upnp_started` and `upnp_connection` keys
are gone. Stopping removes the record, so a late result cannot undo a manual
stop. `xstart()` success establishes a launch attempt, not daemon initialization;
a later completion can retry an exited daemon, without an automatic restart
loop. Final customized bytes and the selected daemon variant are compared.
`config.alt`, `config.custom`, custom replacement and postconf keep authority.
Both MiniUPnPd sources handle SIGUSR1 as address-change notification, not a
reload of `ext_ip`; configuration changes still require a restart.

## Refresh and invalidation policy

A refresh advances ownership and sets state 0, while retaining the previous
address only when its connection snapshot still matches. This address is a
pending-refresh fallback for generated UPnP configuration, not a completed
STUN result: DDNS only reports terminal state 1/2 after its locked final check.
An unchanged DHCP renewal preserves it while revoking the old worker. Returning
the same public IP causes zero restarts; a changed IP causes one application.
Failure or explicit cancellation clears it; a subsequent failure cannot revive
it. Disconnect, reconnect and changed identity revoke it immediately in NVRAM,
and settled WAN handling applies that revocation without waiting for STUN.
There are no new NVRAM commit calls or new cache keys.

This is event-based invalidation, not a wall-clock TTL. Normal STUN attempts
use `_eval`'s existing ten-second child alarm (seven DDNS servers at most),
including DNS inside ministun. The routed worker no longer waits forever for
`link_internet`, a global flag that may not describe its WAN. Its legacy
resolver/route-lock waits are outside PID 1 and outside the publication lock.
Supported termination signals and supersession kill/reap the probe and remove
its private result file. SIGKILL cannot execute cleanup: the child alarm still
bounds ministun, but a leftover result file/pending fallback may survive until
the next request cancellation/failure or WAN invalidation. No autonomous expiry
is promised in the absence of those events. A strict crash-proof TTL would
require additional scheduling and is deliberately outside this patch.

## Coverage and limits

Gated process tests cover reversed completion, unchanged DHCP and public-IP
renewal, new public IP, failure/recovery/repeated failure, same-address reconnect,
interface/gateway changes, failover/LB, rc versus direct/inadyn-style calls,
manual restart/stop, late generations, alias remapping, extended WAN units,
lock-protected publication, cancellation/reaping, consumed tokens, generation
exhaustion, inherited SIGCHLD ignore, child timeout and unavailable global
Internet detection. UPnP tests assert restart counts and compare final custom,
alternative and postconf configurations, including early exit/launch failure.
Redundant fast/renewal/secondary-result cases share these scenario tests.

These are host simulations and static checks, not firmware or packet-level
DDNS/UPnP acceptance. Full WAN event functions and the real rc notification
queue are not executed by the harness. The SDK's `/opt/toolchains` and target
`tools` directory are absent here, so no firmware/package cross-build or flash
was performed. Real boot/failover ordering, daemon initialization and mapping
restoration after a necessary restart still need physical-router validation.
The inherited license on `realip.c` comes from the routed lookup extracted from
`services.c`; its obsolete services.c RCS identifier was removed.
