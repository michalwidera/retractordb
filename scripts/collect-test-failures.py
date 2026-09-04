#!/usr/bin/env python3
"""Zbiera dowody z testow, ktore oblaly, do jednego katalogu do pobrania z CI.

Powod istnienia. 2026-09-04 `it_agse_array` oblal na CI i jedynym sladem byl wpis
w test_results.xml z pustym <system-out/>. Z czasu trwania dalo sie wyliczyc, ze test
wykonal caly lancuch i przegral na porownaniu bajtowym — ale nie bylo wiadomo NA CZYM,
a lokalnie awaria nie wystapila ani razu w 31 pelnych przebiegach. Diagnostyka po fakcie
jest wiec warunkiem, zeby kolejne takie zdarzenie w ogole dalo sie zbadac.

Dla kazdego testu z LastTestsFailed.log zbierane sa:
  * zarejestrowane polecenie i katalog roboczy,
  * fragment LastTest.log dotyczacy tego testu (pelne wyjscie, bo CI biegnie z -V),
  * male pliki tekstowe z katalogu roboczego (wyniki, logi, wyjscia posrednie),
  * gotowa roznica kazdej pary wzorzec/wynik, ktora da sie sparowac.

Mapowanie nazwy testu na katalog roboczy pochodzi z `ctest --show-only=json-v1`
(wlasnosc WORKING_DIRECTORY), a nie ze zgadywania z nazwy: nazwa testu i nazwa katalogu
rozjezdzaja sie (it_simple-vg-run, it_issue31_doc-q-31.a.rql-graph).

Skrypt jest DIAGNOSTYCZNY i konczy sie zerem takze wtedy, gdy nie ma czego zebrac —
nie jego rola jest decydowac o wyniku joba.
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

# Limity sa po to, zeby artefakt dalo sie pobrac. Wzorce i wyniki testow integracyjnych
# mieszcza sie w kilku kilobajtach; wszystko wieksze jest logiem, ktorego i tak czyta sie
# od poczatku.
kMaxFileBytes = 256 * 1024
kMaxFilesPerTest = 40
kCollectSuffixes = (".txt", ".dot", ".log", ".out", ".err", ".pattern", ".script", ".rql")


def ctestRoot(buildDir: Path) -> Path | None:
    """Katalog, z ktorego uruchomiono ctest — tam lezy Testing/Temporary.

    CI wola `cd build/<typ>/test && ctest`, a udokumentowany przebieg lokalny
    `cd build/Debug && ctest`. Oba uklady sa poprawne i oba maja wlasny
    CTestTestfile.cmake, wiec zamiast zakladac jeden z nich wybieramy ten, ktory ma
    swiezszy zapis LastTestsFailed.log.
    """
    candidates = [d for d in (buildDir / "test", buildDir)
                  if (d / "Testing" / "Temporary" / "LastTestsFailed.log").is_file()]
    if not candidates:
        return None
    return max(candidates,
               key=lambda d: (d / "Testing" / "Temporary" / "LastTestsFailed.log").stat().st_mtime)


def failedTests(testDir: Path) -> list[str]:
    listing = testDir / "Testing" / "Temporary" / "LastTestsFailed.log"
    if not listing.is_file():
        return []
    names = []
    for line in listing.read_text(encoding="utf-8", errors="replace").splitlines():
        # Format: "<numer>:<nazwa>"
        _, _, name = line.partition(":")
        if name:
            names.append(name.strip())
    return names


def testCatalogue(testDir: Path) -> dict[str, dict]:
    try:
        raw = subprocess.run(
            ["ctest", "--test-dir", str(testDir), "--show-only=json-v1"],
            capture_output=True, text=True, check=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"collect-test-failures: nie udalo sie odczytac listy testow: {e}", file=sys.stderr)
        return {}
    catalogue = {}
    for test in json.loads(raw).get("tests", []):
        props = {p["name"]: p.get("value") for p in test.get("properties", [])}
        catalogue[test["name"]] = {
            "workingDirectory": props.get("WORKING_DIRECTORY"),
            "command": test.get("command", []),
        }
    return catalogue


def logSection(text: str, name: str) -> str:
    """Fragment LastTest.log dotyczacy jednego testu.

    Dziala na TRESCI wczytanej wczesniej, nie na sciezce, i to nie jest ozdoba:
    `ctest --show-only`, ktorym budujemy katalog testow, NADPISUJE LastTest.log pustym
    naglowkiem. Kolektor zniszczylby wiec dowod, po ktory przyszedl.
    """
    if not text:
        return ""
    # CTest otwiera kazdy rekord linia "<i>/<n> Testing: <nazwa>".
    starts = [(m.start(), m.group(1)) for m in re.finditer(r"^\d+/\d+ Testing: (.+)$", text, re.M)]
    for index, (offset, entry) in enumerate(starts):
        if entry.strip() != name:
            continue
        end = starts[index + 1][0] if index + 1 < len(starts) else len(text)
        return text[offset:end]
    return ""


def failedInThisRun(lastTestLog: str, name: str) -> bool:
    """Czy ten test oblal w przebiegu opisanym przez podany LastTest.log.

    Odsiewa wpisy odziedziczone po wczesniejszym, czerwonym przebiegu: LastTestsFailed.log
    zostaje nietkniety po zielonym biegu, a sekcja w LastTest.log konczy sie wtedy
    "Test Passed.". Bez tego sprawdzenia kolektor produkowalby raport dla testu, ktory
    wlasnie przeszedl.
    """
    section = logSection(lastTestLog, name)
    if not section:
        # Brak sekcji znaczy, ze tego testu w tym przebiegu nie bylo (np. filtr -R).
        # Zbieramy go mimo to: lepszy raport nadmiarowy niz zaden.
        return True
    return "Test Passed." not in section


def patternPairs(workDir: Path) -> list[tuple[Path, Path]]:
    """Pary wzorzec/wynik, ktore da sie sparowac po nazwie.

    Konwencja drzewa jest regularna: pattern.txt -> out.txt, pattern-run.txt -> out-run.txt
    albo out.txt, pattern-dot.txt -> out.dot, count.pattern -> count.txt. Parujemy po
    przyrostku nazwy, a gdy to nie wychodzi — kazdy wzorzec z kazdym wynikiem byloby
    myleniem, wiec zostaje sam pattern.txt/out.txt.
    """
    pairs = []
    for pattern in sorted(workDir.glob("pattern*")) + sorted(workDir.glob("*.pattern")):
        stem = pattern.name
        candidates = []
        if stem.startswith("pattern"):
            suffix = stem[len("pattern"):].removesuffix(".txt")  # "", "-run", "-dot", "_compile"
            candidates = [f"out{suffix}.txt", f"out{suffix}.dot", "out.txt", "out.dot"]
        else:  # <cos>.pattern
            candidates = [stem.removesuffix(".pattern") + ".txt"]
        for candidate in candidates:
            actual = workDir / candidate
            if actual.is_file():
                pairs.append((pattern, actual))
                break
    return pairs


def copyCapped(source: Path, target: Path) -> None:
    size = source.stat().st_size
    if size <= kMaxFileBytes:
        shutil.copy2(source, target)
        return
    with source.open("rb") as handle:
        head = handle.read(kMaxFileBytes)
    target.write_bytes(head)
    with target.open("ab") as handle:
        handle.write(f"\n--- (obciete: plik ma {size} bajtow) ---\n".encode())


def collectOne(name: str, info: dict, lastTestLog: str, reportDir: Path) -> list[str]:
    lines = [f"# {name}"]
    target = reportDir / name.replace("/", "_")
    target.mkdir(parents=True, exist_ok=True)

    command = " ".join(info.get("command") or [])
    workRaw = info.get("workingDirectory")
    (target / "command.txt").write_text(
        f"katalog roboczy: {workRaw}\npolecenie: {command}\n", encoding="utf-8")

    section = logSection(lastTestLog, name)
    if section:
        (target / "ctest-output.log").write_text(section, encoding="utf-8")
        lines.append("  wyjscie z LastTest.log: zebrane")
    else:
        lines.append("  wyjscie z LastTest.log: BRAK (nie znaleziono sekcji tego testu)")

    if not workRaw:
        lines.append("  katalog roboczy: nieznany, plikow nie zebrano")
        return lines
    workDir = Path(workRaw)
    if not workDir.is_dir():
        lines.append(f"  katalog roboczy {workDir} nie istnieje")
        return lines

    collected = 0
    for item in sorted(workDir.iterdir()):
        if collected >= kMaxFilesPerTest:
            break
        if not item.is_file() or item.suffix not in kCollectSuffixes:
            continue
        copyCapped(item, target / item.name)
        collected += 1
    lines.append(f"  plikow z katalogu roboczego: {collected}")

    for pattern, actual in patternPairs(workDir):
        diff = subprocess.run(
            ["diff", "--strip-trailing-cr", "-u", str(pattern), str(actual)],
            capture_output=True, text=True).stdout
        name_ = f"diff-{pattern.name}-vs-{actual.name}.txt"
        (target / name_).write_text(diff or "(brak roznic tekstowych)\n", encoding="utf-8")
        lines.append(f"  roznica {pattern.name} vs {actual.name}: "
                     f"{'zapisana' if diff else 'brak roznic tekstowych'}")
    return lines


def discardStaleReport(reportDir: Path) -> None:
    """Kasuje raport z WCZESNIEJSZEGO przebiegu, gdy biezacy nie ma czego zglosic.

    Raport ma zawsze opisywac ten przebieg, ktory wlasnie sie odbyl. Zostawiony po
    zielonym biegu byl by gorszy niz jego brak: ktos pobralby z CI dowody z awarii,
    ktorej w tym przebiegu nie bylo. Kasujemy tylko katalog, ktory sami zbudowalismy —
    rozpoznajemy go po SUMMARY.txt, zeby nie ruszyc cudzej sciezki podanej z linii polecen.
    """
    if (reportDir / "SUMMARY.txt").is_file():
        shutil.rmtree(reportDir)
        print(f"collect-test-failures: skasowano nieaktualny raport {reportDir}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Zbiera dowody z testow, ktore oblaly.")
    parser.add_argument("buildDir", type=Path, help="katalog build, np. build/Debug")
    parser.add_argument("reportDir", type=Path, nargs="?", default=None,
                        help="katalog raportu (domyslnie <buildDir>/test-failure-report)")
    # Kod wyjscia ctest jest JEDYNYM pewnym sygnalem, czy biezacy przebieg cos oblal.
    # CTest NIE kasuje LastTestsFailed.log po udanym przebiegu — plik z czerwonego biegu
    # zostaje i kolejny, zielony, wygladalby przez niego na czerwony. Sprawdzone wprost.
    parser.add_argument("--ctest-status", type=int, default=None,
                        help="kod wyjscia ctest z tego przebiegu; 0 = nie ma czego zbierac")
    args = parser.parse_args()

    buildDir = args.buildDir
    reportDir = args.reportDir or buildDir / "test-failure-report"

    if args.ctest_status == 0:
        print("collect-test-failures: ctest zakonczyl sie zerem — raport niepotrzebny")
        discardStaleReport(reportDir)
        return 0

    testDir = ctestRoot(buildDir)
    if testDir is None:
        print(f"collect-test-failures: nie znaleziono Testing/Temporary pod {buildDir} "
              f"— czy ctest w ogole biegl?")
        discardStaleReport(reportDir)
        return 0

    # Wczytanie PRZED wolaniem ctest, ktore ten plik nadpisze (patrz logSection).
    lastTestLogFile = testDir / "Testing" / "Temporary" / "LastTest.log"
    lastTestLog = (lastTestLogFile.read_text(encoding="utf-8", errors="replace")
                   if lastTestLogFile.is_file() else "")

    names = [name for name in failedTests(testDir) if failedInThisRun(lastTestLog, name)]
    if not names:
        print("collect-test-failures: brak testow, ktore oblaly w tym przebiegu — "
              "raport niepotrzebny")
        discardStaleReport(reportDir)
        return 0

    if reportDir.exists():
        shutil.rmtree(reportDir)
    reportDir.mkdir(parents=True)
    # Kopia calego logu ladzie w raporcie, bo oryginal na dysku zaraz przestanie
    # istniec w tej postaci — i tak czy tak nie byloby czego pobrac z CI.
    if lastTestLog:
        (reportDir / "LastTest.log").write_text(lastTestLog, encoding="utf-8")

    catalogue = testCatalogue(testDir)
    summary = [f"Testy, ktore oblaly: {len(names)}", ""]
    for name in names:
        summary.extend(collectOne(name, catalogue.get(name, {}), lastTestLog, reportDir))
        summary.append("")

    (reportDir / "SUMMARY.txt").write_text("\n".join(summary) + "\n", encoding="utf-8")
    print("\n".join(summary))
    print(f"collect-test-failures: raport w {reportDir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
