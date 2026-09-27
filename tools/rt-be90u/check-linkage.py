#!/usr/bin/env python3
"""Check firmware ELF dependencies without executing router programs."""

import argparse
import dataclasses
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys


@dataclasses.dataclass
class Elf:
    needed: list
    defined: set
    undefined: set
    machine: str


def parse_readelf(output):
    needed, defined, undefined = [], set(), set()
    machine = ""
    for line in output.splitlines():
        if line.strip().startswith("Machine:"):
            machine = line.split(":", 1)[1].strip()
        match = re.search(r"\(NEEDED\).*\[([^]]+)\]", line)
        if match:
            needed.append(match[1])
        fields = line.split()
        if len(fields) < 8 or not re.fullmatch(r"\d+:", fields[0]):
            continue
        if fields[4] not in ("GLOBAL", "WEAK", "UNIQUE"):
            continue
        name = fields[7].split("@", 1)[0]
        if fields[6] == "UND":
            if fields[4] != "WEAK":
                undefined.add(name)
        elif fields[5] in ("DEFAULT", "PROTECTED"):
            defined.add(name)
    return Elf(needed, defined, undefined, machine)


def rooted_path(root, name):
    """Resolve absolute firmware symlinks inside root, never on the host."""
    parts = list(PurePosixPath("/" + str(name).lstrip("/")).parts[1:])
    resolved, links = [], 0
    while parts:
        part = parts.pop(0)
        if part in ("", "."):
            continue
        if part == "..":
            if resolved:
                resolved.pop()
            continue
        candidate = root.joinpath(*resolved, part)
        if candidate.is_symlink():
            links += 1
            if links > 40:
                raise ValueError("symlink loop: " + str(name))
            target = os.readlink(candidate)
            if target.startswith("/"):
                resolved = []
            parts = list(PurePosixPath(target.lstrip("/")).parts) + parts
        else:
            resolved.append(part)
    return root.joinpath(*resolved)


class Audit:
    def __init__(self, root, readelf="readelf"):
        self.root = Path(root).resolve()
        self.readelf = readelf
        self.cache = {}

    def elf(self, path):
        if path not in self.cache:
            result = subprocess.run(
                [self.readelf, "--wide", "--file-header", "--dynamic", "--dyn-syms", str(path)],
                capture_output=True, text=True, check=True, env={**os.environ, "LC_ALL": "C"},
            )
            self.cache[path] = parse_readelf(result.stdout)
        return self.cache[path]

    def dependency(self, name):
        # musl's default search path; custom loader paths are outside this audit.
        names = [name] if "/" in name else [directory + name for directory in (
            "/lib/", "/usr/local/lib/", "/usr/lib/",
        )]
        for candidate in names:
            path = rooted_path(self.root, candidate)
            if path.is_file():
                return path
        return None

    def check(self, program):
        pending = [rooted_path(self.root, program)]
        closure, missing, wrong_arch = {}, [], []
        while pending:
            path = pending.pop()
            if path in closure:
                continue
            info = self.elf(path)
            closure[path] = info
            label = "/" + str(path.relative_to(self.root))
            if info.machine != "AArch64":
                wrong_arch.append({"file": label, "machine": info.machine})
            for name in info.needed:
                target = self.dependency(name)
                if target is None:
                    missing.append({"file": label, "library": name})
                else:
                    pending.append(target)
        provided = set().union(*(info.defined for info in closure.values()))
        unresolved = {
            "/" + str(path.relative_to(self.root)): sorted(info.undefined - provided)
            for path, info in closure.items() if info.undefined - provided
        }
        return {
            "program": program,
            "libraries": sorted("/" + str(path.relative_to(self.root)) for path in closure),
            "missing_libraries": missing,
            "wrong_architecture": wrong_arch,
            "unresolved_symbols": unresolved,
            "ok": not (missing or wrong_arch or unresolved),
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rootfs", type=Path)
    parser.add_argument("programs", nargs="*", default=["/sbin/rc", "/usr/sbin/httpd"])
    parser.add_argument("--readelf", default="readelf")
    args = parser.parse_args()
    try:
        audit = Audit(args.rootfs, args.readelf)
        results = [audit.check(program) for program in args.programs]
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(2, str(error) + "\n")
    print(json.dumps(results, indent=2))
    return 0 if all(result["ok"] for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
