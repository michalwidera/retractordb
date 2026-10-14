#!/usr/bin/env python3
"""Straznik trybu wyjatkow rdzenia (#511).

Rdzen silnika (rdb, retractorcore, rdbembed) nie rzuca i nie lapie wyjatkow, a pilnuje tego
biblioteka kontrolna <cel>_noexcept_check z -fno-exceptions (RDB_NOEXCEPT_CHECK, korzen
CMakeLists.txt). Wydawane programy kompiluja rdzen Z wyjatkami: jednostki z wyjatkami i bez nich
w jednym programie lamaly ODR - funkcje inline Boosta i spdloga maja w obu trybach inna tresc, a
linker zostawial jedna kopie zaleznie od kolejnosci obiektow.

Obie rzeczy psuja sie po cichu: build przechodzi, testy przechodza. Stad ten test, czytajacy baze
kompilacji (compile_commands.json):

  1. zadna jednostka spoza bibliotek kontrolnych nie ma -fno-exceptions;
  2. przy RDB_NOEXCEPT_CHECK=ON kazdy cel rdzenia ma biblioteke kontrolna, a ta kompiluje z
     -fno-exceptions kazde zrodlo celu poza wyspami. Wyspa (plik wolajacy kod sterowany
     wyjatkami, np. parser ANTLR) musi naprawde zawierac `try` - wylaczenie pliku bez niego
     byloby obejsciem kontroli, nie wyspa.

Samotest idzie pierwszy, na danych przerobionych w pamieci: straznik, ktory nie umie oblac,
niczego nie pilnuje.

Uzycie: exception_mode_guard.py <katalog buildu>
"""

from __future__ import annotations

import copy
import json
import re
import sys
from pathlib import Path

CORE_TARGETS = ("rdb", "retractorcore", "rdbembed")
CHECK_SUFFIX = "_noexcept_check"
NO_EXCEPTIONS = "-fno-exceptions"
TARGET_RE = re.compile(r"CMakeFiles/([^/]+)\.dir/")
TRY_RE = re.compile(r"\btry\s*\{")


def target_of(entry: dict) -> str | None:
    match = TARGET_RE.search(entry.get("output", ""))
    return match.group(1) if match else None


def is_precompiled_header(entry: dict) -> bool:
    return Path(entry["file"]).name.startswith("cmake_pch")


def check_enabled(build_dir: Path) -> bool:
    for line in (build_dir / "CMakeCache.txt").read_text().splitlines():
        if line.startswith("RDB_NOEXCEPT_CHECK:"):
            return line.split("=", 1)[1].strip().upper() in ("ON", "1", "TRUE", "YES")
    return False


def violations(entries: list[dict], enabled: bool, has_try) -> list[str]:
    found = []
    sources: dict[str, set[str]] = {}
    for entry in entries:
        target = target_of(entry)
        if target is None or is_precompiled_header(entry):
            continue
        sources.setdefault(target, set()).add(str(Path(entry["directory"], entry["file"]).resolve()))
        no_exceptions = NO_EXCEPTIONS in entry["command"].split()
        if target.endswith(CHECK_SUFFIX):
            if not no_exceptions:
                found.append(f"{target}: {entry['file']} kompiluje sie bez {NO_EXCEPTIONS}")
        elif no_exceptions:
            found.append(f"{target}: {entry['file']} ma {NO_EXCEPTIONS} w celu wydawanym (ODR, #511)")
    if not enabled:
        return found
    for target in CORE_TARGETS:
        check = target + CHECK_SUFFIX
        if target not in sources:
            found.append(f"{target}: brak celu rdzenia w bazie kompilacji")
            continue
        if check not in sources:
            found.append(f"{target}: brak biblioteki kontrolnej {check}")
            continue
        for source in sorted(sources[target] - sources[check]):
            if not has_try(source):
                found.append(f"{check}: pomija {source}, ktory nie ma `try` - to nie wyspa")
    return found


def file_has_try(source: str) -> bool:
    return bool(TRY_RE.search(Path(source).read_text(errors="replace")))


def selftest(entries: list[dict], enabled: bool) -> list[str]:
    """Kazde z trzech zepsuc musi dac co najmniej jedno naruszenie."""
    failures = []

    def expect_violation(name: str, broken: list[dict], has_try=file_has_try) -> None:
        if not violations(broken, enabled, has_try):
            failures.append(f"samotest '{name}': straznik nie zauwazyl zepsucia")

    release = next((e for e in entries if target_of(e) in CORE_TARGETS and not is_precompiled_header(e)), None)
    if release is None:
        return ["samotest: brak jednostek rdzenia w bazie kompilacji"]
    broken = copy.deepcopy(entries)
    for entry in broken:
        if entry["output"] == release["output"]:
            entry["command"] += f" {NO_EXCEPTIONS}"
    expect_violation("-fno-exceptions w celu wydawanym", broken)

    if enabled:
        checked = [e for e in entries if (target_of(e) or "").endswith(CHECK_SUFFIX)]
        if not checked:
            return failures + ["samotest: brak bibliotek kontrolnych przy RDB_NOEXCEPT_CHECK=ON"]
        dropped = checked[0]["output"]
        expect_violation("zrodlo bez `try` pominiete w kontroli", [e for e in entries if e["output"] != dropped])

        stripped = copy.deepcopy(entries)
        for entry in stripped:
            if entry["output"] == dropped:
                entry["command"] = entry["command"].replace(NO_EXCEPTIONS, "")
        expect_violation("biblioteka kontrolna bez -fno-exceptions", stripped)
    return failures


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    build_dir = Path(sys.argv[1])
    entries = json.loads((build_dir / "compile_commands.json").read_text())
    enabled = check_enabled(build_dir)

    problems = selftest(entries, enabled)
    if problems:
        print("\n".join(problems))
        return 1

    found = violations(entries, enabled, file_has_try)
    if found:
        print("\n".join(found))
        return 1
    if not enabled:
        print("RDB_NOEXCEPT_CHECK=OFF: kontrola rdzenia bez wyjatkow NIESPRAWDZONA, sprawdzono tylko cele wydawane")
    else:
        print("OK: cele wydawane bez -fno-exceptions, rdzen sprawdzony przez biblioteki kontrolne")
    return 0


if __name__ == "__main__":
    sys.exit(main())
