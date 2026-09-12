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
import model as M  # noqa: E402
import plan as P  # noqa: E402
from generator import STRATA, STRATA_WITH_WINDOW, generate  # noqa: E402

RECORDS = 12
SCALES = (Fraction(1, 200), Fraction(1, 100))   # docelowy interwał najszybszego strumienia
BUDGET = Fraction(8)                            # sekundy na pojedynczy przebieg


def select(corpus, per_stratum, strata=STRATA):
    chosen = []
    counts = {name: 0 for name in strata}
    for index, (stratum, item) in enumerate(corpus):
        if counts[stratum] >= per_stratum:
            continue
        counts[stratum] += 1
        chosen.append((index, stratum, item))
    return chosen


def horizon_of(plan, origins, tails):
    """Czas scienny, po ktorym KAZDY wezel ma juz RECORDS rekordow.

    Rekord n jest emitowany w chwili (n+1+W)*Delta, a PIERWSZYM istniejacym jest
    rekord o indeksie `origin` — przed nim rekordow nie ma. Ostatni potrzebny ma
    wiec indeks origin+RECORDS-1 i to on wyznacza horyzont.

    Do 2026-09-12 tego rachunku tu nie bylo: przebieg wymiarowala sama
    ROZPIETOSC interwalow (`(RECORDS+8)*spread`), a origin do wzoru nie wchodzil.
    W planie, w ktorym `>N` skladaja sie w lancuch, origin narasta (zmierzone:
    8 -> 13 -> 26 -> 34) i budzet konczyl sie, zanim najglebszy wezel doszedl do
    wlasnego origin. Artefakt zostawal pusty — poprawnie, bo przed origin nie ma
    rekordow — a bramka raportowala to jako `zero rekordow`, czyli ROZBIEZNOSC
    TRESCI. Byla to granica aparatury podana jako wynik o silniku; trzy takie
    przypadki zatrzymaly poziom bramki w K24f (patrz STOP.md tej kampanii).
    """
    worst = Fraction(0)
    for node in plan.nodes:
        if node.kind == P.SOURCE:
            continue
        last = origins[node.name] + RECORDS - 1
        worst = max(worst, (Fraction(last) + 1 + tails[node.name]) * node.delta)
    return worst


def wakeup_budget(plan, horizon):
    """Gorne ograniczenie liczby pobudek w czasie `horizon`.

    `-m N` jest budzetem SLOTOW, a slot jest chwila, w ktorej tyka co najmniej
    jeden strumien — nie taktem najszybszego strumienia. Pobudek jest wiec
    najwyzej tyle, ile sumarycznie tykniec wszystkich strumieni w horyzoncie;
    chwile wspolne tylko zmniejszaja te liczbe, wiec suma jest bezpieczna.

    Stary wzor mylil te dwie wielkosci i przez to zanizal budzet takze tam,
    gdzie origin byl zerowy: plan wielotaktowy ma WIECEJ pobudek niz taktow
    najszybszego strumienia.
    """
    return sum(int(horizon / node.delta) + 1 for node in plan.nodes)


def run_one(index, stratum, item, binary, workroot):
    outcomes = []
    # Origin i ogon sa wielkosciami INDEKSOWYMI i zaleza wylacznie od ilorazow
    # interwalow, wiec sa te same w obu skalach — liczymy je raz, na planie
    # nieprzeskalowanym. Zrodlem jest MODEL ZDARZENIOWY, nie replika: bramka
    # odwzorowania nie ma prawa wpuscic rachunku silnika do wykonania.
    results = M.evaluate(item, convention=M.C1)
    origins = {r.name: r.origin for r in results}
    tails = {r.name: r.tail for r in results}
    for scale in SCALES:
        scaled = P.rescale(item, scale / P.fastest(item))
        horizon = horizon_of(scaled, origins, tails)
        if horizon > BUDGET:
            return {"plan": index, "stratum": stratum, "status": "poza budżetem",
                    "detail": f"horyzont {float(horizon):.2f} s > {BUDGET} s"}
        loops = wakeup_budget(scaled, horizon) + 24
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
    # Korpus z oknem rekordowym — patrz run_campaign.py. Bramka odwzorowania jest
    # JEDYNYM miejscem, gdzie model TRESCI okna jest sprawdzany wobec bajtow
    # artefaktu; bez tego przelacznika galaz WINDOW w model.content() bylaby
    # aparatura nieuruchomiona.
    parser.add_argument("--with-window", action="store_true",
                        help="dolacz strate okna rekordowego (korpus K24f)")
    args = parser.parse_args()

    binary = E.resolve_binary(args.xretractor)
    workroot = ROOT / "work" / "mapping"
    workroot.mkdir(parents=True, exist_ok=True)
    strata = STRATA_WITH_WINDOW if args.with_window else STRATA
    chosen = select(generate(args.seed, args.count, strata=strata), args.per_stratum, strata)

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
