#!/usr/bin/env python3
"""Import BQ16 39256 GPL components without changing other model namespaces."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys


SDK = Path("release/src-rt-5.04behnd.4916")
ROUTER = Path("release/src/router")
MODEL_DIRS = """
aaews/prebuild ahs/prebuild amas-utils/prebuild asd/prebuild
asusnatnl/natnl/prebuild asuswebstorage/prebuild
bwdpi_source/asus/prebuild bwdpi_source/asus_sql/prebuild bwdpi_source/prebuild
cfg_mnt/prebuild dblog/commands/prebuild dblog/daemon/prebuild
dns_ping/prebuild dnsqd/prebuild dropbox_client/prebuild fsmd/prebuild
ftpclient/prebuild httpd/prebuild infosvr/prebuild inotify/prebuild
libasc/prebuild libasuslog/prebuild libletsencrypt/prebuild libwebapi/prebuild
lighttpd-1.4.39/prebuild networkmap/prebuild nmp-api/networkmap/prebuild
nt_center/lib/prebuild nt_center/prebuild protect_srv/lib/prebuild
protect_srv/prebuild rc/prebuild sambaclient/prebuild shared/prebuild
sw-hw-auth/prebuild sysstate/commands/prebuild sysstate/log_daemon/prebuild
u2ec/prebuild usbclient/prebuild wb/prebuild webdav_client/prebuild
wlc_nt/prebuild bsd/prebuilt dnsspoof/prebuilt eventd/prebuilt
libbcm/prebuilt wlceventd/prebuilt
""".split()
BOOT_OBJECTS = """
net/tftp.o net/ping.o net/net.o net/arp.o common/autoboot.o
board/broadcom/bcmbca/board_sdk.o board/broadcom/bcmbca/board.o
drivers/net/bcmbca/phy/dt_parsing.o drivers/net/bcmbca/bcmbca_xrdp_eth.o
""".split()
KERNEL_OBJECTS = ("mpm/impl1/bcm_mpm.o", "pktrunner/impl2/pktrunner.o")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def within(path, root):
    # The pinned Ubuntu toolchain container uses Python 3.8.
    return os.path.commonpath((str(path), str(root))) == str(root)


def elf_identity(path):
    with path.open("rb") as stream:
        header = stream.read(52)
    if not header.startswith(b"\x7fELF"):
        return None
    if len(header) < 52 or header[5] != 1:
        raise ValueError("invalid or non-little-endian ELF: " + str(path))
    machine = struct.unpack_from("<H", header, 18)[0]
    kind = struct.unpack_from("<H", header, 16)[0]
    if (header[4], machine) not in ((1, 40), (2, 183), (1, 3), (2, 62)):
        raise ValueError("unexpected ELF architecture: " + str(path))
    return {"bits": 32 if header[4] == 1 else 64, "machine": machine, "type": kind}


def inventory(source, destination):
    """Check every required input before returning the complete copy operation."""
    inputs = json.loads((destination / "docs/bq16/inputs.json").read_text())
    cfg_server = source / ROUTER / "cfg_mnt/prebuild/cfg_server"
    if not cfg_server.is_file() or sha256(cfg_server) != inputs["stock_cfg_server_sha256"]:
        raise ValueError("donor cfg_server does not match the pinned BQ16 39256 release")
    profile = source / SDK / "targets/96813GW/96813GW.BQ16"
    if 'BCM_WLIMPL=103' not in profile.read_text():
        raise ValueError("BQ16 donor must select Wi-Fi impl103")
    for name in ("dhd.o", "wl.o", "hnd.o", "emf.o", "igs.o"):
        path = source / SDK / "router-sysdep/hnd_extra/prebuilt" / name
        if not path.is_file() or elf_identity(path) != {"bits": 64, "machine": 183, "type": 1}:
            raise ValueError("required AArch64 wireless object missing or incompatible: " + name)
    for relative in KERNEL_OBJECTS:
        path = source / SDK / "bcmdrivers/broadcom/char" / relative
        if not path.is_file() or elf_identity(path) != {"bits": 64, "machine": 183, "type": 1}:
            raise ValueError("required AArch64 kernel object missing or incompatible: " + relative)
    if elf_identity(cfg_server) != {"bits": 32, "machine": 40, "type": 2}:
        raise ValueError("cfg_server must use the 32-bit ARM userspace ABI")

    pairs = []

    def add_tree(src, dst, exclusions=()):
        directory = source / src
        if not directory.is_dir():
            raise ValueError("required donor directory missing: " + str(src))
        count = 0
        for current, directories, files in os.walk(directory, followlinks=False):
            current = Path(current)
            for name in list(directories):
                path = current / name
                if path.is_symlink():
                    files.append(name)
                    directories.remove(name)
            for name in sorted(files):
                path = current / name
                relative = path.relative_to(directory)
                if str(relative) in exclusions or name in exclusions:
                    continue
                pairs.append((path.relative_to(source), dst / relative))
                count += 1
        if not count:
            raise ValueError("required donor directory is empty: " + str(src))

    for directory in MODEL_DIRS:
        excluded = ("init-broadcom.o",) if directory == "rc/prebuild" else ()
        add_tree(ROUTER / directory, ROUTER / directory / "BQ16", excluded)
    # cfg_mnt's opaque library changed its APG interface between BSP releases.
    # Compile callers against the header shipped with this model's library.
    pairs.append((ROUTER / "cfg_mnt/cfg_mtlan.h",
                  ROUTER / "cfg_mnt/prebuild/BQ16/cfg_mtlan.h"))
    add_tree(SDK / "router-sysdep", SDK / "router-sysdep.bq16",
             ("stress/Makefile", "wlc_types.h"))
    add_tree(ROUTER / "hnd_extra/prebuilt",
             SDK / "router-sysdep.bq16/hnd_extra/prebuilt", ("wlc_types.h",))
    for relative in KERNEL_OBJECTS:
        pairs.append((SDK / "bcmdrivers/broadcom/char" / relative,
                      SDK / "router-sysdep.bq16/hnd_extra/prebuilt" / Path(relative).name))
    add_tree(SDK / "hostTools/prebuilt", SDK / "hostTools/prebuilt/BQ16")
    add_tree(SDK / "bcmdrivers/broadcom/net/wl/impl103/main/components/apps/acsdv2/prebuilt",
             SDK / "bcmdrivers/broadcom/net/wl/impl103/main/components/apps/acsdv2/prebuilt/BQ16")
    add_tree(ROUTER / "www/sysdep/BQ16", ROUTER / "www/sysdep/BQ16")
    add_tree(ROUTER / "tuxera/broadcom_arm_be_6813",
             ROUTER / "tuxera/bq16/broadcom_arm_be_6813")
    for relative in BOOT_OBJECTS:
        pairs.append((SDK / "bootloaders/u-boot-2019.07" / relative,
                      SDK / "bootloaders/obj.bq16" / relative))
    for filename in ("wl", "wl_server_socket"):
        pairs.append((ROUTER / "hnd_extra/prebuilt" / filename,
                      SDK / "bcmdrivers/broadcom/net/wl/impl103/main/src/wl/exe/prebuilt/BQ16" / filename))
    for filename in ("BQ16_2556.nvm", "BQ16_2556_2.nvm"):
        pairs.append((SDK / "targets/nvram/6813" / filename,
                      SDK / "targets/nvram/6813" / filename))
    licenses = sorted((source / SDK / "targets/pakl").glob("*BQ16*"))
    if not licenses:
        raise ValueError("BQ16 board licenses are missing")
    pairs.extend((path.relative_to(source), path.relative_to(source)) for path in licenses)

    # The platform and hnd_extra inputs can overlap. Require identical contents.
    targets = {}
    records = []
    for src, dst in sorted(pairs, key=lambda pair: str(pair[1])):
        path = source / src
        if not path.is_file() and not path.is_symlink():
            raise ValueError("required donor file missing: " + str(src))
        if path.is_symlink():
            target = os.readlink(path)
            if target.startswith("/") or not within(path.resolve(), source):
                raise ValueError("donor symlink leaves source tree: " + str(src))
            record = {"source": str(src), "destination": str(dst), "symlink": target}
            identity = ("symlink", target)
        else:
            digest = sha256(path)
            record = {"source": str(src), "destination": str(dst),
                      "sha256": digest, "bytes": path.stat().st_size}
            elf = elf_identity(path)
            if elf:
                record["elf"] = elf
            identity = ("sha256", digest)
        if dst in targets:
            if targets[dst] != identity:
                raise ValueError("conflicting donor inputs for " + str(dst))
            continue
        targets[dst] = identity
        parent = (destination / dst).parent.resolve()
        if not within(parent, destination):
            raise ValueError("destination parent leaves repository: " + str(dst))
        existing = destination / dst
        if "symlink" in record and existing.exists() and not existing.is_symlink():
            raise ValueError("refusing to replace non-symlink: " + str(dst))
        records.append(record)
    return {"model": "BQ16", "inputs": inputs,
            "excluded": ["rc/prebuild/init-broadcom.o", "stress/Makefile", "wlc_types.h"],
            "files": records}


def verify_manifest(destination):
    manifest = json.loads((destination / "docs/bq16/import-manifest.json").read_text())
    require_model = manifest.get("model") == "BQ16" and bool(manifest.get("files"))
    if not require_model:
        raise ValueError("missing BQ16 import inventory")
    for record in manifest["files"]:
        relative = Path(record["destination"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("invalid manifest destination")
        path = destination / relative
        if not within(path.resolve(), destination):
            raise ValueError("imported path leaves repository: " + str(relative))
        if "symlink" in record:
            if not path.is_symlink() or os.readlink(path) != record["symlink"]:
                raise ValueError("imported symlink differs: " + str(relative))
        elif path.is_symlink() or not path.is_file() or sha256(path) != record["sha256"]:
            raise ValueError("imported file differs: " + str(relative))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--verify-manifest", action="store_true")
    args = parser.parse_args()
    destination = args.destination.resolve()
    if args.verify_manifest and args.dry_run:
        parser.error("--verify-manifest and --dry-run are mutually exclusive")
    if not args.verify_manifest:
        if args.source is None:
            parser.error("--source is required for import")
        source = args.source.resolve()
        if within(source, destination) or within(destination, source):
            parser.error("source and destination must be separate trees")
    try:
        if args.verify_manifest:
            manifest = verify_manifest(destination)
            print("Verified " + str(len(manifest["files"])) + " imported BQ16 files")
            return 0
        manifest = inventory(source, destination)
        if not args.dry_run:
            for record in manifest["files"]:
                src, dst = source / record["source"], destination / record["destination"]
                dst.parent.mkdir(parents=True, exist_ok=True)
                if dst.is_symlink():
                    dst.unlink()
                if "symlink" in record:
                    if dst.exists():
                        raise ValueError("refusing to replace non-symlink: " + str(dst))
                    dst.symlink_to(record["symlink"])
                else:
                    shutil.copy2(src, dst)
            output = destination / "docs/bq16/import-manifest.json"
            output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        print(("Checked" if args.dry_run else "Imported") + " " +
              str(len(manifest["files"])) + " BQ16 files")
    except (OSError, ValueError, KeyError) as error:
        print("BQ16 import failed: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
