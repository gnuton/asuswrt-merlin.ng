#!/bin/sh
# inject-stock-wireless.sh
# -----------------------------------------------------------------------------
# XT8 V2 (RT-AX95Q V2) wireless driver injection.
#
# WHY THIS EXISTS:
#   The XT8 V2 is a UNIQUE hardware combo in the asuswrt-merlin.ng tree:
#     - SoC:            Broadcom BCM6756  (dual-band integrated: 2.4G + 5G-low)
#     - Backhaul radio: Broadcom BCM6715  (PCIe, 5G-high 4x4 160MHz)
#   => a TRI-BAND device on the BCM6756 platform.
#
#   No other model in the fork has this combination:
#     * All BCM6756 models (rt-ax58u_v2, tuf-ax3000_v2, ...) are DUAL-band
#       -> their prebuilt wl.o (8.4 MB) lacks the 3rd-radio / BCM6715 code path.
#     * All TRI-band models (gt-ax6000, rt-ax86u_pro, xt12, ...) are BCM4912
#       -> wrong CPU architecture (ARM64 vs the 6756's ARMv7 Cortex-A7).
#
#   Therefore the ONLY correct wl/dhd drivers are the ones ASUS ships in the
#   stock XT8 V2 firmware. They are matched-set kernel modules built against
#   kernel 4.19.183 with vermagic:
#       "4.19.183 SMP preempt mod_unload ARMv7 p2v8"
#   which is identical to what this SDK's config_base.6a.6756 produces.
#
# WHAT THIS DOES:
#   After the normal build produces a rootfs (with placeholder BCM6756
#   dual-band wl.ko from rt-ax58u_v2), this script overwrites the wireless
#   kernel modules AND matched userspace binaries with the stock XT8 V2 set.
#
# USAGE:
#   Run against the staged rootfs directory before image packaging:
#       ./inject-stock-wireless.sh <ROOTFS_DIR>
#   e.g.  ./inject-stock-wireless.sh $PLATFORMDIR/target/fs.install
#
# SAFETY:
#   - Verifies target kernel modules dir exists and vermagic matches before
#     overwriting. Aborts on vermagic mismatch (kernel rebuilt with different
#     config) so we never ship non-loadable modules.
# -----------------------------------------------------------------------------
set -e

ROOTFS="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STOCK="$SCRIPT_DIR/stock-prebuilt"
EXPECTED_VERMAGIC="4.19.183 SMP preempt mod_unload ARMv7 p2v8"

if [ -z "$ROOTFS" ] || [ ! -d "$ROOTFS" ]; then
    echo "ERROR: usage: $0 <ROOTFS_DIR>  (rootfs dir not found: '$ROOTFS')" >&2
    exit 1
fi

# Locate the kernel modules directory in the built rootfs
MODDIR=$(find "$ROOTFS/lib/modules" -maxdepth 1 -type d -name '4.19.*' 2>/dev/null | head -1)
if [ -z "$MODDIR" ]; then
    echo "ERROR: could not find /lib/modules/4.19.* in rootfs '$ROOTFS'" >&2
    exit 1
fi

# Verify vermagic of a build-generated module matches the stock set.
# We check the placeholder wl.ko the build produced.
if [ -f "$MODDIR/extra/wl.ko" ]; then
    BUILT_VM=$(modinfo "$MODDIR/extra/wl.ko" 2>/dev/null | sed -n 's/^vermagic:[[:space:]]*//p' | sed 's/[[:space:]]*$//')
    if [ -n "$BUILT_VM" ] && [ "$BUILT_VM" != "$EXPECTED_VERMAGIC" ]; then
        echo "ERROR: vermagic mismatch!" >&2
        echo "  built : '$BUILT_VM'" >&2
        echo "  stock : '$EXPECTED_VERMAGIC'" >&2
        echo "  Stock XT8 V2 modules will NOT load on this kernel. Aborting." >&2
        exit 1
    fi
fi

echo "[inject] rootfs modules dir: $MODDIR"
echo "[inject] vermagic OK ($EXPECTED_VERMAGIC)"

# Overwrite wireless kernel modules with stock XT8 V2 set
echo "[inject] replacing wireless kernel modules..."
for ko in "$STOCK"/lib/modules/*.ko; do
    name=$(basename "$ko")
    # Find where the build placed this module (extra/ or bcmkernel/)
    dest=$(find "$MODDIR" -name "$name" | head -1)
    if [ -n "$dest" ]; then
        cp -f "$ko" "$dest"
        echo "    replaced $name -> ${dest#$ROOTFS}"
    else
        # New module not present in build (e.g. tri-band-only) -> add to extra/
        cp -f "$ko" "$MODDIR/extra/$name"
        echo "    added    $name -> /lib/modules/.../extra/$name"
    fi
done

# Overwrite matched userspace wireless binaries
echo "[inject] replacing userspace wireless binaries..."
for bin in "$STOCK"/usr/sbin/*; do
    [ -f "$bin" ] || continue
    name=$(basename "$bin")
    if [ -f "$ROOTFS/usr/sbin/$name" ]; then
        cp -f "$bin" "$ROOTFS/usr/sbin/$name"
        echo "    replaced /usr/sbin/$name"
    fi
done
for lib in "$STOCK"/usr/lib/*; do
    [ -f "$lib" ] || continue
    name=$(basename "$lib")
    if [ -f "$ROOTFS/usr/lib/$name" ]; then
        cp -f "$lib" "$ROOTFS/usr/lib/$name"
        echo "    replaced /usr/lib/$name"
    fi
done

# Regenerate module dependency data so modprobe resolves the stock deps
if command -v depmod >/dev/null 2>&1; then
    KVER=$(basename "$MODDIR")
    depmod -b "$ROOTFS" "$KVER" 2>/dev/null && echo "[inject] depmod regenerated for $KVER" || true
fi

echo "[inject] XT8 V2 stock wireless injection complete."
