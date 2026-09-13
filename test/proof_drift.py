#!/usr/bin/env python3
"""Dryft silnika wobec dowodow Lean z math_proofs.

Wartosci sprawdza ut_proofOracle: tablice policzone definicjami z Profs (proofOracle.hpp) i
wypowiedzi twierdzen przepisane na wlasnosci funkcji z SOperations.hpp. Ten skrypt pilnuje
tego, czego sam test nie widzi z konstrukcji:

- twierdzenia dopisanego do Profs bez wiersza w proof_manifest.tsv (nowy dowod, ktorego nikt
  nie zestawil z silnikiem), albo wiersza po twierdzeniu, ktorego juz nie ma;
- wiersza TEST: wskazujacego test, ktorego nie ma w test_proofOracle.cpp;
- przeterminowanej wyroczni: skroty plikow Profs/*.lean inne niz te, z ktorych policzono
  proofOracle.hpp. Zielony ut_proofOracle porownuje wtedy silnik ze stara wersja dowodow.

Werdykt: ZGODNY (kod 0) albo DRYFT (kod 1). Pokrycie to udzial twierdzen TEST wsrod
twierdzen dotyczacych silnika (TEST + NIEPOKRYTE); lematy POZA_ZAKRESEM sie nie licza.

Uzycie:
    proof_drift.py <katalog profs> <proof_manifest.tsv> <test_proofOracle.cpp> <proofOracle.hpp>
"""

import hashlib
import pathlib
import re
import sys

THEOREM = re.compile(r"^\s*(?:private\s+)?(?:theorem|lemma)\s+([A-Za-z_][A-Za-z0-9_'.]*)", re.MULTILINE)
GTEST = re.compile(r"\bTEST\(\s*(\w+)\s*,\s*(\w+)\s*\)")
SOURCE = re.compile(r'\{\s*"(Profs/[^"]+\.lean)"\s*,\s*"([0-9a-f]{64})"\s*\}')
STATUSES = ("TEST", "NIEPOKRYTE", "POZA_ZAKRESEM")


def lean_theorems(profs: pathlib.Path) -> set[tuple[str, str]]:
    found = set()
    for path in sorted((profs / "Profs").glob("*.lean")):
        for name in THEOREM.findall(path.read_text(encoding="utf-8")):
            found.add((path.stem, name))
    return found


def read_manifest(path: pathlib.Path, problems: list[str]) -> dict[tuple[str, str], tuple[str, str]]:
    rows = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip() or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) != 3 or ":" not in fields[2]:
            problems.append(f"{path.name}:{number}: wiersz nie ma postaci plik<TAB>twierdzenie<TAB>STATUS:opis")
            continue
        status, detail = fields[2].split(":", 1)
        if status not in STATUSES:
            problems.append(f"{path.name}:{number}: nieznany status '{status}'")
            continue
        key = (fields[0], fields[1])
        if key in rows:
            problems.append(f"{path.name}:{number}: powtorzony wiersz {key[0]}.{key[1]}")
        rows[key] = (status, detail)
    return rows


def main() -> int:
    if len(sys.argv) != 5:
        print(__doc__)
        return 2
    profs, manifest_path, test_path, oracle_path = map(pathlib.Path, sys.argv[1:])

    problems: list[str] = []
    theorems = lean_theorems(profs)
    manifest = read_manifest(manifest_path, problems)
    tests = {f"{suite}.{name}" for suite, name in GTEST.findall(test_path.read_text(encoding="utf-8"))}

    for file, name in sorted(theorems - manifest.keys()):
        problems.append(f"{file}.{name}: twierdzenie bez wiersza w {manifest_path.name}")
    for file, name in sorted(manifest.keys() - theorems):
        problems.append(f"{file}.{name}: wiersz w {manifest_path.name}, a twierdzenia nie ma w Profs")
    for (file, name), (status, detail) in sorted(manifest.items()):
        if status == "TEST" and detail not in tests:
            problems.append(f"{file}.{name}: brak testu {detail} w {test_path.name}")

    recorded = dict(SOURCE.findall(oracle_path.read_text(encoding="utf-8")))
    current = {
        f"Profs/{path.name}": hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted((profs / "Profs").glob("*.lean"))
    }
    stale = sorted(f for f in recorded.keys() | current.keys() if recorded.get(f) != current.get(f))
    for file in stale:
        problems.append(f"{file}: inny niz przy generowaniu {oracle_path.name} - uruchom math_proofs/gen-oracle.sh")

    engine = [key for key in manifest if key in theorems and manifest[key][0] != "POZA_ZAKRESEM"]
    covered = [key for key in engine if manifest[key][0] == "TEST"]
    uncovered = sorted(key for key in engine if manifest[key][0] == "NIEPOKRYTE")

    print(f"Twierdzenia w Profs: {len(theorems)}, dotyczace silnika: {len(engine)}, z testem: {len(covered)}")
    if engine:
        print(f"Pokrycie: {100.0 * len(covered) / len(engine):.1f}%")
    for file, name in uncovered:
        print(f"  NIEPOKRYTE {file}.{name}: {manifest[(file, name)][1]}")

    if problems:
        for problem in problems:
            print(f"DRYFT {problem}")
        print("Werdykt: DRYFT")
        return 1
    print("Werdykt: ZGODNY")
    return 0


if __name__ == "__main__":
    sys.exit(main())
