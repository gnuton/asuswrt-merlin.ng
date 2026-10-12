"""Regression checks for malformed or corrupted firmware bundles."""

import hashlib
import importlib.util
from pathlib import Path
import struct
import unittest


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


if __name__ == "__main__":
    unittest.main()
