---
name: Add a New Router Model
description: Enable a new ASUS router model in AsusWrt-Merlin.ng, including target.mak, CI, copy-prebuilt support, model-specific prebuilts, and build verification.
---

# Add a New Router Model

Use this skill when asked to enable a new router model (for example
`RT-BE58U`) from an extracted ASUS GPL tree. It covers the full workflow:
target configuration, CI, prebuilt copying, model-specific U-Boot objects,
ignored binaries, committing, and CI iteration.

For the prebuilt-copy workflow itself, also read
`.opencode/skills/merge-gpl-prebuilts/SKILL.md`.

## Inputs to establish first

1. **Model and build name** — the `BUILD_NAME` from the GPL's
   `release/src-rt/target.mak`, e.g. `RT-BE58U`.
2. **SDK directory** — e.g. `src-rt-5.04behnd.4916`.
3. **Profile** — the `PROFILE="..."` value on the model's base target, e.g.
   `96765GW`. This is used by target files and model selection.
4. **GPL root** — e.g. `/home/gnuton/ASUS/RT-BE58U/asuswrt`.
5. **Repository root** — e.g. `/home/gnuton/ASUS/asuswrt-merlin.ng`.
6. **Current branch** — usually a model-specific branch such as
   `3006.102.9_RT-BE58U`.

Check that `make <lowercase-model>` maps to `BUILD_NAME=<UPPERCASE-MODEL>`.
`release/src-rt/Makefile` derives it from `MAKECMDGOALS`.

## 1. Add the target to target.mak

Extract the exact model block from the GPL, including its base definition if
that base is missing from the repo.

```bash
# GPL source
grep -n '^export <MODEL>' "$GPL/release/src-rt/target.mak"

# Repo target (release/src-rt-5.04.../target.mak is a symlink to this)
grep -n '^export HND-' release/src-rt/target.mak
grep -n '^export <MODEL>' release/src-rt/target.mak
```

Copy the full block, for example:

```make
export HND-96765_BASE := ... PROFILE="96765GW" ...
export RT-BE58U := $(HND-96765_BASE)
export RT-BE58U += BUILD_NAME="RT-BE58U" ... PROFILE settings ...
```

Do not invent or trim flags. Use the GPL block verbatim unless it contains
repo-specific overrides that must be preserved.

## 2. Verify the target profile file

The build uses:

```text
release/src-rt-<sdk>/targets/<PROFILE>/<PROFILE>.<BUILD_NAME>
```

Example:

```text
release/src-rt-5.04behnd.4916/targets/96765GW/96765GW.RT-BE58U
```

If it already exists and matches the GPL, keep it. If missing, copy it from
the GPL. Also check `release/src-rt/Makefile` for model-specific branches
(DTS selection, image naming, special LED/switch handling, etc.).

## 3. Add the model to CI

Edit `.github/workflows/github-actions.yml` matrix. Use the lowercase model
name because CI runs `make "${MODEL}"`.

```yaml
- { model: "<model-lowercase>", sdk: "src-rt-5.04behnd.4916", ui: "default", skip: false }
```

Add `rog`/`tuf` rows only if the model supports that UI. Do not add a ROG row
just because other models have one.

If CI is temporarily limited to one model, comment the others out with an
explicit note and remember to restore them.

## 4. Add copy-prebuilt support

Edit `tools/copy-prebuilt` (the maintained script; `copy-prebuilt-be` is an
older impl101-era script and normally should not be used for current GPLs).

Add the model in all relevant places:

1. **BE/SDK model list** for the branch that matches the SDK.
2. **BCM676x archer branch** if the model uses the archer/RTK path rather than
   `pktrunner`/RTL8372. Example for RT-BE58U:
   ```bash
   if [ "$BUILD_NAME" == "RT-BE92U" ] || [ "$BUILD_NAME" == "RT-BE58_GO" ] || [ "$BUILD_NAME" == "RT-BE58U" ]
   ```
3. **WL implementation selection**:
   - `impl103` for most BCM4916 models
   - `impl105` for `RT-BE58_GO`
   - `impl101` only in the old script/GPLs
4. **acsdv2 / qosmgmt / dongle** paths for the selected impl.
5. **nmp-api** model list if the model ships `libnmpapi.so`.
6. **Preflight `mkdir -p`** list, including the model-specific U-Boot PHY
   prebuilt directory.
7. **Model-specific PHY prebuilts**:
   - `GT-BE98`: `mac_drv.o`, `bus_drv.o`, `mdio_drv_v1.o`,
     `bus_drv_mdio_v1.o`, `mac_drv_unimac.o`, `mac_drv_xport.o`,
     `unimac_drv_impl2.o`
   - `RT-BE58U`: `mac_drv.o`, `bus_drv.o`, `mdio_drv_sf2.o`,
     `bus_drv_sf2.o`, `mac_drv_sf2.o`
8. **Switch/port prebuilts**:
   - `rtl8372/RTL_OBJS.o` (GT-BE98)
   - `rtk/RTK_OBJS.o`, `bcmbca_sysport_v2.o` (BCM676x/RT-BE58U)
9. **Kernel switch prebuilt**: `kernel/linux-4.19/drivers/char/rtk/rtl8367s.o`
   when present.
10. **vlanctl fallback**: some GPLs omit `vlanctl`/`vlanctl_lib` prebuilts;
    reuse the BE4916 reference model (`router-sysdep.rt-be92u`).

## 5. Run the copy script

Run from the GPL root, with `REPO` pointing at the repo:

```bash
cd "$GPL"

REPO=/home/gnuton/ASUS/asuswrt-merlin.ng \
  /home/gnuton/ASUS/asuswrt-merlin.ng/tools/copy-prebuilt <MODEL> \
  | tee /tmp/copy-prebuilt-<model>.log
```

Expected harmless errors:

- `aarch64-buildroot-linux-gnu-strip: command not found` when the toolchain
  is not in `PATH`
- missing optional components such as `asd2.1`, `asus_rbd`, `dhd`
- missing optional objects in the GPL

Review:

```bash
git status --porcelain=v1
git diff --stat
git status --porcelain=v1 | grep '^??'
git ls-files --others --ignored --exclude-standard | grep -i '<MODEL>'
```

## 6. Commit ignored prebuilts

`.gitignore` files exclude `*.so` and some binaries. New model prebuilts are
often silently left untracked. Force-add them:

```bash
git ls-files --others --ignored --exclude-standard \
  | grep -iE '<MODEL>|<model-lowercase>' \
  > /tmp/<model>-ignored.txt

xargs -r git add -f < /tmp/<model>-ignored.txt
```

Check for missing `protect_srv`, `libbcm`, `httpd`, `rc`, `shared`,
`lighttpd`, `networkmap`, `nt_center`, `rc`, `wlc_nt`, etc.

## 7. Handle shared U-Boot PHY objects

Some U-Boot prebuilts differ per model (for example GT-BE98 uses
unimac/xport, RT-BE58U uses SF2). Do not overwrite one model's object with the
other's.

Store variants under:

```text
release/src-rt-5.04behnd.4916/bootloaders/u-boot-2019.07/drivers/net/bcmbca/phy/prebuilt/<MODEL>/
```

`release/src-rt-5.04behnd.4916/bootloaders/Makefile` selects the variant by
`BUILD_NAME`, with a `PROFILE` fallback (`96813GW` -> GT-BE98,
`96765GW` -> RT-BE58U), and copies it before `spl`, `tpl`, and `uboot`.

If adding another model with divergent PHY objects, add its `prebuilt/<MODEL>`
directory and extend the selector.

## 8. Commit and push

Keep commits focused, for example:

```text
RT-BE58U: add target configuration from GPL 102.40585
CI: add RT-BE58U default build
tools/copy-prebuilt: support RT-BE58U
RT-BE58U: import prebuilts from GPL 102.40585
RT-BE58U: force-add ignored prebuilt binaries
```

Push the model branch and let CI build it.

## 9. CI iteration

Use `gh` with an explicit repository (the repo has both `origin` and an
`upstream` remote; without `-R` it may query RMerl):

```bash
gh run list -R gnuton/asuswrt-merlin.ng --branch <branch> --limit 5
gh run view -R gnuton/asuswrt-merlin.ng <run-id> --log-failed
```

For a specific failed job whose run is still in progress:

```bash
gh api --allow-escape-sequences \
  repos/gnuton/asuswrt-merlin.ng/actions/jobs/<job-id>/logs \
  > /tmp/job.log

sed -r 's/\x1B\[[0-9;]*[A-Za-z]//g' /tmp/job.log > /tmp/job-clean.log

grep -iE 'fatal error|undefined reference|No such file|cannot stat|Error [0-9]|\*\*\*' \
  /tmp/job-clean.log | tail -50
```

Fix the first fatal error, commit, push, and repeat. Do not chase known benign
noise (`make_ext4fs`, `dhd_common.c`, `aclocal`, `libwlc_nt_lib.so`) unless it
is followed by a fatal error.

## Known model-enablement failures

| Symptom | Fix |
| --- | --- |
| U-Boot `No rule to make target ..._preb.o` | Copy the missing source `.o` from the model GPL into the same U-Boot source path and add it to `copy-prebuilt`. GT-BE98 needs `rtl8372/RTL_OBJS.o`; RT-BE58U needs `rtk/RTK_OBJS.o`, SF2 PHY objects, and `bcmbca_sysport_v2.o`. |
| U-Boot undefined `mac_drv_unimac` / `mac_drv_xport` / `bus_drv_mdio_v1` | Wrong generic `mac_drv.o`/`bus_drv.o` variant. Use the model-specific PHY prebuilt set. |
| Kernel `No rule to make target .../rtl8367s_preb.o` | Copy `kernel/linux-4.19/drivers/char/rtk/rtl8367s.o` from the GPL. |
| `protect_srv` or other prebuilt missing in CI | The file is ignored by `.gitignore`; force-add it. |
| `prebuilt/vlanctl` missing | Reuse `router-sysdep.rt-be92u/{vlanctl,vlanctl_lib}/prebuilt`, or add the GPL's own files if present. |
| `libwebapi.so` undefined `ookla_eos_support` / `is_valid_netbios_name` | Backport the shared/ookla helpers; see `merge-gpl-prebuilts`. |
| Duplicate `OOKLA_STATE_*` in `httpd/web.c` | Remove the local OOKLA block now provided by `shared.h`. |
| `is_valid_ifname` undefined | Add the shared implementation and declaration. |
| `system_cmd_test` / `do_register_app_cnonce_cgi` undefined | Remove the obsolete `httpd/web.c` hooks. |

## Final verification checklist

- [ ] `target.mak` block matches the GPL
- [ ] target profile file exists
- [ ] CI matrix has the model
- [ ] `copy-prebuilt` handles the model and its SDK
- [ ] model-specific U-Boot PHY prebuilts exist in `phy/prebuilt/<MODEL>`
- [ ] ignored prebuilts force-added
- [ ] `git status` clean
- [ ] CI run succeeds
- [ ] GT-BE98/other temporarily disabled rows restored if they were disabled

## Guardrails

- Do not overwrite shared model-specific objects in place; use model dirs and
  a build-time selector.
- Do not commit unrelated untracked files or editor swap files.
- Do not delete existing model directories with `git clean`.
- Prefer copying from the exact GPL tree the user provided.
- Keep model branches/builds separate when shared binary variants conflict.
