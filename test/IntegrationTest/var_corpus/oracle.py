#!/usr/bin/env python3
"""Niezalezna wyrocznia uc10-uc16 (#538), oparta na modelu etapu B (#533).

Nie parsuje RQL i nie korzysta z kodu silnika. Stan jest liczony od poczatku,
wszystkie odczyty slotu widza stan sprzed niego (D1), a ostatnia regula w tekscie
wygrywa konflikt (D3). Arytmetyka INTEGER/int32 i FLOAT/float32 jak w etapie B.
Odczyt artefaktow sprawdza pelne rekordy binarne i autorytatywne maski NULL.
DUMP ma zakres polotwarty; bez RETENTION kolejny zrzut zastepuje poprzedni.
"""
import argparse
import json
import re
import struct
from pathlib import Path

def f32(x):
    return struct.unpack("f", struct.pack("f", x))[0]


def I(n):
    return ("INTEGER", n) if -2**31 <= n <= 2**31 - 1 else None


def F(x):
    return ("FLOAT", f32(x))


def _pair(a, b):
    if a[0] == b[0]:
        return a[0], a[1], b[1]
    return "FLOAT", f32(float(a[1])), f32(float(b[1]))


def _arith(op, a, b):
    if a is None or b is None:
        return None
    t, x, y = _pair(a, b)
    if op == "/" and y == 0:
        return None
    if t == "INTEGER":
        if op == "/":
            q = abs(x) // abs(y)
            return I(q if (x >= 0) == (y >= 0) else -q)
        return I({"+": x + y, "-": x - y, "*": x * y}[op])
    return F({"+": x + y, "-": x - y, "*": x * y, "/": x / y if op == "/" else 0}[op])


def add(a, b):
    return _arith("+", a, b)


def sub(a, b):
    return _arith("-", a, b)


def mul(a, b):
    return _arith("*", a, b)


def div(a, b):
    return _arith("/", a, b)


def cmp(op, a, b):
    if a is None or b is None:
        return None
    _, x, y = _pair(a, b)
    return I(int({">": x > y, ">=": x >= y, "<": x < y, "<=": x <= y, "=": x == y}[op]))


def truth(v):
    return v is not None and v[1] != 0


def AND(a, b):
    return I(int(truth(a) and truth(b)))


def lit(text):
    return F(float(text)) if "." in text else I(int(text))


ALWAYS = lambda row, v: I(1)  # noqa: E731 - `WHEN 1 = 1`

# --- modele planow -----------------------------------------------------------------------
#
# sources:  nazwa -> (interwal, plik danych)
# streams:  (nazwa, zrodlo, pola(rekord zrodla, zmienne)) w kolejnosci zaleznosci
# rules:    (nazwa, strumien, warunek(rekord, zmienne), akcja) W KOLEJNOSCI TEKSTU PLANU;
#           akcja to ("SET", zmienna, wyrazenie(rekord, zmienne)) albo ("DUMP", przod)
# conflicts: pary regul SET, ktore musza co najmniej raz pisac w tym samym slocie


def uc10():
    return dict(
        variables={"COMP": I(0), "CYCLES": I(0)},
        sources={"temp": (1, "temp.txt"), "hour": (40, "hour.txt")},
        streams=[
            ("status", "temp", lambda r, v: [r[0], v["COMP"], v["CYCLES"]]),
            ("hour_tick", "hour", lambda r, v: [r[0]]),
        ],
        rules=[
            ("cycle", "status", lambda r, v: AND(cmp(">=", r[0], I(50)), cmp("=", v["COMP"], I(0))),
             ("SET", "CYCLES", lambda r, v: add(v["CYCLES"], I(1)))),
            ("comp_on", "status", lambda r, v: cmp(">=", r[0], I(50)), ("SET", "COMP", lambda r, v: I(1))),
            ("comp_off", "status", lambda r, v: cmp("<=", r[0], I(20)), ("SET", "COMP", lambda r, v: I(0))),
            ("hour_reset", "hour_tick", ALWAYS, ("SET", "CYCLES", lambda r, v: I(0))),
            ("short_cycling", "status", lambda r, v: cmp(">", v["CYCLES"], I(6)), ("DUMP", -60, 0)),
        ],
        conflicts=[("cycle", "hour_reset")],
        types={"status": ["INTEGER", "INTEGER", "INTEGER"], "hour_tick": ["INTEGER"]},
    )


def uc11():
    mode = lambda m: lambda r, v: cmp("=", v["MODE"], I(m))  # noqa: E731
    return dict(
        variables={"MODE": I(0), "CALM": I(0)},
        sources={"turb": (1, "turbine.txt")},
        streams=[("state", "turb", lambda r, v: [r[0], r[1], v["MODE"], v["CALM"]])],
        rules=[
            ("m01", "state", lambda r, v: AND(mode(0)(r, v), cmp(">=", r[0], I(35))), ("SET", "MODE", lambda r, v: I(1))),
            ("m12", "state", lambda r, v: AND(mode(1)(r, v), cmp(">=", r[1], I(900))), ("SET", "MODE", lambda r, v: I(2))),
            ("m10", "state", lambda r, v: AND(mode(1)(r, v), cmp("<", r[0], I(30))), ("SET", "MODE", lambda r, v: I(0))),
            ("m23", "state", lambda r, v: AND(mode(2)(r, v), cmp(">=", r[0], I(250))), ("SET", "MODE", lambda r, v: I(3))),
            ("m23_dump", "state", lambda r, v: AND(mode(2)(r, v), cmp(">=", r[0], I(250))), ("DUMP", -30, 30)),
            ("calm_count", "state", lambda r, v: AND(mode(3)(r, v), cmp("<=", r[0], I(200))),
             ("SET", "CALM", lambda r, v: add(v["CALM"], I(1)))),
            ("calm_break", "state", lambda r, v: cmp(">", r[0], I(200)), ("SET", "CALM", lambda r, v: I(0))),
            ("m30", "state", lambda r, v: AND(mode(3)(r, v), cmp(">=", v["CALM"], I(40))), ("SET", "MODE", lambda r, v: I(0))),
        ],
        conflicts=[],
        types={"state": ["INTEGER"] * 4},
    )


def uc12():
    return dict(
        variables={"BASE": F(960.0), "DESAT": I(0), "EVENTS": I(0), "N": I(0)},
        sources={"spo2": (1, "spo2.txt")},
        streams=[("odi", "spo2", lambda r, v: [r[0], v["BASE"], v["DESAT"], v["EVENTS"],
                                               div(mul(v["EVENTS"], I(3600)), v["N"])])],
        rules=[
            ("tick", "odi", ALWAYS, ("SET", "N", lambda r, v: add(v["N"], I(1)))),
            ("base", "odi", lambda r, v: cmp("=", v["DESAT"], I(0)),
             ("SET", "BASE", lambda r, v: add(mul(v["BASE"], lit("0.99")), mul(r[0], lit("0.01"))))),
            ("ev_count", "odi", lambda r, v: AND(cmp("=", v["DESAT"], I(0)), cmp("<=", r[0], sub(v["BASE"], I(30)))),
             ("SET", "EVENTS", lambda r, v: add(v["EVENTS"], I(1)))),
            ("ev_start", "odi", lambda r, v: cmp("<=", r[0], sub(v["BASE"], I(30))), ("SET", "DESAT", lambda r, v: I(1))),
            ("ev_end", "odi", lambda r, v: cmp(">=", r[0], sub(v["BASE"], I(10))), ("SET", "DESAT", lambda r, v: I(0))),
        ],
        conflicts=[],
        types={"odi": ["INTEGER", "FLOAT", "INTEGER", "INTEGER", "INTEGER"]},
    )


def uc14():
    return dict(
        variables={"OCC": I(0), "FULL": I(0)},
        sources={"gate": (1, "gate.txt"), "recount": (40, "recount.txt")},
        streams=[
            ("parking", "gate", lambda r, v: [r[0], r[1], v["OCC"], v["FULL"]]),
            ("recount_tick", "recount", lambda r, v: [r[0]]),
        ],
        rules=[
            ("balance", "parking", ALWAYS, ("SET", "OCC", lambda r, v: add(v["OCC"], sub(r[0], r[1])))),
            ("full_on", "parking", lambda r, v: cmp(">=", v["OCC"], I(400)), ("SET", "FULL", lambda r, v: I(1))),
            ("full_off", "parking", lambda r, v: cmp("<=", v["OCC"], sub(I(400), I(5))), ("SET", "FULL", lambda r, v: I(0))),
            ("correct", "recount_tick", ALWAYS, ("SET", "OCC", lambda r, v: r[0])),
        ],
        conflicts=[("balance", "correct")],
        types={"parking": ["INTEGER"] * 4, "recount_tick": ["INTEGER"]},
    )


def uc13():
    return dict(
        variables={"STA": F(0), "LTA": F(1), "TRIG": I(0)},
        sources={"acc": (1, "acc.txt")},
        streams=[("detector", "acc", lambda r, v: [r[0], v["STA"], v["LTA"], v["TRIG"]])],
        rules=[
            ("sta", "detector", ALWAYS,
             ("SET", "STA", lambda r, v: add(mul(v["STA"], F(.9)), mul(I(abs(r[0][1])), F(.1))))),
            ("lta", "detector", lambda r, v: cmp("=", v["TRIG"], I(0)),
             ("SET", "LTA", lambda r, v: add(mul(v["LTA"], F(.999)), mul(I(abs(r[0][1])), F(.001))))),
            ("onset_dump", "detector", lambda r, v: AND(cmp("=", v["TRIG"], I(0)),
                                                        cmp(">", v["STA"], mul(I(4), v["LTA"]))),
             ("DUMP", -500, 1000)),
            ("trig_on", "detector", lambda r, v: cmp(">", v["STA"], mul(I(4), v["LTA"])),
             ("SET", "TRIG", lambda r, v: I(1))),
            ("trig_off", "detector", lambda r, v: cmp("<", v["STA"], mul(F(1.5), v["LTA"])),
             ("SET", "TRIG", lambda r, v: I(0))),
        ],
        conflicts=[], types={"detector": ["INTEGER", "FLOAT", "FLOAT", "INTEGER"]},
    )


def uc15():
    return dict(
        variables={"LATCH": I(0), "FAULT_NOW": I(0), "PIECES": I(0)},
        sources={"plc": (1, "plc.txt"), "panel": (10, "panel.txt"), "shift": (100, "shift.txt")},
        streams=[("line", "plc", lambda r, v: [r[0], r[1], v["LATCH"], v["PIECES"]]),
                 ("panel_tick", "panel", lambda r, v: [r[0]]),
                 ("shift_tick", "shift", lambda r, v: [r[0]])],
        rules=[
            ("fault_set", "line", lambda r, v: I(int(r[0][1] != 0)), ("SET", "LATCH", lambda r, v: I(1))),
            ("fault_now", "line", ALWAYS, ("SET", "FAULT_NOW", lambda r, v: r[0])),
            ("count", "line", lambda r, v: AND(cmp("=", r[1], I(1)), cmp("=", v["LATCH"], I(0))),
             ("SET", "PIECES", lambda r, v: add(v["PIECES"], I(1)))),
            ("ack", "panel_tick", lambda r, v: AND(cmp("=", r[0], I(1)), cmp("=", v["FAULT_NOW"], I(0))),
             ("SET", "LATCH", lambda r, v: I(0))),
            ("shift_reset", "shift_tick", ALWAYS, ("SET", "PIECES", lambda r, v: I(0))),
        ],
        conflicts=[("fault_set", "ack"), ("count", "shift_reset")],
        types={"line": ["INTEGER"] * 4, "panel_tick": ["INTEGER"], "shift_tick": ["INTEGER"]},
    )


def uc16():
    return dict(
        variables={"TOK": I(50)}, sources={"req": (1, "req.txt")},
        streams=[("bucket", "req", lambda r, v: [r[0], v["TOK"]])],
        rules=[
            ("refill", "bucket", lambda r, v: cmp("<", v["TOK"], I(48)),
             ("SET", "TOK", lambda r, v: add(v["TOK"], I(2)))),
            ("consume", "bucket", lambda r, v: cmp(">=", v["TOK"], r[0]),
             ("SET", "TOK", lambda r, v: sub(v["TOK"], r[0]))),
        ],
        conflicts=[("refill", "consume")], types={"bucket": ["INTEGER", "INTEGER"]},
    )


MODELS = {"uc10": uc10, "uc11": uc11, "uc12": uc12, "uc13": uc13,
          "uc14": uc14, "uc15": uc15, "uc16": uc16}


def simulate_window(directory):
    # Wariant S3: publikacja po zapelnieniu najdluzszego okna (1000 rekordow),
    # potem konwersja AVG do FLOAT. TRIG sprzed slotu, ale AVG z biezacego okna.
    samples = [int(x) for x in (directory / "acc.txt").read_text().split()]
    model = uc13()
    model["rules"] = [r for r in model["rules"] if r[0] not in ("sta", "lta")]
    rows, trig = [], I(0)
    fired = {r[0]: [] for r in model["rules"]}
    for i, sample in enumerate(samples):
        if i < 999:
            continue
        short = [abs(x) for x in samples[max(0, i - 9):i + 1]]
        long = [abs(x) for x in samples[max(0, i - 999):i + 1]]
        sta, lta = F(sum(short) / len(short)), F(sum(long) / len(long))
        rows.append([I(sample), sta, lta, trig])
        index = len(rows) - 1
        if truth(cmp(">", sta, mul(I(4), lta))):
            if trig == I(0):
                fired["onset_dump"].append(index)
            fired["trig_on"].append(index)
            trig = I(1)
        if truth(cmp("<", sta, mul(F(1.5), lta))):
            fired["trig_off"].append(index)
            trig = I(0)
    return dict(model=model, rows={"detector": rows}, fired=fired, conflicts={})


def simulate(case, directory, overrides=None, initial=None):
    model = MODELS[case]()
    data = {}
    for name, (_, filename) in model["sources"].items():
        path = directory / (overrides or {}).get(filename, filename)
        data[name] = [[I(int(x)) for x in line.split()] for line in path.read_text().splitlines()]
    intervals = {name: delta for name, (delta, _) in model["sources"].items()}
    for name, source, _ in model["streams"]:
        intervals[name] = intervals[source]
    end = min(len(rows) * intervals[name] for name, rows in data.items())
    variables = dict(model["variables"] if initial is None else initial)
    output = {name: [] for name, _, _ in model["streams"]}
    fired = {r[0]: [] for r in model["rules"]}
    conflicts = {}
    for slot in range(1, end + 1):
        current = {name: rows[slot // intervals[name] - 1]
                   for name, rows in data.items() if slot % intervals[name] == 0}
        for name, source, fields in model["streams"]:
            if source in current:
                current[name] = fields(current[source], variables)
                output[name].append(current[name])
        writes, writers = {}, {}
        for name, stream, condition, action in model["rules"]:
            if stream not in current or not truth(condition(current[stream], variables)):
                continue
            fired[name].append(len(output[stream]) - 1)
            if action[0] == "SET":
                writes[action[1]] = action[2](current[stream], variables)
                writers.setdefault(action[1], []).append(name)
        for names in writers.values():
            for a, b in zip(names, names[1:]):
                conflicts.setdefault(a + "/" + b, []).append(slot)
        variables.update(writes)
    return dict(model=model, rows=output, fired=fired, conflicts=conflicts, final=variables)


def dump_ranges(result):
    ranges = {}
    for name, stream, _, action in result["model"]["rules"]:
        if action[0] == "DUMP" and result["fired"][name]:
            index = result["fired"][name][-1]
            ranges[f"{stream}_{name}_dump.tmp"] = (stream, index + action[1], index + action[2])
    return ranges


def validate_events(case, result):
    for name, indices in result["fired"].items():
        if not indices:
            raise ValueError(f"{case}: regula {name} nie zostala wyzwolona")
    for a, b in result["model"]["conflicts"]:
        if a + "/" + b not in result["conflicts"]:
            raise ValueError(f"{case}: brak konfliktu {a}/{b}")
    for name, stream, _, action in result["model"]["rules"]:
        if action[0] == "DUMP":
            for i in result["fired"][name]:
                if i + action[1] < 0 or i + action[2] > len(result["rows"][stream]):
                    raise ValueError(f"{case}: niepelny DUMP {name} przy rekordzie {i}")
    if case == "uc10" and result["conflicts"]["cycle/hour_reset"] != [120, 160]:
        raise ValueError("uc10: konflikt D3 poza zaplanowanymi slotami 120 i 160")
    if case == "uc14" and result["conflicts"]["balance/correct"] != list(range(40, 601, 40)):
        raise ValueError("uc14: konflikt korekty poza zaplanowanymi slotami")
    if case == "uc16" and result["rows"]["bucket"][2][1] != I(40):
        raise ValueError("uc16: oczekiwane 40, a nie 42 z dodanego uzupelnienia")


def expected_text(result):
    lines = []
    for stream, rows in result["rows"].items():
        lines.append(dict(kind="schema", stream=stream, types=result["model"]["types"][stream]))
        for index, row in enumerate(rows):
            lines.append(dict(kind="record", stream=stream, index=index,
                              values=[v[1] if v is not None else None for v in row],
                              nulls=sum(1 << i for i, value in enumerate(row) if value is None)))
    for filename, (stream, start, stop) in dump_ranges(result).items():
        lines.append(dict(kind="dump", file=filename, stream=stream, start=start, stop=stop))
    lines.append(dict(kind="events", fired={k: len(v) for k, v in result["fired"].items()},
                      conflicts=result["conflicts"]))
    return "".join(json.dumps(line, separators=(",", ":"), allow_nan=False) + "\n" for line in lines)


def null_masks(path, width, count):
    content = path.read_bytes()
    if content[:8] != bytes(8):
        raise ValueError(f"{path}: nieprawidlowy naglowek metadanych")
    sizes = "Q" if struct.calcsize("P") == 8 else "I"
    entry = struct.Struct("=B" + sizes * 2 + "B")
    if (len(content) - 8) % entry.size:
        raise ValueError(f"{path}: uciete metadane")
    masks = []
    for gap, length, bits, mask in entry.iter_unpack(content[8:]):
        if gap or bits != width or mask >= 1 << width or length == 0 or len(masks) + length > count:
            raise ValueError(f"{path}: nieprawidlowy wpis metadanych {(gap, length, bits, mask)}")
        masks.extend([mask] * length)
    if len(masks) != count:
        raise ValueError(f"{path}: {len(masks)} masek, {count} rekordow")
    return masks


def read_records(path, types, masks=None):
    formats = {"INTEGER": "i", "FLOAT": "f"}
    entry = struct.Struct("=" + "".join(formats[t] for t in types))
    content = path.read_bytes()
    if len(content) % entry.size:
        raise ValueError(f"{path}: uciety rekord binarny")
    count = len(content) // entry.size
    if masks is None:
        masks = [0] * count
    if len(masks) != count:
        raise ValueError(f"{path}: rozna liczba rekordow i masek NULL")
    return [[None if mask & (1 << i) else (types[i], value) for i, value in enumerate(row)]
            for mask, row in zip(masks, entry.iter_unpack(content))]


def check_records(label, actual, expected):
    if len(actual) != len(expected):
        raise ValueError(f"{label}: {len(actual)} rekordow, oczekiwane {len(expected)}")
    for i, (have, want) in enumerate(zip(actual, expected)):
        if have != want:
            raise ValueError(f"{label}[{i}]: silnik {have}, wyrocznia {want}")


def check_storage(directory, result):
    streams = set(result["rows"])
    descriptors = {p.stem for p in directory.glob("*.desc")}
    unknown = descriptors - streams - set(result["model"]["sources"])
    if unknown:
        raise ValueError(f"{directory}: nieoczekiwane strumienie {sorted(unknown)}")
    for stream, rows in result["rows"].items():
        types = result["model"]["types"][stream]
        desc = (directory / (stream + ".desc")).read_text()
        actual_types = re.findall(r"\b(INTEGER|FLOAT|DOUBLE|RATIONAL|UINT|BYTE|STRING)\b", desc)
        if actual_types != types:
            raise ValueError(f"{stream}: typy {actual_types}, oczekiwane {types}")
        masks = null_masks(directory / (stream + ".meta"), len(types), len(rows))
        check_records(stream, read_records(directory / stream, types, masks), rows)
    ranges = dump_ranges(result)
    if {p.name for p in directory.glob("*_dump*")} != set(ranges):
        raise ValueError(f"{directory}: niezgodna lista plikow DUMP, oczekiwane {sorted(ranges)}")
    for filename, (stream, start, stop) in ranges.items():
        rows = result["rows"][stream][start:stop]
        check_records(filename, read_records(directory / filename, result["model"]["types"][stream]), rows)


def check_restart(case_dir, before_dir, after_dir):
    # after_dir to magazyn pierwszego procesu, na ktorym wystartowal drugi. Silnik zaczyna
    # strumienie pochodne od zera, wiec zostaja w nim wylacznie rekordy drugiego przebiegu.
    before = simulate("uc10", case_dir, {"temp.txt": "restart_before.txt"})
    after = simulate("uc10", case_dir, {"temp.txt": "restart_after.txt"})
    continued = simulate("uc10", case_dir, {"temp.txt": "restart_after.txt"}, before["final"])
    check_storage(before_dir, before)
    kept = (after_dir / "status").stat().st_size // struct.calcsize("=iii")
    if kept != len(after["rows"]["status"]):
        raise ValueError(f"restart: {kept} rekordow status po restarcie, oczekiwane {len(after['rows']['status'])}"
                         " - magazyn nie zaczal strumienia od zera")
    check_storage(after_dir, after)
    if before["final"]["COMP"] != I(1):
        raise ValueError("restart: sprezarka nie zostala wlaczona przed zatrzymaniem")
    for i in range(31):
        if after["rows"]["status"][i][1] != I(0) or continued["rows"]["status"][i][1] != I(1):
            raise ValueError(f"restart: utrata zatrzasku nieutrzymana do przekroczenia w rekordzie {i}")
    if any(row[1] != I(1) for row in after["rows"]["status"][31:]):
        raise ValueError("restart: brak odzyskania zatrzasku od rekordu po przekroczeniu")
    print("uc10 restart na tym samym magazynie: utrata stanu i odzyskanie dopiero po przekroczeniu OK")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["check", "files", "restart"])
    parser.add_argument("case_dir", type=Path)
    parser.add_argument("outputs", nargs="*", type=Path)
    args = parser.parse_args()
    case_dir = args.case_dir.resolve()
    if args.action == "restart":
        check_restart(case_dir, *args.outputs)
        return
    case = case_dir.name
    result = simulate(case, case_dir)
    validate_events(case, result)
    if args.action == "files":
        for stream in result["rows"]:
            for suffix in ("", ".desc", ".meta"):
                print(stream + suffix)
        print("\n".join(dump_ranges(result))) if dump_ranges(result) else None
        return
    if (case_dir / "expected.jsonl").read_text() != expected_text(result):
        raise ValueError(f"{case}: zapisany wzorzec jest niezgodny z wyrocznia")
    check_storage(args.outputs[0], result)
    print(f"{case}: pelne wartosci, typy, NULL, kolejnosc i DUMP OK; "
          f"reguly: { {k: len(v) for k, v in result['fired'].items()} }")


if __name__ == "__main__":
    main()
