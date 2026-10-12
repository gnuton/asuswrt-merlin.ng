# BQ16 port on Merlin 3006

Start from `920b77f5f92db14717a27abd5c8e1b06ae6c8ec1` on branch
`port/bq16-3006`. Use the unmodified ASUS BQ16 39256 GPL distribution as
the donor. GT-BE98 remains the build control, not a source of BQ16 binaries.

The target is the EU BQ16 gateway, board `BQ16_2556_2`, with stock AiMesh
nodes over wired and wireless backhaul. BQ16 Pro is outside this port.

## Ordered milestones

1. Freeze source inputs, create private configuration and JFFS backups,
   and document the recovery procedure.
2. Pin the toolchain container. Build ASUS BQ16 and Merlin GT-BE98 in
   independent disposable copies before integrating the port.
3. Add a checked BQ16 importer for router objects, platform libraries,
   wireless components, bootloader objects, and model web assets.
4. Add the vendor BQ16 feature profile with Merlin services. Preserve
   switch, NVRAM size, calibration, MLO, and both BQ16 board variants.
5. Build BQ16, inspect FIT payloads and module ABI, and add CI that builds
   the requested fork SHA. Keep BQ16 updates manual.
6. After explicit approval of a candidate and recovery preparation,
   validate boot, PPPoE/VLAN 835, wireless, Merlin services, and AiMesh.
   Complete three reboot cycles and a 72-hour stability test.

## Execution rules

- Never build or evaluate Makefiles in the immutable ASUS donor.
- Keep backups, credentials, factory data, and live logs outside Git.
- Missing proprietary inputs fail the import; do not borrow another
  model's objects or disable services to make the build pass.
- Retain the factory loader and calibration data. No raw MTD writes.
- A successful offline build does not establish bootloader acceptance.
- Flashing, daily use, publishing, and merging require their own review.

See `docs/bq16/README.md` for the baseline and recovery checklist.
