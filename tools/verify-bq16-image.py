#!/usr/bin/env python3
"""Check BQ16 FIT integrity, board selection, ELF dependencies, and module ABI.

These checks do not establish bootloader acceptance or hardware functionality.
"""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import posixpath
import re
import struct
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise ValueError(message)


def string(value):
    return value.rstrip(b"\0").decode("ascii")


def integer(value):
    require(len(value) in (4, 8), "invalid FIT integer")
    return int.from_bytes(value, "big")


def fdt(data):
    require(len(data) >= 40, "truncated FDT header")
    header = struct.unpack_from(">10I", data)
    magic, size, structure, strings = header[:4]
    require(magic == 0xD00DFEED and header[5] == 17, "not a version 17 FDT")
    require(40 <= size <= len(data), "FDT size outside image")
    end = structure + header[9]
    require(40 <= structure <= end <= size, "FDT structure outside header")
    require(40 <= strings <= strings + header[8] <= size, "FDT strings outside header")
    names = data[strings:strings + header[8]]
    nodes, stack = {}, []
    index = structure
    finished = False
    while index < end:
        require(index + 4 <= end, "truncated FDT token")
        token = struct.unpack_from(">I", data, index)[0]
        index += 4
        if token == 1:
            stop = data.find(b"\0", index, end)
            require(stop >= 0, "unterminated FDT node")
            stack.append(data[index:stop].decode("ascii"))
            index = (stop + 4) & ~3
            name = "/".join(stack)
            require(name not in nodes, "duplicate FDT node")
            nodes[name] = {}
        elif token == 2:
            require(bool(stack), "unbalanced FDT node")
            stack.pop()
        elif token == 3:
            require(bool(stack) and index + 8 <= end, "property outside FDT node")
            length, offset = struct.unpack_from(">II", data, index)
            index += 8
            require(index + length <= end, "FDT property outside structure")
            stop = names.find(b"\0", offset)
            require(0 <= offset < len(names) and stop >= offset, "invalid FDT property name")
            name = names[offset:stop].decode("ascii")
            props = nodes["/".join(stack)]
            require(name not in props, "duplicate FDT property")
            props[name] = data[index:index + length]
            index = (index + length + 3) & ~3
        elif token == 4:
            continue
        elif token == 9:
            require(not stack and "" in nodes, "unclosed or missing FDT root")
            finished = True
            break
        else:
            raise ValueError("unknown FDT token")
    require(finished, "missing FDT end token")
    return size, nodes


def fit_images(data):
    size, nodes = fdt(data)
    images = {}
    for name, props in nodes.items():
        if not name.startswith("/images/") or name.count("/") != 2:
            continue
        if "data" in props:
            payload = props["data"]
            offset = None
        else:
            count = integer(props["data-size"])
            if "data-position" in props:
                offset = integer(props["data-position"])
            else:
                offset = ((size + 3) & ~3) + integer(props["data-offset"])
            require(offset >= size and offset + count <= len(data), "FIT payload outside bundle: " + name)
            payload = data[offset:offset + count]
        hashes = [p for n, p in nodes.items()
                  if n.startswith(name + "/") and n.count("/") == 3 and "algo" in p]
        require(bool(hashes), "missing payload hash: " + name)
        for props_hash in hashes:
            require(string(props_hash["algo"]) == "sha256", "payload needs a SHA-256 hash: " + name)
            require(hashlib.sha256(payload).digest() == props_hash["value"], "payload hash mismatch: " + name)
        images[name.split("/")[-1]] = (payload, offset)
    require(bool(images), "FIT has no images")
    return nodes, images


def check_flash_capacity(bootfs_bytes, rootfs_bytes, layout):
    leb = layout["leb_bytes"]
    budget = sum(layout["image_volumes"].values()) + layout["available_lebs"]
    # Stock bootfs1 includes a four-byte boot-state trailer. Reserve it for both.
    boot_lebs = (bootfs_bytes + 4 + leb - 1) // leb
    root_lebs = (rootfs_bytes + leb - 1) // leb
    required = 2 * (boot_lebs + root_lebs)
    require(required <= budget, "dual firmware slots exceed recorded UBI capacity without shrinking protected volumes")
    return {"required_lebs": required, "budget_lebs": budget, "leb_bytes": leb}


def run(*args, input=None):
    return subprocess.run(args, check=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, input=input).stdout


def check_filesystem(image, offset, kernel, expected_vermagic):
    listing = run("unsquashfs", "-o", str(offset), "-ll", str(image)).decode()
    entries = {}
    for line in listing.splitlines():
        if "squashfs-root/" not in line or not line.startswith(("-", "l")):
            continue
        path = line.split("squashfs-root/", 1)[1]
        name, separator, target = path.partition(" -> ")
        entries[name] = target if separator else None

    def resolve(path, optional=False):
        for _ in range(20):
            if optional and path not in entries:
                return None
            require(path in entries, "missing filesystem entry: " + path)
            target = entries[path]
            if target is None:
                return path
            path = posixpath.normpath(target.lstrip("/") if target.startswith("/")
                                      else posixpath.join(posixpath.dirname(path), target))
        raise ValueError("filesystem symlink loop")

    contents = {}

    def read(path):
        path = resolve(path)
        if path not in contents:
            contents[path] = run("unsquashfs", "-o", str(offset), "-cat", str(image), path)
        return contents[path]

    def identity(data, bits, machine, name):
        require(len(data) >= 52 and data[:4] == b"\x7fELF", "not ELF: " + name)
        require(data[4] == bits and data[5] == 1 and
                struct.unpack_from("<H", data, 18)[0] == machine, "wrong ELF ABI: " + name)
        if machine == 40:
            require(not struct.unpack_from("<I", data, 36)[0] & 0x400, "hard-float ARM object: " + name)

    required = ["sbin/rc", "usr/sbin/httpd", "usr/sbin/wl",
                "usr/sbin/cfg_server", "usr/sbin/cfg_client", "lib/libnvram.so"]
    queue, checked = [(path, 1, 40) for path in required], {}
    # Include enabled services and utilities, not only the router's core daemons.
    # Resolve BusyBox aliases once and avoid treating radio firmware as programs.
    program_entries = {path: resolve(path, optional=True) for path in entries
                       if posixpath.dirname(path) in ("bin", "sbin", "usr/bin", "usr/sbin")}
    programs = set(program_entries.values()) - {None}
    for path in sorted(programs):
        data = read(path)
        if not data.startswith(b"\x7fELF"):
            continue
        require(len(data) >= 52, "truncated ELF program: " + path)
        abi = (data[4], struct.unpack_from("<H", data, 18)[0])
        require(abi in ((1, 40), (2, 183)), "unsupported program architecture: " + path)
        queue.append((path, *abi))
    with tempfile.TemporaryDirectory(prefix="bq16-elf-") as temporary:
        elf_path = Path(temporary) / "file.elf"
        while queue:
            path, bits, machine = queue.pop()
            path = resolve(path)
            data = read(path)
            identity(data, bits, machine, path)
            if path in checked:
                continue
            elf_path.write_bytes(data)
            dynamic = run("readelf", "-d", str(elf_path)).decode()
            needed = re.findall(r"\(NEEDED\).*\[([^\]]+)\]", dynamic)
            program_headers = run("readelf", "-l", str(elf_path)).decode()
            interpreter = re.search(r"Requesting program interpreter: ([^\]]+)\]", program_headers)
            if interpreter:
                queue.append((interpreter.group(1).lstrip("/"), bits, machine))
            search = [posixpath.dirname(path)]
            if bits == 2:
                search.extend(("lib64", "usr/lib64"))
            search.extend(("lib", "usr/lib"))
            for rpath in re.findall(r"\((?:RUNPATH|RPATH)\).*\[([^\]]+)\]", dynamic):
                search.extend(p.replace("${ORIGIN}", posixpath.dirname(path))
                              .replace("$ORIGIN", posixpath.dirname(path)).lstrip("/")
                              for p in rpath.split(":"))
            for library in needed:
                matches = [posixpath.normpath(posixpath.join(directory, library)) for directory in search]
                matches = [p for p in matches if p in entries]
                require(bool(matches), "missing ELF dependency " + library + " for " + path)
                queue.append((matches[0], bits, machine))
            checked[path] = {"sha256": hashlib.sha256(data).hexdigest(), "needed": needed,
                             "bits": 32 if bits == 1 else 64, "machine": machine,
                             "interpreter": interpreter.group(1) if interpreter else None}
        modules = {}
        for path in sorted(entries):
            if not path.startswith("lib/modules/") or not path.endswith(".ko"):
                continue
            require(path.startswith("lib/modules/" + kernel + "/"), "module installed for wrong kernel: " + path)
            data = read(path)
            identity(data, 2, 183, path)
            vermagic = re.findall(rb"vermagic=([^\0]+)", data)
            require(len(vermagic) == 1, "missing or ambiguous module vermagic: " + path)
            value = vermagic[0].decode("ascii")
            require(value.split() == expected_vermagic.split(), "module vermagic mismatch: " + path)
            modules[path] = {"vermagic": value, "sha256": hashlib.sha256(data).hexdigest()}
    require("lib/modules/" + kernel + "/extra/dhd.ko" in modules, "missing DHD module")
    return {"elf": checked, "modules": modules,
            "runtime_program_aliases": sorted(path for path, target in program_entries.items()
                                              if target is None)}


def verify(image, kernel="4.19.294"):
    data = image.read_bytes()
    outer, images = fit_images(data)
    require(string(outer[""]["description"]) == "BQ16", "package is not BQ16")
    require(string(outer[""]["asus"]) == "ASUSFLAG#1", "unexpected ASUS capability flag")
    config_name = string(outer["/configurations"]["default"])
    config = outer["/configurations/" + config_name]
    require(set(config) == {"description", "bootfs", "rootfs", "compatible"},
            "unexpected package configuration fields")
    compatible = dict(part.split("=", 1) for part in string(config["compatible"]).split(";"))
    require(compatible.get("flash") == "nand" and compatible.get("chip") == "6813"
            and compatible.get("fstype") == "squashfs", "wrong platform compatibility")
    bootfs, _ = images[string(config["bootfs"])]
    rootfs, rootfs_offset = images[string(config["rootfs"])]
    require(set(images) == {string(config["bootfs"]), string(config["rootfs"])},
            "unexpected outer payloads, loader-inclusive packages are not accepted")
    require(rootfs_offset is not None and rootfs[:4] == b"hsqs", "missing external SquashFS payload")
    inputs = json.loads((Path(__file__).resolve().parents[1] / "docs/bq16/inputs.json").read_text())
    flash_capacity = check_flash_capacity(len(bootfs), len(rootfs), inputs["flash_layout"])
    inner, boot_images = fit_images(bootfs)
    for board in ("BQ16_2556", "BQ16_2556_2"):
        require("fdt_" + board in boot_images, "missing board DTB: " + board)
        fdt(boot_images["fdt_" + board][0])
        for prefix in ("conf_lx_", "conf_ub_"):
            require(string(inner["/configurations/" + prefix + board]["fdt"]) == "fdt_" + board,
                    "incorrect board selection: " + prefix + board)
    require(all(name in boot_images for name in ("kernel", "uboot", "atf")), "incomplete bootfs")
    require(string(inner["/images/kernel"]["compression"]) == "lzo", "unexpected kernel compression")
    decoded_kernel = run("lzop", "-dc", input=boot_images["kernel"][0])
    banner = re.search(rb"Linux version ([^ ]+)", decoded_kernel)
    require(banner is not None and banner.group(1).decode() == kernel, "kernel payload version mismatch")
    start = decoded_kernel.find(b"IKCFG_ST")
    end = decoded_kernel.find(b"IKCFG_ED", start + 8)
    require(start >= 0 and end > start + 8, "kernel lacks embedded ABI configuration")
    config_text = gzip.decompress(decoded_kernel[start + 8:end]).decode()
    kernel_config = dict(line.split("=", 1) for line in config_text.splitlines()
                         if line.startswith("CONFIG_") and "=" in line)
    require(kernel_config.get("CONFIG_ARM64") == "y", "kernel is not AArch64")
    # This pinned BSP uses no module CRCs. A versioned-module kernel needs a
    # separate Module.symvers comparison rather than pretending vermagic suffices.
    require(kernel_config.get("CONFIG_MODVERSIONS") != "y", "versioned modules need CRC verification")
    flags = [name for key, name in (("CONFIG_SMP", "SMP"), ("CONFIG_PREEMPT", "preempt"),
                                   ("CONFIG_MODULE_UNLOAD", "mod_unload"))
             if kernel_config.get(key) == "y"]
    expected_vermagic = " ".join([kernel] + flags + ["aarch64"])
    report = {"sha256": hashlib.sha256(data).hexdigest(), "model": "BQ16",
              "boards": ["BQ16_2556", "BQ16_2556_2"], "compatibility": compatible,
              "outer_payloads": len(images), "bootfs_payloads": len(boot_images),
              "signature_nodes": [name for name in outer if "signature" in name] +
                                 [name for name in inner if "signature" in name],
              "kernel_payload_version": banner.group(1).decode(),
              "expected_module_vermagic": expected_vermagic,
              "flash_capacity": flash_capacity,
              "bootloader_acceptance_verified": False,
              "hardware_tests_verified": False}
    report.update(check_filesystem(image, rootfs_offset, kernel, expected_vermagic))
    report["offline_checks_passed"] = True
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    try:
        help_result = subprocess.run(["unsquashfs", "-help"], stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE)
        require(b"-cat" in help_result.stdout + help_result.stderr,
                "unsquashfs must support -cat; use the pinned Ubuntu 24.04 validation environment")
        report = verify(args.image)
        if args.report:
            args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        print("BQ16 offline checks passed: " + report["sha256"])
        print("Bootloader acceptance and hardware tests remain unverified.")
    except (OSError, ValueError, KeyError, UnicodeError, struct.error, subprocess.CalledProcessError) as error:
        print("BQ16 verification failed: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
