#!/usr/bin/env python3
"""Wyrocznia niezalezna dla akcji DO SET (#531, etap B, R3).

Liczy kazdy rekord kazdego strumienia planu sekwencyjnie od startu przebiegu, z modelu jezyka,
nie z kodu silnika, i porownuje z wypisem `xtrdb` zebranym przez run.sh (wartosci i typy z `.desc`).
Plan jest tu przepisany recznie na Pythona (MODELS) - wyrocznia nie parsuje RQL.

Model wykonania (kontrakt, nie implementacja):

* zrodlo o interwale D oddaje rekord n (wiersz n pliku) w slocie T = (n + 1) * D; strumien
  pochodny liczy swoj rekord n w tym samym slocie z rekordu n swojego zrodla;
* przebieg `-u` konczy sie po slocie, w ktorym pierwsze zrodlo oddalo swoj ostatni wiersz;
* D1: kazde pole i kazdy warunek w slocie czyta zmienne sprzed slotu; zapisy SET staja sie
  widoczne od nastepnego slotu;
* D3: z kilku zapisow tej samej zmiennej w slocie wygrywa regula pozniejsza w tekscie planu;
* D4: zmienna ma typ literalu z deklaracji; NULL (przepelnienie INTEGER, dzielenie przez zero)
  jest zapisywany i propaguje sie;
* arytmetyka: INTEGER w int32 (poza zakresem NULL, dzielenie z obcieciem), FLOAT w float32,
  operand INTEGER obok FLOAT podnoszony do FLOAT; porownanie daje INTEGER 1/0, warunek NULL
  jest falszywy.

Poza wartosciami wyrocznia pilnuje, ze test cos sprawdza: kazda regula SET odpalila co najmniej
raz, oczekiwane konflikty D3 rzeczywiscie zaszly, a zrzuty DUMP sa dokladnie tam, gdzie regula
odpalila.

    python3 oracle.py <plan.rql> <wypis run.sh>
"""
import math
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

# --- wartosci: (typ, liczba) albo None dla NULL ---------------------------------------------


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


def hysteresis():
    return dict(
        variables={"ALARM": I(0), "COUNT": I(0), "E": F(0.0), "P": I(1)},
        sources={"ch": (1, "signal.txt")},
        streams=[("st", "ch", lambda r, v: [r[0], v["ALARM"], v["COUNT"], v["E"], v["P"]])],
        rules=[
            ("alarm_on", "st", lambda r, v: cmp(">=", r[0], I(250)), ("SET", "ALARM", lambda r, v: I(1))),
            ("alarm_off", "st", lambda r, v: cmp("<", r[0], I(200)), ("SET", "ALARM", lambda r, v: I(0))),
            ("count", "st", lambda r, v: cmp(">", r[0], I(100)), ("SET", "COUNT", lambda r, v: add(v["COUNT"], I(1)))),
            ("ema", "st", ALWAYS, ("SET", "E", lambda r, v: add(mul(v["E"], lit("0.9")), mul(r[0], lit("0.1"))))),
            ("grow", "st", ALWAYS, ("SET", "P", lambda r, v: mul(v["P"], I(1000)))),
        ],
        conflicts=[],
        types={"st": ["INTEGER", "INTEGER", "INTEGER", "FLOAT", "INTEGER"]},
    )


def slot():
    return dict(
        variables={"X": I(0)},
        sources={"src": (1, "slot_src.txt"), "slow": (2, "slot_slow.txt")},
        streams=[
            ("early", "src", lambda r, v: [r[0], v["X"]]),
            ("writer", "early", lambda r, v: [r[0], v["X"]]),
            ("late", "writer", lambda r, v: [r[0], v["X"]]),
            ("other", "slow", lambda r, v: [r[0], v["X"]]),
        ],
        rules=[("w", "writer", ALWAYS, ("SET", "X", lambda r, v: r[0]))],
        conflicts=[],
        types={name: ["INTEGER", "INTEGER"] for name in ("early", "writer", "late", "other")},
    )


def conflict(order):
    rules = {
        "r_down": ("r_down", "down", lambda r, v: cmp(">", r[0], I(0)), ("SET", "X", lambda r, v: r[0])),
        "r_up": ("r_up", "up", lambda r, v: cmp(">", r[0], I(50)), ("SET", "X", lambda r, v: sub(I(0), r[0]))),
    }
    return dict(
        variables={"X": I(0)},
        sources={"src": (1, "conflict.txt")},
        streams=[("up", "src", lambda r, v: [r[0], v["X"]]), ("down", "up", lambda r, v: [r[0], v["X"]])],
        rules=[rules[name] for name in order],
        conflicts=[("r_down", "r_up")],
        types={"up": ["INTEGER", "INTEGER"], "down": ["INTEGER", "INTEGER"]},
    )


def uc10():
    return dict(
        variables={"COMP": I(0), "CYCLES": I(0)},
        sources={"temp": (1, "uc10_temp.txt"), "hour": (10, "uc10_hour.txt")},
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
            ("short_cycling", "status", lambda r, v: cmp(">", v["CYCLES"], I(2)), ("DUMP", 0)),
        ],
        conflicts=[("cycle", "hour_reset")],
        types={"status": ["INTEGER", "INTEGER", "INTEGER"], "hour_tick": ["INTEGER"]},
    )


def uc11():
    mode = lambda m: lambda r, v: cmp("=", v["MODE"], I(m))  # noqa: E731
    return dict(
        variables={"MODE": I(0), "CALM": I(0)},
        sources={"turb": (1, "uc11_turbine.txt")},
        streams=[("state", "turb", lambda r, v: [r[0], r[1], v["MODE"], v["CALM"]])],
        rules=[
            ("m01", "state", lambda r, v: AND(mode(0)(r, v), cmp(">=", r[0], I(35))), ("SET", "MODE", lambda r, v: I(1))),
            ("m12", "state", lambda r, v: AND(mode(1)(r, v), cmp(">=", r[1], I(900))), ("SET", "MODE", lambda r, v: I(2))),
            ("m10", "state", lambda r, v: AND(mode(1)(r, v), cmp("<", r[0], I(30))), ("SET", "MODE", lambda r, v: I(0))),
            ("m23", "state", lambda r, v: AND(mode(2)(r, v), cmp(">=", r[0], I(250))), ("SET", "MODE", lambda r, v: I(3))),
            ("m23_dump", "state", lambda r, v: AND(mode(2)(r, v), cmp(">=", r[0], I(250))), ("DUMP", 3)),
            ("calm_count", "state", lambda r, v: AND(mode(3)(r, v), cmp("<=", r[0], I(200))),
             ("SET", "CALM", lambda r, v: add(v["CALM"], I(1)))),
            ("calm_break", "state", lambda r, v: cmp(">", r[0], I(200)), ("SET", "CALM", lambda r, v: I(0))),
            ("m30", "state", lambda r, v: AND(mode(3)(r, v), cmp(">=", v["CALM"], I(20))), ("SET", "MODE", lambda r, v: I(0))),
        ],
        conflicts=[],
        types={"state": ["INTEGER"] * 4},
    )


def uc12():
    return dict(
        variables={"BASE": F(960.0), "DESAT": I(0), "EVENTS": I(0), "N": I(0)},
        sources={"spo2": (1, "uc12_spo2.txt")},
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
        sources={"gate": (1, "uc14_gate.txt"), "recount": (20, "uc14_recount.txt")},
        streams=[
            ("parking", "gate", lambda r, v: [r[0], r[1], v["OCC"], v["FULL"]]),
            ("recount_tick", "recount", lambda r, v: [r[0]]),
        ],
        rules=[
            ("balance", "parking", ALWAYS, ("SET", "OCC", lambda r, v: add(v["OCC"], sub(r[0], r[1])))),
            ("full_on", "parking", lambda r, v: cmp(">=", v["OCC"], I(40)), ("SET", "FULL", lambda r, v: I(1))),
            ("full_off", "parking", lambda r, v: cmp("<=", v["OCC"], sub(I(40), I(5))), ("SET", "FULL", lambda r, v: I(0))),
            ("correct", "recount_tick", ALWAYS, ("SET", "OCC", lambda r, v: r[0])),
        ],
        conflicts=[("balance", "correct")],
        types={"parking": ["INTEGER"] * 4, "recount_tick": ["INTEGER"]},
    )


MODELS = {
    "hysteresis": hysteresis,
    "slot": slot,
    "conflict_ab": lambda: conflict(["r_down", "r_up"]),
    "conflict_ba": lambda: conflict(["r_up", "r_down"]),
    "uc10": uc10,
    "uc11": uc11,
    "uc12": uc12,
    "uc14": uc14,
}

# --- symulacja -----------------------------------------------------------------------------


def read_source(name):
    return [[I(int(x)) for x in line.split()] for line in (HERE / name).read_text().splitlines() if line.strip()]


def simulate(model):
    data = {name: read_source(path) for name, (_, path) in model["sources"].items()}
    interval = {name: delta for name, (delta, _) in model["sources"].items()}
    for name, source, _ in model["streams"]:
        interval[name] = interval[source]
    end = min(len(rows) * interval[name] for name, rows in data.items())

    variables = dict(model["variables"])
    output = {name: [] for name, _, _ in model["streams"]}
    fired = {rule[0]: [] for rule in model["rules"]}  # numery rekordow, w ktorych regula odpalila
    together = set()
    for t in range(1, end + 1):
        current = {}
        for name, rows in data.items():
            if t % interval[name] == 0:
                current[name] = rows[t // interval[name] - 1]
        for name, source, fields in model["streams"]:
            if source in current:
                current[name] = fields(current[source], variables)
                output[name].append(current[name])
        writes = {}  # zmienna -> wartosc; reguly w kolejnosci tekstu, wiec pozniejsza nadpisuje (D3)
        writers = {}
        for name, stream, condition, action in model["rules"]:
            if stream not in current or not truth(condition(current[stream], variables)):
                continue
            fired[name].append(len(output[stream]) - 1)
            if action[0] == "SET":
                writes[action[1]] = action[2](current[stream], variables)
                writers.setdefault(action[1], []).append(name)
        for names in writers.values():
            together.update((a, b) for a in names for b in names if a != b)
        variables.update(writes)  # D1: koniec slotu
    return output, fired, together


# --- porownanie z wypisem silnika --------------------------------------------------------


def read_output(path):
    records, types, dumps, current = {}, {}, [], None
    for line in Path(path).read_text().splitlines():
        if line.startswith("== dumps"):
            current = "__dumps__"
        elif line.startswith("== desc "):
            current = ("desc", line.split()[2])
            types[current[1]] = []
        elif line.startswith("== "):
            current = line.split()[1]
            records[current] = []
        elif current == "__dumps__" and line.strip():
            dumps.append(line.strip())
        elif isinstance(current, tuple):
            types[current[1]] += re.findall(r"\b(INTEGER|FLOAT|DOUBLE|RATIONAL|BYTE|UINT|STRING)\b", line)
        elif current and line.startswith("{"):
            records[current].append([m.group(1) for m in re.finditer(r"\w+?_\d+:(\S*)", line)])
    return records, types, dumps


def matches(expected, text):
    if expected is None:
        return text == "null"
    if text == "null":
        return False
    if expected[0] == "INTEGER":
        return int(text) == expected[1]
    return math.isclose(float(text), expected[1], rel_tol=1e-5, abs_tol=1e-6)


def show(value):
    return "null" if value is None else f"{value[0]}({value[1]})"


def check(plan_path, out_path):
    stem = Path(plan_path).stem
    model = MODELS[stem]()
    expected, fired, together = simulate(model)
    records, types, dumps = read_output(out_path)
    errors = []

    for name, rows in expected.items():
        got = records.get(name)
        if got is None:
            errors.append(f"{name}: brak wypisu strumienia")
            continue
        if types.get(name) != model["types"][name]:
            errors.append(f"{name}: typy {types.get(name)}, oczekiwane {model['types'][name]}")
        if len(got) != len(rows):
            errors.append(f"{name}: {len(got)} rekordow, oczekiwane {len(rows)}")
        for n, (want, have) in enumerate(zip(rows, got)):
            if len(want) != len(have) or not all(matches(w, h) for w, h in zip(want, have)):
                errors.append(f"{name}[{n}]: silnik {have}, wyrocznia {[show(w) for w in want]}")
                break  # pierwszy rozjazd strumienia wystarcza, dalsze sa jego skutkiem

    # Test, ktory nic nie wyzwala, nic nie sprawdza.
    for name, stream, _, action in model["rules"]:
        if action[0] == "SET" and not fired[name]:
            errors.append(f"regula SET {name} nie odpalila ani razu - dane jej nie wyzwalaja")
    for a, b in model["conflicts"]:
        if (a, b) not in together:
            errors.append(f"reguly {a} i {b} ani razu nie pisaly w tym samym slocie - konflikt D3 niesprawdzony")

    # Zrzut powstaje dla reguly DUMP, ktora odpalila z pelnym zakresem w przod przed koncem danych.
    for name, stream, _, action in model["rules"]:
        if action[0] != "DUMP":
            continue
        complete = [n for n in fired[name] if n + action[1] < len(expected[stream])]
        if len(complete) != len(fired[name]):
            errors.append(f"regula {name} odpalila za blisko konca danych - zrzut niejednoznaczny")
        present = any(f"{stream}_{name}_dump" in d for d in dumps)
        if bool(complete) != present:
            errors.append(f"zrzut reguly {name}: silnik {'jest' if present else 'brak'}, wyrocznia "
                          f"{'jest' if complete else 'brak'} (odpalenia w rekordach {fired[name]})")
        if not complete:
            errors.append(f"regula DUMP {name} nie odpalila - dane jej nie wyzwalaja")

    for e in errors:
        print(f"WYROCZNIA {stem}: {e}")
    return not errors


if __name__ == "__main__":
    sys.exit(0 if check(sys.argv[1], sys.argv[2]) else 1)
