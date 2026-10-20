#!/usr/bin/env python3
"""Dane uc10-uc16 (#538), tlo losowe i wymuszone zdarzenia; bez uruchamiania silnika.

Plany sa reczne, wzorce liczy niezaleznie oracle.py. Test czyta zapisane pliki,
a --check odtwarza dane i wzorce w pamieci, wykrywajac ich nieaktualnosc.
"""
import argparse
import random
from pathlib import Path

HERE = Path(__file__).resolve().parent
CASES = tuple(f"uc{n}" for n in range(10, 17))


def datasets(case):
    rng = random.Random(53800 + int(case[2:]))
    if case == "uc10":
        temp = [rng.randint(21, 49) for _ in range(480)]
        temp[0] = 10
        # Rekord i liczy sie w slocie i+1. 120 i 160: cycle kontra hour_reset.
        for i in [119, *range(123, 153, 4), 159, 300]:
            temp[i] = rng.randint(55, 70)
            temp[i + 1] = rng.randint(5, 15)
        return {"temp.txt": temp, "hour.txt": list(range(1, 15)),
                "restart_before.txt": [10, 70] + [35] * 28,
                "restart_after.txt": [35] * 30 + [70] + [35] * 29}
    if case == "uc11":
        rows = []
        # Postoj, nieudany rozruch, produkcja, burza, przerwana cisza, powrot.
        for _ in range(2):
            for count, wind, rpm in [(100, (0, 29), (0, 100)), (5, (35, 60), (100, 800)),
                                     (4, (10, 29), (100, 800)), (5, (35, 60), (100, 800)),
                                     (80, (60, 190), (900, 1500)), (2, (250, 300), (900, 1500)),
                                     (15, (100, 190), (0, 100)), (1, (210, 240), (0, 100)),
                                     (50, (100, 190), (0, 100)), (100, (0, 29), (0, 100))]:
                rows.extend((rng.randint(*wind), rng.randint(*rpm)) for _ in range(count))
        return {"turbine.txt": rows}
    if case == "uc12":
        rows = [rng.randint(955, 970) for _ in range(60)]
        for _ in range(4):
            rows += [rng.randint(900, 920) for _ in range(15)]
            rows += [rng.randint(935, 940) for _ in range(10)]
            rows += [rng.randint(955, 970) for _ in range(70)]
        return {"spo2.txt": rows}
    if case == "uc13":
        rows = [rng.choice([-3, -2, -1, 1, 2, 3]) for _ in range(3600)]
        # Kazdy onset ma >=500 rekordow historii i >=1000 rekordow do konca.
        for start in (650, 2000):
            rows[start:start + 120] = [rng.choice([-1, 1]) * rng.randint(200, 300) for _ in range(120)]
        return {"acc.txt": rows}
    if case == "uc14":
        gate = []
        for i in range(600):
            phase = i % 40
            vin, vout = ((rng.randint(55, 65), 0) if phase < 10 else
                         (0, rng.randint(55, 65)) if phase < 20 else
                         (rng.randint(0, 3), rng.randint(0, 3)))
            gate.append((vin, vout))
        # Korekta zawsze koliduje z niezerowym bilansem bramek.
        for i in range(39, len(gate), 40):
            gate[i] = (3, 1)
        return {"gate.txt": gate, "recount.txt": [410, 300, 405, 200] * 5}
    if case == "uc15":
        plc = [[0, rng.randint(0, 1)] for _ in range(600)]
        panel = [0] * 62
        for offset in range(0, 600, 100):
            for i in [*range(19, 25), *range(50, 60)]:
                plc[offset + i][0] = 7
            plc[offset + 99][1] = 1
            # Slot 20: ack widzi stary FAULT_NOW=0; slot 60: odmowa, potem ack.
            for i in (1, 2, 5, 6):
                panel[offset // 10 + i] = 1
        return {"plc.txt": plc, "panel.txt": panel, "shift.txt": list(range(1, 9))}
    if case == "uc16":
        return {"req.txt": [5, 5, 100, 1, 100] + [rng.randint(1, 15) if i % 7 else 100 for i in range(295)]}
    raise ValueError(case)


def encoded(rows):
    return "".join(" ".join(map(str, row if isinstance(row, (tuple, list)) else [row])) + "\n" for row in rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    from oracle import expected_text, simulate, validate_events
    for case in CASES:
        files = datasets(case)
        for name, rows in files.items():
            path = HERE / case / name
            content = encoded(rows)
            if args.check:
                if path.read_text() != content:
                    raise ValueError(f"{path}: dane inne niz wynik generatora")
            else:
                path.write_text(content, encoding="ascii")
        result = simulate(case, HERE / case)
        validate_events(case, result)
        content = expected_text(result)
        path = HERE / case / "expected.jsonl"
        if args.check:
            if path.read_text() != content:
                raise ValueError(f"{path}: wzorzec inny niz wynik wyroczni")
        else:
            path.write_text(content, encoding="ascii")
        print(f"{case}: dane, wzorzec i pokrycie regul OK")


if __name__ == "__main__":
    main()
