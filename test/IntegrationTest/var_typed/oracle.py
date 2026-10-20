#!/usr/bin/env python3
"""Wyrocznia niezalezna dla jawnego typu zmiennej VAR (#546, etap H).

Liczy plan typed.rql sekwencyjnie od startu przebiegu, z modelu jezyka, nie z kodu silnika, i porownuje
z wypisem `xtrdb` zebranym przez run.sh (wartosci i typy z `.desc`). Plan jest tu przepisany recznie.

Model wykonania (kontrakt, nie implementacja):

* rekord n strumienia `stan` liczy sie z wiersza n pliku data.txt i czyta zmienne sprzed slotu (D1);
* zapisy SET sa widoczne od nastepnego slotu, a z kilku zapisow jednej zmiennej wygrywa regula
  pozniejsza w tekscie planu (D3);
* typy z adnotacji: licznik UINT (liczba calkowita 0..2^32-1), suma DOUBLE (float Pythona), prog
  RATIONAL (Fraction, licznik i mianownik w int32); literal 0.1 w SET to dokladnie 1/10 (O3);
* porownanie DOUBLE z RATIONAL liczy sie w DOUBLE.

Poza wartosciami wyrocznia pilnuje, ze test cos sprawdza: kazda regula SET odpalila, konflikt D3 na
liczniku zaszedl, a akumulacja we FLOAT dalaby inny wydruk niz w DOUBLE.

    python3 oracle.py <wypis run.sh>
"""
import re
import struct
import sys
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
INT32 = range(-2**31, 2**31)


def f32(x):
    return struct.unpack("f", struct.pack("f", x))[0]


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

    licznik, suma, prog = 0, 0.0, Fraction(1, 4)
    suma32 = f32(0.0)
    expected = []
    fired = set()
    conflicts = 0
    precision_visible = False
    for x_text, k_text in rows:
        x, k = float(x_text), int(k_text)
        expected.append("{ " + " ".join(f"stan_{i}:{text(v)}" for i, v in
                                       enumerate([x, k, licznik, suma, suma - 16777216, prog])) + " }")
        precision_visible |= text(suma - 16777216) != text(float(f32(suma32 - 16777216)))

        # Reguly w kolejnosci tekstu; slownik zapisow realizuje D3.
        writes = {}
        if x > float(prog):
            fired.add("count")
            writes["licznik"] = licznik + 1
        fired.add("acc")
        writes["suma"] = suma + x
        if k > 0:
            fired.add("up")
            writes["prog"] = prog * 2
        if k < 0:
            fired |= {"zero", "back"}
            conflicts += "licznik" in writes
            writes["licznik"] = 0
            writes["prog"] = Fraction(1, 10)

        licznik = writes.get("licznik", licznik)
        suma = writes["suma"]
        suma32 = f32(suma32 + f32(x))
        prog = writes.get("prog", prog)
        assert 0 <= licznik < 2**32, "licznik poza UINT - dane testu do poprawy"
        assert prog.numerator in INT32 and prog.denominator in INT32, "prog poza RATIONAL - dane do poprawy"

    errors = []
    actual = [line for line in listing if line.startswith("{ stan_")]
    if actual != expected:
        errors.append("wartosci strumienia stan rozne od wyroczni:")
        for n in range(max(len(actual), len(expected))):
            got = actual[n] if n < len(actual) else "<brak>"
            want = expected[n] if n < len(expected) else "<brak>"
            if got != want:
                errors.append(f"  rekord {n}: jest  {got}")
                errors.append(f"  rekord {n}: ma byc {want}")

    desc = re.findall(r"^\{?\t(\w+) (stan_\d+)$", "\n".join(listing), re.MULTILINE)
    types = ["DOUBLE", "INTEGER", "UINT", "DOUBLE", "DOUBLE", "RATIONAL"]
    if desc != [(t, f"stan_{i}") for i, t in enumerate(types)]:
        errors.append(f"typy pol stan: {desc}, oczekiwane {types}")

    # Czulosc testu: bez tych warunkow zgodnosc z wyrocznia niczego by nie dowodzila.
    missing = {"count", "acc", "up", "zero", "back"} - fired
    if missing:
        errors.append(f"dane testu nie wyzwalaja regul: {sorted(missing)}")
    if conflicts == 0:
        errors.append("dane testu nie daja konfliktu D3 na liczniku (count i zero w jednym slocie)")
    if not precision_visible:
        errors.append("dane testu nie odrozniaja akumulacji DOUBLE od FLOAT w wydruku")

    if errors:
        print("\n".join(errors))
        sys.exit(1)
    print(f"oracle: {len(expected)} rekordow zgodnych, konflikty D3 na liczniku: {conflicts}")


if __name__ == "__main__":
    main()
