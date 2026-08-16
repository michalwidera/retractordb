#!/usr/bin/env python3
"""Reproducer jedynego znaleziska bramki odwzorowania: plan 38, ziarno 20260804.

Bramka zgłosiła dla węzła `n3` (klasa `-`) „zero rekordów". Skrypt uruchamia
ten jeden plan w kilku długościach przebiegu i wypisuje, ile rekordów faktycznie
powstało w każdym węźle — wobec liczby, której oczekuje model zdarzeniowy.

Rozstrzygane pytanie: czy zero rekordów jest defektem silnika, czy skutkiem
zbyt krótkiego przebiegu. Budżet bramki (`loops`) jest liczony z rozpiętości
interwałów i **nie uwzględnia origin**, który po przestemplowaniu potrafi być
duży (tu 43 sloty dla `n2`).

    python3 repro_plan38.py
"""

import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "oracle"))

import engine as E  # noqa: E402
import execute as X  # noqa: E402
import model as M  # noqa: E402
import plan as P  # noqa: E402
from generator import generate  # noqa: E402

SEED = 20260804
INDEX = 38
SCALE = Fraction(1, 200)


def main():
    binary = E.resolve_binary(None)
    target = None
    for index, (stratum, item) in enumerate(generate(SEED, 10_010)):
        if index == INDEX:
            target = (stratum, item)
            break
    stratum, item = target
    res = {r.name: r for r in M.evaluate(item, convention=M.C1)}
    scaled = P.rescale(item, SCALE / P.fastest(item))
    fastest = P.fastest(scaled)
    spread = P.slowest(scaled) / fastest
    gate_loops = int((12 + 8) * spread) + 24

    print(f"plan {INDEX} ({stratum}), pętli w bramce: {gate_loops}\n")
    for loops in (gate_loops, 2 * gate_loops, 4 * gate_loops):
        workdir = ROOT / "work" / "repro38" / f"m{loops}"
        X.run_plan(scaled, binary, workdir, loops=loops, records=2048)
        print(f"-- pętli {loops} (czas symulowany {float(loops * fastest):.3f} s)")
        for node in scaled.nodes:
            if node.kind == P.SOURCE:
                continue
            payload = workdir / node.name
            got = len(X.decode_payload(payload, P.payload_words(node))) if payload.exists() else 0
            slots = float(loops * fastest / node.delta)
            silence = res[node.name].origin + res[node.name].tail
            expected = max(0, slots - silence)
            findings = [f for f in X.compare_content(scaled, workdir, limit=8)
                        if f["node"] == node.name]
            state = "zgodne" if not findings else findings[0]["issue"]
            print(f"   {node.name:4s} {node.kind:7s} rekordów={got:4d}  "
                  f"model~{expected:7.2f}  (origin+ogon={silence:3d})  {state}")
        print()


if __name__ == "__main__":
    main()
