#!/usr/bin/env python3
"""Bramka odwzorowania — podpróba wykonywana end-to-end, w dwóch skalach.

Cel: rozstrzygnąć, czy rozbieżność ogona jest rozbieżnością ogona, czy różnicą
w definicji operatora. Silnik jest uruchamiany, a treść rekordów porównywana
z modelem treści oracle'a.

Bramka skali (kryterium zamrozone w kampanii K24, §9): ogon i odwzorowanie zależą wyłącznie od
ilorazów interwałów, więc ten sam plan uruchomiony w dwóch skalach musi dać tę
samą treść. Różnica między skalami oznacza, że silnik nie nadążył — przebieg
jest wtedy dyskwalifikowany jako aparatura, a nie raportowany jako znalezisko.
"""

import argparse
import csv
import json
import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "oracle"))

import engine as E  # noqa: E402
import execute as X  # noqa: E402
import plan as P  # noqa: E402
from generator import STRATA, generate  # noqa: E402

RECORDS = 12
SCALES = (Fraction(1, 200), Fraction(1, 100))   # docelowy interwał najszybszego strumienia
BUDGET = Fraction(8)                            # sekundy na pojedynczy przebieg


def select(corpus, per_stratum):
    chosen = []
    counts = {name: 0 for name in STRATA}
    for index, (stratum, item) in enumerate(corpus):
        if counts[stratum] >= per_stratum:
            continue
        counts[stratum] += 1
        chosen.append((index, stratum, item))
    return chosen


def run_one(index, stratum, item, binary, workroot):
    outcomes = []
    for scale in SCALES:
        scaled = P.rescale(item, scale / P.fastest(item))
        spread = P.slowest(scaled) / P.fastest(scaled)
        loops = int((RECORDS + 8) * spread) + 24
        if loops * P.fastest(scaled) > BUDGET:
            return {"plan": index, "stratum": stratum, "status": "poza budżetem",
                    "detail": f"rozpiętość {spread}, pętli {loops}"}
        workdir = Path(workroot) / f"p{index}_s{scale.denominator}"
        try:
            X.run_plan(scaled, binary, workdir, loops=loops, records=1024)
        except Exception as exc:  # noqa: BLE001
            return {"plan": index, "stratum": stratum, "status": "awaria",
                    "detail": str(exc)[:200]}
        outcomes.append((scaled, workdir, X.compare_content(scaled, workdir, limit=RECORDS)))

    (_, _, first), (_, _, second) = outcomes
    if [item_["node"] for item_ in first] != [item_["node"] for item_ in second]:
        return {"plan": index, "stratum": stratum, "status": "niestabilne w skali",
                "detail": f"{len(first)} vs {len(second)} rozbieżności"}
    if not first:
        return {"plan": index, "stratum": stratum, "status": "zgodne", "detail": ""}
    return {"plan": index, "stratum": stratum, "status": "rozbieżność treści",
            "detail": json.dumps(first[:3], ensure_ascii=False, default=str),
            "kinds": ",".join(sorted({row["kind"] for row in first}))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=20260807)
    parser.add_argument("--count", type=int, default=10_010)
    parser.add_argument("--per-stratum", type=int, default=8)
    parser.add_argument("--xretractor", default=None)
    parser.add_argument("--out", default=str(ROOT / "raw" / "mapping_gate.csv"))
    args = parser.parse_args()

    binary = E.resolve_binary(args.xretractor)
    workroot = ROOT / "work" / "mapping"
    workroot.mkdir(parents=True, exist_ok=True)
    chosen = select(generate(args.seed, args.count), args.per_stratum)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    with out.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=("plan", "stratum", "status", "kinds", "detail"))
        writer.writeheader()
        for index, stratum, item in chosen:
            row = run_one(index, stratum, item, binary, workroot)
            row.setdefault("kinds", "")
            writer.writerow(row)
            handle.flush()
            rows.append(row)
            print(f"{index:6d} {stratum:20s} {row['status']}")

    summary = {}
    for row in rows:
        summary[row["status"]] = summary.get(row["status"], 0) + 1
    print("\npodsumowanie:", summary)


if __name__ == "__main__":
    main()
