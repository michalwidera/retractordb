#!/usr/bin/env python3
"""Zestawienie „przed/po" dla bramki odwzorowania — krok 2.9 planu naprawy.

Porównuje dwa przebiegi `run_mapping_gate.py` plan po planie i wypisuje, co
faktycznie zmieniła naprawa. Każda zmiana statusu jest wypisana z nazwą planu,
żeby dało się ją sprawdzić pojedynczo.
"""

import argparse
import collections
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def load(path):
    with Path(path).open(encoding="utf-8") as handle:
        return {row["plan"]: row for row in csv.DictReader(handle)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--before", default=str(ROOT / "raw" / "mapping_gate.csv"))
    parser.add_argument("--after", default=str(ROOT / "raw" / "mapping_gate_after_P1.csv"))
    args = parser.parse_args()

    before = load(args.before)
    after = load(args.after)

    counts_before = collections.Counter(row["status"] for row in before.values())
    counts_after = collections.Counter(row["status"] for row in after.values())
    statuses = sorted(set(counts_before) | set(counts_after))

    print(f"{'status':24s} {'przed':>7s} {'po':>7s} {'zmiana':>8s}")
    for status in statuses:
        b, a = counts_before.get(status, 0), counts_after.get(status, 0)
        print(f"{status:24s} {b:7d} {a:7d} {a - b:+8d}")

    changed = [(plan, before[plan]["status"], after[plan]["status"])
               for plan in sorted(before, key=int)
               if plan in after and before[plan]["status"] != after[plan]["status"]]
    print(f"\nplanów o zmienionym statusie: {len(changed)}")
    for plan, was, now in changed:
        mark = "poprawa" if now == "zgodne" else ("REGRESJA" if was == "zgodne" else "zmiana")
        print(f"  plan {plan:>5s}: {was} -> {now}   [{mark}]")

    regressions = [item for item in changed if item[1] == "zgodne" and item[2] != "zgodne"]
    print("\nREGRESJE: " + (f"{len(regressions)} — patrz wyżej" if regressions else "brak"))


if __name__ == "__main__":
    main()
