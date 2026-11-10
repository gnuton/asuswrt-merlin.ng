# Real-IP lookup regression checks

Run on Linux with a C compiler, Python 3 and BusyBox installed:

```sh
python3 tools/tests/realip/test_realip.py
sh -n release/src/router/rom/apps_scripts/getrealip.sh
busybox ash -n release/src/router/rom/apps_scripts/getrealip.sh
shellcheck release/src/router/rom/apps_scripts/getrealip.sh
```

The tests compile the production `rc/realip.c` in GNU89 mode with warnings
as errors for GETREALIP alone, DUALWAN, MULTIWAN_IF and both options. They
also compile it with GETREALIP disabled. `rc.h` and `host.c` supply host-only
firmware API shims: file-backed volatile NVRAM, real fcntl locks/processes,
a fake STUN executable and a route-command log. No external STUN server or
router is contacted. Temporary binaries and state are created under `/tmp`.

The concurrency protocol has these invariants:

- One short `file_lock("realip")` protects generation reservation, connection
  invalidation, ownership checks and the entire IP/state publication sequence.
  DNS, STUN, child waits and route commands run outside this lock.
- Each request owns a generation and connection snapshot for its WAN. rc
  reserves ownership before spawning a worker. Direct script calls reserve
  both selected LB WANs under one lock. No worker can claim the second WAN
  later in its execution.
- Disconnect/stop paths revoke ownership even in LB. Connected transitions,
  including unchanged DHCP renewals, revoke previous requests. Multi-WAN
  aliases also revoke ownership on their underlying WAN. Reserved C launches
  include the canonical prefix to prevent token collisions after remapping.
- Only a pending, current request can complete. Success writes a validated IP
  before state 2; failure clears the IP and writes state 1. Invalidated state
  is 0. Completed tokens cannot publish again. No previous-IP fallback or
  persistent NVRAM commit is used.
- Probe helpers own, cancel and reap their ministun child. Routed C lookups
  use private result files and a separate lock for temporary route changes.
  Normal shell termination stops its probe helper and resolves only its own
  still-pending tokens. SIGKILL cannot run cleanup; a killed worker cannot
  publish, and ownership is still revoked by the next request/WAN transition.
- Script output retains `External IP is ...` / `Failed.` and the original LB
  lines. Normal STUN failure still returns zero. Superseded/interrupted
  requests return nonzero with no IP report. LB reports are emitted only after
  one final locked check of all selected WANs: inadyn's `shell_transaction()`
  ignores `pclose()` status and parses stdout even on command failure.

Controlled process gates cover reversed completion, same-address reconnects
and renewals, success/failure sequences, rc versus inadyn/direct calls,
failover, LB registration/publication, alias remapping, extended service units,
SIGTERM and child reaping, invalid IPv4 results, consumed tokens and counter
exhaustion. A delayed NVRAM writer tests invalidation during publication;
a gated registration tests that LB reservations cannot interleave.

These are host simulation and conditional-compilation checks. They do not
replace a firmware cross-build or DDNS/UPnP acceptance on physical hardware.
The first synchronization commit left the UPnP wait loop unchanged. The
second commit adds the asynchronous reconciliation described below.


## Asynchronous UPnP reconciliation

`start_upnp()` no longer waits for STUN. It generates the configuration and
runs the existing custom-config/postconf hooks while MiniUPnPd is serving.
Only a changed final file, daemon variant or WAN connection requires stopping
and launching the daemon. The applied file is kept in `/etc/upnp/config.applied`
and the volatile `upnp_connection`/`upnp_started` keys record connection identity
and whether the service was started. These are internal runtime state, not user
settings, and are never explicitly committed to flash.

Request reservation, invalidation and completion trigger `update_upnp` through
the existing `notify_rc()` API from workers. PID 1 reconciles directly after
releasing the real-IP lock, so it never waits for its own notification handler.
Completion events carry no IP or WAN argument: the handler selects the same
WAN as `start_upnp()` and obtains a locked snapshot of its current result.
Superseded/replayed completions cannot publish or notify. Events for another
WAN can cause a comparison, but cannot change the selected WAN's address or
restart an unchanged configuration. A late event after `stop_upnp()` cannot
start the service again. A manually requested restart still stops and starts
immediately, even during a lookup.

The existing generation/context/lock/probe-cancellation protocol and DDNS
stdout contracts remain in place. A separate `wanN_realip_epoch`, copied from
the generation at connection invalidation under the same lock, distinguishes
an actual disconnect/reconnect from a connected DHCP renewal. It never grants
lookup ownership and is not a previous-address cache. Interface, private WAN
address, gateway, selected unit and WAN mode also participate in the applied
connection comparison. The fallback-WAN selection now checks each candidate's
own protocol and UPnP setting.

### Address policy and continuity tradeoff

There is deliberately **no previous-IP fallback or grace interval**. A private
address and interface remaining unchanged, even briefly, cannot establish that
the upstream NAT retained its public IP. A new request revokes the old result;
pending, failed, cancelled, disconnected and context-mismatched results omit
`ext_ip`. There is consequently no old-result TTL, expiry worker or polling
loop to maintain. A killed STUN worker can leave state 0, but cannot leave an
old generated `ext_ip` eligible for reuse. Connection changes are applied
without waiting for a result, including same-address reconnects.

If STUN agrees with the WAN interface address, no `ext_ip` override is needed:
MiniUPnPd reads that interface itself. This keeps a public-WAN daemon running
across unchanged DHCP renewals and STUN failures. In dual NAT, removing an old
override and later installing a fresh result may require two restarts even if
the newly discovered address equals the old address. Until STUN succeeds,
MiniUPnPd remains running without a verified public override; external mapping
or address advertisement behind private upstream NAT may be unavailable.
Repeated pending/failure/completion events with the same effective config do
not restart it. This deliberately prefers losing the override over advertising
an unverified cached public address. There is no automatic retry/restart loop;
a later existing WAN/DDNS/UI/service lookup supplies recovery.

Both `miniupnpd/miniupnpd.c` and `miniupnpd-igdv2/miniupnpd.c` were inspected:
`sigusr1()` sets `should_send_public_address_change_notif`; the main loop sends
events (or performs the daemon's separately configured native STUN), without
rereading the options file or updating a configured `use_ext_ip_addr`.
`readoptionsfile()` and `UPNPEXT_IP` assignment happen at initialization.
A changed generated `ext_ip` therefore requires a restart, not SIGUSR1.

Explicit `config.alt`, custom config and postconf overrides retain their
existing authority. The generated-IP validity/ownership guarantee concerns
the generated line; administrator-provided `ext_ip` overrides are not rewritten.
The final customized file is compared, so custom changes still take effect.

### Additional host coverage

The same Python command also extracts the complete production UPnP service
functions from `services.c`, compiles them in GNU89 mode and executes them
against `upnp_host.c`. File access is redirected into each test's `/tmp`
directory; daemon processes, firmware helpers, hooks and the notification queue
are mocked. Single/dual WAN, GETREALIP-disabled service code, IGDv2, AUPNPC,
AuraSync, GeForceNow, HTTPS, WebDAV, tunnel and app conditionals compile. The
UPnP harness suppresses existing unused-variable/misleading-indentation warnings
in the extracted legacy function, plus unused host stubs; other warnings remain
errors. The original real-IP compilation retains all warnings as errors.

| Scenario | Checked behavior |
| --- | --- |
| Fast STUN at boot | Current verified address included on initial launch |
| Gated slow/unreachable STUN | Service start returns within 2 seconds while probe remains gated; daemon starts without override |
| Unchanged DHCP renewal | Daemon preserved when effective config/connection agree; public WAN needs no STUN override |
| Public address change | Selected WAN's fresh override replaces the applied address |
| Temporary failure and recovery | Old override removed; failure alone adds no restart; later success applied |
| WAN down/reconnect | Result revoked; connection epoch forces application even for identical interface/address |
| Failover/LB | Selected secondary result used; unrelated secondary completion leaves primary daemon alone |
| Reversed concurrent results | Existing ownership tests reject obsolete publishers; UPnP snapshot rejects stale contexts |
| Manual restart/stop during STUN | Restart immediate; late completion cannot undo stop |
| Custom config/IGDv2 | Final hook/alternative file compared and correct daemon variant chosen |
| Early daemon exit/start failure | Completion can recover a previously started service; failed launch stores no applied snapshot |

These are host process simulations and C compilation checks, not a boot test,
firmware cross-build or packet-level UPnP/DDNS acceptance. The proprietary
`notify_rc` implementation/real firmware event queue and daemon lifecycle on
hardware are outside this harness. No router is contacted or flashed. In this
environment the SDK's default `/opt/toolchains/` is absent, so a firmware/package
cross-build was not attempted.
