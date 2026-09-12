#!/usr/bin/env python3
"""Dryft dokumentacji: lista funkcji skalarnych w dokumentacji wobec rqlFunctions.hpp.

Jedyna lista nazw i arnosci funkcji skalarnych RQL stoi w `src/include/rqlFunctions.hpp` —
czyta ja i kompilator (`checkFunctionCalls`), i ewaluator. Dokumentacja uzytkownika powtarza
te liste wlasnym tekstem, w dwoch repozytoriach (PL i EN), i wlasnie dlatego cicho sie
rozjezdza: funkcja dopisana do tabeli w naglowku dziala, ale nikt o niej nie wie, a funkcja
usunieta z naglowka zostaje w dokumentacji jako obietnica bez pokrycia.

Bramka `-c` tego nie zlapie — dokumentacja nie jest kompilowana. Nie zlapie tego takze
`test_drift`, ktory konfrontuje SILNIK z modelem zdarzeniowym i z portem we Flinku:
teksty sa dla niego niewidoczne z konstrukcji.

Porownanie jest na pisowni KANONICZNEJ, nie na nazwie zlozonej do malych liter. Parser
zapisuje do planu wlasnie postac kanoniczna, dokumentacja obiecuje to czytelnikowi wprost,
wiec `sqrt` w tabeli zamiast `Sqrt` jest dryftem, a nie drobiazgiem typograficznym.

Uzycie:
    doc_drift_scalar_functions.py <rqlFunctions.hpp> <etykieta>=<plik.md> [...]
"""

import pathlib
import re
import sys

# `.canonical = "Sqrt"` — pole wyliczane wprost w kRqlFunctions, jeden wpis na linie.
CANONICAL = re.compile(r'\.canonical\s*=\s*"([^"]+)"')

# Naglowki tabeli funkcji w obu jezykach. Tabela jest rozpoznawana po naglowku, a nie po
# pozycji w pliku, zeby przestawienie rozdzialow nie unieruchamialo tej kontroli.
TABLE_HEADERS = (("grupa", "funkcje"), ("group", "functions"))

BACKTICKED = re.compile(r"`([^`]+)`")


def canonical_names(header: pathlib.Path) -> list[str]:
    names = CANONICAL.findall(header.read_text(encoding="utf-8"))
    if not names:
        sys.exit(f"{header}: nie znaleziono ani jednego `.canonical` — zmienil sie ksztalt tabeli?")
    return names


def cells(row: str) -> list[str]:
    return [cell.strip() for cell in row.strip().strip("|").split("|")]


def documented_names(doc: pathlib.Path) -> list[str] | None:
    """Nazwy z tabeli funkcji skalarnych albo None, gdy takiej tabeli w pliku nie ma."""
    lines = doc.read_text(encoding="utf-8").splitlines()
    for index, line in enumerate(lines):
        if not line.lstrip().startswith("|"):
            continue
        head = [cell.lower() for cell in cells(line)]
        if len(head) != 2 or tuple(head) not in TABLE_HEADERS:
            continue

        names: list[str] = []
        for row in lines[index + 2 :]:  # +2 pomija wiersz separatora `|---|---|`
            if not row.lstrip().startswith("|"):
                break
            names.extend(BACKTICKED.findall(cells(row)[-1]))
        return names
    return None


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)

    header = pathlib.Path(sys.argv[1])
    expected = canonical_names(header)
    duplicates = {name for name in expected if expected.count(name) > 1}
    problems: list[str] = []

    if duplicates:
        problems.append(f"{header}: nazwa powtorzona w kRqlFunctions: {', '.join(sorted(duplicates))}")

    for argument in sys.argv[2:]:
        label, _, path = argument.partition("=")
        doc = pathlib.Path(path)
        found = documented_names(doc)

        if found is None:
            problems.append(f"{label}: w {doc} nie ma tabeli funkcji skalarnych — rozdzial zniknal albo zmienil naglowek")
            continue

        missing = [name for name in expected if name not in found]
        extra = [name for name in found if name not in expected]
        if missing:
            problems.append(f"{label}: funkcje silnika nieopisane w dokumentacji: {', '.join(missing)}")
        if extra:
            problems.append(f"{label}: dokumentacja obiecuje funkcje, ktorych silnik nie zna: {', '.join(extra)}")

    if problems:
        print("DRYFT DOKUMENTACJI", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        print(
            f"\nLista obowiazujaca ({len(expected)}): {', '.join(expected)}",
            file=sys.stderr,
        )
        return 1

    print(f"ZGODNY: {len(expected)} funkcji, {len(sys.argv) - 2} dokumentacji")
    return 0


if __name__ == "__main__":
    sys.exit(main())
