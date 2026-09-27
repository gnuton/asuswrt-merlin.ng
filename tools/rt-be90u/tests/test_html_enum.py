import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


script = Path(__file__).parents[1] / 'html-enum.py'
spec = importlib.util.spec_from_file_location('html_enum', script)
html = importlib.util.module_from_spec(spec)
spec.loader.exec_module(html)


class DictionaryConversion(unittest.TestCase):
    def test_translates_keys_and_preserves_unicode_whitespace_and_other_code(self):
        page = 'æ\r\n<#hello#> <% nvram_get("x"); %>\n'.encode()
        result, missing = html.convert(page, b'41=hello\n')
        self.assertEqual(result, 'æ\r\n<#41#> <% nvram_get("x"); %>\n'.encode())
        self.assertEqual(missing, [])

    def test_handles_long_minified_lines(self):
        page = b'x' * 200000 + b'<#hello#>' + b'y' * 200000
        self.assertEqual(html.convert(page, b'1=hello\n')[0],
                         b'x' * 200000 + b'<#1#>' + b'y' * 200000)

    def test_reports_missing_keys_without_reprocessing_inserted_numbers(self):
        result, missing = html.convert(b'<#hello#> <#1#>', b'1=hello\n')
        self.assertEqual(result, b'<#1#> <#*** not_found_dict : 1***#>')
        self.assertEqual(missing, [b'1'])

    def test_rejects_malformed_or_conflicting_enumerations(self):
        for enumeration in (b'bad', b'abc=hello', b'1=', b'1=hello\n2=hello'):
            with self.subTest(enumeration=enumeration), self.assertRaises(ValueError):
                html.convert(b'<#hello#>', enumeration)

    def test_cli_updates_only_the_page_and_preserves_permissions(self):
        with tempfile.TemporaryDirectory() as tmp:
            page = Path(tmp) / 'page.asp'
            enumeration = Path(tmp) / 'enum.txt'
            log = Path(tmp) / 'missing.txt'
            page.write_bytes(b'<#hello#>')
            page.chmod(0o640)
            enumeration.write_bytes(b'0=hello\n')
            subprocess.run([sys.executable, str(script), str(page), str(enumeration), str(log)], check=True)
            self.assertEqual(page.read_bytes(), b'<#0#>')
            self.assertEqual(page.stat().st_mode & 0o777, 0o640)
            self.assertEqual(enumeration.read_bytes(), b'0=hello\n')
            self.assertEqual(log.read_bytes(), b'')


if __name__ == '__main__':
    unittest.main()
