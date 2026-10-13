# Offline candidate validation

The offline implementation checkpoint passed on 2026-10-13. This is an
experimental candidate, not approval to flash or use it as a daily gateway.

## Candidate

- Firmware source commit: `b6e6f202cf8691a7ddbdc8e98dd49ff57912ec8c`.
- Package: `BQ16_3006_102.6_bq16-alpha1-gb6e6f202_nand_squashfs.pkgtb`.
- Size: 68,311,344 bytes.
- SHA-256: `e4e1d15e92f4e504dca5c01a3c13be8486c47ed812f22be6147002638fc67147`.
- Build: `make bq16 PARALLEL_BUILD=-j12 EXTENDNO=bq16-alpha1-gb6e6f202 MERLINUPDATE=n`, exit 0.
- Toolchain image and environment: pinned in `inputs.json`.
- Import inventory: 1,452 files verified against `import-manifest.json`.

The local review bundle is outside Git at
`.bq16-builds/artifacts/bq16-alpha1-b6e6f202/` in the parent workspace.
It contains the standard package, portable SHA-256 file, source commit,
input/import manifests, offline report, build configuration, validation
tool versions, and the shared SDK object audit. It excludes the generated
loader-inclusive package. Complete build logs remain in
`.bq16-builds/logs/`; configuration and JFFS backups remain private.

## Checks that passed

- Outer and inner FIT payload SHA-256 checks, BQ16 identity, ASUS capability
  flag, NAND/6813/SquashFS compatibility, and complete bootfs components.
- Both `BQ16_2556` and `BQ16_2556_2` DTBs and their boot configurations.
- Kernel 4.19.294, embedded AArch64 kernel configuration, and matching module
  vermagic `4.19.294 SMP preempt mod_unload aarch64`.
- 409 ELF programs, loaders, and libraries. 397 are ARM32 and 12 are AArch64.
  Dependencies retain the caller's architecture; core router programs use
  ARM32 soft-float. All 149 kernel modules use AArch64 and matching flags.
- Two bootfs/rootfs pairs need 1,078 logical eraseblocks against the recorded
  1,332-block budget. This does not require shrinking the protected volumes.
- Generated configuration enables OpenVPN, WireGuard, Merlin VPN policy
  routing, DNS privacy, DNSSEC, SNMP, Tor, MLO_CONFIG_556, and MLO_DEFAULT_ACTIVE.
  NFS, forced upgrades, FRS live updates, and the GNUton update feed are off.
- 20 tooling regression tests. Actionlint and CI shell checks pass. The
  GitHub workflow is configured, but no remote CI run or publication has
  occurred as part of this checkpoint.

The stock image also passed the expanded verifier with 404 ELF files and
138 modules. The 669 shared SDK `.o_saved` files match the donor by hash.
The remaining generated runtime alias is `usr/sbin/lld2d.conf`, which points
to `/tmp/lld2d.conf`; it is configuration data created at runtime.

## Independent controls

Both unmodified controls completed with exit 0 before model integration.

| Control | Standard NAND package SHA-256 |
|---|---|
| ASUS BQ16 39256, 65,873,952 bytes | `fd95c687366a9027b3225a8191c9c8a07d1fe0742305840b9688b7e6e2aa3ff1` |
| Merlin GT-BE98 at base `920b77f5`, 77,193,456 bytes | `1a5189b3c55ba5f9ede4336d8f073d50d7d5927a0c6f9c7af7b5f5578a845564` |

A clean GT-BE98 regression build at `22f7bd73` also completed with exit 0.
After the BQ16-specific C guards, GT-BE98's `rc`, web server, and their install
targets passed at `fbc47372`. The SDK `buildimage` target then produced the
updated standard package, 77,197,552 bytes, SHA-256
`1f73f057af3974e5c0ada5ca5c3f48a91f2eee587b0ee660099bf73f71aca959`.
Later staging changes apply only to BQ16. Keep using independent disposable
model copies; the cached GT-BE98 rebuild exposed an existing NFS configuration
inconsistency that the clean control did not have.

## Gates that remain open

The reports set `bootloader_acceptance_verified` and
`hardware_tests_verified` to false. Neither stock nor candidate FITs have
signature nodes. This does not establish the factory loader's enforcement
policy or the stock updater's acceptance rules. The NAND check establishes
capacity, not the updater's resizing behavior.

The actual unit's recovery path has not been exercised. No custom firmware
was flashed, no router reboot was performed, and no hardware acceptance
test or 72-hour traffic test has passed. Firmware verification and recovery
must be established before seeking approval for this specific image and a
maintenance window. Review the older Merlin base against ASUS 39256,
including security changes, before daily use.
