#!/usr/bin/env python3
"""Sledzenie wymagan: ID w komentarzach przy kodzie, pokrycie deklarowane w testach.

Wymaganie to punkt bloku "... powinna:" w src/ lub api/, zaczynajacy sie od ID z wersja:

    /// - [svc.write.empty-content~1] dla pustej tresci zostawiac pusty plik docelowy.

ID: czlony malymi literami, cyframi i '-' rozdzielone kropka, pierwszy zaczyna sie od litery,
co najmniej dwa czlony; wersja ~N rosnie, gdy zmienia sie sens wymagania (nie przy poprawce
redakcyjnej). Test pokrywajacy wymaganie deklaruje to komentarzem na poczatku linii, w C++
na poczatku ciala TEST, w Pythonie i bashu na poczatku funkcji testu albo pliku, w CMake
bezposrednio nad add_test testu integracyjnego:

    // covers: svc.write.empty-content~1, svc.write.tmp-rename~1
    # covers: xtrdb.command.usage~1

Blok trwa od linii konczacej sie "powinna:" (powinien, powinno, powinny, powinni) do pierwszej
linii, ktora nie jest komentarzem ///; strefa punktu siega do nastepnego punktu. Linia strefy
"///   Nietestowalne: ..." albo "///   Kontrola strukturalna: ..." zwalnia wymaganie z `covers`.

Bledy (kod wyjscia 1): punkt bloku bez ID, ID o zlej skladni, zduplikowane ID, linia `Test:`
w strefie punktu z ID (format sprzed migracji), `covers` z nieznanym ID, `covers` ze stara
wersja, wymaganie bez zadnego `covers` i bez zwolnienia. --report wypisuje macierz
wymaganie -> testy w Markdownie.

Uzycie: req-trace.py [<katalog repozytorium>] [--report]
"""

import argparse
import os
import re
import sys
from pathlib import Path

ID = r"[a-z][a-z0-9]*(?:\.[a-z0-9][a-z0-9-]*)+~[1-9][0-9]*"
REQUIREMENT = re.compile(r"^\s*/// - \[(" + ID + r")\] ")
# Nawias bez spacji z tylda w miejscu ID: zamierzone ID z bledem skladni, nie proza.
ID_LIKE = re.compile(r"^\s*/// - \[([^\]\s]*~[^\]\s]*)\]")
BLOCK_HEAD = re.compile(r"^\s*///.*\bpowin(?:na|no|ny|ni|ien):\s*$")
BULLET = re.compile(r"^\s*/// - ")
DOC = re.compile(r"^\s*///")
EXEMPT = re.compile(r"^\s*///\s+(?:Nietestowalne|Kontrola strukturalna):")
OLD_TEST = re.compile(r"^\s*///\s+Test:")
COVERS = re.compile(r"^\s*(?://|#)\s*covers:\s*(.*?)\s*$")
TEST_CPP = re.compile(r"^\s*TEST(?:_F|_P)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)")
TEST_PY = re.compile(r"^\s*def\s+(test_\w+)\s*\(")

CODE_SUFFIXES = {".hpp", ".h", ".cpp"}
TEST_SUFFIXES = {".hpp", ".h", ".cpp", ".py", ".sh", ".cmake"}
SKIPPED_DIRS = {"build", ".antlr", "__pycache__", "node_modules"}


def walk(root, top, accept):
    for directory, subdirs, files in os.walk(root / top):
        subdirs[:] = sorted(d for d in subdirs if d not in SKIPPED_DIRS and not d.startswith(".venv"))
        for name in sorted(files):
            path = Path(directory) / name
            if accept(path):
                yield path


def split_id(versioned):
    name, version = versioned.rsplit("~", 1)
    return name, int(version)


def requirement_bullets(lines):
    """Punkty blokow "... powinna:" jako (indeks linii punktu, linie jego strefy)."""
    bullets, in_block, zone = [], False, None
    for index, line in enumerate(lines):
        if BLOCK_HEAD.match(line) or not DOC.match(line):
            in_block, zone = bool(BLOCK_HEAD.match(line)), None
        elif in_block and BULLET.match(line):
            zone = []
            bullets.append((index, zone))
        elif zone is not None:
            zone.append(line)
    return bullets


def collect(root):
    errors = []
    requirements = {}  # nazwa -> (wersja, miejsce, zwolnione z covers)
    for top in ("src", "api"):
        for path in walk(root, top, lambda p: p.suffix in CODE_SUFFIXES):
            lines = path.read_text(errors="replace").splitlines()
            for index, zone in requirement_bullets(lines):
                where = f"{path.relative_to(root)}:{index + 1}"
                match = REQUIREMENT.match(lines[index])
                if not match:
                    malformed = ID_LIKE.match(lines[index])
                    errors.append(f"{where}: ID o zlej skladni: {malformed[1]}" if malformed
                                  else f"{where}: punkt wymagania bez ID")
                    continue
                if any(OLD_TEST.match(line) for line in zone):
                    errors.append(f"{where}: linia Test: przy punkcie z ID - pokrycie deklaruje test przez covers")
                name, version = split_id(match[1])
                if name in requirements:
                    errors.append(f"{where}: zduplikowane ID {name} (pierwsze w {requirements[name][1]})")
                    continue
                requirements[name] = (version, where, any(EXEMPT.match(line) for line in zone))

    coverage = {name: [] for name in requirements}
    is_test_file = lambda p: p.suffix in TEST_SUFFIXES or p.name == "CMakeLists.txt"
    for path in [*walk(root, "test", is_test_file), *walk(root, "api", is_test_file)]:
        owner = None
        for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            test = TEST_CPP.match(line) or TEST_PY.match(line)
            if test:
                owner = ".".join(test.groups())
                continue
            if line.startswith("}"):  # koniec ciala TEST: dalsze znaczniki sa na poziomie pliku
                owner = None
            match = COVERS.match(line)
            if not match:
                continue
            where = f"{path.relative_to(root)}:{number}"
            evidence = f"{path.relative_to(root)}" + (f" {owner}" if owner else "")
            for token in (part.strip() for part in match[1].split(",")):
                if not re.fullmatch(ID, token):
                    errors.append(f"{where}: covers z ID o zlej skladni: {token!r}")
                    continue
                name, version = split_id(token)
                if name not in requirements:
                    errors.append(f"{where}: covers wskazuje nieznane wymaganie {token}")
                elif version != requirements[name][0]:
                    errors.append(f"{where}: covers wskazuje {token}, a wymaganie ma wersje "
                                  f"~{requirements[name][0]} ({requirements[name][1]})")
                elif evidence not in coverage[name]:
                    coverage[name].append(evidence)

    for name, (version, where, exempt) in sorted(requirements.items()):
        if not coverage[name] and not exempt:
            errors.append(f"{where}: wymaganie {name}~{version} nie ma zadnego covers")
    return requirements, coverage, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("root", nargs="?", default=Path(__file__).resolve().parent.parent, type=Path)
    parser.add_argument("--report", action="store_true", help="macierz wymaganie -> testy w Markdownie")
    args = parser.parse_args()

    requirements, coverage, errors = collect(args.root.resolve())
    if args.report:
        print("| Wymaganie | Miejsce | Testy |\n|---|---|---|")
        for name, (version, where, exempt) in sorted(requirements.items()):
            tests = "<br>".join(coverage[name]) or ("nietestowalne" if exempt else "**brak**")
            print(f"| `{name}~{version}` | {where} | {tests} |")
    for error in errors:
        print(f"req-trace: {error}", file=sys.stderr)
    covered = sum(1 for tests in coverage.values() if tests)
    exempt = sum(1 for name, (_, _, free) in requirements.items() if free and not coverage[name])
    print(f"req-trace: {len(requirements)} wymagan z ID, {covered} pokrytych, {exempt} nietestowalnych, "
          f"{len(errors)} bledow", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
