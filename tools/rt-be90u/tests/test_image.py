import importlib.util
import gzip
import lzma
from pathlib import Path
import struct
import unittest
import zlib


spec = importlib.util.spec_from_file_location('check_image', Path(__file__).parents[1] / 'check-image.py')
image = importlib.util.module_from_spec(spec)
spec.loader.exec_module(image)


def kernel_payload(config=None):
    if config is None:
        config = ''.join(name + '=y\n' for name in image.VPN_KERNEL_OPTIONS)
    kernel = b'kernel prefix' + b'IKCFG_ST' + gzip.compress(config.encode()) + b'IKCFG_ED'
    return lzma.compress(kernel, format=lzma.FORMAT_ALONE) + b'squashfs payload'


def firmware(model=b'TUF-BE9400', arch=22, payload=None):
    if payload is None:
        payload = kernel_payload()
    name = bytes((3, 0, 0, 6)) + model.ljust(12, b'\0') + bytes(16)
    header = image.HEADER.pack(0x27051956, 0, 1, len(payload), 0x40080000,
                               0x40080000, zlib.crc32(payload), 5, arch, 2, 3, name)
    return header[:4] + struct.pack('>I', zlib.crc32(header)) + header[8:] + payload


class ImageChecks(unittest.TestCase):
    def test_accepts_expected_hardware_header_without_claiming_flash_compatibility(self):
        result = image.check_image(firmware())
        self.assertEqual(result['model'], 'TUF-BE9400')
        self.assertEqual(result['flash_compatibility'], 'not established')

    def test_rejects_other_boards_even_with_valid_checksums(self):
        with self.assertRaisesRegex(ValueError, 'product ID'):
            image.check_image(firmware(model=b'TUF-BE6500'))

    def test_rejects_kernel_without_ipv6_policy_routing_even_with_valid_checksums(self):
        config = ('CONFIG_IPV6=y\nCONFIG_IP_MULTIPLE_TABLES=y\nCONFIG_FIB_RULES=y\n'
                  '# CONFIG_IPV6_MULTIPLE_TABLES is not set\n')
        with self.assertRaisesRegex(ValueError, 'CONFIG_IPV6_MULTIPLE_TABLES'):
            image.check_image(firmware(payload=kernel_payload(config)))

    def test_rejects_kernel_without_embedded_configuration(self):
        payload = lzma.compress(b'kernel without IKCONFIG', format=lzma.FORMAT_ALONE)
        with self.assertRaisesRegex(ValueError, 'Missing embedded kernel configuration'):
            image.check_image(firmware(payload=payload))

    def test_rejects_other_cpu_architectures(self):
        with self.assertRaisesRegex(ValueError, 'AArch64'):
            image.check_image(firmware(arch=2))

    def test_rejects_header_corruption(self):
        data = bytearray(firmware())
        data[8] ^= 1
        with self.assertRaisesRegex(ValueError, 'Header CRC32'):
            image.check_image(data)

    def test_rejects_payload_corruption(self):
        data = bytearray(firmware())
        data[-1] ^= 1
        with self.assertRaisesRegex(ValueError, 'Payload CRC32'):
            image.check_image(data)

    def test_rejects_truncation_and_extra_data(self):
        data = firmware()
        for invalid in (b'', data[:63], data[:-1], data + b'\0'):
            with self.subTest(length=len(invalid)), self.assertRaises(ValueError):
                image.check_image(invalid)

    def test_rejects_empty_payload(self):
        with self.assertRaisesRegex(ValueError, 'length'):
            image.check_image(firmware(payload=b''))

    def test_rejects_image_larger_than_observed_ubi_volume(self):
        data = firmware(payload=b'\0' * image.LINUX_VOLUME_BYTES)
        with self.assertRaisesRegex(ValueError, 'UBI volume'):
            image.check_image(data)


if __name__ == '__main__':
    unittest.main()
