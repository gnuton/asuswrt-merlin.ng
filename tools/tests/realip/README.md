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
The existing UPnP wait loop is deliberately unchanged by this package.
