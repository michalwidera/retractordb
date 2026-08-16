#!/usr/bin/env python3
"""Reprezentacja planu K24 — węzły, interwały i serializacja do RQL.

Plan jest zdekomponowany: każdy operator ma własny, jawnie nazwany strumień
wyjściowy. Dzięki temu każdy węzeł planu jest obserwowalny w zrzucie planu
(`xretractor -c`), a porównanie postaci zamkniętej z oracle'em odbywa się na
poziomie węzła, nie tylko korzenia.

Interwały wyprowadzane są z definicji operatorów (§Formal Foundations
artykułu), nie z kodu silnika.
"""

from dataclasses import dataclass
from fractions import Fraction
from math import gcd

SOURCE = "SOURCE"
PASS = "PASS"
SHIFT = "SHIFT"
HASH = "HASH"
ADD = "ADD"
SUB = "SUB"
THETA = "THETA"
NTHETA = "NTHETA"
AGSE = "AGSE"
REDUCE = "REDUCE"

OPERATOR_CLASSES = (PASS, SHIFT, HASH, ADD, SUB, THETA, NTHETA, AGSE, REDUCE)

REDUCERS = ("sumc", "avg", "min", "max")


class PlanError(ValueError):
    """Plan niepoprawny — generator nie ma prawa go wyprodukować."""


@dataclass(frozen=True)
class Node:
    name: str
    kind: str
    delta: Fraction
    width: int                      # liczba płaskich pól rekordu
    children: tuple = ()            # nazwy węzłów źródłowych
    param: object = None            # N, Fraction, (step, length) albo nazwa reduktora


@dataclass(frozen=True)
class Plan:
    nodes: tuple                    # kolejność topologiczna, źródła najpierw
    seed_index: int = -1

    def by_name(self, name):
        for node in self.nodes:
            if node.name == name:
                return node
        raise KeyError(name)

    @property
    def depth(self):
        return sum(1 for node in self.nodes if node.kind != SOURCE)

    @property
    def root(self):
        return self.nodes[-1]


def _frac(value):
    return Fraction(value)


def make_source(name, delta, width):
    delta = _frac(delta)
    if delta <= 0:
        raise PlanError("interwał źródła musi być dodatni")
    if width < 1:
        raise PlanError("źródło musi mieć co najmniej jedno pole")
    return Node(name=name, kind=SOURCE, delta=delta, width=width)


def make_pass(name, src):
    return Node(name=name, kind=PASS, delta=src.delta, width=src.width, children=(src.name,))


def make_shift(name, src, offset):
    if offset < 1:
        raise PlanError(">N wymaga N >= 1")
    return Node(name=name, kind=SHIFT, delta=src.delta, width=src.width, children=(src.name,), param=int(offset))


def make_hash(name, left, right):
    if left.width != right.width:
        raise PlanError("przeplot wymaga zgodnej szerokości rekordu")
    delta = left.delta * right.delta / (left.delta + right.delta)
    return Node(name=name, kind=HASH, delta=delta, width=left.width, children=(left.name, right.name))


def make_add(name, left, right):
    delta = min(left.delta, right.delta)
    return Node(name=name, kind=ADD, delta=delta, width=left.width + right.width,
                children=(left.name, right.name))


def make_sub(name, src, target):
    target = _frac(target)
    if target < src.delta:
        raise PlanError("różnica nie może być szybsza od źródła")
    return Node(name=name, kind=SUB, delta=target, width=src.width, children=(src.name,), param=target)


def _dehash_delta(src, other, symbol):
    if other == src.delta:
        raise PlanError(f"{symbol} wymaga argumentu różnego od interwału źródła")
    delta = src.delta * other / abs(src.delta - other)
    # Składowa rozplotu nie może być szybsza od strumienia rozplatanego —
    # kompilator odrzuca taki plan ("You cannot make faster div from slower
    # source", compiler.cpp:103 i :123).
    if src.delta > delta:
        raise PlanError(f"{symbol} nie może dać składowej szybszej od źródła")
    return delta


def make_theta(name, src, other):
    """Θ — odzyskanie lewej składowej; ``other`` to interwał prawej składowej."""
    other = _frac(other)
    delta = _dehash_delta(src, other, "Θ")
    return Node(name=name, kind=THETA, delta=delta, width=src.width, children=(src.name,), param=other)


def make_ntheta(name, src, other):
    """~Θ — reszta rozplotu; ``other`` to interwał lewej składowej."""
    other = _frac(other)
    delta = _dehash_delta(src, other, "~Θ")
    return Node(name=name, kind=NTHETA, delta=delta, width=src.width, children=(src.name,), param=other)


def make_agse(name, src, step, length):
    if step < 1:
        raise PlanError("AGSE wymaga kroku >= 1")
    if length == 0:
        raise PlanError("AGSE wymaga niezerowej długości okna")
    delta = Fraction(step) * src.delta / src.width
    return Node(name=name, kind=AGSE, delta=delta, width=abs(int(length)),
                children=(src.name,), param=(int(step), int(length)))


def make_reduce(name, src, reducer):
    if reducer not in REDUCERS:
        raise PlanError(f"nieznany reduktor {reducer}")
    return Node(name=name, kind=REDUCE, delta=src.delta, width=1, children=(src.name,), param=reducer)


def rational_text(value):
    value = Fraction(value)
    if value.denominator == 1:
        return str(value.numerator)
    return f"{value.numerator}/{value.denominator}"


def node_expression(node):
    """Wyrażenie strumieniowe RQL dla pojedynczego węzła."""
    if node.kind == PASS:
        return node.children[0]
    if node.kind == SHIFT:
        return f"{node.children[0]}>{node.param}"
    if node.kind == HASH:
        return f"{node.children[0]}#{node.children[1]}"
    if node.kind == ADD:
        return f"{node.children[0]}+{node.children[1]}"
    if node.kind == SUB:
        return f"{node.children[0]}-{rational_text(node.param)}"
    if node.kind == THETA:
        return f"{node.children[0]}&{rational_text(node.param)}"
    if node.kind == NTHETA:
        return f"{node.children[0]}%{rational_text(node.param)}"
    if node.kind == AGSE:
        step, length = node.param
        return f"{node.children[0]}@({step},{length})"
    if node.kind == REDUCE:
        return f"{node.children[0]}.{node.param}"
    raise PlanError(f"nieznany rodzaj węzła {node.kind}")


def to_rql(plan, storage=".", substrat="memory", data_file=None):
    """Serializacja planu. ``data_file=None`` daje osobny plik na źródło."""
    lines = [f"STORAGE '{storage}'", f"SUBSTRAT '{substrat}'", ""]
    for node in plan.nodes:
        if node.kind != SOURCE:
            continue
        fields = ", ".join(f"{node.name}_f{i} INTEGER" for i in range(node.width))
        source_file = data_file if data_file else f"{node.name}.txt"
        lines.append(f"DECLARE {fields} STREAM {node.name}, {rational_text(node.delta)} FILE '{source_file}'")
    lines.append("")
    for node in plan.nodes:
        if node.kind == SOURCE:
            continue
        lines.append(f"SELECT * STREAM {node.name} FROM {node_expression(node)}")
    lines.append("")
    return "\n".join(lines)


def payload_words(node):
    """Liczba słów 32-bitowych w rekordzie artefaktu.

    Reduktory zapisują pole RATIONAL (licznik + mianownik), pozostałe operatory
    zachowują szerokość INTEGER-ową źródła.
    """
    return 2 if node.kind == REDUCE else node.width


def rescale(plan, factor):
    """Plan o wszystkich interwałach przemnożonych przez ``factor``.

    Ogon zależy wyłącznie od ilorazów interwałów, więc przeskalowanie planu
    nie zmienia żadnego ogona. Służy wyłącznie wykonaniu: silnik pracuje
    w czasie rzeczywistym, a bramka odwzorowania musi zmieścić się w budżecie
    czasu. Wartość ``factor`` nie wchodzi do żadnego wyniku.
    """
    factor = _frac(factor)
    if factor <= 0:
        raise PlanError("współczynnik skali musi być dodatni")
    scaled = []
    for node in plan.nodes:
        param = node.param
        if node.kind in (SUB, THETA, NTHETA):
            param = _frac(param) * factor
        scaled.append(Node(name=node.name, kind=node.kind, delta=node.delta * factor,
                           width=node.width, children=node.children, param=param))
    return Plan(nodes=tuple(scaled), seed_index=plan.seed_index)


def fastest(plan):
    return min(node.delta for node in plan.nodes)


def slowest(plan):
    return max(node.delta for node in plan.nodes)


def reduced_ratio(left, right):
    ratio = Fraction(left) / Fraction(right)
    return ratio.numerator, ratio.denominator


def period_hint(node, children):
    """Górne oszacowanie okresu fazowego węzła, w slotach wyjścia.

    Używane wyłącznie do dobrania okna sondowania oracle'a; nie wchodzi do
    żadnego wyniku. Poprawność okna weryfikuje test stabilności w model.py.
    """
    if node.kind in (HASH, ADD):
        p, q = reduced_ratio(children[0].delta, children[1].delta)
        return p + q
    if node.kind in (SUB, THETA, NTHETA):
        p, q = reduced_ratio(node.delta, children[0].delta)
        return p + q
    if node.kind == AGSE:
        step, _ = node.param
        width = children[0].width
        return width // gcd(width, step)
    return 1
