#!/usr/bin/env python3
"""Replace ASUS web translation keys using dictenum.txt, without line limits."""

import os
from pathlib import Path
import re
import sys
import tempfile


def convert(page, enumeration):
    entries = {}
    for line in enumeration.splitlines():
        if not line:
            continue
        number, separator, key = line.partition(b'=')
        if not separator or not number.isdigit() or not key:
            raise ValueError('Invalid dictionary enumeration')
        if key in entries and entries[key] != number:
            raise ValueError('Conflicting dictionary key')
        entries[key] = number

    missing = []

    def replace(match):
        key = match.group(1)
        if key in entries:
            return b'<#' + entries[key] + b'#>'
        missing.append(key)
        return b'<#*** not_found_dict : ' + key + b'***#>'

    return re.sub(rb'<#([^<>\r\n]*?)#>', replace, page), missing


def main():
    if len(sys.argv) != 4:
        raise SystemExit('Usage: html-enum.py PAGE ENUMERATION MISSING_LOG')
    page, enumeration, log = map(Path, sys.argv[1:])
    result, missing = convert(page.read_bytes(), enumeration.read_bytes())
    with log.open('ab') as output:
        for key in missing:
            output.write(b'dict no found : ' + os.fsencode(page) + b' , ' + key + b'\n')
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=page.parent, delete=False) as output:
            temporary = Path(output.name)
            output.write(result)
        temporary.chmod(page.stat().st_mode & 0o777)
        os.replace(temporary, page)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()


if __name__ == '__main__':
    main()
