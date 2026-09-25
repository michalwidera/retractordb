#!/usr/bin/env python3
"""Sprawdza, czy biblioteki wspoldzielone spelniaja polityke manylinux obrazu.

auditwheel ocenia kola, nie pojedyncze pliki, wiec skrypt pakuje podane pliki .so
do tymczasowego kola i pyta `auditwheel show --json`, jaki tag platformy mu
przysluguje. Wzorzec bierze z AUDITWHEEL_PLAT, ktore ustawia sam obraz manylinux
(np. manylinux_2_28_x86_64). Wynik jest dobry, gdy przyznany tag wymaga glibc nie
nowszej niz wzorzec - tak samo oceni kolo `auditwheel repair` w fazie J4.

Uzycie (wewnatrz obrazu docker/wheel):
    python3 audit-so.py <plik.so> [<plik.so> ...]
"""

from __future__ import annotations

import base64
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

# manylinux_2_28_x86_64 -> (2, 28). Tagi sprzed PEP 600 (manylinux2014 itd.)
# auditwheel 6 zglasza juz pod nazwami manylinux_X_Y, wiec innych nie obslugujemy.
_TAG = re.compile(r"^manylinux_(\d+)_(\d+)_")


def _glibc(tag: str) -> tuple[int, int] | None:
    match = _TAG.match(tag)
    return (int(match.group(1)), int(match.group(2))) if match else None


def _record_line(name: str, data: bytes) -> str:
    digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode()
    return f"{name},sha256={digest},{len(data)}"


def _pack(libraries: list[Path], wheel: Path) -> None:
    dist_info = "rdbaudit-0.dist-info"
    entries = {f"rdbaudit/{lib.name}": lib.read_bytes() for lib in libraries}
    entries[f"{dist_info}/METADATA"] = b"Metadata-Version: 2.1\nName: rdbaudit\nVersion: 0\n"
    entries[f"{dist_info}/WHEEL"] = (
        f"Wheel-Version: 1.0\nGenerator: audit-so\nRoot-Is-Purelib: false\nTag: py3-none-linux_{platform.machine()}\n"
    ).encode()
    record = [_record_line(name, data) for name, data in entries.items()]
    record.append(f"{dist_info}/RECORD,,")
    entries[f"{dist_info}/RECORD"] = ("\n".join(record) + "\n").encode()
    with zipfile.ZipFile(wheel, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in entries.items():
            archive.writestr(name, data)


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    expected = os.environ.get("AUDITWHEEL_PLAT", "")
    ceiling = _glibc(expected)
    if ceiling is None:
        print(
            f"audit-so: AUDITWHEEL_PLAT='{expected}' is not a manylinux_X_Y tag - run inside a manylinux image",
            file=sys.stderr,
        )
        return 2
    libraries = [Path(arg) for arg in argv[1:]]
    for lib in libraries:
        if not lib.is_file():
            print(f"audit-so: no such file: {lib}", file=sys.stderr)
            return 2

    with tempfile.TemporaryDirectory() as tmp:
        wheel = Path(tmp) / f"rdbaudit-0-py3-none-linux_{platform.machine()}.whl"
        _pack(libraries, wheel)
        shown = subprocess.run(
            ["auditwheel", "show", "--json", str(wheel)], capture_output=True, text=True, check=False
        )
    if shown.returncode != 0:
        print(shown.stdout + shown.stderr, file=sys.stderr)
        print("audit-so: auditwheel show failed", file=sys.stderr)
        return 1

    report = json.loads(shown.stdout)
    granted = report.get("overall_tag", "")
    names = ", ".join(lib.name for lib in libraries)
    glibc = _glibc(granted)
    if glibc is not None and glibc <= ceiling and not report.get("external_libs"):
        print(f"audit-so: {names}: {granted} (image policy {expected}) - OK")
        return 0

    print(f"audit-so: {names}: auditwheel grants '{granted}', image policy is {expected}", file=sys.stderr)
    if report.get("external_libs"):
        print(f"  external libraries outside the policy: {report['external_libs']}", file=sys.stderr)
    for library, versions in report.get("versioned_symbols", {}).items():
        print(f"  {library}: {', '.join(versions)}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
