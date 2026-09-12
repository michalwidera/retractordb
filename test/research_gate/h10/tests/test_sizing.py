#!/usr/bin/env python3
"""Bramka wymiarowania przebiegu — czy budżet `-m` wystarcza, żeby każdy węzeł
doszedł do RECORDS rekordów.

Poziom nie orzeka o silniku i nie należy do żadnej hipotezy. Pilnuje APARATURY:
`execute.horizon_of()` i `execute.wakeup_budget()` wymiarują każdy przebieg
end-to-end, a przebieg wymierzony za krótko daje artefakt bez rekordów — co
skrypty czytają jako rozbieżność treści (`run_mapping_gate.py`) albo jako objaw
niedomiaru pojemności (`check_agse_capacity.py`). Jedno i drugie jest wtedy
granicą aparatury podaną jako wynik o silniku; dokładnie to zatrzymało poziom
bramki odwzorowania w K24f i dokładnie tego ten poziom ma nie dopuścić ponownie.

Trzy poziomy, żaden nie powtarza wzoru, który sprawdza:

1. **wystarczalność wobec silnika** — plany o znanej trudności wymiarowania
   uruchomione na policzonym budżecie; każdy węzeł niebędący źródłem musi mieć
   co najmniej RECORDS rekordów w artefakcie;
2. **budżet slotów jako ograniczenie górne** — liczba RÓŻNYCH chwil tyknięcia
   w horyzoncie, wyliczona wprost przez wypisanie tych chwil, musi mieścić się
   w `wakeup_budget()`. Slot jest chwilą, w której tyka co najmniej jeden
   strumień, więc suma tyknięć wszystkich strumieni ma być bezpieczna;
3. **moc detekcyjna** — wzór sprzed naprawy K24f (`(RECORDS+8)*spread+24`,
   budżet liczony taktem najszybszego strumienia) musi być niewystarczający co
   najmniej na jednym z tych planów. Gdyby wystarczał na wszystkich, poziom 1
   przechodziłby niezależnie od naprawy i nie znaczyłby nic; taki wynik kończy
   się kodem 2 jako błąd aparatury.

    python3 tests/test_sizing.py [<xretractor>]
"""

import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "oracle"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import engine as E  # noqa: E402
import execute as X  # noqa: E402
import model as M  # noqa: E402
import plan as P  # noqa: E402
from engine import EngineError  # noqa: E402
from hand_cases import hand_cases  # noqa: E402

RECORDS = 12
SCALE = Fraction(1, 200)        # docelowy interwał najszybszego strumienia
MAX_INSTANTS = 200_000          # zabezpieczenie poziomu 2 przed planem o skrajnym ilorazie


def engine_cases():
    """Plany o znanej trudności wymiarowania — po jednej trudności na plan."""
    cases = []

    # Łańcuch `>N`: origin narasta wzdłuż planu (8, 13, 26, 34), więc najgłębszy
    # węzeł zaczyna emitować dopiero po 34 taktach. To kształt, na którym
    # wymiarowanie sprzed K24f kończyło przebieg przed pierwszym rekordem.
    a = P.make_source("s0", 1, 1)
    n0 = P.make_shift("n0", a, 8)
    n1 = P.make_shift("n1", n0, 5)
    n2 = P.make_shift("n2", n1, 13)
    cases.append(("łańcuch >8 >5 >13 >8",
                  P.Plan(nodes=(a, n0, n1, n2, P.make_shift("n3", n2, 8)))))

    # Plan wielotaktowy: przeplot 1 # 1/3 daje trzeci interwał, różny od obu
    # składowych. Liczba pobudek jest tu WIĘKSZA niż liczba taktów najszybszego
    # strumienia — druga połowa naprawy K24f, niezależna od origin.
    b0 = P.make_source("s0", 1, 1)
    b1 = P.make_source("s1", Fraction(1, 3), 1)
    h = P.make_hash("n0", b0, b1)
    cases.append(("przeplot 1 # 1/3 z projekcją",
                  P.Plan(nodes=(b0, b1, h, P.make_pass("n1", h)))))

    # Okno `@` nad przesunięciem: origin bierze się i z rozpiętości okna,
    # i z `>N`, a interwał zmienia się względem źródła.
    c0 = P.make_source("s0", Fraction(1, 2), 2)
    moved = P.make_shift("n0", c0, 6)
    cases.append(("@(1,4) nad >6",
                  P.Plan(nodes=(c0, moved, P.make_agse("n1", moved, 1, 4)))))

    return cases


def sized(item):
    """Plan przeskalowany do wykonania wraz z jego budżetem — jak w skryptach."""
    scaled = P.rescale(item, SCALE / P.fastest(item))
    results = M.evaluate(item, convention=M.C1)
    origins = {r.name: r.origin for r in results}
    tails = {r.name: r.tail for r in results}
    horizon = X.horizon_of(scaled, origins, tails, RECORDS)
    return scaled, horizon, X.wakeup_budget(scaled, horizon) + 24


def legacy_budget(scaled):
    """Wzór sprzed 2026-09-12 — wyłącznie do pomiaru mocy detekcyjnej."""
    spread = P.slowest(scaled) / P.fastest(scaled)
    return int((RECORDS + 8) * spread) + 24


def record_counts(scaled, binary, workdir, loops):
    X.run_plan(scaled, binary, workdir, loops=loops, records=1024)
    counts = {}
    for node in scaled.nodes:
        if node.kind == P.SOURCE:
            continue
        payload = Path(workdir) / node.name
        if not payload.exists():
            counts[node.name] = 0
            continue
        counts[node.name] = len(X.decode_payload(payload, P.payload_words(node)))
    return counts


def level_sufficiency(binary, workroot):
    """Poziom 1 — policzony budżet musi doprowadzić KAŻDY węzeł do RECORDS."""
    failures = []
    for number, (label, item) in enumerate(engine_cases()):
        scaled, horizon, loops = sized(item)
        counts = record_counts(scaled, binary, workroot / f"case{number}", loops)
        short = {name: got for name, got in counts.items() if got < RECORDS}
        print(f"  {label:28s} horyzont {float(horizon):.3f} s, -m {loops:5d}, "
              f"rekordów {min(counts.values())}..{max(counts.values())}")
        for name, got in sorted(short.items()):
            failures.append(f"{label}: {name} ma {got} rekordów, wymagane {RECORDS}")
    return failures


def level_slot_bound():
    """Poziom 2 — `wakeup_budget` musi ograniczać liczbę RÓŻNYCH chwil z góry.

    Chwile są wypisywane, nie liczone wzorem: gdyby poziom liczył je tą samą
    sumą, sprawdzałby, że wzór równa się sobie.
    """
    failures = []
    corpus = [(label, item) for label, item, _, _, _ in hand_cases()]
    corpus += engine_cases()
    checked = skipped = 0
    for label, item in corpus:
        scaled, horizon, _ = sized(item)
        budget = X.wakeup_budget(scaled, horizon)
        if budget > MAX_INSTANTS:
            skipped += 1
            continue
        instants = set()
        for node in scaled.nodes:
            instants.update(Fraction(k) * node.delta
                            for k in range(int(horizon / node.delta) + 1))
        checked += 1
        if len(instants) > budget:
            failures.append(f"{label}: chwil {len(instants)}, budżet {budget}")
    print(f"  planów sprawdzonych: {checked}, pominiętych (ponad {MAX_INSTANTS} chwil): {skipped}")
    if checked == 0:
        failures.append("żaden plan nie wszedł do poziomu — ograniczenie nie zostało sprawdzone")
    return failures


def level_power(binary, workroot):
    """Poziom 3 — stary wzór musi być na tych planach niewystarczający."""
    witnesses = []
    for number, (label, item) in enumerate(engine_cases()):
        scaled, _, _ = sized(item)
        loops = legacy_budget(scaled)
        counts = record_counts(scaled, binary, workroot / f"legacy{number}", loops)
        short = {name: got for name, got in counts.items() if got < RECORDS}
        if short:
            name, got = sorted(short.items())[0]
            witnesses.append(f"{label}: dawne -m {loops} daje {name} = {got} rekordów")
    return witnesses


def main(argv):
    binary = E.resolve_binary(argv[1] if len(argv) > 1 else None)
    workroot = ROOT / "work" / "sizing"

    print("poziom 1 — wystarczalność budżetu wobec silnika")
    try:
        failures = level_sufficiency(binary, workroot)
    except EngineError as exc:
        print(f"BRAMKA WYMIAROWANIA: BŁĄD APARATURY — {exc}")
        return 2

    print("poziom 2 — budżet slotów jako ograniczenie górne")
    failures += level_slot_bound()

    print("poziom 3 — moc detekcyjna wobec wzoru sprzed K24f")
    try:
        witnesses = level_power(binary, workroot)
    except EngineError as exc:
        print(f"BRAMKA WYMIAROWANIA: BŁĄD APARATURY — {exc}")
        return 2
    for witness in witnesses:
        print(f"  {witness}")
    if not witnesses:
        print("BRAMKA WYMIAROWANIA: BŁĄD APARATURY — zerowa moc detekcyjna, "
              "dawne wymiarowanie wystarcza na każdym planie tego poziomu")
        return 2

    for failure in failures:
        print(f"  NIEDOMIAR {failure}")
    if failures:
        print(f"BRAMKA WYMIAROWANIA: OBLAŁA ({len(failures)})")
        return 1
    print("BRAMKA WYMIAROWANIA: PRZESZŁA")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
