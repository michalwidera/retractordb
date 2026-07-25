#!/usr/bin/env python3
"""Most oracle <-> silnik: porównanie artefaktu RetractorDB z ciągiem oracle'a.

Bez tego kroku macierz z `test_equivalence.py` weryfikowałaby wyłącznie modele
Pythona względem siebie. Tutaj porównywany jest binarny artefakt wyprodukowany
przez `xretractor` z ciągiem (source-id, source-index) wyznaczonym niezależnie.

Kodowanie źródeł w danych wejściowych:
    A[k] = k + 1                (wartości 1, 2, 3, ...)
    B[j] = B_BASE + j           (wartości 1000000, 1000001, ...)
dzięki czemu każdy rekord wyniku jednoznacznie identyfikuje źródło i indeks.

Uruchomienie:
    python3 engine_check.py [--xretractor ŚCIEŻKA] [--json PLIK]
"""

import argparse
import json
import os
import shutil
import struct
import subprocess
import sys
from fractions import Fraction

from reference import SOURCE_A, SOURCE_B, interleave_trace, output_interval, ratio_terms

B_BASE = 1000000

# (nazwa, delta_a, delta_b, limit iteracji, liczba rekordów źródłowych)
CASES = [
    ("p3_engine_test", Fraction(1, 10), Fraction(1, 5), 60, 200),
    ("p2_equal", Fraction(1, 4), Fraction(1, 4), 60, 200),
    ("p8_coprime", Fraction(1, 5), Fraction(1, 3), 120, 200),
    ("p9_skewed", Fraction(1, 2), Fraction(1, 7), 200, 300),
    ("p17_coprime", Fraction(1, 10), Fraction(1, 7), 300, 400),
    ("p3_unreduced", Fraction(2, 20), Fraction(2, 10), 60, 200),
    ("p103_skewed", Fraction(3, 100), Fraction(1, 1), 400, 500),
    ("p307_audio", Fraction(160, 1000), Fraction(147, 1000), 2800, 600),
]

QUERY_TEMPLATE = """STORAGE '.'
SUBSTRAT 'memory'

DECLARE value INTEGER STREAM A, {delta_a} FILE 'a.txt'
DECLARE value INTEGER STREAM B, {delta_b} FILE 'b.txt'

SELECT * STREAM hashed FROM A#B
"""


def rql_interval(value):
    return f"{value.numerator}/{value.denominator}"


def decode_artifact(path):
    """Artefakt INTEGER: 4 bajty na rekord, little-endian."""
    with open(path, "rb") as handle:
        raw = handle.read()
    count = len(raw) // 4
    return list(struct.unpack(f"<{count}i", raw[: count * 4]))


def values_to_trace(values):
    trace = []
    for value in values:
        if value >= B_BASE:
            trace.append((SOURCE_B, value - B_BASE))
        elif value >= 1:
            trace.append((SOURCE_A, value - 1))
        else:
            trace.append(("ZERO", value))
    return trace


def run_case(binary, workroot, name, delta_a, delta_b, limit, records):
    workdir = os.path.join(workroot, name)
    shutil.rmtree(workdir, ignore_errors=True)
    os.makedirs(workdir)

    with open(os.path.join(workdir, "a.txt"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(str(k + 1) for k in range(records)) + "\n")
    with open(os.path.join(workdir, "b.txt"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(str(B_BASE + j) for j in range(records)) + "\n")
    with open(os.path.join(workdir, "query.rql"), "w", encoding="utf-8") as handle:
        handle.write(
            QUERY_TEMPLATE.format(
                delta_a=rql_interval(delta_a), delta_b=rql_interval(delta_b)
            )
        )

    compile_out = subprocess.run(
        [binary, "query.rql", "-c"],
        cwd=workdir,
        capture_output=True,
        text=True,
        check=True,
    ).stdout

    subprocess.run(
        [binary, "query.rql", "-r", "-k", "-m", str(limit)],
        cwd=workdir,
        capture_output=True,
        text=True,
        check=True,
    )

    artifact = os.path.join(workdir, "hashed")
    if not os.path.exists(artifact):
        return {"case": name, "status": "BRAK ARTEFAKTU"}

    actual = values_to_trace(decode_artifact(artifact))
    expected = interleave_trace(delta_a, delta_b, len(actual))
    a, b, g, period = ratio_terms(delta_a, delta_b)
    delta_c = output_interval(delta_a, delta_b)

    # Źródło plikowe czytane poza koniec danych zaczyna indeksować od zera.
    # Porównujemy wyłącznie prefiks pokryty wygenerowanymi danymi, a brak
    # pokrycia raportujemy jawnie, zamiast mylić go z rozbieżnością semantyki.
    usable = len(expected)
    for n, (_, index) in enumerate(expected):
        if index >= records:
            usable = n
            break

    declared = f"hashed({delta_c.numerator}/{delta_c.denominator})"
    interval_ok = declared in compile_out

    mismatch = None
    for n, (want, got) in enumerate(zip(expected[:usable], actual[:usable])):
        if want != got:
            mismatch = f"pozycja {n}: oracle {want}, silnik {got}"
            break

    return {
        "case": name,
        "delta_a": str(delta_a),
        "delta_b": str(delta_b),
        "ratio": f"{a}/{b}",
        "P": period,
        "delta_c": str(delta_c),
        "records": len(actual),
        "compared": usable,
        "periods_covered": round(usable / period, 2),
        "interval_declared_by_compiler": interval_ok,
        "zero_prefix": sum(1 for source, _ in actual if source == "ZERO"),
        "status": "OK" if (mismatch is None and interval_ok) else "ROZBIEŻNOŚĆ",
        "detail": mismatch or ("" if interval_ok else f"brak {declared} w planie"),
    }


def main():
    parser = argparse.ArgumentParser()
    default_binary = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "..",
        "build",
        "Debug",
        "src",
        "retractor",
        "xretractor",
    )
    parser.add_argument("--xretractor", default=os.path.normpath(default_binary))
    parser.add_argument("--workdir", default="work")
    parser.add_argument("--json", default=None)
    args = parser.parse_args()

    binary = os.path.abspath(args.xretractor) if os.path.exists(args.xretractor) else None
    if binary is None:
        found = shutil.which("xretractor")
        binary = os.path.abspath(found) if found else None
    if binary is None:
        print("nie znaleziono xretractor — podaj --xretractor")
        return 2

    print(f"xretractor: {binary}\n")
    results = []
    for name, delta_a, delta_b, limit, records in CASES:
        result = run_case(binary, args.workdir, name, delta_a, delta_b, limit, records)
        results.append(result)
        print(
            f"  {result['case']:16s} {result.get('ratio', '?'):>10s} "
            f"P={result.get('P', '?'):<5} porównano={result.get('compared', 0):<6} "
            f"okresów={result.get('periods_covered', 0):<7} {result['status']} "
            f"{result.get('detail', '')}"
        )

    failed = [r for r in results if r["status"] != "OK"]
    print(f"\nWYNIK: {len(results) - len(failed)}/{len(results)} przypadków zgodnych")

    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump({"binary": binary, "cases": results}, handle, indent=2, ensure_ascii=False)

    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
