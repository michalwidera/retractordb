#!/usr/bin/env python3
"""Porownanie rezimow dokladnosci ogona startowego: odniesienie wobec przebiegu.

Bramka nie jest symetryczna. Rozwoj silnika ma prawo poprawiac dokladnosc
rachunku ogona i taka zmiana NIE moze zatrzymywac pracy; utrata dokladnosci
musi ja zatrzymac.

Porzadek rezimow, od najgorszego do najlepszego:

    zanizajaca  <  zawyzajaca  <  dokladna

* ``zanizajaca`` — ogon krotszy niz wymaga model zdarzeniowy. Rekord wychodzi,
  zanim wszystkie jego zaleznosci sa okreslone. To defekt poprawnosci, nie
  utrata precyzji: BLAD zawsze, takze gdy odniesienie juz go mialo.
* ``zawyzajaca`` — nigdy nie zaniza, ale nie jest rowna. Bezpieczne, kosztuje
  slot opoznienia.
* ``dokladna`` — postac zamknieta rowna oracle'owi wszedzie.

Kody wyjscia: 0 — bez regresji (mozliwe poprawy); 1 — regresja; 2 — blad
odczytu albo rozjazd zestawu klas.
"""

import argparse
import re
import sys

RANK = {"zaniżająca": 0, "zawyżająca": 1, "dokładna": 2}
ROW = re.compile(r"^\|\s*`(?P<cls>[A-Z_]+)`\s*\|(?P<rest>.*)\|\s*$")
HEADING = re.compile(r"^##\s")

# Werdykt ma dwie tabele z ta sama kolumna rezimu: ogon startowy (sekcja 1)
# i poczatek logiczny (sekcja 1b). Bez zakotwiczenia w sekcji druga nadpisuje
# pierwsza — i bramka orzeka o czym innym, niz mysli.
SECTIONS = {
    "ogon startowy": "H10a — dokładność, per klasa operatora",
    "początek logiczny": "H10a — początek logiczny, per klasa operatora",
}


def die(message):
    """Blad aparatury: kod 2, jak w verdict.py kampanii. Kod 1 jest zarezerwowany
    dla waznego wyniku negatywnego i nie moze byc osiagalny przez awarie."""
    print(f"BLAD: {message}", file=sys.stderr)
    sys.exit(2)


def read_regimes(path, section):
    """Czyta rezim per klasa z jednej, wskazanej tabeli werdyktu.

    Czyta tabele, nie liste wypunktowana pod nia: lista jest streszczeniem,
    a tabela zrodlem. Brak sekcji albo brak wierszy jest BLEDEM, nie pustym
    wynikiem — pusta strona porownania zamienilaby bramke w tautologie.
    """
    out = {}
    inside = False
    seen_section = False
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if HEADING.match(line):
                inside = section in line
                seen_section = seen_section or inside
                continue
            if not inside:
                continue
            match = ROW.match(line)
            if not match:
                continue
            cells = [c.strip() for c in match.group("rest").split("|")]
            regime = next((c for c in cells if c in RANK), None)
            if regime is not None:
                out[match.group("cls")] = regime
    if not seen_section:
        die(f"brak sekcji {section!r} w {path}")
    if not out:
        die(f"sekcja {section!r} w {path} nie ma wierszy z rezimem")
    return out


def compare(ref, cur, label):
    print(f"  [{label}]")
    if set(ref) != set(cur):
        print(f"  BLAD: zestaw klas sie zmienil; brak={sorted(set(ref) - set(cur))} "
              f"nowe={sorted(set(cur) - set(ref))}")
        return 2

    regressions, improvements, unsafe = [], [], []
    for cls in sorted(ref):
        before, after = ref[cls], cur[cls]
        if after == "zaniżająca":
            unsafe.append(f"{cls}: {after}")
        elif RANK[after] < RANK[before]:
            regressions.append(f"{cls}: {before} -> {after}")
        elif RANK[after] > RANK[before]:
            improvements.append(f"{cls}: {before} -> {after}")

    exact = sorted(c for c, r in cur.items() if r == "dokładna")
    over = sorted(c for c, r in cur.items() if r == "zawyżająca")
    print(f"  dokladne  ({len(exact)}/{len(cur)}): {', '.join(exact) or 'brak'}")
    print(f"  zawyzajace ({len(over)}/{len(cur)}): {', '.join(over) or 'brak'}")

    for item in improvements:
        print(f"  POPRAWA   {item}")
    if improvements:
        print("  Poprawa nie zatrzymuje bramki. Odnotuj ja: artykul twierdzi")
        print(f"  {len([c for c in ref.values() if c == 'dokładna'])}/{len(ref)} klas dokladnych "
              "i to twierdzenie staje sie zachowawcze.")

    for item in unsafe:
        print(f"  DEFEKT    {item} — wartosc mniejsza niz wymaga model zdarzeniowy")
    for item in regressions:
        print(f"  REGRESJA  {item}")

    if unsafe or regressions:
        print("  Nie poprawiaj bramki. To zmiana semantyki, nie usterka testu.")
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", required=True, help="zamrozony werdykt odniesienia")
    parser.add_argument("--current", required=True, help="werdykt biezacego przebiegu")
    args = parser.parse_args()

    worst = 0
    for label, section in SECTIONS.items():
        ref = read_regimes(args.reference, section)
        cur = read_regimes(args.current, section)
        worst = max(worst, compare(ref, cur, label))
    return worst


if __name__ == "__main__":
    sys.exit(main())
