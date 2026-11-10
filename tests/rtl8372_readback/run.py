#!/usr/bin/env python3
"""Host fault injection and ILP32/LP64 UAPI layout checks, no device access."""
import os
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
KERNEL = ROOT / "release/src-rt-5.04behnd.4916/kernel/linux-4.19"

with tempfile.TemporaryDirectory(prefix="rtl8372-readback-", dir="/tmp") as temp:
    work = Path(temp)
    fake = work / "linux"
    fake.mkdir()
    for name in ("build_bug errno export sched stddef string capability compat fs "
                 "miscdevice module slab uaccess vmalloc types ioctl").split():
        (fake / (name + ".h")).write_text('#include "kernel_test.h"\n')
    compiler = os.environ.get("CC", "cc")
    for compat in (False, True):
        binary = work / ("readback-compat" if compat else "readback-native")
        cmd = [compiler, "-std=gnu11", "-O2", "-Wall", "-Wextra", "-Werror",
               "-Wno-unused-parameter", "-Wno-unused-function",
               "-Wno-unused-variable", "-I" + str(work), "-I" + str(HERE),
               "-I" + str(KERNEL / "include"), str(HERE / "readback_test.c"),
               "-o", str(binary)]
        if compat:
            cmd.append("-DCONFIG_COMPAT")
        subprocess.run(cmd, check=True)
        subprocess.run([str(binary)], check=True)

    # Compile the actual UAPI with system Linux integer definitions, not the
    # mock kernel types. No multilib runtime is needed for compile-only checks.
    layout = work / "layout.c"
    layout.write_text('#include "' + str(KERNEL / "include/uapi/linux/gtbe98_rtl8372.h") + '"\n' + '''#include <stddef.h>
_Static_assert(sizeof(struct gtbe98_rtl8372_identity) == 88, "identity");
_Static_assert(sizeof(struct gtbe98_rtl8372_capture) == 24, "capture");
_Static_assert(offsetof(struct gtbe98_rtl8372_capture, snapshot) == 8, "pointer");
_Static_assert(sizeof(struct gtbe98_rtl8372_snapshot) == 65728, "snapshot");
_Static_assert(offsetof(struct gtbe98_rtl8372_snapshot, ports) == 64, "ports");
_Static_assert(offsetof(struct gtbe98_rtl8372_snapshot, vlans) == 192, "vlans");
_Static_assert(GTBE98_RTL8372_IOC_IDENTITY == 0x80584771U, "identity ioctl");
_Static_assert(GTBE98_RTL8372_IOC_CAPTURE == 0x40184772U, "capture ioctl");
''')
    clang = shutil.which("clang")
    if not clang:
        raise SystemExit("clang required for ILP32/LP64 compile-only UAPI checks")
    for target in ("i386-linux-gnu", "x86_64-linux-gnu", "aarch64-linux-gnu"):
        subprocess.run([clang, "--target=" + target, "-std=c11", "-Werror",
                        "-I/usr/include/x86_64-linux-gnu",
                        "-c", str(layout), "-o", str(work / (target + ".o"))], check=True)
    print("UAPI layouts and ioctl numbers: ILP32, LP64, AArch64 passed")

    # Exercise the real Kbuild input rule without compiling/loading a module.
    # A changed input must be rejected even when the output already exists.
    objects = work / "objects"
    objects.mkdir()
    makefile = work / "Makefile"
    makefile.write_text("include " + str(KERNEL / "drivers/char/rtl8372/Makefile") +
                        "\n.PHONY: FORCE show\nFORCE:\nshow:\n" +
                        "\t@printf '%s\\n' '$(rtl8372-objs)' '$(obj-m)'\n")
    target = str(objects / "rtl8372_preb.o")
    make = ["make", "-s", "-f", str(makefile), "srctree=" + str(KERNEL),
            "obj=" + str(objects)]
    subprocess.run(make + ["MODEL=GTBE98", target], check=True)
    expected = "3e1948756cec3f05e217c637dc9debe7a9addfc93749feb3653fb9a3006e38f7"
    assert hashlib.sha256(Path(target).read_bytes()).hexdigest() == expected
    bad = work / "wrong-prebuilt.o"
    bad.write_bytes(b"wrong model or SDK revision")
    result = subprocess.run(make + ["MODEL=GTBE98", "gtbe98-prebuilt=" + str(bad), target],
                            capture_output=True, text=True)
    assert result.returncode != 0 and "unsupported prebuilt SHA256" in result.stderr
    assert hashlib.sha256(Path(target).read_bytes()).hexdigest() == expected
    for model in ("GTBE98", "GTBE98_PRO", "RTBE92U"):
        result = subprocess.run(make + ["MODEL=" + model, "show"],
                                check=True, capture_output=True, text=True)
        assert ("gtbe98_readback_adapter.o" in result.stdout) == (model == "GTBE98")
        assert ("gtbe98_rtl8372_readback.o" in result.stdout) == (model == "GTBE98")
    print("Kbuild: pinned input, wrong-hash rejection, model scope passed")
