#!/usr/bin/env python3
"""P1 — wyprowadzenie odległości wstecznej i porównanie z modelem pojemności.

Kroki 2.1–2.2 z `plan-naprawy-defektow.md` §7. Skrypt liczy dwie rzeczy dla
każdej pary (konsument, składowa) w korpusie K24:

  * **odległość żądaną** — ile rekordów wstecz od czoła składowej sięga
    konsument w chwili, w której faktycznie czyta. Wyprowadzona z modelu
    zdarzeniowego, tym samym odwzorowaniem rekordów co oracle;
  * **pojemność zapewnioną** — replika `compiler::computeRequiredCapacities()`.

Defektem jest każda para, w której zapewniona pojemność nie pokrywa żądanej
odległości. Skrypt nie proponuje poprawki — dostarcza liczb do kroku 2.3.

Model odległości
----------------
Konsument o ogonie `W_out` przetwarza slot `n` w chwili

    t_n = (n + 1 + W_out) * Delta_out.

Składowa o ogonie `W_src` wyemitowała do tej chwili

    count_src(t_n) = floor(t_n / Delta_src) - W_src   rekordów,

więc rekord o indeksie `idx` leży `count_src(t_n) - 1 - idx` pozycji od czoła.
Pojemność musi pokryć maksimum tej wielkości po slotach, powiększone o jeden
(bufor musi pomieścić oba końce zakresu).

Ogony pochodzą tu z **postaci zamkniętej silnika**, a nie z oracle'a: pojemność
ma pokryć zachowanie silnika, jakie jest, a nie jakie powinno być.
"""

import argparse
import collections
import csv
import sys
from fractions import Fraction
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "oracle"))

import closedform as C  # noqa: E402
import model as M  # noqa: E402
from generator import generate  # noqa: E402
from plan import (ADD, AGSE, HASH, NTHETA, PASS, REDUCE, SHIFT, SOURCE, SUB,  # noqa: E402
                  THETA)

JUNCTION_HISTORY = 4      # kJunctionHistory z compiler.cpp
PROBE_SLOTS = 96          # sondowanie slotów konsumenta

# Deklaracja wyprzedza konsumenta o rekord uzbrojony przy otwarciu storage
# i o zerowy prefetch, więc jej czoło jest dalej, niż wynika z samego czasu.
# Człon wykryty empirycznie: bez niego model uznawał plany 10 i 32 podpróby za
# pokryte, a silnik emitował tam rekord all-NULL.
#
# UWAGA — człon nie jest jednolity. Zastosowany do WSZYSTKICH ścieżek odczytu
# daje przewidywania niedomiaru dla `>N`, `#`, `Θ` i `~Θ`, których nie
# potwierdza żadna obserwacja: w podpróbie 112 planów wykonanych end-to-end
# te klasy nie dały ani jednego rekordu all-NULL ani awarii. Dla `>N` jest to
# wytłumaczalne — `fetchBack` adresuje wstecznie, więc wyprzedzenie czoła się
# skraca. Dla `#`, `Θ` i `~Θ` pozostaje nierozstrzygnięte i jest raportowane
# osobno jako przewidywanie bez potwierdzenia (patrz sekcja "nierozstrzygnięte"
# w wyniku skryptu), a nie jako defekt.
#
# `+` dołączył do listy potwierdzonych w etapie 4: po zmianie odwzorowania
# bramka odwzorowania nie pokazała ani jednego rekordu all-NULL pochodzącego
# ze składowej DEKLAROWANEJ. Dwa rekordy z NULL, które w bramce zostały,
# pochodzą ze składowych OBLICZANYCH (plany 55 i 97) i są skutkiem zaniżonego
# ogona `+`, nie pojemności — dla strumieni obliczanych limit pojemności nie
# wiąże (patrz komentarz przy `binding` w main()).
DECLARATION_PREFETCH = 2
PREFETCH_CONFIRMED = (SUB, AGSE, ADD)

# Operatory faktycznie sięgające do historii składowej: `>N` przez fetchBack,
# `#`, `&`, `%`, `-`, `+` przez fetchForward, `@` przez constructAgsePayload.
# Projekcja i redukcje czytają wyłącznie bieżący payload składowej
# (dataModel.cpp:206-214, :259-266), więc nie stawiają wymagań pojemnościowych
# i nie wchodzą do tej analizy.
#
# `+` dołączył tu w etapie 4 (K24/P2 wariant A): odwzorowanie z Definicji sumy
# strumieni adresuje składowe po indeksie ⌊n·Δout/Δsrc⌋ przez fetchForward,
# zamiast brać bieżący payload obu składowych.
HISTORY_READERS = (SHIFT, HASH, THETA, NTHETA, SUB, AGSE, ADD)


def _floor(value):
    return value.numerator // value.denominator


def _ceil(value):
    return -((-value.numerator) // value.denominator)


def deepest_indexes(node, children, n):
    """Najstarszy (najgłębiej wstecz) indeks czytany z każdej składowej.

    Dla większości operatorów jest to ten sam indeks, co w `dependencies()`.
    AGSE jest wyjątkiem: o dostępności decyduje pole najnowsze, a o pojemności
    historii — najstarsze pole okna.
    """
    if node.kind == AGSE:
        src = children[0]
        step, length = node.param
        # Okno stemplowane KOŃCEM przedziału: najstarsze pole leży w pozycji
        # n*step-(|L|-1), a nie n*step. Dzielenie Pythona zaokrągla w dół także
        # dla ujemnych liczników, więc pozycja sprzed początku źródła trafia do
        # rekordu ujemnego — tak jak floorDiv() w silniku.
        return {src.name: (n * step - abs(length) + 1) // src.width}
    result = {}
    for child_name, index, _delay in M.dependencies(node, children, n):
        result[child_name] = min(result.get(child_name, index), index)
    return result


def required_lookback(plan, tails, origins, probe=PROBE_SLOTS):
    """{(konsument, składowa): maksymalna odległość wsteczna}.

    Sondowanie zaczyna się od początku logicznego konsumenta: sloty przed
    origin nie są rekordami, więc nie stawiają żadnego wymagania pojemnościowego.
    """
    demand = {}
    for node in plan.nodes:
        if node.kind not in HISTORY_READERS:
            continue
        children = [plan.by_name(name) for name in node.children]
        for n in range(origins[node.name], origins[node.name] + probe):
            moment = (Fraction(n) + 1 + tails[node.name]) * node.delta
            for child_name, index in deepest_indexes(node, children, n).items():
                child = plan.by_name(child_name)
                count = _floor(moment / child.delta) - tails[child_name]
                if child.kind == SOURCE and node.kind in PREFETCH_CONFIRMED:
                    count += DECLARATION_PREFETCH
                back = count - 1 - index
                key = (node.name, child_name)
                if back > demand.get(key, -1):
                    demand[key] = back
    return demand


def engine_capacity(plan, tails, origins):
    """Replika compiler::computeRequiredCapacities() (compiler.cpp:738-858)."""
    cap = {node.name: 1 for node in plan.nodes if node.kind == SOURCE}

    def bump(name, value):
        cap[name] = max(cap.get(name, 1), value)

    for node in plan.nodes:
        if node.kind == SOURCE:
            continue
        children = [plan.by_name(name) for name in node.children]
        first = children[0]
        if node.kind == PASS:
            continue
        if node.kind == SHIFT:
            bump(first.name, node.param + 1)
        elif node.kind == AGSE:
            # Issue 227: postać zamknięta zastąpiona DOKŁADNYM przeglądem jednego
            # okresu fazowego od origin. Obie części dystansu zmieniają się
            # okresowo, więc maksimum na jednym okresie jest maksimum globalnym.
            step, length = node.param
            period = first.width // gcd(first.width, step)
            distance = 0
            for n in range(origins[node.name], origins[node.name] + period):
                newest = ((n + 1 + tails[node.name]) * step) // first.width - tails[first.name] - 1
                oldest = (n * step - abs(length) + 1) // first.width
                distance = max(distance, newest - oldest)
            required = distance + (DECLARATION_PREFETCH if first.kind == SOURCE else 1)
            bump(first.name, max(required, 1))
        elif node.kind in (HASH, THETA, NTHETA):
            targets = children if node.kind == HASH else [first]
            for child in targets:
                delayed = _ceil(Fraction(tails[node.name]) * node.delta / child.delta) - tails[child.name] + 2
                bump(child.name, max(JUNCTION_HISTORY, delayed))
        elif node.kind == SUB:
            ratio = node.delta / first.delta
            if first.kind == SOURCE:
                required = max(_floor(Fraction(tails[node.name]) * ratio) + 2,
                               _floor(Fraction(1 + tails[node.name]) * ratio) + DECLARATION_PREFETCH)  # K24/P1
            else:
                required = _ceil(Fraction(tails[node.name]) * ratio - tails[first.name])
            bump(first.name, max(required, 1))
        elif node.kind == ADD:
            # K24/P2 wariant A: max_n [floor((n+1+Wout)*ratio) - floor(n*ratio)]
            # = ceil((1+Wout)*ratio) dla ratio = Delta_out/Delta_src <= 1.
            for child in children:
                ratio = node.delta / child.delta
                required = _ceil(Fraction(1 + tails[node.name]) * ratio) - tails[child.name]
                if child.kind == SOURCE:
                    required += DECLARATION_PREFETCH
                bump(child.name, max(required, 1))
        elif node.kind == REDUCE:
            continue
    return cap


def analyse(seed, count, probe=PROBE_SLOTS):
    rows = []
    for index, (stratum, plan) in enumerate(generate(seed, count)):
        tails = C.evaluate(plan)
        origins = C.evaluate_origins(plan)
        try:
            demand = required_lookback(plan, tails, origins, probe)
        except M.OracleError:
            continue
        cap = engine_capacity(plan, tails, origins)
        for (consumer, child), back in demand.items():
            node = plan.by_name(consumer)
            provided = cap.get(child)
            if provided is None:
                continue
            rows.append({
                "plan": index, "stratum": stratum, "consumer": consumer,
                "kind": node.kind, "child": child,
                "child_declared": int(plan.by_name(child).kind == SOURCE),
                "needed": back + 1, "provided": provided,
                "deficit": (back + 1) - provided,
            })
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=20260807)
    parser.add_argument("--count", type=int, default=10_010)
    parser.add_argument("--out", default=str(ROOT / "raw" / "capacity.csv"))
    args = parser.parse_args()

    rows = analyse(args.seed, args.count)
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    # Limit pojemności wiąże wyłącznie dla składowych DEKLAROWANYCH: strażniki
    # w dataModel::fetchForward/fetchBack i w storage::revRead sprawdzają
    # historię tylko pod warunkiem isDeclared(). Strumienie obliczane czytają
    # z magazynu, który zachowuje komplet rekordów. Zawężenie potwierdzone
    # empirycznie: bez niego model daje 4 fałszywe alarmy na planach, które
    # przeszły bramkę odwzorowania czysto; z nim — zero.
    binding = [row for row in rows if row["child_declared"] == 1]
    per = collections.defaultdict(lambda: {"n": 0, "bad": 0, "worst": 0, "witness": None})
    for row in binding:
        entry = per[row["kind"]]
        entry["n"] += 1
        if row["deficit"] > 0:
            entry["bad"] += 1
            if row["deficit"] > entry["worst"]:
                entry["worst"] = row["deficit"]
                entry["witness"] = row
    print(f"par (konsument, składowa): {len(rows)}, w tym wiążących (składowa deklarowana): {len(binding)}\n")
    print(f"{'klasa':8s} {'par':>7s} {'niedomiar':>10s} {'udział':>8s} {'najgorszy':>10s}  świadek")
    for kind in sorted(per, key=lambda key: -per[key]["bad"]):
        entry = per[kind]
        witness = entry["witness"]
        mark = (f"plan {witness['plan']} {witness['consumer']}<-{witness['child']} "
                f"trzeba {witness['needed']}, jest {witness['provided']}") if witness else ""
        print(f"{kind:8s} {entry['n']:7d} {entry['bad']:10d} {entry['bad'] / entry['n']:7.1%} "
              f"{entry['worst']:10d}  {mark}")


if __name__ == "__main__":
    main()
