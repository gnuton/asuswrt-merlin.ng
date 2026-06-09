# AGENTS.md - AsusWrt-Merlin.ng

This repository is a fork of AsusWrt-Merlin designed to support additional router models. It contains a mix of open-source (GPL) and proprietary components.

## Git Workflow & Branches
- **Upstream**: The original source. Data from upstream branches or tags is merged into origin branches.
- **Origin**: The target for pushing changes.
- **Firmware Versions**:
  - `master`: Version 3004.
  - `master-3006`: Version 3006.
- **Version Format**: Versions in changelogs follow the pattern `3004.388.11_1-gnuton1`.

## Supported Models (via GitHub Actions)
- **Version 3006**: `GT-BE98`.
- **Version 3004**: `rt-ax92u`, `dsl-ax82u`, `tuf-ax5400`, `tuf-ax3000`, `rt-ax82u`, `rt-ax95q`, `rt-axe95q`, `rt-ax82u_v2`, `rt-ax5400`, `tuf-ax3000_v2`, `rt-ax58u_v2`.

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

# AGENTS.md - AsusWrt-Merlin.ng

This repository is a fork of AsusWrt-Merlin designed to support additional router models. It contains a mix of open-source (GPL) and proprietary components.

## Git Workflow & Branches
- **Upstream**: The original source. Data from upstream branches or tags is merged into origin branches.
- **Origin**: The target for pushing changes.
- **Firmware Versions**:
  - `master`: Version 3004.
  - `master-3006`: Version 3006.
- **Version Format**: Versions in changelogs follow the pattern `3004.388.11_1-gnuton1`.

## Supported Models (via GitHub Actions)
- **Version 3006**: `GT-BE98`.
- **Version 3004**: `rt-ax92u`, `dsl-ax82u`, `tuf-ax5400`, `tuf-ax3000`, `rt-ax82u`, `rt-ax95q`, `rt-axe95q`, `rt-ax82u_v2`, `rt-ax5400`, `tuf-ax3000_v2`, `rt-ax58u_v2`.

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

## Development Constraints
- **Proprietary Code**: Proprietary components from ASUSTeK, Broadcom, Trend Micro, and Tuxera are present. These are licensed ONLY for original ASUSTeK devices.
- **Prebuilt Components**: Many components in `prebuilt` directories are precompiled binaries. Source code for these is not available; changes requiring modifications to these binaries cannot be implemented.
- **Style**: Follow existing C and shell script conventions found in the `release/src/router` and `release/src-rt-*` directories. Avoid introducing modern styles that might break compatibility with the legacy toolchain.

## Verification
- There is no single global test suite (like `npm test`). Verification is typically performed by building the image and flashing it to hardware or using specific package build targets (`mk-[package]`).
- Use `nvram get productid` via SSH on a physical router to verify model versions.


## Verification
- There is no single global test suite (like `npm test`). Verification is typically performed by building the image and flashing it to hardware or using specific package build targets (`mk-[package]`).
- Use `nvram get productid` via SSH on a physical router to verify model versions.
