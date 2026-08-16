#!/usr/bin/env python3
"""Bramka niezależności oracle'a — wykrycie 100% zamrożonych mutantów.

Mutant jest wykryty, gdy istnieje węzeł korpusu bramkowego, na którym
oracle == replika != mutant. Warunek zgodności oracle'a z repliką jest
istotny: bez niego „wykryciem” byłaby dowolna stała różnica.

Bramka biegnie osobno dla obu wielkości, które silnik niesie po
przestemplowaniu z 2026-08-06: mutanty ogona sprawdzane są na ogonie, mutanty
początku logicznego — na początku logicznym. Mieszanie ich (np. porównywanie
sumy origin+ogon) ukryłoby dokładnie te błędy, dla których origin powstał:
przesunięcie milczenia między członami przy zachowanej sumie.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "oracle"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import closedform as C  # noqa: E402
import model as M  # noqa: E402
from mutants import ORIGIN_MUTANTS, TAIL_MUTANTS  # noqa: E402
from plan import SOURCE  # noqa: E402
from hand_cases import hand_cases  # noqa: E402


def _detect(corpus, mutants, oracle_of, replica_of):
    detected = {}
    for name, mutation in mutants.items():
        witnesses = []
        for label, plan, _, _, _ in corpus:
            oracle = oracle_of(plan)
            replica = replica_of(plan, None)
            mutant = replica_of(plan, mutation)
            for node in plan.nodes:
                if node.kind == SOURCE:
                    continue
                key = node.name
                if oracle[key] == replica[key] and mutant[key] != replica[key]:
                    witnesses.append(f"{label}:{key} (oracle={oracle[key]}, mutant={mutant[key]})")
        detected[name] = witnesses
    return detected


def main():
    corpus = hand_cases()

    tails = _detect(corpus, TAIL_MUTANTS,
                    lambda plan: M.tails_by_name(plan, convention=M.C1),
                    lambda plan, mutation: C.evaluate(plan, mutation=mutation))
    origins = _detect(corpus, ORIGIN_MUTANTS,
                      lambda plan: M.origins_by_name(plan, convention=M.C1),
                      lambda plan, mutation: C.evaluate_origins(plan, mutation=mutation))

    missing = []
    for title, detected in (("ogon", tails), ("origin", origins)):
        print(f"--- mutanty: {title} ---")
        for name, hits in detected.items():
            state = f"wykryty ({len(hits)} świadków, np. {hits[0]})" if hits else "NIEWYKRYTY"
            print(f"{name:24s} {state}")
            if not hits:
                missing.append(f"{title}/{name}")
    print("BRAMKA MUTANTÓW: " + ("PRZESZŁA (100%)" if not missing
                                 else f"NIE PRZESZŁA — niewykryte: {', '.join(missing)}"))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
