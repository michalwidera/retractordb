#!/usr/bin/env python3
"""Bramka oracle'a: zgodność z przypadkami o ręcznie wyprowadzonej odpowiedzi.

Bramka jest warunkiem wstępnym kampanii. Nie dotyka silnika — porównuje
wyłącznie oracle z rachunkiem ręcznym: ogon w obu konwencjach dostępności
i początek logiczny, który konwencji nie ma (istnienie rekordu nie zależy od
tego, czy odczyt w tym samym takcie jest dozwolony).
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "oracle"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import model as M  # noqa: E402
from hand_cases import hand_cases  # noqa: E402


def main():
    failures = []
    checked = 0
    for label, plan, expected_c1, expected_c2, expected_origin in hand_cases():
        for convention, expected in ((M.C1, expected_c1), (M.C2, expected_c2)):
            try:
                results = M.evaluate(plan, convention=convention)
            except M.OracleError as exc:
                failures.append(f"{label} [{convention}]: awaria oracle'a — {exc}")
                continue
            tails = {item.name: item.tail for item in results}
            origins = {item.name: item.origin for item in results}
            for name, want in expected.items():
                checked += 1
                got = tails.get(name)
                if got != want:
                    failures.append(f"{label} [{convention}] {name}: ogon ręcznie {want}, oracle {got}")
            for name, want in expected_origin.items():
                checked += 1
                got = origins.get(name)
                if got != want:
                    failures.append(f"{label} [{convention}] {name}: origin ręcznie {want}, oracle {got}")

    print(f"przypadków ręcznych: {len(hand_cases())}, porównań: {checked}")
    for line in failures:
        print(f"NIEZGODNOŚĆ {line}")
    print("BRAMKA ORACLE: " + ("PRZESZŁA" if not failures else f"NIE PRZESZŁA ({len(failures)})"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
