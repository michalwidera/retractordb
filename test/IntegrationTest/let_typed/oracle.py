#!/usr/bin/env python3
"""Wyrocznia niezalezna dla jawnego typu stalej LET (#547).

Liczy plan typed.rql sekwencyjnie, z modelu jezyka, nie z kodu silnika, i porownuje z wypisem `xtrdb`
zebranym przez run.sh (wartosci i typy z `.desc`). Plan jest tu przepisany recznie.

Model wykonania (kontrakt, nie implementacja):

* stala z adnotacja ma w wyrazeniu typ z adnotacji (L4): ?K to double 0.1, ?Q to ulamek 1/4, ?N to
  UINT 3000000000; stala bez adnotacji ?C = 0.1 to FLOAT (#440 D1), promowany do DOUBLE przy polu DOUBLE;
* typ wyniku operatora to wiekszy z typow operandow: DOUBLE * FLOAT = DOUBLE, INTEGER * RATIONAL = RATIONAL;
* rekord n czyta zmienna ?trafienia sprzed slotu, a SET jest widoczny od nastepnego slotu (D1).

Poza wartosciami wyrocznia pilnuje, ze test cos sprawdza: blad float 0.1 widac w wydruku, ulamek nie
jest calkowity, a porownanie z ?N rozroznia wartosci tuz ponizej, rowna i powyzej progu.

    python3 oracle.py <wypis run.sh>
"""
import re
import struct
import sys
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
K = 0.1
C = struct.unpack("f", struct.pack("f", 0.1))[0]
Q = Fraction(1, 4)
N = 3000000000


def text(value):
    """Wydruk wartosci jak `xtrdb list`: ostream z domyslna precyzja (%g) i ulamek boost `n/d`."""
    if isinstance(value, Fraction):
        return f"{value.numerator}/{value.denominator}"
    if isinstance(value, float):
        return "%g" % value
    return str(value)


def main():
    listing = Path(sys.argv[1]).read_text().splitlines()
    rows = [line.split() for line in (HERE / "data.txt").read_text().splitlines() if line.strip()]

    trafienia = 0
    expected = []
    precision_visible = False
    above, at_or_below = 0, 0
    for x_text, k_text, u_text in rows:
        x, k, u = float(x_text), int(k_text), int(u_text)
        values = [x, x * K, x * C, x * K - x * C, k * Q, u, trafienia]
        expected.append("{ " + " ".join(f"wynik_{i}:{text(v)}" for i, v in enumerate(values)) + " }")
        precision_visible |= text(x * K - x * C) != "0"
        if u > N:
            above += 1
            trafienia += 1
        else:
            at_or_below += 1
        assert trafienia < 2**32, "trafienia poza UINT - dane testu do poprawy"

    errors = []
    actual = [line for line in listing if line.startswith("{ wynik_")]
    if actual != expected:
        errors.append("wartosci strumienia wynik rozne od wyroczni:")
        for n in range(max(len(actual), len(expected))):
            got = actual[n] if n < len(actual) else "<brak>"
            want = expected[n] if n < len(expected) else "<brak>"
            if got != want:
                errors.append(f"  rekord {n}: jest  {got}")
                errors.append(f"  rekord {n}: ma byc {want}")

    desc = re.findall(r"^\{?\t(\w+) (wynik_\d+)$", "\n".join(listing), re.MULTILINE)
    types = ["DOUBLE", "DOUBLE", "DOUBLE", "DOUBLE", "RATIONAL", "UINT", "UINT"]
    if desc != [(t, f"wynik_{i}") for i, t in enumerate(types)]:
        errors.append(f"typy pol wynik: {desc}, oczekiwane {types}")

    # Czulosc testu: bez tych warunkow zgodnosc z wyrocznia niczego by nie dowodzila.
    if not precision_visible:
        errors.append("dane testu nie pokazuja roznicy miedzy stala DOUBLE a FLOAT w wydruku")
    if all((int(k) * Q).denominator == 1 for _, k, _ in rows):
        errors.append("dane testu nie daja ulamka niecalkowitego ze stalej RATIONAL")
    if above == 0 or at_or_below == 0 or not any(int(u) == N for _, _, u in rows):
        errors.append("dane testu nie rozrozniaja wartosci ponizej, rownej i powyzej progu ?N")

    if errors:
        print("\n".join(errors))
        sys.exit(1)
    print(f"oracle: {len(expected)} rekordow zgodnych, trafienia ponad ?N: {above}")


if __name__ == "__main__":
    main()
