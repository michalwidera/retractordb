#!/usr/bin/env python3
"""K24d — kontrola członu (b) H10 na silniku po naprawach 3d i 3c.

Pochodzi z kampanii K24b. Populacja twierdzenia,
progi i predeklarowana postać rozjazdu są **bez zmian**; zmienił się wyłącznie
oracle (origin jako osobna wielkość) oraz reguła lokalna A, z której zniknął
wyjątek dla `>N` — patrz komentarz przy local_rule_a().

Oryginalny opis K24b:

Nie uruchamia silnika. Porównuje **naturalną regułę lokalną** (wariant A
z predeklaracji §2) z **prawdziwym ogonem** z modelu zdarzeniowego (oracle),
na całym korpusie i w populacji twierdzenia.

Kryteria (zamrożone w kampanii K24b, §4):
  1. próg gęstości  — rozjazd w >= 5% planów korpusu;
  2. postać         — deficyt == ceil((p+q-1)/p) w 100% populacji twierdzenia;
  3. dodatniość     — deficyt ostro dodatni w 100% populacji.
Kontrole negatywne — zero rozjazdu w planach bez `#` oraz w HC_SINGLE
ograniczonym do operatorów bez własnego ogona.
"""

import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "oracle"))

import model as M  # noqa: E402
from generator import generate  # noqa: E402
from plan import (AGSE, HASH, NTHETA, PASS, REDUCE, SHIFT, SOURCE, SUB,  # noqa: E402
                  THETA)

# Operatory pozbawione własnego ogona — jedyne, w których reguła lokalna
# z definicji nie może się rozjechać. `@` i `-` własny ogon mają, więc dosłowna
# kontrola HC_SINGLE z K24 pękała na nich (defekt kontroli, nie wynik).
PHASE_FREE = (PASS, SHIFT, REDUCE)


def _to_slots(width, delta_source, delta_target):
    if width <= 0:
        return 0
    value = (width * delta_source) / delta_target
    return -((-value.numerator) // value.denominator)


def local_rule_a(plan, oracle_tails):
    """Wariant A: własny ogon każdego operatora zero, ogon składowej przez takt.

    Ogony składowych brane z oracle'a — atrybucja izolowana, żeby rozjazd
    w węźle nie był dziedziczony po dziecku.

    K24p: wyjątek `tails = oracle_tails[skladowa] + N` dla `>N` USUNIĘTY.
    Po przestemplowaniu z 2026-08-06 `N` nie jest ogonem — przeszło do origin —
    więc reguła lokalna z członem `+N` przypisywałaby przesunięciu ogon, którego
    nie ma ani w silniku, ani w modelu zdarzeniowym. Skutkiem byłby sztucznie
    zawyżony próg gęstości (kryterium 1), liczony na korzyść H10b. Populacja
    twierdzenia (węzły `#` o obu składowych DEKLAROWANYCH) jest na tę zmianę
    niewrażliwa: składowe deklarowane mają ogon i origin zerowe.
    """
    tails = {}
    for node in plan.nodes:
        if node.kind == SOURCE:
            tails[node.name] = 0
            continue
        children = [plan.by_name(name) for name in node.children]
        tails[node.name] = max(_to_slots(oracle_tails[c.name], c.delta, node.delta) for c in children)
    return tails


def predeclared_form(left, right):
    """ceil((p+q-1)/p) dla nieskracalnego p/q = D_a / D_b."""
    ratio = left.delta / right.delta
    p, q = ratio.numerator, ratio.denominator
    return -((-(p + q - 1)) // p)


def analyse(seed, count):
    rows, plans_with_divergence, plans_total = [], 0, 0
    controls = {"plany bez `#`": [0, 0], "HC_SINGLE bez własnego ogona": [0, 0]}
    for index, (stratum, plan) in enumerate(generate(seed, count)):
        try:
            oracle = M.tails_by_name(plan, convention=M.C1)
        except M.OracleError:
            continue
        plans_total += 1
        local = local_rule_a(plan, oracle)
        operators = [n for n in plan.nodes if n.kind != SOURCE]
        if any(oracle[n.name] != local[n.name] for n in operators):
            plans_with_divergence += 1

        no_hash_phase_free = all(n.kind in PHASE_FREE for n in operators)
        for node in operators:
            if no_hash_phase_free:
                controls["plany bez `#`"][0] += 1
                controls["plany bez `#`"][1] += oracle[node.name] != local[node.name]
            if "HC_SINGLE" in stratum and node.kind in PHASE_FREE:
                controls["HC_SINGLE bez własnego ogona"][0] += 1
                controls["HC_SINGLE bez własnego ogona"][1] += oracle[node.name] != local[node.name]

            if node.kind != HASH:
                continue
            left, right = (plan.by_name(n) for n in node.children)
            if left.kind != SOURCE or right.kind != SOURCE:
                continue  # populacja twierdzenia: obie składowe deklarowane
            rows.append({
                "plan": index, "stratum": stratum, "node": node.name,
                "delta_a": str(left.delta), "delta_b": str(right.delta),
                "oracle": oracle[node.name], "local_a": local[node.name],
                "deficit": oracle[node.name] - local[node.name],
                "form": predeclared_form(left, right),
            })
    return rows, plans_total, plans_with_divergence, controls


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--count", type=int, default=10_010)
    parser.add_argument("--out", default=None)
    args = parser.parse_args()

    rows, plans_total, diverging, controls = analyse(args.seed, args.count)
    out = Path(args.out or ROOT / "raw" / f"member_b_seed{args.seed}.csv")
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    share = diverging / max(plans_total, 1)
    matching = sum(1 for r in rows if r["deficit"] == r["form"])
    positive = sum(1 for r in rows if r["deficit"] > 0)
    print(f"ziarno {args.seed}, planów {plans_total}, populacja twierdzenia {len(rows)} węzłów\n")
    print(f"1. próg gęstości  : rozjazd w {diverging} planach = {share:.1%} (próg >= 5%)"
          f"  -> {'SPEŁNIONE' if share >= 0.05 else 'NIESPEŁNIONE'}")
    print(f"2. postać         : deficyt == ceil((p+q-1)/p) w {matching}/{len(rows)}"
          f"  -> {'SPEŁNIONE' if matching == len(rows) else 'NIESPEŁNIONE'}")
    print(f"3. dodatniość     : deficyt > 0 w {positive}/{len(rows)}"
          f"  -> {'SPEŁNIONE' if positive == len(rows) else 'NIESPEŁNIONE'}")
    print("\nkontrole negatywne (wymagane zero):")
    for label, (total, breaches) in controls.items():
        print(f"  {label:32s} {breaches} / {total}  -> {'OK' if breaches == 0 else 'ZŁAMANA'}")

    verdict = share >= 0.05 and matching == len(rows) and positive == len(rows) \
        and all(b == 0 for _t, b in controls.values())
    print(f"\nH10b: {'WSPARTA' if verdict else 'BEZ WSPARCIA'}")


if __name__ == "__main__":
    main()
