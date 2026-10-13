"""Regression checks for malformed or corrupted firmware bundles."""

import hashlib
import importlib.util
from pathlib import Path
import struct
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "verify-bq16-image.py"
SPEC = importlib.util.spec_from_file_location("bq16_image", SCRIPT)
IMAGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMAGE)


def fixture(payload=b"firmware"):
    names = b"data-size\0data-offset\0algo\0value\0"

    def node(name):
        value = name.encode() + b"\0"
        return struct.pack(">I", 1) + value + b"\0" * (-len(value) % 4)

    def prop(name_offset, value):
        return struct.pack(">III", 3, len(value), name_offset) + value + b"\0" * (-len(value) % 4)

    end_node = struct.pack(">I", 2)
    structure = (node("") + node("images") + node("kernel") +
                 prop(0, struct.pack(">I", len(payload))) +
                 prop(10, struct.pack(">I", 0)) + node("hash-1") +
                 prop(22, b"sha256\0") + prop(27, hashlib.sha256(payload).digest()) +
                 end_node * 4 + struct.pack(">I", 9))
    strings_offset = 56 + len(structure)
    size = strings_offset + len(names)
    header = struct.pack(">10I", 0xD00DFEED, size, 56, strings_offset,
                         40, 17, 16, 0, len(names), len(structure))
    result = header + b"\0" * 16 + structure + names
    return result + b"\0" * (-len(result) % 4) + payload


class FitSafetyTests(unittest.TestCase):
    def test_dual_slot_capacity_preserves_other_volumes(self):
        layout = {"leb_bytes": 126976, "available_lebs": 192,
                  "image_volumes": {"bootfs1": 112, "rootfs1": 458, "bootfs2": 112, "rootfs2": 458}}
        capacity = IMAGE.check_flash_capacity(13111268, 57012224, layout)
        self.assertLessEqual(capacity["required_lebs"], capacity["budget_lebs"])

    def test_oversized_dual_slot_image_is_rejected(self):
        layout = {"leb_bytes": 126976, "available_lebs": 192,
                  "image_volumes": {"bootfs1": 112, "rootfs1": 458, "bootfs2": 112, "rootfs2": 458}}
        with self.assertRaisesRegex(ValueError, "protected volumes"):
            IMAGE.check_flash_capacity(13111268, 100 * 1024 * 1024, layout)

    def test_valid_external_payload(self):
        _, images = IMAGE.fit_images(fixture())
        self.assertEqual(images["kernel"][0], b"firmware")

    def test_corrupt_payload_is_rejected(self):
        data = bytearray(fixture())
        data[-1] ^= 1
        with self.assertRaisesRegex(ValueError, "hash mismatch"):
            IMAGE.fit_images(bytes(data))

    def test_truncated_payload_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "payload outside bundle"):
            IMAGE.fit_images(fixture()[:-1])

    def test_fdt_header_cannot_extend_past_input(self):
        data = bytearray(fixture())
        struct.pack_into(">I", data, 4, len(data) + 1)
        with self.assertRaisesRegex(ValueError, "FDT size outside image"):
            IMAGE.fdt(bytes(data))

    def test_bad_property_name_is_rejected(self):
        data = bytearray(fixture())
        token = data.index(struct.pack(">III", 3, 4, 0))
        struct.pack_into(">I", data, token + 8, 0xFFFFFFFF)
        with self.assertRaisesRegex(ValueError, "property name"):
            IMAGE.fdt(bytes(data))

    def test_missing_end_token_is_rejected(self):
        data = bytearray(fixture())
        header = struct.unpack_from(">10I", data)
        struct.pack_into(">I", data, header[2] + header[9] - 4, 4)
        with self.assertRaisesRegex(ValueError, "missing FDT end token"):
            IMAGE.fdt(bytes(data))

    def test_unsupported_hash_is_rejected(self):
        data = fixture().replace(b"sha256\0", b"sha512\0", 1)
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            IMAGE.fit_images(data)


class ServiceDependencyTests(unittest.TestCase):
    def setUp(self):
        self.vermagic = "4.19.294 SMP preempt mod_unload aarch64"

        def elf(path, bits=1, machine=40):
            header = bytearray(64)
            header[:6] = b"\x7fELF" + bytes((bits, 1))
            struct.pack_into("<H", header, 18, machine)
            return bytes(header) + path.encode()

        self.elf = elf
        names = ("sbin/rc", "usr/sbin/httpd", "usr/sbin/wl", "usr/sbin/cfg_server",
                 "usr/sbin/cfg_client", "lib/libnvram.so", "usr/bin/stubby",
                 "lib/libgetdns.so.10")
        self.files = {name: elf(name) for name in names}
        name = "lib/modules/4.19.294/extra/dhd.ko"
        self.files[name] = elf(name, 2, 183) + b"\0vermagic=" + self.vermagic.encode() + b"\0"

    def mock_tool(self, *args, **kwargs):
        if args[0] == "unsquashfs" and "-ll" in args:
            lines = ["-rwxr-xr-x squashfs-root/" + name for name in self.files]
            lines.append("lrwxrwxrwx squashfs-root/sbin/lld2d.conf -> /tmp/lld2d.conf")
            return "\n".join(lines).encode()
        if args[0] == "unsquashfs" and "-cat" in args:
            return self.files[args[-1]]
        if args[0] == "readelf":
            data = Path(args[-1]).read_bytes()
            if "-d" in args and data.endswith(b"usr/bin/stubby"):
                return b" (NEEDED) Shared library: [libgetdns.so.10]\n"
            if data.endswith(b"usr/sbin/tdts_rule_agent"):
                if "-d" in args:
                    return b" (NEEDED) Shared library: [libc.so.6]\n"
                return b" [Requesting program interpreter: /lib64/ld-linux-aarch64.so.1]\n"
            return b""
        raise AssertionError(args)

    def check(self):
        with patch.object(IMAGE, "run", self.mock_tool):
            return IMAGE.check_filesystem(Path("fixture.pkgtb"), 0, "4.19.294", self.vermagic)

    def test_service_outside_core_daemons_and_its_library_are_checked(self):
        report = self.check()
        self.assertIn("usr/bin/stubby", report["elf"])
        self.assertIn("lib/libgetdns.so.10", report["elf"])
        self.assertEqual(report["runtime_program_aliases"], ["sbin/lld2d.conf"])

    def test_missing_service_library_is_rejected(self):
        del self.files["lib/libgetdns.so.10"]
        with self.assertRaisesRegex(ValueError, "missing ELF dependency libgetdns.so.10"):
            self.check()

    def add_aarch64_helper(self):
        for name in ("usr/sbin/tdts_rule_agent", "lib64/libc.so.6",
                     "lib64/ld-linux-aarch64.so.1"):
            self.files[name] = self.elf(name, 2, 183)
        self.files["lib/libc.so.6"] = self.elf("lib/libc.so.6")

    def test_aarch64_helper_uses_aarch64_libraries_and_interpreter(self):
        self.add_aarch64_helper()
        report = self.check()
        self.assertEqual(report["elf"]["usr/sbin/tdts_rule_agent"]["bits"], 64)
        self.assertIn("lib64/libc.so.6", report["elf"])
        self.assertIn("lib64/ld-linux-aarch64.so.1", report["elf"])

    def test_aarch64_helper_cannot_load_arm32_library(self):
        self.add_aarch64_helper()
        self.files["lib64/libc.so.6"] = self.elf("lib64/libc.so.6")
        with self.assertRaisesRegex(ValueError, "wrong ELF ABI: lib64/libc.so.6"):
            self.check()

    def test_missing_program_interpreter_is_rejected(self):
        self.add_aarch64_helper()
        del self.files["lib64/ld-linux-aarch64.so.1"]
        with self.assertRaisesRegex(ValueError, "missing filesystem entry: lib64/ld-linux-aarch64.so.1"):
            self.check()

    def test_core_router_program_must_stay_arm32(self):
        self.files["usr/sbin/httpd"] = self.elf("usr/sbin/httpd", 2, 183)
        with self.assertRaisesRegex(ValueError, "wrong ELF ABI: usr/sbin/httpd"):
            self.check()


if __name__ == "__main__":
    unittest.main()
