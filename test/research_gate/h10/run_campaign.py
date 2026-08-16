#!/usr/bin/env python3
"""Kampania K24 — porównanie postaci zamkniętej z oracle'em na całym korpusie.

Wynik jest zapisywany per węzeł, nigdy agregatem. Kampania nie mierzy czasu,
nie zajmuje workera i nie porównuje systemów: dla każdego planu kompiluje plan
(`xretractor -c`), czyta ogon ze zrzutu i zestawia go z oracle'em zdarzeniowym
w obu konwencjach dostępności oraz z regułą lokalną członu (b).
"""

import argparse
import csv
import os
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "oracle"))

import closedform as C  # noqa: E402
import engine as E  # noqa: E402
import model as M  # noqa: E402
from generator import generate, _hard_classes_of  # noqa: E402
from plan import HASH, PASS, SHIFT, SOURCE, to_rql  # noqa: E402

FIELDS = ("plan", "stratum", "hard_classes", "depth", "node", "kind", "delta",
          "engine_tail", "oracle_c1", "oracle_c2", "replica_tail",
          "step_c1", "step_c2", "agree_c1", "agree_c2", "agree_step_c1", "agree_step_c2",
          "engine_origin", "oracle_origin", "replica_origin", "step_origin",
          "agree_origin", "agree_step_origin",
          "engine_silence", "oracle_silence", "agree_silence",
          "local_a", "local_b", "h10b_eligible", "divergence_a", "divergence_b",
          "predicted_form")


def local_rule_a(plan, given_tails=None):
    """Reguła lokalna A (predeklarowana): własny ogon każdego operatora = 0.

    Ogon składowej przeliczany przez takt wzorem ceil(w*D_src/D_dst). Dokładnie
    „suma ogonów operatorów przeliczona przez takt, bez składnika fazowego”.

    Zmiana wobec K24r: wyjątek dla `>N` (ogon składowej + N) zniknął, bo po
    przestemplowaniu z 2026-08-06 `N` nie jest już ogonem — przeszło do origin.
    Utrzymanie starego wyjątku dokładałoby regule lokalnej człon, którego nie ma
    ani w silniku, ani w modelu zdarzeniowym, i zafałszowałoby rozjazd, którym
    mierzy się człon (b). Populacja członu (b) (składowe deklarowane) jest na tę
    zmianę niewrażliwa; wpływ ma ona wyłącznie na kolumny diagnostyczne.
    """
    tails = {}
    for node in plan.nodes:
        if node.kind == SOURCE:
            tails[node.name] = 0
            continue
        children = [plan.by_name(name) for name in node.children]
        source_tails = given_tails if given_tails is not None else tails
        converted = [C.to_slots(source_tails[child.name], child.delta, node.delta)
                     for child in children]
        tails[node.name] = max(converted)
    return tails


def local_rule_b(plan, given_tails=None):
    """Reguła lokalna B (diagnostyczna): pełna postać zamknięta z członem
    pierwszej fazy `ceil(q/p)` w miejscu `ceil((p+q-1)/p)` — wariant sprzed K2.

    Wariant B jest jedynym, przy którym kontrola negatywna `HC_INT` może
    wypaść zerowa, bo dla ilorazu całkowitego oba człony się pokrywają.
    Nie jest to reguła predeklarowana; służy diagnozie sprzeczności
    w specyfikacji członu (b) — patrz REPORT.md §5.
    """
    return C.evaluate(plan, mutation={"hash_first_phase": True}, given_tails=given_tails)


def h10b_form(plan):
    """Predeklarowana postać rozjazdu: ceil((p+q-1)/p) dla jedynego `#`."""
    hashes = [node for node in plan.nodes if node.kind == HASH]
    if len(hashes) != 1:
        return None
    left, right = (plan.by_name(name) for name in hashes[0].children)
    ratio = left.delta / right.delta
    p, q = ratio.numerator, ratio.denominator
    return -((-(p + q - 1)) // p)


def eligible_h10b(plan):
    operators = [node for node in plan.nodes if node.kind != SOURCE]
    kinds = {node.kind for node in operators}
    return kinds <= {PASS, SHIFT, HASH} and sum(1 for n in operators if n.kind == HASH) == 1


def evaluate_one(item):
    index, stratum, plan, binary, workroot = item
    # Katalog roboczy musi być prywatny dla procesu: dwa plany w jednym
    # katalogu nadpisują sobie query.rql i dają zrzut cudzego planu.
    workdir = Path(workroot) / f"pid{os.getpid()}"
    rows = []
    try:
        dump, _ = E.compile_plan(to_rql(plan), Path(binary), workdir)
    except E.EngineError as exc:
        return {"error": f"plan {index}: {exc}"}
    engine_plan = E.parse_plan(dump)
    try:
        results1 = M.evaluate(plan, convention=M.C1)
        results2 = M.evaluate(plan, convention=M.C2)
    except M.OracleError as exc:
        return {"error": f"plan {index}: oracle — {exc}"}
    oracle1 = {item.name: item.tail for item in results1}
    oracle2 = {item.name: item.tail for item in results2}
    # Origin nie ma konwencji dostępności: istnienie rekordu nie zależy od tego,
    # czy odczyt w tym samym takcie jest dozwolony. Bierzemy go z przebiegu C1.
    oracle_origin = {item.name: item.origin for item in results1}
    replica = C.evaluate(plan)
    replica_origin = C.evaluate_origins(plan)
    # Atrybucja izolowana: każdy węzeł liczony z wielkości składowych wziętych
    # z oracle'a, żeby niezgodność nie była dziedziczona po dziecku.
    step_c1 = C.evaluate(plan, given_tails=oracle1)
    step_c2 = C.evaluate(plan, given_tails=oracle2)
    step_origin = C.evaluate_origins(plan, given_origins=oracle_origin)
    local_a = local_rule_a(plan, given_tails=oracle1)
    local_b = local_rule_b(plan, given_tails=oracle1)
    hard = ",".join(sorted(_hard_classes_of(plan.nodes)))
    eligible = eligible_h10b(plan)
    form = h10b_form(plan) if eligible else None

    for node in plan.nodes:
        if node.kind == SOURCE:
            continue
        got = engine_plan.get(node.name)
        if got is None:
            return {"error": f"plan {index}: brak {node.name} w zrzucie planu"}
        interval, tail, origin = got
        if interval != node.delta:
            return {"error": f"plan {index}: interwał {node.name} {node.delta} vs {interval}"}
        is_root = node.name == plan.root.name
        engine_silence = tail + origin
        oracle_silence = oracle1[node.name] + oracle_origin[node.name]
        rows.append({
            "plan": index, "stratum": stratum, "hard_classes": hard, "depth": plan.depth,
            "node": node.name, "kind": node.kind, "delta": str(node.delta),
            "engine_tail": tail, "oracle_c1": oracle1[node.name], "oracle_c2": oracle2[node.name],
            "replica_tail": replica[node.name],
            "step_c1": step_c1[node.name], "step_c2": step_c2[node.name],
            "agree_c1": int(tail == oracle1[node.name]),
            "agree_c2": int(tail == oracle2[node.name]),
            "agree_step_c1": int(step_c1[node.name] == oracle1[node.name]),
            "agree_step_c2": int(step_c2[node.name] == oracle2[node.name]),
            "engine_origin": origin, "oracle_origin": oracle_origin[node.name],
            "replica_origin": replica_origin[node.name], "step_origin": step_origin[node.name],
            "agree_origin": int(origin == oracle_origin[node.name]),
            "agree_step_origin": int(step_origin[node.name] == oracle_origin[node.name]),
            # Suma slotów milczenia jest jedyną wielkością porównywalną
            # z kampaniami sprzed przestemplowania: tam origin nie istniał,
            # a jego zawartość siedziała w ogonie.
            "engine_silence": engine_silence, "oracle_silence": oracle_silence,
            "agree_silence": int(engine_silence == oracle_silence),
            "local_a": local_a[node.name], "local_b": local_b[node.name],
            "h10b_eligible": int(eligible and is_root),
            "divergence_a": oracle1[node.name] - local_a[node.name],
            "divergence_b": oracle1[node.name] - local_b[node.name],
            "predicted_form": form if (eligible and is_root) else "",
        })
    return {"rows": rows}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=20260807)
    parser.add_argument("--count", type=int, default=10_010)
    parser.add_argument("--xretractor", default=None)
    parser.add_argument("--out", default=str(ROOT / "raw" / "campaign.csv"))
    parser.add_argument("--workers", type=int, default=8)
    args = parser.parse_args()

    binary = E.resolve_binary(args.xretractor)
    workroot = ROOT / "work" / "campaign"
    workroot.mkdir(parents=True, exist_ok=True)
    corpus = generate(args.seed, args.count)
    payload = [(index, stratum, plan, str(binary), str(workroot))
               for index, (stratum, plan) in enumerate(corpus)]

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    errors = []
    written = 0
    with out.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        with ProcessPoolExecutor(max_workers=args.workers) as pool:
            for result in pool.map(evaluate_one, payload, chunksize=16):
                if "error" in result:
                    errors.append(result["error"])
                    continue
                for row in result["rows"]:
                    writer.writerow(row)
                    written += 1

    print(f"planów: {len(corpus)}, wierszy: {written}, błędów aparatury: {len(errors)}")
    for line in errors[:20]:
        print(f"APARATURA {line}")
    if errors:
        print("KAMPANIA ZATRZYMANA — plan odrzucony przez kompilator jest błędem aparatury")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
