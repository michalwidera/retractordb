#!/usr/bin/env python3
"""Wierność repliki postaci zamkniętej wobec silnika.

Replika jest narzędziem bramki mutantów; jeżeli rozjeżdża się ze zrzutem planu
silnika, bramka mutantów nie znaczy nic. Test kompiluje każdy plan
kalibracyjny i porównuje ogon węzła po węźle.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "oracle"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import closedform as C  # noqa: E402
import engine as E  # noqa: E402
from plan import SOURCE, to_rql  # noqa: E402
from hand_cases import hand_cases  # noqa: E402


def main(argv):
    binary = E.resolve_binary(argv[1] if len(argv) > 1 else None)
    workdir = ROOT / "work" / "closedform"
    failures = []
    checked = 0
    for label, plan, _, _, _ in hand_cases():
        dump, _ = E.compile_plan(to_rql(plan), binary, workdir)
        engine_plan = E.parse_plan(dump)
        replica = C.evaluate(plan)
        replica_origins = C.evaluate_origins(plan)
        for node in plan.nodes:
            if node.kind == SOURCE:
                continue
            checked += 1
            got = engine_plan.get(node.name)
            if got is None:
                failures.append(f"{label} {node.name}: brak w zrzucie planu")
                continue
            interval, tail, origin = got
            if interval != node.delta:
                failures.append(f"{label} {node.name}: interwał {node.delta} vs {interval}")
            if tail != replica[node.name]:
                failures.append(f"{label} {node.name}: ogon — silnik {tail}, replika {replica[node.name]}")
            if origin != replica_origins[node.name]:
                failures.append(f"{label} {node.name}: origin — silnik {origin}, "
                                f"replika {replica_origins[node.name]}")

    print(f"węzłów porównanych: {checked}")
    for line in failures:
        print(f"NIEZGODNOŚĆ {line}")
    print("WIERNOŚĆ REPLIKI: " + ("POTWIERDZONA" if not failures else f"ZŁAMANA ({len(failures)})"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
