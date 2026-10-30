---
name: Merge ASUS GPL Prebuilts
description: Copy and merge model-specific ASUS GPL prebuilt binaries into AsusWrt-Merlin.ng, then diagnose build and CI failures caused by prebuilt/source mismatches.
---

# Merge ASUS GPL Prebuilts

Use this skill when the user provides a new ASUS GPL source tree for a router
model (for example `GPL_GT-BE98_3.0.0.6.102.40585-*.tgz`) and asks to copy or
merge its prebuilt binaries into this repository, or when a build fails because
a model-specific prebuilt was not copied or no longer matches the source.

## Inputs to establish first

1. **GPL root**: the extracted ASUS source directory. It must contain
   `release/src-rt-5.04behnd.4916` for BE models. Example:
   `/home/gnuton/ASUS/GT-BE98/asuswrt`.
2. **Repository root**: usually `/home/gnuton/ASUS/asuswrt-merlin.ng`.
3. **Model/build name**: the `BUILD_NAME` used by the GPL, for example
   `GT-BE98`. This is not always the same as the marketing model.
4. **Target branch**: confirm the current branch and that the tree is clean
   before copying.

Do not assume the GPL directory layout; verify the SDK directory
(`release/src-rt-5.04behnd.4916`, `release/src-rt-5.04axhnd.675x`, etc.) and the
`implNNN` WL directory in the GPL source.

## Copy workflow

### 1. Choose the correct copy script

- Use `tools/copy-prebuilt` for current GPLs and BE models. It is actively
  maintained and handles `impl103`/`impl105`, newer bootloader objects,
  `nmp-api`, and additional model-specific prebuilts.
- Use `tools/copy-prebuilt-be` only for older GPLs or when the source still
  uses the obsolete `impl101` WL path. It is a 2024-era script and copies a
  smaller subset.

If unsure, inspect the GPL tree:

```bash
ls release/src-rt-5.04behnd.4916/bcmdrivers/broadcom/net/wl/
```

Use the script whose expected paths exist.

### 2. Run the script from the GPL root

The script uses relative source paths, so the working directory must be the GPL
root. `REPO` must point at this repository.

```bash
cd /home/gnuton/ASUS/GT-BE98/asuswrt

REPO=/home/gnuton/ASUS/asuswrt-merlin.ng \
  /home/gnuton/ASUS/asuswrt-merlin.ng/tools/copy-prebuilt GT-BE98 \
  | tee /tmp/copy-prebuilt-gt-be98.log
```

Expected non-fatal noise:

- `aarch64-buildroot-linux-gnu-strip: command not found` if the cross
  toolchain is not in `PATH`. Existing repo objects are often unstripped too;
  do not block on this.
- `cp: cannot stat 'release/src/router/asd2.1/prebuild/*'` and similar for
  components absent from the GPL.
- `cp: cannot stat 'prebuilt/make_ext4fs'` during host-tool setup; normally
  ignored by the build.

If `aarch64-buildroot-linux-gnu-strip` is required, source the firmware
toolchain environment before running the script.

### 3. Review what changed

```bash
git status --porcelain=v1
git diff --stat
git diff --name-status | grep '^D'          # deletions
git status --porcelain=v1 | grep '^??'      # new files/directories
```

Typical results:

- Modified model-specific binaries under `release/src/router/*/prebuild/`,
  `release/src/router/*/prebuilt/`, `router-sysdep.<model>/`, and
  `bootloaders/obj.<model>/`.
- Source-file changes because the script syncs whole `router-sysdep` trees
  (`make` files, scripts, C files). This is expected.
- Deletions caused by `rsync --del` when the new GPL no longer ships a file.
  Review them; do not blindly restore everything.

Verify important files against the GPL source with hashes, for example:

```bash
src=/home/gnuton/ASUS/GT-BE98/asuswrt/release/src-rt-5.04behnd.4916
dst=release/src-rt-5.04behnd.4916/router-sysdep.gt-be98
md5sum "$src/router-sysdep/hnd_extra/prebuilt/dhd.o" \
       "$dst/hnd_extra/prebuilt/dhd.o"
```

### 4. Restore files the build still needs

`rsync --del` can remove model prebuilts that the new GPL does not ship but the
current build recipes still copy. Check deletions before committing.

Known example for GT-BE98:

- The GPL omitted `router-sysdep.gt-be98/vlanctl/prebuilt/{vlanctl,rawSocketTest}`
  and `vlanctl_lib/prebuilt/libvlanctl.so`, while `vlanctl/Makefile` still does
  `cp -f prebuilt/$(EXE) .`. Restore those from `HEAD` if the build would
  otherwise fail.

`csimond/csimond.c` may legitimately disappear because the new GPL switched
that package to the prebuilt path. Only restore it if `PREBUILT_BCMBIN` is not
used for the target.

### 4b. Use per-model U-Boot/kernel prebuilt trees

U-Boot and kernel objects differ by model and CPU architecture. Do not copy
one model's objects into the shared source tree and expect all models to
build. Store a complete tree per model:

```text
release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/prebuilt/<MODEL>/
```

with `net/`, `common/`, `board/`, `arch/.../ubus`, `arch/.../rng`, the full
`drivers/net/bcmbca/phy/` set (including serdes subdirectories), and switch
objects. `bootloaders/Makefile` selects it by `BUILD_NAME`/`PROFILE`.

Architecture examples:

- GT-BE98 / RT-BE58U U-Boot: AArch64
- RT-BE82U U-Boot: 32-bit ARM

Kernel prebuilts without a `.<chip>.o_saved` fallback also need the model
architecture, for example `bp3/impl1/fsaux.o` and
`gdx/impl1/prebuilt/gdx_hw.o` for RT-BE82U.

### 5. Commit and push (only when asked)

Keep commits focused and use the model and GPL version in the message:

```bash
git add -A -- release

git commit -m "GT-BE98: update prebuilts from GPL 102.40585"
git push origin refs/heads/3006.102.9:refs/heads/3006.102.9
```

Do not stage unrelated untracked files such as editor swap files.

## CI monitoring

The workflow is `.github/workflows/github-actions.yml` and triggers on every
push and pull request. It builds two jobs: `default` and `rog`.

Always pass the repository explicitly; `gh` may otherwise resolve the
`upstream` remote (RMerl) instead of the fork:

```bash
gh run list -R gnuton/asuswrt-merlin.ng --branch 3006.102.9 --limit 5
gh run view -R gnuton/asuswrt-merlin.ng <RUN_ID> --log-failed
```

For long runs, poll with retries instead of `gh run watch` (which aborts on a
transient GitHub API error):

```bash
while true; do
  st=$(gh api repos/gnuton/asuswrt-merlin.ng/actions/runs/<RUN_ID> \
        --jq '.status' 2>/dev/null) || st=""
  [ "$st" = "completed" ] && break
  sleep 60
done
```

## Known post-copy failure patterns

These were found while updating GT-BE98 to GPL 102.40585. Treat them as
patterns: check whether a newer GPL has already fixed them before applying.

### Missing U-Boot `RTL_OBJS.o`

Symptom:

```
cp: cannot stat '.../drivers/net/bcmbca/rtl8372/RTL_OBJS.o'
make[...]: *** [.../rtl8372/RTL_OBJS_preb.o] Error 1
```

The upstream 3006.102.9 tag removed
`release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/drivers/net/bcmbca/rtl8372/RTL_OBJS.o`,
but the GT-BE98 `rtl8372/Makefile` still requires it:

```make
obj-y += RTL_OBJS.o
```

Fix: copy `RTL_OBJS.o` from the model's GPL into the **per-model** U-Boot
prebuilt tree, not the shared source path:

```bash
mkdir -p release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/prebuilt/<MODEL>/drivers/net/bcmbca/rtl8372
cp "$GPL/release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/drivers/net/bcmbca/rtl8372/RTL_OBJS.o" \
   release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/prebuilt/<MODEL>/drivers/net/bcmbca/rtl8372/
```

### Architecture mismatch in U-Boot/kernel prebuilts

Symptom:

```
net/arp_preb.o: file not recognized: file format not recognized
fsaux.o: file not recognized: file format not recognized
gdx_hw.o: file not recognized: file format not recognized
```

The shared objects are for another CPU architecture (for example AArch64)
while the model being built is 32-bit ARM. Use the complete per-model
`u-boot-2019.07/prebuilt/<MODEL>/` tree and copy kernel objects such as
`bp3/impl1/fsaux.o` and `gdx/impl1/prebuilt/gdx_hw.o` from the model GPL.

### New `libwebapi` symbols

Symptom:

```
libwebapi.so: undefined reference to `is_valid_netbios_name'
libwebapi.so: undefined reference to `ookla_eos_support'
```

The new `priv_webapi.o` expects newer `shared` code than the current tree has.

Fix, matching the GPL source:

1. Copy `shared/ookla_common.h` and `shared/ookla_utils.c` from the GPL.
2. Add to `shared/Makefile`, after `OBJS += timer_utils.o`:

   ```make
   ifeq ($(or $(RTCONFIG_OOKLA),$(RTCONFIG_OOKLA_LITE)),y)
   OBJS += ookla_utils.o
   endif
   ```

3. Add to `shared/shared.h`:

   ```c
   #if defined(RTCONFIG_OOKLA) || defined(RTCONFIG_OOKLA_LITE)
   #include "ookla_common.h"
   #endif
   ```

4. Add `is_valid_netbios_name()` to `shared/misc.c` and its declaration to
   `shared/shared.h`.
5. Remove the duplicate local implementation from
   `libdisk/write_smb_conf.c` (the GPL relies on the shared one).

### Duplicate OOKLA definitions in `httpd/web.c`

Symptom:

```
web.c: error: redeclaration of enumerator 'OOKLA_STATE_IDLE'
```

`shared.h` now includes `ookla_common.h`, while `httpd/web.c` still defines its
own `OOKLA_FOLDER`, `OOKLA_STATE_*` enum, and `OOKLA_DBG` block.

Fix: remove that local block from `httpd/web.c`; the header now provides it.

### Missing `is_valid_ifname`

Symptom:

```
prebuild/web-broadcom.o: undefined reference to `is_valid_ifname'
```

Fix: add the GPL implementation of `is_valid_ifname()` to `shared/misc.c` and
declare it in `shared/shared.h`, next to `is_valid_netbios_name`.

### Removed `web_hook.o` hooks

Symptom:

```
web.o: undefined reference to `system_cmd_test'
web.o: undefined reference to `do_register_app_cnonce_cgi'
```

The new GPL's `web_hook.o` no longer exports these. The GPL's `web.c` removed
the corresponding code.

Fix in `release/src/router/httpd/web.c`:

- Remove the `Main_AdmStatus_Content.asp` branch that calls
  `system_cmd_test()`.
- Remove the `extern void do_register_app_cnonce_cgi(...)` declaration.
- Remove the `register_app_cnonce.cgi` entry from `mime_handlers[]`.

Keep the `Main_ConnStatus_Content.asp` branch unless the GPL also removed the
corresponding page; it does not reference a missing symbol.

## General iteration rule

After each push, download only the failed log:

```bash
gh run view -R gnuton/asuswrt-merlin.ng <RUN_ID> --log-failed \
  > /tmp/run-failed.log

grep -iE 'fatal error|undefined reference|No such file|cannot stat|Error [0-9]|\*\*\*' \
  /tmp/run-failed.log | tail -50
```

Fix the first real fatal error, commit, push, and repeat. Ignore known benign
noise such as missing `dhd_common.c`, missing `make_ext4fs`, `aclocal: warning:
couldn't open directory 'm4'`, and missing `libwlc_nt_lib.so` unless the log
shows a later fatal failure caused by it.

## Guardrails

- Do not commit unrelated untracked files, editor swap files, or build output.
- Do not use `git clean -fd` on a tree containing user work.
- Do not blindly restore every deletion made by `rsync --del`; check whether
  the build still references the file.
- Prefer copying files from the same GPL source the user provided. If a
  fallback to the previous branch is needed, explain why and use the known-good
  object from `HEAD`.
- Keep source changes minimal and match the GPL version, so future GPL merges
  stay easy.
