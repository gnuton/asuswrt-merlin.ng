# AGENTS.md - AsusWrt-Merlin.ng (3004)

This repository is the **3004 firmware line** of GNUton's AsusWrt-Merlin.ng
fork. It is self-contained: sources, CI, the firmware update feed and releases
all live here. The 3006/WiFi 7 line lives in
[Asuswrt-Merlin/asuswrt-merlin.ng-3006.gnuton](https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3006.gnuton)
and must not be modified as part of work in this repo.

## Git Workflow & Branches
- **Upstream**: [RMerl/asuswrt-merlin.ng](https://github.com/RMerl/asuswrt-merlin.ng). Data from upstream branches or tags is merged into origin branches.
- **Origin**: `Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton`, the target for pushing changes.
- **Firmware line**: `master` is the 3004 line.
- **Legacy branch**: `master-3006` is retained temporarily for rollback and legacy 3006 links. Do not add 3006 work here.
- **Version format**: Versions and tags follow `3004.388.11_1-gnuton1`.
- **Tags**: Only `3004.*` tags are built. CI fails before building any other tag (in particular 3006 tags).

## Supported Models (via GitHub Actions)
- `rt-ax92u`, `dsl-ax82u`
- `tuf-ax5400` (default and TUF UI)
- `tuf-ax3000` (default and TUF UI)
- `rt-ax82u`, `rt-ax95q`, `rt-axe95q`
- `rt-ax82u_v2`, `rt-ax5400`
- `tuf-ax3000_v2` (default and TUF UI)
- `rt-ax58u_v2`

3006/WiFi 7 models are supported by the separate
`Asuswrt-Merlin/asuswrt-merlin.ng-3006.gnuton` repository, not here.

## Firmware Update Feed
- Feed URL used by `release/src/router/rom/webs_scripts/merlin_webs_update.sh`:
  `https://raw.githubusercontent.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/master/updates`
- Manifest read by 3004 routers: `updates/manifest2.txt`, one line per model:
  `<MODEL>#FW3004.388.11#EXT1-gnuton1`.
- `updates/manifest_3006.txt` is kept frozen during the grace period for
  pre-split 3006 devices whose updater still points at the old repo. Do not
  delete it while the old feed URLs are still in use.

> The update feed is critical: installed routers read it over
> `raw.githubusercontent.com`. Never rename the repository or change the feed
> path without testing the old raw URLs and planning a rollback.

## Architecture & Structure
- `release/src/`: Main source tree for the firmware.
- `release/src-rt-*/`: Model-specific source trees (e.g., `src-rt-5.04axhnd.675x`).
- `www/`: Web user interface files.
- `scripts/` and `tools/`: Build and helper utilities.

## Build System
The build system uses a complex set of Makefiles centered around `release/src/Makefile`.

### Key Build Commands
Run from `release/src/`:
- `make [model_id]`: Builds the image for a specific model.
- `make mk-[package]`: Builds a specific package.
- `make distclean`: Cleans all configurations and builds.
- `make cleanimage`: Removes generated images in `image/`.
- `make cleankernel`: Performs a `distclean` of the Linux kernel while preserving `.config`.

### Build Process Notes
- The build process involves copying "base" config files to "target" config files and then applying modifications based on the target profile.
- Trust the `Makefile` targets and logic over prose documentation for the exact build sequence.
- CI is repository-agnostic: it clones `${GITHUB_REPOSITORY}` into `/project/repo`. Do not reintroduce hardcoded `asuswrt-merlin.ng` workspace paths.

## Development Constraints
- **Proprietary Code**: Proprietary components from ASUSTeK, Broadcom, Trend Micro, and Tuxera are present. These are licensed ONLY for original ASUSTeK devices.
- **Prebuilt Components**: Many components in `prebuilt` directories are precompiled binaries. Source code for these is not available; changes requiring modifications to these binaries cannot be implemented.
- **Style**: Follow existing C and shell script conventions found in the `release/src/router` and `release/src-rt-*` directories. Avoid introducing modern styles that might break compatibility with the legacy toolchain.

## Verification
- There is no single global test suite (like `npm test`). Verification is typically performed by building the image and flashing it to hardware or using specific package build targets (`mk-[package]`).
- Use `nvram get productid` via SSH on a physical router to verify model versions.