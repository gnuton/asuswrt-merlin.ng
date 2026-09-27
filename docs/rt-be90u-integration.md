# Experimental RT-BE90U integration on 3006

This branch makes the RT-BE90U firmware build from checked-out repository
sources. It is based on `master-3006` at
`920b77f5f92db14717a27abd5c8e1b06ae6c8ec1`. It imports the previously tested
dev14 port (`0dd6c693e2f12bebde29d38b1cd6b0934cce5cad`) and labels this first
native build `58138-rtbe90u-dev15-vpn`. Dev17 consolidates VPN, rc wrapper and
script-hook code into the shared 3006 source tree. Dev18 repairs OpenVPN and
WireGuard profile-reset service dispatch. Dev19 adds conservative, one-time
Fusion profile and policy migration. Dev20 repairs policy-save persistence and
publication failures; dev21 preserves IPv4 VPN protection during SDN refresh.
Dev22 extends default/unassigned SDN and concurrent-transition handling and
quarantines identifiable traffic when stock VPN migration defers. Dev23 extends
quarantine to ambiguous network/DNS scopes and protects correction/retry.
Dev24 serializes full firewall reloads with VPN updates and retains guards
when IPv6 is disabled.

This is a proposed platform layout for maintainer review. It does not add the
model to the supported-model list or release workflow. Recovery and hardware
validation have not been performed; the image is not ready to flash.

## Source ownership and layout

| Location | Build role and provenance |
| --- | --- |
| `release/src/` | 68,212 common inputs, including consolidated VPN/script-hook files and the QCA deferred-migration helper |
| `release/src-qca-ipq53xx/linux`, `ipq53xx`, and other platform directories | 68,029 vendor platform inputs from ASUS GPL TUF-BE9400 3.0.0.6.102.58138 |
| `release/src-qca-ipq53xx/source-overlay/release/src/` | 13,914 differing or additional common-tree inputs, including QCA-specific rc/shared/httpd integration, package versions and UI |
| `release/src-qca-ipq53xx/vendor/` | SDK build glue and auxiliary sources needed to reconstruct the SDK layout |
| `release/src-qca-ipq53xx/sources.json` | Explicit common/override/platform file lists, SDK aliases, required empty directories and import provenance |
| `tools/rt-be90u/` | Source assembly, builder provisioning, image/linkage checks and offline runtime fixtures |

The original SDK archive is `GPL_TUF_BE9400_300610258138.zip`, SHA-256
`930142d207b1f9dfc11e2ad17c767abc59a5df5777223dfc99f12a69f1d07b9e`.
The import script verifies the archive and extracted inputs before copying.
The raw vendor import and the applied port are separate commits. The recorded
`dev14-*` manifests describe the imported baseline, before the dev15 version
change; subsequent integration changes are recorded in Git.

The overrides avoid replacing Broadcom sources with the older QCA SDK. They
also create maintenance work: fixes to an overridden common file do not flow
into this platform automatically. The OpenVPN library and custom script hooks now use common sources with
IPQ53xx conditionals. Remaining rc, networking and UI changes need consolidation with appropriate QCA conditionals after agreement on this layout.
Large package differences include curl, strongSwan, wget, libxml2 and iproute2.
These versions need their own compatibility/security review. This branch has
not established a supported update path for all vendor packages.

The SDK contains proprietary Wi-Fi/NSS components and other prebuilt vendor
libraries/tools. Sources are not available for all of these. Preserve their
existing notices and review redistribution and update responsibilities before
merging. This work does not establish license clearance. U-Boot source is not
imported into this firmware target; no bootloader build or update is exposed.

## Build

Use an x86-64 Linux host, Docker, Python 3.8 or newer and sufficient disk space
for both repository inputs and a private build copy. A full checkout contains
all required firmware inputs. A sparse checkout must include every common file
listed in `sources.json`; `make check` fails if one is missing. No SDK archive,
patch application, second Merlin checkout or Git fetch is needed to build.

The SDK archive's toolchain entries are Git LFS pointers. The toolchain currently
used is `openwrt-gcc750_musl1124.aarch64` from the existing
`SWRT-dev/qca-toolchains` checkout at
`5e5b8ae593edb9c3f0711c56ef7fe73e113506d0`. Its compiler targets
`aarch64-openwrt-linux-musl`, GCC 7.5.0/musl 1.1.24. Equivalence to ASUS's
unavailable LFS objects has not been established. The toolchain is an external,
pinned build dependency, not a firmware source dependency.

Provision host packages once (this step requires package access):

```sh
docker build -f tools/rt-be90u/Dockerfile.deps \
  -t rt-be90u-build-deps:20.04 tools/rt-be90u
```

Then create the builder from the pinned, already available toolchain checkout:

```sh
tools/rt-be90u/prepare-toolchain.sh /path/to/qca-toolchains
```

The toolchain script checks the revision and cleanliness, exports Git objects
only, requires a local dependency image, and disables network during assembly.
An optional third argument selects an existing compatible Ubuntu 20.04 builder
as the dependency image. The local validation used the cached
`rt-be90u-build:58138` image, ID
`sha256:74cad8c3c730d9bd59d2618f9b3d70eb315ae7b25f089e5d8a812e0530575461`,
to create `rt-be90u-build:3006` offline. Fresh OS-package provisioning has since
passed using `Dockerfile.deps` with `--pull --no-cache`; the experimental CI job
also provisions these dependencies on every run.

From the repository root:

```sh
make -C release/src-qca-ipq53xx check
docker run --rm --network none --ulimit core=0 \
  --user "$(id -u):$(id -g)" \
  --mount "type=bind,src=$PWD,dst=/work" \
  rt-be90u-build:3006 make rt-be90u
```

The native entry point is `make -C release/src-qca-ipq53xx rt-be90u`. The builder
already sets that working directory. The existing Broadcom entry points and CI matrix are unchanged. The image
workflow now fetches from the repository that triggered the push, or the pull
request head repository, so fork branches can run those existing builds.

`prepare-native.py` assembles a private SDK tree in
`release/src-qca-ipq53xx/.build/source`. Normal copies keep generated writes
away from checked-in inputs. Inputs in one staging batch receive the same
mtime, preventing artificial stamp/header dependencies caused by copy order.
Repository edits are copied with a fresh timestamp; unchanged build-generated
files are retained on incremental runs. Removed inputs require `make clean`.
A lock serializes preparation, building and cleanup. Vendor Git probes cannot
walk out of the disposable tree into this checkout.

The private SDK invokes the existing `tuf-be6500` board profile required by this
hardware. Packaging must retain product ID `TUF-BE9400`; the device's ODM ID is
`RT-BE90U`. The wrapper checks the exact versioned BE9400 image, CRCs,
architecture, load/entry addresses and observed volume limit before collecting
it in `release/src-qca-ipq53xx/image/`. These checks do not establish flash
compatibility. Do not use the vendor's generic image symlink.

For a clean rebuild:

```sh
make -C release/src-qca-ipq53xx clean
```

This removes the private build tree and retains collected versioned images.
Preserve a completed candidate and choose a new development version before
making further firmware changes. Identical firmware bytes across rebuilds
are not promised: vendor packaging includes timestamps and generated metadata.

### Experimental CI

`.github/workflows/rt-be90u.yml` runs separately from the release workflow. It
checks relevant changes on `master-3006`, `DEV_*` branches and pull requests
targeting `master-3006`. Manual dispatch is declared for use once the workflow
is on the repository's default branch. The default Actions
checkout selects the event commit (the merge commit for pull requests), uses a
sparse checkout of the required source trees and does not retain credentials.

The job provisions Ubuntu build packages from scratch, checks out the pinned
QCA toolchain revision and builds with Docker networking disabled. It rejects
an existing build directory or collected image, and uses no source/build cache.
It runs helper tests, 2,560 common-source comparisons, 32 invalid-profile
rejections, native input checks, uImage identity/CRC/size checks, extracted
rc/httpd/OpenVPN linkage checks and the source-compiled script/config/event
fixture and six basic ARM configuration/service/HTTP suites. The common-source
checker explicitly leaves 96 changed QCA profiles to compiled runtime validation.
Network/tunnel, browser, Entware and hardware suites remain separate from CI.

The common-source checker normally requires local baseline objects. In shallow
CI clones, `--fetch-baselines` explicitly fetches the two recorded commits and
the twelve selected files at both baselines before running comparisons without lazy
fetching. This is test provisioning; the firmware build still consumes only
checked-out inputs.

Each run uploads logs and validation reports, including failures, for 14 days.
Firmware is included only after all image/linkage/script checks pass, together
with SHA-256 checksums and an experimental-use notice. The artifact name
contains the event commit and run attempt; `source-commit.txt` records the
actual checked-out commit. Build-input hashes, toolchain revision, Docker image
identities and installed host-package versions are saved alongside the logs.
This job does not publish releases or update manifests. It tests clean build
reproducibility, not byte-identical output: Ubuntu packages and build timestamps
are not frozen, and the firmware retains its development version label.

## Validation and remaining integration work

Before the dev15 version change, all 150,610 assembled inputs matched the
independent fresh dev14 preparation in contents, executable mode and symlink
target, including all 599 port inputs. Helper tests cover image/linkage checks,
HTML parsing, source isolation, missing inputs, aliases, removed inputs,
path traversal and source timestamp ordering:

```sh
python3 -m unittest discover -s tools/rt-be90u/tests -p 'test_*.py'
```

Once the image is extracted, the existing runtime fixtures can be run with:

```sh
python3 tools/rt-be90u/validate-image.py /path/to/extracted/rootfs \
  --logs /path/to/new-validation-directory
```

This requires the locally provisioned images described by
`tools/rt-be90u/tests/Dockerfile` and `Dockerfile.network`, and `/dev/net/tun`
for encrypted tunnel tests. The runner never pulls an image. It mounts firmware,
matching build sources and fixtures read-only, disables external networking,
and records the commands and exit statuses. Network tests receive capabilities
inside their disposable containers. Use `--source` to select a preserved build
source, `--group` to run one suite, and `--official-rootfs` plus `--candidate`
to include the stock userspace image validator. Its acceptance does not prove
that the bootloader will accept the image. In this fixture it also accepts
a synthetic wrong-model header, so it does not establish model enforcement.

Offline ARM fixtures execute packaged programs under QEMU and use the host
kernel in isolated Docker networking. They do not reproduce the router kernel,
rc boot orchestration, Wi-Fi, hardware acceleration or persistent NVRAM. The
IPv6 guard blocks entire protected interfaces even while the IPv4 tunnel is
up; it does not route IPv6 through VPNs or protect IPv6 identities selected
only by IPv4 device rules.

Next integration work includes maintainer agreement on source ownership,
consolidation of common-code overrides, expanded automated runtime coverage,
release integration, and completion of runtime service/migration/add-on support.
Recovery rehearsal and controlled hardware
validation remain separate prerequisites for a support PR.

## Dev15 result

The first native candidate completed a clean build (using the SDK's required
prebuilt components) and all 11 runtime suites above. The 25 helper tests,
image structure/CRC checks and rc/httpd/OpenVPN linkage checks passed. The image
contains 792 AArch64 ELFs and 11 unchanged vendor coprocessor ELFs, with no test
fixtures packaged. The kernel configuration and packaged ECM selector match
dev14. Independent preparation and a post-build Git-input audit both passed.

The versioned image is 59,049,669 bytes; SHA-256:
`30fd584fa05e77ee2c9245372ca0a2ddd3a5845121c8681612d7123b771141b5`.
SquashFS begins at byte 4,322,736. The exact
[validation summary](rt-be90u-dev15-validation.json) records source identity,
suites and limits. Browser and Entware baselines remain dev12 and dev11; those
suites were not rerun for this source-layout change. Other-model sources were
checked for changes, but their firmware builds were not run.

## Dev16 shared-code consolidation

All eight remaining `libovpn` overrides and `shared/scripts.c` now live in
`release/src/router`. Their platform-overlay copies have been removed. Future
upstream edits to these files therefore reach the QCA build directly.
IPQ53xx guards retain its VPN routing/IPv6 blocking, longer client-password ABI,
NVRAM custom-config fallback and stricter script-enable handling. Other models
retain their previous behavior. The one deliberate common fix removes an extra
integer placeholder from a parser log message that had only one argument.

```sh
python3 tools/rt-be90u/check-common-sources.py
```

This check needs the recorded baseline Git objects and a C preprocessor. At
dev16 it compared nine files across 128 architecture/feature profiles each (1,152
comparisons), including IPv6, WireGuard, multi-LAN and automatic-WAN variants.
IPQ53xx must match the validated dev15 port; other profiles must match upstream
apart from the documented diagnostic fix. Includes are removed for this check:
it tests conditional code selection, not header compatibility, linking or a
complete Broadcom build.

Dev16 was built incrementally in the native private tree after preserving the
completed dev15 source. Independent fresh source assembly verifies all 150,610
inputs; the input/index audit confirms that no untracked source is required.
The runtime tunnel fixture now includes the actual target `rtconfig.h` and
asserts the IPQ53xx password-field size before testing the packaged library.
Without that target configuration, a standalone consumer used the non-QCA
structure layout, which the static-key tunnel test correctly rejected.

The final dev16 image passed all 11 VPN/image-validator suites, the script-hook
fixture, actual Entware installation/postinst/packaged-hook lifecycle tests,
25 helper tests, linkage and content audits. The kernel configuration and all
81 ECM selector instruction words match dev15. No router was modified.

The image is 59,049,753 bytes; SHA-256:
`c064768688b2b8f62b4ebccadf07d37d15d83b67e89dcaf09aa988b5859c2d02`.
SquashFS begins at byte 4,322,736. See the
[dev16 validation summary](rt-be90u-dev16-validation.json) for exact source,
fixture and check identities. Browser validation remains at dev12; hardware and
full firmware service orchestration still require validation.

The subsequent [GT-BE98 default and ROG compatibility builds](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36252624626)
both passed on `cb0c3d00acaacbd9f6116c2bd02f3165a75f82a7`. The native CI
tooling commits retain the same firmware sources, so this validates compilation
of the shared dev16 changes for the existing 3006 model as well.

## Clean CI result

The [RT-BE90U experimental workflow](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36254108733)
passed on `d82460889c4859c3bc0d7d6c75908033d6adc55d`, including fresh dependency
installation, firmware compilation, all checks described above and artifact
upload. The downloaded firmware's SHA-256 and image structure were independently
verified. All 150,610 hosted input records exactly match preserved dev16.
The CI image is 59,048,789 bytes; SHA-256:
`31c9cbf1d8a70af31e62ec8644029e67ebf8395cf23d452080256185fe12b941`.

An independent fresh local checkout and newly provisioned builder also completed
the build, image/linkage checks and script/config/event fixture. Its image is a
separate rebuild with different timestamps; the completed original dev16 image
and sources remain preserved. See the [CI validation summary](rt-be90u-ci-validation.json)
for exact commits, image identities, checks and remaining limits.

The first hosted run exposed an exporter issue specific to sparse toolchain
checkouts: archiving the commit with a path filter requested an unrelated absent
blob. Provisioning now archives only the pinned package subtree. This passed
with that unrelated blob still absent; all 3,714 entries match the prior export
in names, types, executable modes, symlink targets and file contents.

## Dev17 rc wrapper consolidation

`rc/openvpn.c` and `rc/wireguard.c` now use the common source tree; both overlay
copies are removed. IPQ53xx keeps its externally emitted OpenVPN wrappers,
explicit-zero WireGuard keepalive, server-address host-route fallback and IPv6
guard refresh calls. Other platform profiles retain their prior source tokens.
The common-source check now covers eleven files: 2,144 valid comparisons and
32 rejected QCA profiles that omit the required Merlin VPN configuration.

A new ARM fixture links the firmware build's actual `rc/openvpn.o`. It checks
all six exported wrappers, event dispatch, DNS/SDN callback ordering and invalid
arguments using recorded service stubs. It passes against preserved dev16 and
is part of the offline basic suite for dev17. It does not exercise complete rc
service orchestration. The broader offline suite checks WireGuard import/export,
keepalive and encrypted tunnels, four OpenVPN tunnel modes, policy routing and
IPv6 blocking. These runtime suites remain separate from hosted build CI.

The local dev17 build passed all 11 selected VPN suites, the script/config/event
fixture, packaged linkage and content checks. Independent source assembly and
Git-index verification cover all 150,610 inputs. Both rc objects were recompiled
and are byte-identical to dev16, as is the kernel configuration.

The local image is 59,049,809 bytes; SHA-256:
`7574fbaa257e85dd14d0b864976b757596855798f6475694ba93594aaf4a602a`.
SquashFS begins at byte 4,322,736. See the
[dev17 validation summary](rt-be90u-dev17-validation.json) for exact identities
and scope. Unlike dev16's 11-suite run, this run includes the new wrapper fixture
and omits the unchanged stock image validator. Entware remains at dev16 and
browser testing at dev12; neither was rerun for this consolidation.

A follow-up service audit found that dev17's `clearovpnclient` / `clearovpnserver` are
inside the inactive `RTCONFIG_YANDEXDNS` branch of the services overlay. Their
action strings are absent from the compiled services object, although the
OpenVPN UI pages call those reset actions. This predates dev17 and is outside
the wrapper/tunnel fixtures. Correcting the dispatch and adding service-level
regression coverage is required in the next candidate. The WireGuard client UI
also calls `clearwgclient`, whose dispatcher is missing from the services
overlay and compiled object. Dev18 addresses these three dispatcher gaps as
described below.

The [clean dev17 hosted build](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36262918390)
passed on `3d17d4047ea047725225afc00fb587e8fbe90781`, including fresh provisioning,
compilation, image/linkage checks and the script/config/event fixture. The
downloaded firmware checksum was verified, and all 150,610 hosted input records
match local dev17. This separate rebuild is 59,049,349 bytes, SHA-256
`1d34eb0c06444d89df2766abd6773c0167a140ad7fc1258253ddba4824d5f352`.
The SDK's ignored Ookla source-copy error remains; its packaged tracked prebuilt
is byte-identical to dev16. Speedtest runtime behavior was not tested.

Both [GT-BE98 default and ROG compatibility builds](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36262918415)
also passed on `3d17d4047ea047725225afc00fb587e8fbe90781`, including artifact
upload. Release and manifest publishing were skipped. This verifies compilation
of the shared changes for both existing-model variants; it is not GT-BE98
hardware or runtime validation.

## Dev18: VPN profile reset dispatch

The OpenVPN reset handlers now sit alongside the OpenVPN lifecycle handlers,
outside the unrelated Yandex DNS conditional. A WireGuard client reset handler
is also present. Each requires exactly one complete decimal profile number in
the supported range (OpenVPN servers 1–2; OpenVPN/WireGuard clients 1–5).
Missing, extra, malformed, overflowing and out-of-range arguments leave profiles
unchanged. This repairs the reset commands already sent by the shipped UI pages.

The new offline fixture links the firmware's actual `rc/services.o` against the
packaged `libovpn`, `libshared` and real `router_defaults` table. It runs 42 cases:
all 12 UI stop/reset sequences, three accepted leading-zero forms and 27 invalid
requests. Valid requests are repeated to check idempotence. It checks every
packaged profile default, autostart membership, saved custom configuration,
certificate/key removal, unchanged neighbouring profiles, commit counts and
service-hook ordering. Separate ARM processes reload a disposable NVRAM store
to check committed state. The fixture fails on preserved dev17 at its first
unrecognized reset command and passes on dev18.

A local audit also preprocesses the full services source with the target
configuration and all four MULTILAN/Yandex DNS combinations. All dev18 reset
handlers remain at the main dispatcher level; all four dev17 variants fail
that check. Includes are removed for this audit, so it checks conditional
selection rather than complete alternate-profile compilation.

The stop, DPI refresh and script-hook calls are observed substitutes; unrelated
rc service dependencies abort if reached. This verifies reset dispatch and
stored settings, not full service shutdown, real NVRAM flash persistence,
reboot/migration behavior or router hardware. The basic ARM suite now runs in
the experimental hosted workflow after each clean image build.

The local dev18 build, image/linkage checks, all 12 selected VPN suites,
script/config/event fixture, 25 helper tests and 2,144 common-source comparisons
(plus 32 expected invalid-profile rejections) passed. An independent assembly
verified all 150,610 inputs; only `rc/services.c` and the version label changed
from dev17's firmware inputs. The local image is 59,049,729 bytes, with SquashFS
at byte 4,322,736 and SHA-256
`f290945bf865039cc38f3a3668a2aa20cd56c4ff32415d2bba36671e67853dfe`.
The [clean dev18 hosted build](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36286495995)
passed, including four basic ARM suites and all 42 reset cases. Downloaded
firmware checksums and all 150,610 input records were verified against local
dev18. Its separate rebuild is 59,050,293 bytes, SHA-256
`42d9136c4801742add54e958eb133fe7c973a065e27ded44fa6bb064223dadef`.
The [dev18 GT-BE98 default/ROG builds](https://github.com/fredrir/asuswrt-merlin.ng/actions/runs/36286495996)
both passed, including artifact upload.

A separate synthetic migration probe links the actual `rc/format.o` and
packaged libraries. It confirms an upgrade problem in dev18: retained ASUS
`vpnc_clientlist` entries can restore OpenVPN autostart and WireGuard enable
flags during `adjust_vpnc_config`, including after a profile reset. Init still
calls this compatibility handler. Stock Fusion profile indices also differ from
Merlin's fixed SDN mapping: a stock OpenVPN client 1 at index 5 is interpreted as
Merlin WireGuard client 5. Dev18 does not migrate that retained state. Dev19 adds the scoped
conversion and compatibility-pass tests described below. See
the [dev18 evidence](rt-be90u-dev18-validation.json) for the synthetic input and
observed outputs. No router boot or configuration was changed for this probe.


## Dev19: conservative Fusion configuration migration

The QCA boot compatibility handler now plans a one-time conversion of ordinary
OpenVPN and WireGuard Fusion profiles. It copies descriptions and OpenVPN
credentials, translates IPv4 device policy to VPN Director, translates SDN and
default-WAN references to Merlin's fixed indices, and preserves unrelated
profiles with their positionally aligned PPTP options. The profile reader no
longer rewrites OpenVPN/WireGuard enable settings from retained Fusion records.

All changed NVRAM values, policy and the completion marker are committed
together. Before writing, the converter saves the original values in a private
mode-0600 `/jffs/openvpn/fusion-migration.backup`; it never overwrites a different
backup. Converted policy is read from committed NVRAM while the JFFS VPN Director
file is absent. An existing file takes precedence, including an explicit empty
file. The library unsets the fallback on save, but follow-up review found that
the HTTP caller does not reliably commit that removal; see the dev19 gaps below. Migration does not
rewrite certificate/key files or custom configurations.

Conversion is deliberately limited. It accepts ordinary 7–12-field profile
records, including current UI origin metadata, and IPv4 address/CIDR policies
without interface selectors. Conflicting existing Merlin settings, unsupported ordinary-profile variants,
provider slot conflicts or references to retained profiles, ambiguous records, IPv6/MAC policy,
missing active-client configuration, or unwritable backup/state defer the whole
conversion and retain the original settings. Explicitly disabled or reset
profiles are not silently re-enabled. Deferred configurations still need manual
resolution and broader routing validation; preservation alone does not establish
safe routing during a real upgrade.

The new ARM fixture links the actual compiled `init.o`, `format.o` and
`vpn_migrate.o` against the packaged libraries and defaults. Its 101 process
invocations cover the compatibility handler, ordinary and provider profile
readers, all five client slots, repeated conversion, reset then conversion,
fresh-process reload, 38 invalid/conflicting inputs, setter/commit failures,
backup sync/link failures, non-regular backups, an interruption before commit,
and policy fallback/save behavior. Other compatibility helpers are observed
substitutes. File-backed synthetic NVRAM does not establish physical flash
atomicity, whole-router boot order, or recovery from an actual power loss.

The local build, image/linkage checks, all 13 selected VPN suites, script/config/
event fixture, 25 helper tests and 2,144 conditional-source comparisons (plus
32 expected rejections) passed. An independent assembly and Git-index audit
account for all 150,611 inputs, including the new converter. Shared source
checks separately account for the QCA-only policy fallback; non-QCA behavior
remains equal to the upstream baseline under the checked profiles, apart from
the documented residual-state diagnostic correction in `openvpn_options.c`.
See [dev19 validation evidence](rt-be90u-dev19-validation.json) for source and
image identities, hosted status and the limits of these checks.

Broader WAN/DNS/firewall/SDN transitions, deferred migration formats, certificate
and add-on lifecycle support, IPv6 policy limitations, recovery and hardware
validation remain outstanding. The image is still not ready to flash.


Follow-up independent review found two dev19 policy-save gaps: a policy-only
HTTP apply does not mark fallback retirement for NVRAM commit, and the existing
in-place policy writer can leave a partial file after failure that masks the
preserved fallback. The migration fixture supplies its own caller commit and
only tests failures before truncation, so it does not establish those paths.
Dev20 repairs both with actual apply integration coverage and failed-write
checks around atomic file publication, as described below. Dev19 remains an experimental checkpoint.


## Dev20: durable policy retirement and atomic saves

Policy-only Apply now marks removal of a nonempty migration fallback as an
NVRAM modification after a successful save. The existing apply handler therefore
commits that removal. Later file-only edits do not add unnecessary NVRAM commits.

On QCA, policy saves use a private mode-0600 temporary file, complete checked
writes and file sync before atomic rename. Failed writes, sync, close or rename
before publication preserve the existing policy file, or leave an absent file
absent so the migration fallback remains readable. If directory sync fails after
rename, the complete new file may already be visible while the fallback remains
committed; this boundary is explicitly tested. Physical flash durability and
power-loss behavior remain unverified.

The expanded migration fixture passes 133 ARM process invocations, including
write/partial-write/zero-write, parent/file/directory sync, close and rename
faults. A new fixture logs into the unmodified packaged HTTP daemon and posts
real policy-only Apply requests inside a container with loopback networking only.
Its five scenarios verify failed-save behavior, committed fallback retirement,
later empty saves, fresh-process policy reads after file loss, and a first empty
save while a nonempty fallback exists. The real HTTP/library chain reaches QCA's external `nvram commit` dispatch;
synthetic persistence is recorded at that boundary. Other service/process calls
are substituted.

Both defects were reproduced against preserved dev19: real HTTP Apply skipped
the commit, and an injected direct-write failure published an empty file that
masked the intact fallback. These are separate from the original migration
fixture's manually committed save path. The new HTTP fixture is included in
hosted basic validation, which now contains six ARM suites.

The local build, image/linkage/content checks, all 14 selected VPN/HTTP suites,
script/config/event fixture, 25 helper tests and 2,144 conditional-source
comparisons (plus 32 expected rejections) passed. All 150,611 inputs are accounted
for by independent assembly and Git-index checks. Only the version label,
QCA HTTP apply source and shared policy writer changed among firmware inputs;
existing non-QCA policy behavior remains unchanged. See
[dev20 validation evidence](rt-be90u-dev20-validation.json) for exact identities.
The clean hosted build and all six basic ARM suites passed; downloaded firmware
and all 150,611 input records were verified. Both existing GT-BE98 build variants
also passed. The network and tunnel suites remain local validation.

## Dev21: preserve VPN protection during SDN refresh

The compiled SDN refresh previously removed every IPv4 rule for the refreshed
interface. WireGuard protection disappeared; OpenVPN also exposed brief gaps
while rebuilding SDN and Director guards. A packet fixture against preserved
dev20 reproduced 126 failures across 866 checks at 98 routing-command boundaries,
including deletion of an unrelated custom route rule.

QCA refresh now installs or retains the SDN guard before replacing only its
owned VPN lookups. Director, global and custom rules survive the refresh.
Assigned inferred guards use the SDN priority, so disabling enforcement releases
the correct guard. Command failures retain installed protection and return an
error; refresh serializes with the existing VPN routing service lock.

The new fixture links actual compiled `sdn.o` with packaged VPN/shared libraries.
It executes native routing commands in an isolated network namespace and pauses
after each IPv4 rule mutation for packet probes. It covers OpenVPN1/index6 and
WireGuard1/index1 on `br1`, explicit and inferred guard creation, repeated refresh,
enable/enforce changes, unrelated routes, WAN exceptions, and failed guard/lookup
installation followed by retry. WAN exceptions are checked with empty VPN tables;
populated-table probes separately verify tunnel forwarding. NVRAM and independent
firewall-generation calls are substitutes. This does not establish whole-router
service orchestration.

All 15 local runtime suites passed, including 772 SDN packet checks across 78
routing-command boundaries (four injected failures). Image/linkage/content checks,
independent assembly of all 150,611 inputs, Git-index accounting, script/config/event
checks, 25 helper tests, 2,144 common-source comparisons and 32 invalid-profile
rejections passed. Firmware
input changes from dev20 are limited to the version, QCA `sdn.c` and shared
`amvpn_routing.c`; checked non-QCA token behavior remains unchanged. See
[dev21 validation evidence](rt-be90u-dev21-validation.json) for exact identities
and hosted results. The clean hosted build and six basic ARM suites passed;
downloaded firmware and all 150,611 input records were verified. Both GT-BE98
variants also passed. SDN packet, network and tunnel coverage remains local.

Dev22 extends this coverage to default-LAN/unassigned SDNs, multiple SDNs,
concurrent callers and identifiable deferred stock VPN bindings, as described below.
Broader WAN/DNS/firewall transitions, IPv6 policy limitations,
recovery and hardware validation remain outstanding. No router state was changed.

## Dev22: default SDNs, concurrent transitions and deferred migration

Default-LAN and unassigned SDNs now reconcile through the selected SDN, including
assignment removal. SDN WAN/VPN refresh reloads the current SDN after acquiring
the routing lock; the `vpnrouting` routing/DNS update holds the same lock.
Changed Director rule sets install protection before retiring the old rules.
Kernel rule protocol metadata distinguishes generations at the same priority.
Ambiguous custom rules cause cleanup to stop with existing protection retained.

Unresolved stock VPN indices are kept separate from explicit Merlin client
identities. Identifiable affected SDNs and policy selectors receive routing
guards; identified source/interface scopes also receive UDP/TCP port-53 guards.
Protection is reconciled after migration or configuration correction; earlier
guards can remain while another malformed record prevents safe cleanup.
Dedicated deferred SDN resolver instances receive an empty configuration and
cannot start Stubby or append alternate VPN/custom resolvers. The main shared
resolver remains available to unaffected clients. The tested Fusion policy/DNS
callbacks cannot reinterpret an old OpenVPN index as an unrelated WireGuard client.
Raw policy parsing retains full source/destination CIDRs and interface selectors;
cleanup uses the exact captured kernel address when legacy rules contain host bits.
No migration settings are rewritten by this quarantine.

| Local validation | Result |
| --- | --- |
| Packaged ARM suites | 18 passed |
| Default/unassigned/multiple SDNs | 91 checks at 6 command boundaries |
| Concurrent callbacks and rule replacement | 263 packet checks at 37 command boundaries; 4 scenarios |
| Deferred routing/DNS and migration retry | 50 migration controls and 57 firewall checks |
| Source assembly and Git index | 150,614 inputs verified |
| Helpers/common sources | 25 tests; 2,560 comparisons; 32 rejected profiles |

The common-source checker explicitly excludes 96 changed QCA profiles from token
equivalence claims; compiled runtime tests cover the selected QCA behavior.
Preserved dev21 reproduces default-SDN, concurrency and deferred-routing failures.
The new tests use actual compiled rc entry points and packaged libraries, real
POSIX locks, command barriers and native kernel packet forwarding in isolated
containers. Filter replacement uses the production shared emitter; full firewall
and cold-start orchestration remain unverified. The packaged ARM `ip` probe checks
protocol-attribute encoding; kernel operations use native host tools because QEMU
does not support the firmware binary's full netlink path.

See [dev22 validation evidence](rt-be90u-dev22-validation.json) for exact source,
image, runtime counts and hosted status. Dev23 below adds the approved broader
blocking for unidentifiable malformed policies, unknown IPv6 identity and
destination-only shared-resolver requests. Port-53 guards also block cached/local
answers for affected sources; encrypted DNS classification and IPv6 VPN routing are not established.
Broader WAN/firewall/service transitions, recovery and hardware testing remain
outstanding. No router state was changed.

## Dev23: block ambiguous VPN scope until corrected

When a retained VPN policy cannot identify its original client, dev23 blocks
forwarded traffic and port-53 DNS on the affected network. If no network can be
identified safely, the block covers all configured LAN and guest ingress. Valid
IPv4 client selectors retain their IPv4 scope and block IPv6 on their possible
networks. Destination-only policies retain their destination traffic scope and
block shared-resolver DNS across possible ingress networks.

The parser reads original active/index/selector values and validates SDN network
references before trusting them. Malformed rows cannot silently become disabled
or WAN policies. Source and interface selectors retain their original union;
a valid interface does not narrow a separate unidentifiable source branch.
Valid disabled/WAN rows add no quarantine of their own.

The filter drops quarantined forwarded packets regardless of earlier route
selection and runs before existing FORWARD acceptance rules. Updates stage
old and new protection before inspecting routes. Failed corrections retain
previously installed quarantine. Corrected scopes are released after successful
reconciliation;
ambiguous leftover routes or invalid interface definitions must also be resolved.
Corrected fixed SDNs retain staging until their owner installs the replacement
lookup and required normal guards. Successful correction resumes existing normal
DNS behavior; ordinary VPN DNS enforcement is unchanged. Unowned routes are
preserved. Router management by address, DHCP and router-origin traffic remain
available; DNS filtering also blocks affected cached/local answers.

| Local validation | Result |
| --- | --- |
| Packaged ARM suites | 19 passed |
| Broad quarantine | 514 checks at 53 command boundaries |
| Existing deferred migration/firewall controls | 50 migration controls and 58 firewall checks |
| Parser | 39 selector cases; no NVRAM writes |
| Source assembly and Git index | 150,614 inputs verified |
| Helpers/common source checks | 25 tests; 2,560 comparisons; 32 rejected profiles |

Preserved dev22 reproduces the missing broader protection. New native packet
checks cover known/unknown networks, raw SDN ambiguity, earlier routing/DNS
exceptions, management/DHCP/router-origin controls, replacement boundaries and
failed correction/retry. See [dev23 validation evidence](rt-be90u-dev23-validation.json)
for exact identities, counts and hosted status. Whole-router startup, physical
router forwarding/acceleration, IPv6 VPN transport, recovery and hardware remain
unverified. The image is experimental and is not ready to flash.

## Dev24: firewall reload and service lifecycle

Full QCA filter generation and replacement now hold the same routing lock as
incremental VPN updates, followed by the IPv6 kill-switch lock. This prevents
an older generated table from overwriting a newer quarantine update. These direct
filter writers also serialize access to their shared output files.

Default-filter setup restores both families before releasing its locks and
invoking SDN callbacks. Its IPv6 table now declares and attaches the SDN chains
those callbacks require. A failed restore skips that callback. When IPv6 is
disabled, one atomic filter transaction replaces the earlier flush, retaining
VPN guards and existing built-in policies. Lock failures preserve installed
filter tables. Existing void generator APIs still report restore errors through logs.

| Local validation | Result |
| --- | --- |
| Packaged ARM suites | 20 passed |
| Firewall/service lifecycle | 1456 checks at 471 command boundaries |
| Preserved dev23 regression | 71 expected failures across 1605 checks |
| Source assembly and Git index | 150,614 inputs verified; only version and QCA firewall inputs changed |
| Helpers/common source checks | 25 tests; 2,560 comparisons; 32 rejected profiles |
| Stock-library image checks | Candidate accepted; corrupt magic/payload rejected; model/signature enforcement remains unproven |

The new fixture invokes compiled full filter generators and service dispatch,
shared configuration reads, real locks and native kernel table replacement in
isolated containers. It observes packet behavior during concurrent updates and
failed restores. Deletion-only cleanup may find that old rules are already gone;
new rule restores must succeed, and cleanup still receives packet checks.
Selected feature configurations are covered; disabled optional
and hardware dependencies are explicit fixture boundaries. This is not a whole
router boot or complete feature-matrix result.

See [dev24 evidence](rt-be90u-dev24-validation.json) for exact identities and
counts. [Recovery readiness](rt-be90u-recovery-readiness.md) records refreshed
read-only stock metadata, verified rollback files and the next physical gate.
Recovery access, candidate boot, radios, acceleration, storage persistence and
power-loss behavior still need separate hardware evidence.
