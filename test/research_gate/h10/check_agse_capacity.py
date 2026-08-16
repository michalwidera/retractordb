#!/usr/bin/env python3
"""Celowana kontrola niedomiaru pojemności `@` przewidzianego przez capacity.py.

`capacity.py` przewiduje dla klasy `@` niedomiar dokładnie jednego rekordu
w ~69% par (konsument `@`, składowa deklarowana). Przewidywanie bierze się
z jednego założenia modelu żądania: że czoło deklaracji wyprzedza rachunek
czasowy o `DECLARATION_PREFETCH = 2` rekordy w chwili odczytu. Silnik zakłada
w tym miejscu wyprzedzenie o jeden (`required = dystans + kDeclarationPrefetch`,
gdzie sam dystans jest już liczony od rekordu najnowszego).

Rozstrzygnąć może to wyłącznie wykonanie: niedomiar pojemności historii objawia
się rekordem all-NULL albo przerwaniem w `storage::revRead`, nigdy cicho.
Skrypt bierze WYŁĄCZNIE plany, dla których model przewiduje niedomiar, i puszcza
je end-to-end. Zero objawów na próbie tej wielkości oznacza, że przewidywanie
pochodzi z założenia modelu, a nie z silnika — i tak ma być raportowane.

    python3 check_agse_capacity.py --seed 20260804 --limit 60
"""

import argparse
import csv
import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "oracle"))

import capacity as CAP  # noqa: E402
import closedform as C  # noqa: E402
import engine as E  # noqa: E402
import execute as X  # noqa: E402
import model as M  # noqa: E402
import plan as P  # noqa: E402
from generator import generate  # noqa: E402

RECORDS = 12
SCALE = Fraction(1, 200)
BUDGET = Fraction(8)


def deficit_plans(seed, count):
    """Indeksy planów, w których model przewiduje niedomiar dla `@`."""
    hits = []
    for index, (stratum, item) in enumerate(generate(seed, count)):
        tails = C.evaluate(item)
        origins = C.evaluate_origins(item)
        try:
            demand = CAP.required_lookback(item, tails, origins)
        except M.OracleError:
            continue
        cap = CAP.engine_capacity(item, tails, origins)
        for (consumer, child), back in demand.items():
            node = item.by_name(consumer)
            if node.kind != P.AGSE or item.by_name(child).kind != P.SOURCE:
                continue
            if (back + 1) - cap.get(child, 0) > 0:
                hits.append((index, stratum, item, consumer, child,
                             back + 1, cap.get(child, 0)))
                break
    return hits


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=20260804)
    parser.add_argument("--count", type=int, default=10_010)
    parser.add_argument("--limit", type=int, default=60)
    parser.add_argument("--xretractor", default=None)
    parser.add_argument("--out", default=None)
    args = parser.parse_args()

    binary = E.resolve_binary(args.xretractor)
    workroot = ROOT / "work" / "agse_capacity"
    workroot.mkdir(parents=True, exist_ok=True)
    out = Path(args.out or ROOT / "raw" / f"agse_capacity_seed{args.seed}.csv")
    out.parent.mkdir(parents=True, exist_ok=True)

    candidates = deficit_plans(args.seed, args.count)
    print(f"planów z przewidywanym niedomiarem `@`: {len(candidates)}, "
          f"do wykonania: {min(args.limit, len(candidates))}\n")

    rows = []
    for index, stratum, item, consumer, child, needed, provided in candidates[:args.limit]:
        scaled = P.rescale(item, SCALE / P.fastest(item))
        spread = P.slowest(scaled) / P.fastest(scaled)
        loops = int((RECORDS + 8) * spread) + 24
        if loops * P.fastest(scaled) > BUDGET:
            rows.append({"plan": index, "stratum": stratum, "consumer": consumer,
                         "child": child, "needed": needed, "provided": provided,
                         "status": "poza budżetem", "detail": ""})
            continue
        workdir = workroot / f"p{index}"
        try:
            X.run_plan(scaled, binary, workdir, loops=loops, records=1024)
            findings = X.compare_content(scaled, workdir, limit=RECORDS)
        except Exception as exc:  # noqa: BLE001
            rows.append({"plan": index, "stratum": stratum, "consumer": consumer,
                         "child": child, "needed": needed, "provided": provided,
                         "status": "awaria", "detail": str(exc)[:180]})
            continue
        agse = [f for f in findings if f["node"] == consumer]
        status = "zgodne" if not agse else agse[0]["issue"]
        rows.append({"plan": index, "stratum": stratum, "consumer": consumer,
                     "child": child, "needed": needed, "provided": provided,
                     "status": status,
                     "detail": str(agse[0])[:180] if agse else ""})
        print(f"{index:6d} {consumer:5s}<-{child:5s} trzeba {needed}, jest {provided}  {status}")

    with out.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    summary = {}
    for row in rows:
        summary[row["status"]] = summary.get(row["status"], 0) + 1
    print(f"\npodsumowanie: {summary}")
    symptomatic = sum(count for status, count in summary.items()
                      if status not in ("zgodne", "poza budżetem"))
    print("OBJAWY NIEDOMIARU: " + ("BRAK" if symptomatic == 0 else f"{symptomatic}"))
    return 1 if symptomatic else 0


if __name__ == "__main__":
    sys.exit(main())
