#!/usr/bin/env python3
"""Replika rachunku silnika: ``compiler::computeLogicalOrigin()``
i ``compiler::computeStartupLatency()``.

UWAGA — ten moduł istnieje **wyłącznie** na potrzeby bramki mutantów. Oracle
(model.py) nie importuje go i nie ma prawa go importować. Replika odtwarza
rachunek z `src/retractor/lib/compiler.cpp` oraz `src/include/SOperations.hpp`
w wersji przypiętej tagiem kampanii; jej wierność sprawdza test_closedform.py,
porównując ją ze zrzutem planu silnika.

Mutanty w mutants.py psują tę replikę w jednym miejscu każdy. Bramka wymaga,
by oracle odróżnił replikę od każdego mutanta.

**Zmiana wobec K24r (przestemplowanie z 2026-08-06).** Silnik niesie teraz dwie
wielkości zamiast jednej. Origin nie jest w pełni postacią zamkniętą: dla `+`,
`#`, `-`, `Θ` i `~Θ` silnik szuka najmniejszego indeksu spełniającego warunek
przez połowienie po niemalejącym odwzorowaniu (``firstIndexReaching``), a nie
przez wzór. Replika odtwarza to wiernie — razem z tym ograniczeniem, które jest
przedmiotem raportu, a nie usterką repliki.
"""

from fractions import Fraction
from math import gcd

from plan import (ADD, AGSE, HASH, NTHETA, PASS, REDUCE, SHIFT, SOURCE, SUB,
                  THETA)

# compiler.cpp: kOriginSearchLimit
ORIGIN_SEARCH_LIMIT = 1 << 24


class ReplicaError(RuntimeError):
    """Replika nie potrafiła odtworzyć rachunku silnika."""


def _floor(value):
    return value.numerator // value.denominator


def _ceil(value):
    return -((-value.numerator) // value.denominator)


def to_slots(width, delta_source, delta_target):
    if width <= 0:
        return 0
    return _ceil(Fraction(width) * delta_source / delta_target)


# --- odwzorowania indeksu (SOperations.hpp) -----------------------------------
#
# Te same funkcje, którymi silnik ADRESUJE składowe w dataModel. Origin liczy
# się po nich, bo origin jest pytaniem „od którego indeksu odwzorowanie trafia
# w istniejący rekord”.

def map_add(delta_out, delta_src, n):
    if delta_out == delta_src:
        return n
    return _floor(Fraction(n) * delta_out / delta_src)


def map_subtract(delta_source, delta_target, n):
    if delta_source == delta_target:
        return n
    return _ceil(Fraction(n) * delta_target / delta_source)


def map_div(delta_a, delta_b, n):
    """Θ — lewa składowa: a_i = c_{i+ceil((i+1)*dA/dB)}."""
    return n + _ceil(Fraction(n + 1) * delta_a / delta_b)


def map_mod(delta_a, delta_b, n):
    """~Θ — prawa składowa: b_i = c_{i+floor(i*dB/dA)}."""
    return n + _floor(Fraction(n) * delta_b / delta_a)


def first_index_reaching(mapping, threshold, node_id="?"):
    """Najmniejsze n >= 0, dla którego niemalejące odwzorowanie osiąga próg.

    Replika ``firstIndexReaching`` z compiler.cpp: podwajanie górnego
    ograniczenia, potem połowienie.
    """
    if threshold <= 0:
        return 0
    hi = 1
    while mapping(hi) < threshold:
        if hi > ORIGIN_SEARCH_LIMIT:
            raise ReplicaError(f"replika: poszukiwanie origin rozbiegło się dla '{node_id}'")
        hi *= 2
    lo = 0
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if mapping(mid) < threshold:
            lo = mid + 1
        else:
            hi = mid
    return lo


# --- postacie zamknięte ogona -------------------------------------------------

def hash_pick(delta_left, delta_right, n):
    """(strona, indeks) dla rekordu ``n`` przeplotu — replika ``Hash()``.

    Zwraca ``('right', j)`` albo ``('left', j)``; ``j`` jest indeksem
    postępującym w wybranej składowej.
    """
    zet = delta_right / (delta_left + delta_right)
    if _floor(zet * n) == _floor(zet * (n + 1)):
        return "right", n - _floor(zet * n)
    return "left", _floor(zet * n)


# SOperations.hpp: kHashPhaseScanLimit
HASH_PHASE_SCAN_LIMIT = 100_000


def hash_tail(delta_left, delta_right, delta_out, tail_left, tail_right):
    """Ogon przeplotu — replika ``HashStartupLatency()`` (krok 3c, 2026-08-07).

    Maksimum po jednym okresie fazowym ``p+q`` z warunku dostępności
    ``W >= ceil((j(i)+1+W_src(i))*D_src(i)/D_c) - 1 - i``. Powyżej progu silnik
    wraca do postaci O(1) i replika musi wrócić razem z nim — inaczej bramka
    wierności repliki (test_closedform.py) zgłosiłaby rozjazd, którego w silniku
    nie ma.
    """
    ratio = delta_left / delta_right
    period = ratio.numerator + ratio.denominator
    if period > HASH_PHASE_SCAN_LIMIT:
        return hash_tail_o1(delta_left, delta_right, delta_out, tail_left, tail_right)

    result = 0
    for index in range(period):
        side, position = hash_pick(delta_left, delta_right, index)
        delta_src = delta_left if side == "left" else delta_right
        tail_src = tail_left if side == "left" else tail_right
        required = _ceil(Fraction(position + 1 + tail_src) * delta_src / delta_out) - 1 - index
        result = max(result, required)
    return result


def hash_tail_o1(delta_left, delta_right, delta_out, tail_left, tail_right, **kwargs):
    """Postać O(1) zastąpiona 2026-08-07: max(conv(W_A), conv(W_B) + own).

    Zgadzała się z granicą zdarzeniową w 92,1% węzłów `#` korpusu K24p
    i zawyżała o slot w pozostałych. Zostaje w replice z dwóch powodów:
    jest ścieżką powyżej progu przeglądu ORAZ rodziną mutantów (`hash_o1`).
    """
    own = hash_own(delta_left, delta_right, **kwargs)
    return max(to_slots(tail_left, delta_left, delta_out),
               to_slots(tail_right, delta_right, delta_out) + own)


def hash_own(delta1, delta2, phase_delta=0, swap=False, drop_own=False, first_phase=False):
    """Własny ogon przeplotu: ceil((p+q-1)/p) dla zredukowanego delta1/delta2.

    ``first_phase`` daje wariant sprzed K2 — człon pierwszej fazy ceil(q/p),
    który chroni B[0], ale nie najgorszą fazę późniejszą. Używa go reguła
    lokalna B w analizie członu (b); nie jest to mutant.
    """
    if drop_own:
        return 0
    if first_phase:
        return _ceil(delta2 / delta1)
    ratio = (delta1 / delta2) if swap else (delta2 / delta1)
    period = ratio.denominator
    advance = ratio.numerator
    return (period + advance - 2) // period + 1 + phase_delta


def phase_tail(phase_bound, ratio, source_tail):
    """Wspólna postać ogona `-`, `Θ` i `~Θ` (silnik: PhaseStartupLatency).

    Odwzorowanie indeksu rozkłada się na idx(n) = n*r + e(n), więc warunek
    dostępności redukuje się do ceil((e(n)+1+W_src)/r) - 1, a maksimum wypada
    tam, gdzie e(n) osiąga kres. Kres jest osiągany (gcd = 1 po skróceniu),
    stąd postać jest dokładna, nie oszacowana.
    """
    return max(0, _ceil((Fraction(phase_bound) + source_tail + 1) / ratio) - 1)


def subtract_tail(delta_source, delta_target, source_tail, source_declared=False,
                  declaration_slot=False):
    """`-`: e(n) = (q - n*p mod q)/q, kres (q-1)/q.

    ``declaration_slot`` odtwarza postać sprzed 2026-08-18 (mutant
    ``subtract_declaration_slot``): człon fazowy doklejany do ogona składowej
    zamiast do indeksu, plus osobna gałąź dla deklaracji. Zawyżała o slot
    w 80,9% węzłów `-` korpusu.
    """
    ratio = delta_target / delta_source
    q = ratio.denominator
    phase = Fraction(q - 1, q)
    if declaration_slot:
        if source_declared:
            return _floor(phase / ratio) + 1
        return _ceil((Fraction(source_tail) + phase) / ratio)
    return phase_tail(phase, ratio, source_tail)


def theta_tail(delta_source, delta_target, other, source_tail):
    """`Θ`: e(n) = (a-t)/b dla t = 0, inaczej (a+b-t)/b; kres (a+b-1)/b.

    Przy ilorazie całkowitym (b = 1) kres wynosi a, co po podzieleniu przez r
    daje ogon własny ZERO — dlatego stała jedynka sprzed 2026-08-18 zawyżała
    w 40,3% węzłów `Θ`.
    """
    span = delta_target / other
    bound = Fraction(span.numerator + span.denominator - 1, span.denominator)
    return phase_tail(bound, delta_target / delta_source, source_tail)


def ntheta_tail(delta_source, delta_target, source_tail):
    """`~Θ`: e(n) = -(n*a mod b)/b <= 0, kres 0 przy n = 0."""
    return phase_tail(0, delta_target / delta_source, source_tail)


def agse_tail(source_width, step, source_tail):
    """Postać po przestemplowaniu: ceil((1+W_src)*F/step) - 1.

    Człon fazowy P = floor((|L|-1)/gcd(F,step))*gcd(F,step), obecny w postaci
    z K24r, zniknął z ogona: rozpiętość okna nie jest już czekaniem, tylko
    niedefiniowalnością, i przeszła do origin (agse_origin niżej). Minimum
    reszty (n*step) mod F wynosi zero i jest osiągane niezależnie od tego, od
    którego n zaczyna się strumień, więc origin nie wchodzi do ogona.
    """
    return _ceil(Fraction((1 + source_tail) * source_width, step)) - 1


def agse_origin(source_width, step, length, source_origin, drop_span=False, off_by_one=0):
    """Początek logiczny okna: ceil((O_src*F + |L| - 1)/step).

    Okno rekordu n sięga pozycji n*step-(|L|-1); warunek mieszczenia się
    w istniejącej części źródła daje wprost powyższy wzór.
    """
    span = 0 if drop_span else abs(length) - 1
    return _ceil(Fraction(source_origin * source_width + span, step)) + off_by_one


def add_tail(delta_source, delta_target, source_tail):
    """ceil((1+W_src)*D_src/D_out) - 1 per składowa (bez zmian wobec K24r)."""
    return _ceil(Fraction(1 + source_tail) * delta_source / delta_target) - 1


# --- przebiegi ----------------------------------------------------------------

def evaluate_origins(plan, mutation=None, given_origins=None):
    """Początki logiczne wg rachunku silnika (opcjonalnie zmutowanego)."""
    mutation = mutation or {}
    origins = {}
    for node in plan.nodes:
        if node.kind == SOURCE:
            origins[node.name] = 0
            continue

        children = [plan.by_name(name) for name in node.children]
        first = children[0]
        source_origins = given_origins if given_origins is not None else origins
        o1 = source_origins[first.name]
        result = o1

        if node.kind in (PASS, REDUCE):
            result = o1
        elif node.kind == SHIFT:
            result = o1 + (0 if mutation.get("shift_drop_origin", False) else node.param)
        elif node.kind == AGSE:
            _step, length = node.param
            step = node.param[0]
            result = agse_origin(first.width, step, length, o1,
                                 drop_span=mutation.get("agse_drop_span", False),
                                 off_by_one=mutation.get("agse_origin_delta", 0))
        elif node.kind == SUB:
            result = first_index_reaching(
                lambda n: map_subtract(first.delta, node.delta, n), o1, node.name)
        elif node.kind == THETA:
            result = first_index_reaching(
                lambda n: map_div(node.delta, node.param, n), o1, node.name)
        elif node.kind == NTHETA:
            result = first_index_reaching(
                lambda n: map_mod(node.param, node.delta, n), o1, node.name)
        elif node.kind == ADD:
            second = children[1]
            o2 = source_origins[second.name]
            result = max(
                first_index_reaching(lambda n: map_add(node.delta, first.delta, n), o1, node.name),
                first_index_reaching(lambda n: map_add(node.delta, second.delta, n), o2, node.name))
        elif node.kind == HASH:
            second = children[1]
            o2 = source_origins[second.name]
            zet = second.delta / (first.delta + second.delta)
            left = first_index_reaching(lambda n: _floor(zet * n), o1, node.name)
            right = first_index_reaching(lambda n: n - _floor(zet * n), o2, node.name)
            if mutation.get("hash_origin_left_only", False):
                result = left
            else:
                result = max(left, right)
        else:
            raise ReplicaError(f"replika: nieznany węzeł {node.kind}")

        origins[node.name] = result
    return origins


def evaluate(plan, mutation=None, given_tails=None):
    """Ogony wszystkich węzłów wg postaci zamkniętej (opcjonalnie zmutowanej).

    ``given_tails`` podmienia ogony składowych na zadane z zewnątrz. Służy
    atrybucji per klasa operatora: wtedy każdy węzeł jest liczony z ogonów
    składowych wziętych z oracle'a, więc niezgodność w węźle pochodzi z reguły
    tego węzła, a nie jest odziedziczona po dziecku.
    """
    mutation = mutation or {}
    tails = {}
    for node in plan.nodes:
        if node.kind == SOURCE:
            tails[node.name] = 0
            continue

        children = [plan.by_name(name) for name in node.children]
        first = children[0]
        source_tails = given_tails if given_tails is not None else tails
        w1 = source_tails[first.name]
        result = to_slots(w1, first.delta, node.delta)

        if node.kind == PASS:
            result = w1
        elif node.kind == SHIFT:
            # Rekord n czyta rekord n-N producenta, czyli STARSZY od bieżącego.
            # `N` siedzi w origin, a deficyt przesunięcia jest stały i równy
            # W_src - N, więc ogonem jest max(0, W_src - N) — krok 3d, 2026-08-07.
            # Dwie postacie historyczne zostają jako mutanty:
            #   shift_tail_keeps_n     -> W_src + N   (semantyka sprzed 2026-08-06)
            #   shift_tail_keeps_source -> W_src      (db4a360, fetchBack offsetem względnym)
            if mutation.get("shift_tail_keeps_n", False):
                result = w1 + node.param
            elif mutation.get("shift_tail_keeps_source", False):
                result = w1
            else:
                result = max(0, w1 - node.param)
        elif node.kind == HASH:
            second = children[1]
            w2 = source_tails[second.name]
            # Postać O(1) jest teraz mutantem (`hash_o1`) albo ścieżką powyżej
            # progu przeglądu; obowiązująca jest reguła z kroku 3c.
            if (mutation.get("hash_o1", False) or mutation.get("hash_phase", 0)
                    or mutation.get("hash_swap", False) or mutation.get("hash_drop_own", False)
                    or mutation.get("hash_first_phase", False)):
                result = hash_tail_o1(first.delta, second.delta, node.delta, w1, w2,
                                      phase_delta=mutation.get("hash_phase", 0),
                                      swap=mutation.get("hash_swap", False),
                                      drop_own=mutation.get("hash_drop_own", False),
                                      first_phase=mutation.get("hash_first_phase", False))
            elif mutation.get("hash_scan_half_period", False):
                # Mutant: przegląd o połowę za krótki — sprawdza, czy bramka
                # wykryje regułę, która trafia w większość węzłów, ale nie we
                # wszystkie. To jest realny sposób, w jaki ten rachunek może się
                # zepsuć przy refaktoryzacji.
                ratio = first.delta / second.delta
                period = max(1, (ratio.numerator + ratio.denominator) // 2)
                result = 0
                for index in range(period):
                    side, position = hash_pick(first.delta, second.delta, index)
                    delta_src = first.delta if side == "left" else second.delta
                    tail_src = w1 if side == "left" else w2
                    result = max(result, _ceil(Fraction(position + 1 + tail_src) * delta_src / node.delta) - 1 - index)
            else:
                result = hash_tail(first.delta, second.delta, node.delta, w1, w2)
        elif node.kind == ADD:
            second = children[1]
            result = max(add_tail(first.delta, node.delta, w1),
                         add_tail(second.delta, node.delta, source_tails[second.name]))
        elif node.kind == THETA:
            # Postać sprzed 2026-08-18 — stały człon własny doklejany do
            # przeliczonego ogona składowej — jest teraz mutantem
            # (`theta_constant_own`), tak jak `hash_closed_form_o1` w K24d.
            if mutation.get("theta_constant_own", False):
                result += 0 if mutation.get("theta_zero_own", False) else 1
            elif mutation.get("theta_zero_own", False):
                result = phase_tail(0, node.delta / first.delta, w1)
            else:
                result = theta_tail(first.delta, node.delta, node.param, w1)
        elif node.kind == NTHETA:
            # Sprzed 2026-08-18: samo przeliczenie ogona składowej przez takt,
            # z zaokrągleniem w górę liczonym OSOBNO — mutant
            # `ntheta_rounds_source_tail`.
            if not mutation.get("ntheta_rounds_source_tail", False):
                result = ntheta_tail(first.delta, node.delta, w1)
        elif node.kind == SUB:
            result = subtract_tail(first.delta, node.delta, w1, first.kind == SOURCE,
                                   declaration_slot=mutation.get("subtract_declaration_slot", False))
        elif node.kind == AGSE:
            step, length = node.param
            if mutation.get("agse_tail_keeps_phase", False):
                unit = gcd(first.width, step)
                phase_bound = ((abs(length) - 1) // unit) * unit
                result = _ceil(Fraction(phase_bound + (1 + w1) * first.width, step)) - 1
            else:
                result = agse_tail(first.width, step, w1)
        elif node.kind == REDUCE:
            pass
        else:
            raise ReplicaError(f"replika: nieznany węzeł {node.kind}")

        tails[node.name] = result
    return tails
