"""Reject altered imports and manifest paths outside the repository."""

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "import-bq16.py"
SPEC = importlib.util.spec_from_file_location("bq16_import", SCRIPT)
IMPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORT)


class ImportManifestTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "repository"
        self.root.mkdir()
        self.file = self.root / "component.o"
        self.file.write_bytes(b"original component")
        self.records = [{"destination": "component.o",
                         "sha256": hashlib.sha256(self.file.read_bytes()).hexdigest()}]

    def save(self):
        path = self.root / "docs/bq16/import-manifest.json"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps({"model": "BQ16", "files": self.records}))

    def test_valid_manifest(self):
        self.save()
        self.assertEqual(len(IMPORT.verify_manifest(self.root)["files"]), 1)

    def test_changed_file_is_rejected(self):
        self.save()
        self.file.write_bytes(b"changed component")
        with self.assertRaisesRegex(ValueError, "imported file differs"):
            IMPORT.verify_manifest(self.root)

    def test_parent_traversal_is_rejected(self):
        self.records[0]["destination"] = "../component.o"
        self.save()
        with self.assertRaisesRegex(ValueError, "invalid manifest destination"):
            IMPORT.verify_manifest(self.root)

    def test_absolute_path_is_rejected(self):
        self.records[0]["destination"] = "/tmp/component.o"
        self.save()
        with self.assertRaisesRegex(ValueError, "invalid manifest destination"):
            IMPORT.verify_manifest(self.root)

    def test_symlink_escape_is_rejected(self):
        external = self.root.parent / "external.o"
        external.write_bytes(b"original component")
        self.file.unlink()
        self.file.symlink_to(external)
        self.save()
        with self.assertRaisesRegex(ValueError, "leaves repository"):
            IMPORT.verify_manifest(self.root)


if __name__ == "__main__":
    unittest.main()
