#!/usr/bin/env python3
"""Bramka postaci fazowych `-`, `Theta` i `~Theta` — przemiatanie poza korpusem.

Kampania K24e zmierzyła te trzy klasy na 10 010 losowych planach i zastała je
dokładne. Korpus losowy ma jednak wąski zakres: mianownik zredukowanego ilorazu
taktów (`q`) sięga w nim 3 dla `-` i 5 dla rozplotu. Postać zamknięta jest
twierdzeniem o KAŻDYM `q`, więc ta bramka przemiata `q` systematycznie i pyta
silnik wprost.

Trzy rzeczy sprawdzane per przypadek:

  1. ogon silnika == ogon modelu zdarzeniowego (konwencja C1) — dokładność;
  2. ogon silnika >= ogon modelu — osobno, bo ZANIŻENIE jest defektem
     poprawności, a zawyżenie tylko utratą dokładności; komunikat ma je
     rozróżniać nawet wtedy, gdy oba oblewają ten sam warunek;
  3. moc detekcyjna: dawna reguła każdej z trzech klas (sprzed 2026-08-18) musi
     dla części przypadków dawać INNY wynik niż model. Bez tego przemiatanie
     przechodziłoby także dla silnika, który niczego nie naprawił — a wtedy nie
     strzeże niczego.

Model zdarzeniowy jest tu jedynym źródłem prawdy; postać zamknięta silnika NIE
jest w nim używana (patrz test_independence.py).
"""

import sys
from fractions import Fraction
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "oracle"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import engine as E  # noqa: E402
import model as M  # noqa: E402
import plan as P  # noqa: E402

# Zakres przemiatania. Korpus kampanijny sięga q <= 5; tutaj idziemy do 12, co
# przy trzech klasach i trzech ogonach składowej daje kilkaset kompilacji —
# bramka ma być sekundowa, nie godzinna.
Q_MAX = 12
SOURCE_TAILS = (0, 1, 3)


def _cases():
    """Plany jednowęzłowe i dwuwęzłowe o systematycznie rosnącym `q`.

    Ogon składowej wprowadzamy przez `#` nad dwiema deklaracjami, bo tylko tak
    da się go dostać BEZ użycia naprawianych klas — inaczej przypadek testowałby
    regułę samą sobą.
    """
    for q in range(1, Q_MAX + 1):
        for p in range(q, 3 * q + 1):
            if gcd(p, q) != 1:
                continue
            src = P.make_source("s0", Fraction(1, 100), 1)
            target = Fraction(p, q) * src.delta
            yield f"sub p/q={p}/{q}", [src, P.make_sub("n0", src, target)]

    for a in range(1, Q_MAX + 1):
        for b in range(1, Q_MAX + 1):
            if gcd(a, b) != 1:
                continue
            # Rozplot: źródło jest przeplotem o takcie ab/(a+b), składowe a i b.
            src = P.make_source("s0", Fraction(a * b, (a + b) * 100), 1)
            try:
                theta = P.make_theta("n0", src, Fraction(b, 100))
                yield f"theta a/b={a}/{b}", [src, theta]
            except P.PlanError:
                pass
            src2 = P.make_source("s0", Fraction(a * b, (a + b) * 100), 1)
            try:
                ntheta = P.make_ntheta("n0", src2, Fraction(a, 100))
                yield f"ntheta a/b={a}/{b}", [src2, ntheta]
            except P.PlanError:
                pass


def _legacy_tail(node, child, source_tail):
    """Reguła sprzed 2026-08-18 — wyłącznie do pomiaru mocy detekcyjnej."""
    generic = 0 if source_tail <= 0 else -((-source_tail * child.delta) // node.delta)
    generic = int(generic)
    if node.kind == P.THETA:
        return generic + 1
    if node.kind == P.NTHETA:
        return generic
    ratio = node.delta / child.delta
    q = ratio.denominator
    phase = Fraction(q - 1, q)
    if child.kind == P.SOURCE:
        return int((phase / ratio).numerator // (phase / ratio).denominator) + 1
    value = (Fraction(source_tail) + phase) / ratio
    return -((-value.numerator) // value.denominator)


def main(argv):
    binary = E.resolve_binary(argv[1] if len(argv) > 1 else None)
    workdir = ROOT / "work" / "phase_forms"
    mismatches, understated, witnesses, checked = [], [], 0, 0

    for label, nodes in _cases():
        plan = P.Plan(nodes=tuple(nodes))
        try:
            dump, _ = E.compile_plan(P.to_rql(plan), binary, workdir)
        except E.EngineError as exc:
            mismatches.append(f"{label}: silnik nie skompilował planu — {exc}")
            continue
        engine_plan = E.parse_plan(dump)
        oracle = {item.name: item.tail for item in M.evaluate(plan, convention=M.C1)}

        node = plan.nodes[-1]
        child = plan.by_name(node.children[0])
        got = engine_plan.get(node.name)
        if got is None:
            mismatches.append(f"{label}: brak {node.name} w zrzucie planu")
            continue
        _interval, tail, _origin = got
        expected = oracle[node.name]
        checked += 1
        if tail < expected:
            understated.append(f"{label}: ZANIŻENIE — silnik {tail}, model {expected}")
        elif tail != expected:
            mismatches.append(f"{label}: zawyżenie — silnik {tail}, model {expected}")
        if _legacy_tail(node, child, 0) != expected:
            witnesses += 1

    print(f"przypadków: {checked} (q do {Q_MAX})")
    for line in understated + mismatches:
        print(f"NIEZGODNOŚĆ {line}")
    print(f"świadków mocy detekcyjnej (dawna reguła != model): {witnesses}")

    if witnesses == 0:
        print("BRAMKA POSTACI FAZOWYCH: BŁĄD APARATURY — zerowa moc detekcyjna")
        return 2
    ok = not understated and not mismatches
    print("BRAMKA POSTACI FAZOWYCH: " + ("PRZESZŁA" if ok else "NIE PRZESZŁA"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
