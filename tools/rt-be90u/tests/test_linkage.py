import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


spec = importlib.util.spec_from_file_location(
    "check_linkage", Path(__file__).parents[1] / "check-linkage.py"
)
linkage = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = linkage
spec.loader.exec_module(linkage)


class LinkageTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name).resolve()
        for name in ("lib", "usr/lib", "sbin"):
            (self.root / name).mkdir(parents=True)

    def test_absolute_and_relative_symlinks_stay_in_firmware(self):
        (self.root / "lib/libc.so").touch()
        (self.root / "usr/lib/libc.so").symlink_to("../../lib/libc.so")
        (self.root / "lib/ld-musl-aarch64.so.1").symlink_to("//usr/lib/libc.so")
        self.assertEqual(
            linkage.rooted_path(self.root, "/lib/ld-musl-aarch64.so.1"),
            self.root / "lib/libc.so",
        )
        self.assertEqual(linkage.rooted_path(self.root, "/../../lib"), self.root / "lib")

    def test_symlink_loop_is_rejected(self):
        (self.root / "lib/loop").symlink_to("/lib/loop")
        with self.assertRaisesRegex(ValueError, "symlink loop"):
            linkage.rooted_path(self.root, "/lib/loop")

    def test_missing_strong_symbol_is_reported_but_optional_weak_symbol_is_allowed(self):
        output = """
  Machine:                           AArch64
 0x0000000000000001 (NEEDED)             Shared library: [libshared.so]
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     1: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND killall_tk_period_wait
     2: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND optional_hook
     3: 0000000000000410    42 FUNC    GLOBAL DEFAULT   12 ovpn_start_client
     4: 0000000000000520    24 FUNC    GLOBAL HIDDEN    12 private_helper
     5: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND SSL_new@OPENSSL_1_1_0 (2)
"""
        info = linkage.parse_readelf(output)
        self.assertEqual(info.needed, ["libshared.so"])
        self.assertEqual(info.defined, {"ovpn_start_client"})
        self.assertEqual(info.undefined, {"killall_tk_period_wait", "SSL_new"})

    def test_transitive_dependencies_and_program_exports_resolve_library_symbols(self):
        audit = linkage.Audit(self.root)
        entries = {
            "sbin/rc": linkage.Elf(["libovpn.so"], {"router_defaults"}, {"ovpn_start_client"}, "AArch64"),
            "usr/lib/libovpn.so": linkage.Elf(["libshared.so"], {"ovpn_start_client"}, {"router_defaults", "run_postconf"}, "AArch64"),
            "usr/lib/libshared.so": linkage.Elf([], {"run_postconf"}, set(), "AArch64"),
        }
        for name, info in entries.items():
            path = self.root / name
            path.touch()
            audit.cache[path] = info
        self.assertTrue(audit.check("/sbin/rc")["ok"])
        audit.cache[self.root / "usr/lib/libshared.so"].defined.clear()
        result = audit.check("/sbin/rc")
        self.assertFalse(result["ok"])
        self.assertEqual(result["unresolved_symbols"], {"/usr/lib/libovpn.so": ["run_postconf"]})

    def test_missing_library_and_host_architecture_fail(self):
        audit = linkage.Audit(self.root)
        program = self.root / "sbin/rc"
        program.touch()
        audit.cache[program] = linkage.Elf(["libvpn.so"], set(), set(), "Advanced Micro Devices X86-64")
        result = audit.check("/sbin/rc")
        self.assertFalse(result["ok"])
        self.assertEqual(result["missing_libraries"], [{"file": "/sbin/rc", "library": "libvpn.so"}])
        self.assertEqual(result["wrong_architecture"][0]["file"], "/sbin/rc")


if __name__ == "__main__":
    unittest.main()
