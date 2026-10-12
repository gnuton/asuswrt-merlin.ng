# BQ16 implementation

- [x] Create the pinned port branch.
- [x] Record hashes and verify private configuration/JFFS backups.
- [x] Pin and verify the soft-float ARM and AArch64 toolchains.
- [x] Build the unmodified ASUS BQ16 control. Exit 0, NAND image and hashes recorded.
- [x] Build the unmodified Merlin GT-BE98 control. Exit 0, NAND image and hashes recorded.
- [x] Checkpoint: both independent control builds pass.
- [x] Implement and verify the BQ16 proprietary importer.
- [x] Import BQ16 components with provenance and hashes. 1443 entries verified.
- [x] Add BQ16 hardware profile, Merlin services, and web assets.
- [ ] Checkpoint: BQ16 builds and GT-BE98 still builds.
- [x] Implement offline FIT and ABI checks against stock, with corruption tests.
- [x] Fix fork/SHA selection in CI and add experimental BQ16 artifacts. Actionlint passes.
- [ ] Checkpoint: offline candidate validation passes.
- [ ] Verify firmware acceptance and stock recovery on the actual unit.
- [ ] Obtain approval for a specific candidate and maintenance window.
- [ ] Validate boot, routing, wireless, Merlin services, and AiMesh.
- [ ] Complete three reboot cycles and the 72-hour stability test.
- [ ] Review security differences before daily use.
