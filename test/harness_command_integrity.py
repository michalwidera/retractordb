#!/usr/bin/env python3
"""Straznik oprawy: kazdy zarejestrowany test ma wykonywac CALY swoj skrypt.

Powod istnienia. Makro `add_test` z IntegrationTest/CMakeLists.txt
przekazuje argumenty przez `_add_test(${ARGV})`. W makrze `${ARGV}` jest lista
sklejona srednikami, wiec ponowne rozwiniecie tnie kazdy argument po jego
WEWNETRZNYCH srednikach. Zapis

    add_test(NAME t COMMAND sh -c "set -e ; a ; b")

rejestrowal sie jako `sh "-c" "set -e " "a " "b "`. Powloka wykonywala samo
`set -e`, konczyla zerem i test byl ZAWSZE ZIELONY, nigdy nic nie sprawdzajac.
Makra CMake sa globalne, wiec dotyczylo to takze test/UnitTest. Tak zgnily
cztery testy naraz (it_consistency, it_issue167_triarg, ut_dataModel-compile,
ut_dataModel-compare) i nikt tego nie zauwazyl, bo objaw jest odwrotny do
awarii: zielony wynik w zerowym czasie.

Regula domowa, ktorej pilnuje ten straznik: po `-c` stoi DOKLADNIE JEDEN
argument. Skrypt z wieloma poleceniami idzie do wlasnego pliku `run.sh`
wolanego jako `COMMAND bash run.sh`, tak jak robi to wiekszosc katalogow serii.
"""

import re
import sys
from pathlib import Path

ARG = re.compile(r'"((?:[^"\\]|\\.)*)"')
NAME = re.compile(r"^add_test\(\s*(?:\[=\[(?P<bracket>.*?)\]=\]|(?P<bare>[^\s\"()]+))")
SHELLS = {"sh", "bash", "/bin/sh", "/bin/bash", "/usr/bin/sh", "/usr/bin/bash"}


def offenders(build_dir: Path):
    for testfile in sorted(build_dir.rglob("CTestTestfile.cmake")):
        for line in testfile.read_text(encoding="utf-8", errors="replace").splitlines():
            if not line.startswith("add_test("):
                continue
            matched = NAME.match(line)
            name = (matched.group("bracket") or matched.group("bare")) if matched else "?"
            args = ARG.findall(line)
            if len(args) < 2 or args[0] not in SHELLS or args[1] != "-c":
                continue
            rest = args[2:]
            if len(rest) > 1:
                yield name, testfile, rest


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: harness_command_integrity.py <CMAKE_BINARY_DIR>", file=sys.stderr)
        return 2

    found = list(offenders(Path(sys.argv[1])))
    if not found:
        print("OK: kazdy test z `-c` dostaje dokladnie jeden argument.")
        return 0

    for name, testfile, rest in found:
        print(f"ROZBITE POLECENIE: test '{name}' ({testfile})")
        print(f"  powloka wykona TYLKO: {rest[0]!r}")
        print(f"  a te {len(rest) - 1} czesci trafia w argumenty pozycyjne i przepadna:")
        for part in rest[1:]:
            print(f"    {part.strip()!r}")
        print("  Naprawa: przeniesc skrypt do run.sh i wolac `COMMAND bash run.sh`.")
    print(f"\n{len(found)} test(y) nie wykonuja calego swojego skryptu.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
