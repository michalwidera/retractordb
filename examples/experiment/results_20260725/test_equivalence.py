#!/usr/bin/env python3
"""Macierz poprawności: oracle kontra cztery realizacje, plus kontrole mutacyjne.

Kolejność jest celowa. Najpierw sprawdzamy, że testy w ogóle wykrywają błąd
(mutacje kontrolne), a dopiero potem raportujemy zgodność. Test, który nie
zawodzi po wstrzyknięciu błędu, nie jest kontrolą.

Uruchomienie:
    python3 test_equivalence.py [--quick]
"""

import argparse
import json
import random
import sys
from fractions import Fraction
from math import gcd

from models import MODELS, csdf_explicit, representation_cost
from reference import (
    SOURCE_A,
    SOURCE_B,
    interleave_trace,
    labels,
    minimal_period,
    ratio_terms,
)

SPECIAL_CASES = [
    (1, 1),  # równe interwały
    (1, 2),  # przypadek z testów silnika (delta 0.1 / 0.2)
    (2, 1),
    (3, 2),  # przykład z dokumentacji (Epsilon # Alfa)
    (6, 4),  # nieskrócony zapis 3/2 — musi dać ten sam ciąg
    (3, 1),
    (1, 3),
    (1, 16),  # skrajnie skośny
    (16, 1),
    (7, 11),  # względnie pierwsze
    (160, 147),  # proporcja 44.1/48 kHz, P = 307
    (1000, 999),
]


def positions_for(period, factor, floor_count, cap):
    return min(max(floor_count, factor * period), cap)


def check_invariants(trace):
    """Pokrycie, rozłączność i porządek indeksów źródłowych.

    Każde źródło musi być konsumowane od zera, bez luk i bez powtórzeń — to
    jest własność round-trip: de-interleave odtworzy oryginalne ciągi.
    """
    expected = {SOURCE_A: 0, SOURCE_B: 0}
    for source, index in trace:
        if index != expected[source]:
            return False
        expected[source] += 1
    return True


def compare_case(delta_a, delta_b, count, models):
    """Zwraca (ok, opis_rozbieżności)."""
    expected = interleave_trace(delta_a, delta_b, count)
    for name in models:
        actual = MODELS[name](delta_a, delta_b, count)
        if actual != expected:
            for n, (want, got) in enumerate(zip(expected, actual)):
                if want != got:
                    return False, f"{name}: pozycja {n}, oracle {want}, model {got}"
            return False, f"{name}: różna długość śladu"
    if not check_invariants(expected):
        return False, "naruszony porządek indeksów źródłowych"
    return True, ""


def check_period(delta_a, delta_b):
    """Lemat o okresie: minimalny okres słowa wyboru wynosi P = (a+b)/g,
    a w jednym okresie występuje b/g tokenów z A i a/g z B."""
    a, b, g, period = ratio_terms(delta_a, delta_b)
    word = labels(interleave_trace(delta_a, delta_b, 4 * period))
    observed = minimal_period(word)
    if observed != period:
        return False, f"okres: oczekiwano {period}, zaobserwowano {observed}"
    first = word[:period]
    if first.count(SOURCE_A) != b // g or first.count(SOURCE_B) != a // g:
        return False, (
            f"liczności w okresie: A={first.count(SOURCE_A)} (oczekiwano {b // g}), "
            f"B={first.count(SOURCE_B)} (oczekiwano {a // g})"
        )
    return True, ""


# --------------------------------------------------------------------------
# Kontrole mutacyjne
# --------------------------------------------------------------------------


def mutant_beatty_off_by_one(delta_a, delta_b, count):
    """Decyzja przesunięta o jeden slot."""
    z = Fraction(delta_b) / (Fraction(delta_a) + Fraction(delta_b))
    trace = []
    for n in range(count):
        f1 = ((n + 1) * z).numerator // ((n + 1) * z).denominator
        f2 = ((n + 2) * z).numerator // ((n + 2) * z).denominator
        if f1 == f2:
            trace.append((SOURCE_B, n - f2))
        else:
            trace.append((SOURCE_A, f1))
    return trace


def mutant_beatty_a_index(delta_a, delta_b, count):
    """Poprawne słowo wyboru, błędny indeks źródłowy po stronie A.

    W gałęzi wyboru A zachodzi f1 = f0 + 1, więc użycie f1 zamiast f0 jest
    klasycznym off-by-one w indeksie, niewidocznym w samym słowie wyboru.
    Kontrola, czy macierz patrzy na indeksy, a nie tylko na etykiety źródeł.

    Uwaga metodyczna: pierwotnie mutowano indeks po stronie B
    (`n - f0` zamiast `n - f1`). Okazało się to tożsamością, bo w gałęzi B
    warunek rozgałęzienia brzmi dokładnie f0 == f1. Mutacja, która niczego
    nie zmienia, nie jest kontrolą — stąd przeniesienie mutacji na stronę A.
    """
    z = Fraction(delta_b) / (Fraction(delta_a) + Fraction(delta_b))
    trace = []
    for n in range(count):
        f0 = (n * z).numerator // (n * z).denominator
        f1 = ((n + 1) * z).numerator // ((n + 1) * z).denominator
        if f0 == f1:
            trace.append((SOURCE_B, n - f1))
        else:
            trace.append((SOURCE_A, f1))
    return trace


def mutant_oracle_tie_to_b(delta_a, delta_b, count):
    """Odwrócona reguła remisu w samej definicji odniesienia."""
    return interleave_trace(delta_a, delta_b, count, tie_to_a=False)


def mutant_csdf_period_minus_one(delta_a, delta_b, count):
    """Tablica faz poprawna, moduł o jeden za mały."""
    _, _, _, period = ratio_terms(delta_a, delta_b)
    word = labels(interleave_trace(delta_a, delta_b, period))
    modulus = max(1, period - 1)
    next_index = {SOURCE_A: 0, SOURCE_B: 0}
    trace = []
    for n in range(count):
        source = word[n % modulus]
        trace.append((source, next_index[source]))
        next_index[source] += 1
    return trace


def mutant_unreduced_ratio(delta_a, delta_b, count):
    """Okres liczony bez skrócenia stosunku: P' = a+b zamiast (a+b)/g.

    Ta mutacja NIE powinna zmienić śladu — słowo długości a+b jest słowem
    długości P powtórzonym g razy. Służy jako kontrola negatywna: pokazuje,
    że macierz testów nie zgłasza fałszywych alarmów.
    """
    ratio = Fraction(delta_a) / Fraction(delta_b)
    period = ratio.numerator + ratio.denominator
    word = labels(interleave_trace(delta_a, delta_b, period))
    next_index = {SOURCE_A: 0, SOURCE_B: 0}
    trace = []
    for n in range(count):
        source = word[n % period]
        trace.append((source, next_index[source]))
        next_index[source] += 1
    return trace


MUTANTS = {
    "beatty_off_by_one": (mutant_beatty_off_by_one, True),
    "beatty_a_index": (mutant_beatty_a_index, True),
    "oracle_tie_to_b": (mutant_oracle_tie_to_b, True),
    "csdf_period_minus_one": (mutant_csdf_period_minus_one, True),
    "unreduced_ratio_is_benign": (mutant_unreduced_ratio, False),
}


def run_mutations():
    """Dla każdej mutacji: na ilu przypadkach kontrolnych została wykryta."""
    results = {}
    for name, (mutant, should_be_detected) in MUTANTS.items():
        detected = []
        for a, b in SPECIAL_CASES:
            delta_a, delta_b = Fraction(a), Fraction(b)
            _, _, _, period = ratio_terms(delta_a, delta_b)
            count = positions_for(period, 4, 60, 1500)
            expected = interleave_trace(delta_a, delta_b, count)
            if mutant(delta_a, delta_b, count) != expected:
                detected.append(f"{a}/{b}")
        results[name] = {
            "should_be_detected": should_be_detected,
            "detected_on": len(detected),
            "cases": len(SPECIAL_CASES),
            "first_case": detected[0] if detected else None,
            "verdict": (
                "OK"
                if (bool(detected) == should_be_detected)
                else "PROBLEM: kontrola mutacyjna nie zadziałała"
            ),
        }
    return results


# --------------------------------------------------------------------------
# Kampanie
# --------------------------------------------------------------------------


def run_campaign(pairs, factor, floor_count, cap, models, label, verbose=False):
    checked = 0
    positions = 0
    failures = []
    period_failures = []
    for a, b in pairs:
        delta_a, delta_b = Fraction(a), Fraction(b)
        _, _, _, period = ratio_terms(delta_a, delta_b)
        count = positions_for(period, factor, floor_count, cap)

        ok, why = compare_case(delta_a, delta_b, count, models)
        if not ok:
            failures.append({"case": f"{a}/{b}", "detail": why})

        if period <= cap // 2:
            ok_period, why_period = check_period(delta_a, delta_b)
            if not ok_period:
                period_failures.append({"case": f"{a}/{b}", "detail": why_period})

        checked += 1
        positions += count
        if verbose and checked % 500 == 0:
            print(f"  {label}: {checked}/{len(pairs)}", file=sys.stderr)

    return {
        "label": label,
        "models": models,
        "cases": checked,
        "compared_positions": positions,
        "mismatches": failures,
        "period_mismatches": period_failures,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--quick", action="store_true", help="mniejsza macierz")
    parser.add_argument("--seed", type=int, default=20260725)
    parser.add_argument("--json", default=None, help="zapisz podsumowanie do pliku")
    args = parser.parse_args()

    random.seed(args.seed)
    all_models = list(MODELS)
    campaigns = []

    print("== kontrole mutacyjne ==")
    mutations = run_mutations()
    for name, info in mutations.items():
        print(
            f"  {name:26s} wykryta na {info['detected_on']:2d}/{info['cases']} "
            f"przypadkach — {info['verdict']}"
        )
    blocking = [n for n, i in mutations.items() if i["verdict"] != "OK"]
    if blocking:
        print(f"PRZERWANO: kontrole mutacyjne zawiodły: {blocking}")
        return 1

    exhaustive_limit = 24 if args.quick else 64
    print(f"\n== wyczerpująco 1 <= a,b <= {exhaustive_limit} ==")
    pairs = [(a, b) for a in range(1, exhaustive_limit + 1) for b in range(1, exhaustive_limit + 1)]
    campaigns.append(
        run_campaign(pairs, 4, 60, 1024, all_models, f"exhaustive<={exhaustive_limit}", True)
    )

    medium_count = 300 if args.quick else 3000
    print(f"\n== losowo {medium_count} par, a,b <= 256 ==")
    pairs = [(random.randint(1, 256), random.randint(1, 256)) for _ in range(medium_count)]
    campaigns.append(run_campaign(pairs, 3, 60, 1500, all_models, "random<=256", True))

    large_count = 100 if args.quick else 800
    print(f"\n== losowo {large_count} par, a,b <= 10^6 (oracle vs beatty_online) ==")
    pairs = [(random.randint(1, 10**6), random.randint(1, 10**6)) for _ in range(large_count)]
    campaigns.append(run_campaign(pairs, 2, 2000, 2000, ["beatty_online"], "random<=1e6", True))

    print("\n== przypadki obowiązkowe ==")
    campaigns.append(run_campaign(SPECIAL_CASES, 6, 120, 3000, all_models, "special", False))

    print("\n== podsumowanie ==")
    total_cases = sum(c["cases"] for c in campaigns)
    total_positions = sum(c["compared_positions"] for c in campaigns)
    total_mismatches = sum(len(c["mismatches"]) for c in campaigns)
    total_period = sum(len(c["period_mismatches"]) for c in campaigns)
    for c in campaigns:
        print(
            f"  {c['label']:20s} {c['cases']:6d} przypadków, "
            f"{c['compared_positions']:9d} pozycji, "
            f"{len(c['mismatches'])} rozbieżności, "
            f"{len(c['period_mismatches'])} błędów okresu"
        )
        for bad in c["mismatches"][:5] + c["period_mismatches"][:5]:
            print(f"      {bad}")

    # Nieskrócony zapis musi dać dokładnie ten sam ślad co skrócony.
    same = interleave_trace(6, 4, 200) == interleave_trace(3, 2, 200)
    print(f"  nieskrócony 6/4 == 3/2: {same}")

    cost = representation_cost(160, 147)
    print(f"  koszt reprezentacji dla 160/147 (P={cost['P']}):")
    for name in all_models:
        c = cost[name]
        print(
            f"      {name:16s} phases={c['phases']:5d} static_words={c['static_words']:5d} "
            f"startup_tokens={c['startup_tokens']:5d} input_highwater={c['input_highwater']:5d}"
        )

    summary = {
        "seed": args.seed,
        "quick": args.quick,
        "mutations": mutations,
        "campaigns": campaigns,
        "totals": {
            "cases": total_cases,
            "positions": total_positions,
            "mismatches": total_mismatches,
            "period_mismatches": total_period,
        },
        "unreduced_ratio_equal": same,
    }
    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(summary, handle, indent=2, ensure_ascii=False)

    verdict = total_mismatches == 0 and total_period == 0
    print(f"\nWYNIK: {'zero rozbieżności' if verdict else 'ROZBIEŻNOŚCI OBECNE'}")
    return 0 if verdict else 1


if __name__ == "__main__":
    sys.exit(main())
