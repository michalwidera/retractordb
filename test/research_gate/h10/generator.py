#!/usr/bin/env python3
"""Generator korpusu planów K24 — zamrożony, deterministyczny, stratyfikowany.

Generator produkuje wyłącznie plany poprawne. Plan odrzucony przez kompilator
jest błędem aparatury i zatrzymuje iterację; nie wolno go cicho pominąć.

Stratyfikacja: dziewięć klas operatorów i pięć predeklarowanych klas trudnych.
Każda strata dostaje równą liczbę planów, więc przy N = 10 010 każda ma 715
instancji — powyżej predeklarowanego progu 500.

Klasy trudne:
  HC_NONINT  — `#` o ilorazie taktów p/q z q niedzielącym p (klasa członu (b))
  HC_SHIFT_UNDER_HASH — `>` zagnieżdżone pod `#`
  HC_INT     — `#` o ilorazie całkowitym (kontrola negatywna członu (b))
  HC_SINGLE  — plan jednotaktowy bez `#` (kontrola negatywna członu (b))
  HC_DEEP    — kompozycja głębokości >= 4

Strata WINDOW (okno rekordowe w liście SELECT) jest DOŁOŻONA i domyślnie wyłączona:
wchodzi tylko przez `generate(..., strata=STRATA_WITH_WINDOW)`. Powód przy definicji
`STRATA_WITH_WINDOW` — dołożenie straty do `STRATA` zmieniłoby korpus zamrożony w K24e.
"""

import random
from fractions import Fraction

from plan import (ADD, AGSE, HASH, NTHETA, PASS, REDUCE, SHIFT, SOURCE, SUB,
                  THETA, WINDOW, WINDOW_REDUCERS, Plan, PlanError, make_add,
                  make_agse, make_hash, make_ntheta, make_pass, make_reduce,
                  make_shift, make_source, make_sub, make_theta, make_window,
                  reduced_ratio)

INTERVALS = tuple(Fraction(text) for text in (
    "1", "1/2", "1/3", "1/4", "1/5", "1/8", "1/10", "1/16", "1/100",
    "2/3", "2/5", "3/8", "3/10", "5/8", "4/25", "147/1000"))

WIDTHS = (1, 2, 3, 4)
SHIFTS = (1, 2, 3, 5, 8)
AGSE_STEPS = (1, 2, 3, 4)
AGSE_LENGTHS = (1, 2, 3, 4, -2, -3, -4)
SUB_FACTORS = tuple(Fraction(text) for text in ("1", "2", "3", "4", "5/2", "7/3", "3/2", "8/3"))

# Ograniczenie licznika i mianownika interwału. Silnik liczy interwały
# w boost::rational<int>, a wzory operatorów mnożą licznik przez licznik
# i mianownik przez mianownik ((D_a*D_b)/|D_a-D_b| dla rozplotu, analogicznie
# dla przeplotu). Iloczyn dwóch wartości poniżej 40 000 mieści się w int32
# z zapasem; wyżej silnik cicho przepełnia typ i odrzuca plan. Obserwacja
# odnotowana w REPORT.md §4 wraz z reproducerem.
MAX_TERMS = 40_000
MAX_WIDTH = 8
MAX_ATTEMPTS = 400

HC_NONINT = "HC_NONINT"
HC_SHIFT_UNDER_HASH = "HC_SHIFT_UNDER_HASH"
HC_INT = "HC_INT"
HC_SINGLE = "HC_SINGLE"
HC_DEEP = "HC_DEEP"

HARD_CLASSES = (HC_NONINT, HC_SHIFT_UNDER_HASH, HC_INT, HC_SINGLE, HC_DEEP)
OPERATOR_STRATA = (PASS, SHIFT, HASH, ADD, SUB, THETA, NTHETA, AGSE, REDUCE)
STRATA = OPERATOR_STRATA + HARD_CLASSES

# Strata okna REKORDOWEGO — DOLOZONA, nie wlaczona domyslnie.
#
# Strata przydzielana jest ROTACYJNIE po indeksie planu (`STRATA[index % len(STRATA)]`),
# a kazdy plan ciagnie z tego samego `random.Random(seed)`. Dopisanie pietnastej straty
# do `STRATA` przesuwa wiec WSZYSTKIE losowania i daje dla zamrozonych ziaren INNY
# korpus — a `compare_regimes.py` zwraca kod 2 przy jakiejkolwiek zmianie zestawu klas,
# czyli `ninja test_gate` przestalby przechodzic z powodu korpusu, nie silnika.
# Korpus zamrozony w K24e musi zostac bajtowo ten sam, wiec strata wchodzi wylacznie
# przez jawne `strata=` w generate().
STRATA_WITH_WINDOW = STRATA + (WINDOW,)

# Szerokosci okna rekordowego i liczba agregatow w liscie SELECT. Gorna granica
# szerokosci jest ta sama co dla okna AGSE (MAX_WIDTH), a liczba agregatow jest mala,
# bo jej jedynym zadaniem jest wejsc w galez wyboru NAJSZERSZEGO okna.
WINDOW_WIDTHS = (1, 2, 3, 5, 8)
WINDOW_AGGREGATE_COUNTS = (1, 1, 2, 3)


class GeneratorError(RuntimeError):
    """Generator nie potrafił wyprodukować planu dla straty — błąd aparatury."""


def _bounded(delta):
    return (abs(delta.numerator) <= MAX_TERMS and delta.denominator <= MAX_TERMS)


def _rational_ok(node):
    return _bounded(node.delta)


def _candidates(kind, nodes, rng):
    """Losowa próba konstrukcji węzła ``kind`` nad istniejącymi węzłami."""
    name = f"n{sum(1 for node in nodes if node.kind != SOURCE)}"
    # `WINDOW` jest liściem tak samo jak `REDUCE`: oba wydają pola RATIONAL, a
    # `payload_words()` i `content()` liczą słowa INTEGER-owe źródła. Wpuszczenie
    # takiego węzła jako producenta dałoby węzeł o szerokości 1 nad rekordem
    # dwusłowowym, czyli po cichu zły model treści — i to jest powód wykluczenia,
    # nie żadne ograniczenie reguły origin (okno nad oknem kompiluje się i liczy
    # origin poprawnie, zmierzone; ten kształt pilnuje przypadek ręczny).
    usable = [node for node in nodes if node.kind not in (REDUCE, WINDOW)]
    if not usable:
        usable = list(nodes)

    if kind == PASS:
        return make_pass(name, rng.choice(usable))
    if kind == SHIFT:
        return make_shift(name, rng.choice(usable), rng.choice(SHIFTS))
    if kind == REDUCE:
        source = rng.choice([node for node in usable if node.width >= 2] or usable)
        return make_reduce(name, source, rng.choice(("sumc", "avg", "min", "max")))
    if kind == HASH:
        left = rng.choice(usable)
        same = [node for node in usable if node.width == left.width and node.name != left.name]
        if not same:
            raise PlanError("brak drugiego argumentu o zgodnej szerokości")
        return make_hash(name, left, rng.choice(same))
    if kind == ADD:
        left = rng.choice(usable)
        other = [node for node in usable if node.name != left.name
                 and node.width + left.width <= MAX_WIDTH]
        if not other:
            raise PlanError("brak drugiego argumentu sumy")
        return make_add(name, left, rng.choice(other))
    if kind == SUB:
        source = rng.choice(usable)
        return make_sub(name, source, source.delta * rng.choice(SUB_FACTORS))
    if kind in (THETA, NTHETA):
        source = rng.choice(usable)
        factor = rng.choice(SUB_FACTORS)
        other = source.delta * factor
        maker = make_theta if kind == THETA else make_ntheta
        return maker(name, source, other)
    if kind == AGSE:
        source = rng.choice(usable)
        length = rng.choice(AGSE_LENGTHS)
        if abs(length) > MAX_WIDTH:
            raise PlanError("okno AGSE za szerokie")
        return make_agse(name, source, rng.choice(AGSE_STEPS), length)
    if kind == WINDOW:
        source = rng.choice(usable)
        count = rng.choice(WINDOW_AGGREGATE_COUNTS)
        widths = tuple(rng.choice(WINDOW_WIDTHS) for _ in range(count))
        return make_window(name, source, rng.choice(WINDOW_REDUCERS), widths)
    raise PlanError(f"nieznana klasa {kind}")


def _hard_classes_of(nodes):
    found = set()
    lookup = {node.name: node for node in nodes}
    operators = [node for node in nodes if node.kind != SOURCE]
    if len(operators) >= 4:
        found.add(HC_DEEP)
    intervals = {node.delta for node in nodes}
    if len(intervals) == 1 and not any(node.kind == HASH for node in operators):
        found.add(HC_SINGLE)
    for node in operators:
        if node.kind != HASH:
            continue
        left, right = (lookup[name] for name in node.children)
        p, q = reduced_ratio(left.delta, right.delta)
        found.add(HC_INT if p % q == 0 else HC_NONINT)
        for child in node.children:
            if lookup[child].kind == SHIFT:
                found.add(HC_SHIFT_UNDER_HASH)
    return found


def build(rng, stratum):
    """Plan zawierający cechę wskazanej straty."""
    for _ in range(MAX_ATTEMPTS):
        source_count = rng.randint(1, 3)
        if stratum in (HASH, ADD, HC_NONINT, HC_INT, HC_SHIFT_UNDER_HASH):
            source_count = max(source_count, 2)
        if stratum == HC_SINGLE:
            base = rng.choice(INTERVALS)
            deltas = [base] * source_count
        else:
            deltas = [rng.choice(INTERVALS) for _ in range(source_count)]
        width = rng.choice(WIDTHS)
        nodes = [make_source(f"s{i}", deltas[i], width) for i in range(source_count)]

        depth = rng.randint(1, 6)
        if stratum == HC_DEEP:
            depth = rng.randint(4, 6)
        if stratum == HC_SINGLE:
            depth = rng.randint(1, 3)

        allowed = list(OPERATOR_STRATA)
        if stratum == HC_SINGLE:
            allowed = [PASS, SHIFT, REDUCE, SUB, AGSE]
        # Strata okna buduje plan MIESZANY z wymogiem obecności okna, a nie łańcuch
        # samych okien — okno jest liściem (patrz _candidates), więc łańcuch nie ma
        # jak powstać, a kompozycja `okno nad <dowolnym operatorem>` jest tym, o co
        # pyta twierdzenie: reguła musi trafiać nad producentem o dowolnym origin.
        if stratum == WINDOW:
            allowed = list(OPERATOR_STRATA) + [WINDOW]
        ok = True
        for _step in range(depth):
            kind = stratum if stratum in OPERATOR_STRATA else rng.choice(allowed)
            if stratum in (HC_NONINT, HC_INT, HC_SHIFT_UNDER_HASH):
                kind = rng.choice([HASH, SHIFT, PASS, SUB, AGSE])
            try:
                node = _candidates(kind, nodes, rng)
            except PlanError:
                ok = False
                break
            if not _rational_ok(node) or node.width > MAX_WIDTH:
                ok = False
                break
            nodes.append(node)
        if not ok or len(nodes) == source_count:
            continue

        candidate = Plan(nodes=tuple(nodes))
        operators = [node for node in candidate.nodes if node.kind != SOURCE]
        if stratum in OPERATOR_STRATA and not any(node.kind == stratum for node in operators):
            continue
        if stratum == WINDOW and not any(node.kind == WINDOW for node in operators):
            continue
        if stratum in HARD_CLASSES and stratum not in _hard_classes_of(candidate.nodes):
            continue
        return candidate
    raise GeneratorError(f"nie udało się zbudować planu dla straty {stratum}")


def generate(seed, count, strata=None):
    """Korpus ``count`` planów; strata przydzielana rotacyjnie.

    ``strata`` domyślnie jest zamrożonym zestawem K24e (``STRATA``) i wołanie bez
    tego argumentu daje korpus BAJTOWO ten sam co przed dołożeniem okna. Korpus
    z oknem rekordowym zamawia się jawnie: ``strata=STRATA_WITH_WINDOW``.
    """
    strata = tuple(strata) if strata is not None else STRATA
    rng = random.Random(seed)
    corpus = []
    for index in range(count):
        stratum = strata[index % len(strata)]
        candidate = build(rng, stratum)
        corpus.append((stratum, Plan(nodes=candidate.nodes, seed_index=index)))
    return corpus


if __name__ == "__main__":
    import argparse
    import collections

    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=20260803)
    parser.add_argument("--count", type=int, default=len(STRATA) * 10)
    parser.add_argument("--with-window", action="store_true",
                        help="dolacz strate okna rekordowego (korpus K24f)")
    args = parser.parse_args()

    selected = STRATA_WITH_WINDOW if args.with_window else STRATA
    corpus = generate(args.seed, args.count, strata=selected)
    kinds = collections.Counter()
    strata = collections.Counter()
    for stratum, item in corpus:
        strata[stratum] += 1
        for node in item.nodes:
            if node.kind != SOURCE:
                kinds[node.kind] += 1
    print(f"planów: {len(corpus)}")
    print("strata:", dict(strata))
    print("węzły wg klasy:", dict(kinds))
