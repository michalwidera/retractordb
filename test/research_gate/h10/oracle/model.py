#!/usr/bin/env python3
"""Niezależny oracle zdarzeniowy K24p — początek logiczny i ogon strumienia,
wyprowadzone z definicji operatorów i czasu zdarzeń.

Oracle NIE używa postaci zamkniętej z ``compiler::computeStartupLatency()``
ani z ``compiler::computeLogicalOrigin()``. Nie występuje tu ani
``ceil((p+q-1)/p)``, ani ``AgseStartupLatency``, ani ``AgseLogicalOrigin``,
ani ``SubtractStartupLatency``. Obie wielkości są tu **wyprowadzone**:

    origin O_S — najmniejszy indeks n, dla którego rekord n ma komplet
                 istniejących zależności. Rekordy o indeksie mniejszym nie
                 istnieją: ich definicja sięga przed początek strumienia
                 źródłowego. To nie jest oczekiwanie, tylko brak definicji;
    ogon  W_S  — rekord n strumienia S jest emitowany w chwili
                 (n + 1 + W_S) * Delta_S; W_S jest najmniejszą liczbą
                 całkowitą >= 0, dla której emisja **każdego istniejącego**
                 rekordu (n >= O_S) wypada nie wcześniej niż dostępność
                 wszystkich jego zależności.

Rozdzielenie tych dwóch wielkości jest przedmiotem K24p. Do 2026-08-06 silnik
niósł jedną wielkość (ogon), w której oba człony były zsumowane; `>N` trzymało
opóźnienie w ogonie, a okno `@` — swoją rozpiętość. Suma slotów milczenia
(origin + ogon) jest niezmiennikiem: ciąg wydanych rekordów jest ten sam,
zmienia się wyłącznie indeks logiczny, pod którym rekord się pojawia.

Odwzorowanie rekordów (który rekord składowej wchodzi do rekordu n) pochodzi
z definicji operatorów w §Formal Foundations artykułu, czyli z semantyki,
a nie z rachunku ogona.

Konwencja dostępności (zamrożona w kampanii K24):
  C1 — nieostra: rekord dostępny w chwili swojej emisji; konsument o slocie
       kończącym się w tej samej chwili może go użyć (producenci publikują
       przed konsumentami w takcie — dataModel::processRows przetwarza
       deklaracje jako pierwsze, dalej porządek topologiczny).
  C2 — ostra: odczyt w tym samym takcie jest niedozwolony.
Werdykt główny liczony jest w C1; C2 raportowany jest jako kolumna wrażliwości.
"""

from dataclasses import dataclass
from fractions import Fraction

from plan import (ADD, AGSE, HASH, NTHETA, PASS, REDUCE, SHIFT, SOURCE, SUB,
                  THETA, period_hint)

C1 = "C1"
C2 = "C2"

ADD_MAPPING = "spec"   # "spec" | "engine" — patrz dependencies(); kampania: "spec"

MIN_PROBE = 64
PROBE_FACTOR = 4

# Origin jest ograniczony rozpiętością okien i przesunięć w planie, więc realnie
# jest małą liczbą (generator: |L| <= 4, N <= 8, głębokość <= 6). Limit chroni
# wyłącznie przed odwzorowaniem, które wbrew założeniu nie rośnie — wtedy
# poszukiwanie nie ma prawa zakończyć się cicho.
#
# K24e (2026-08-18): stała była WARTOŚCIĄ BEZWZGLĘDNĄ, nie powiązaną z oknem
# sondowania, przez co strzelała w planach o dużym okresie fazowym — czyli
# w przebiegu całkowicie legalnym. Skan potrzebuje `last_missing + window`
# kroków, a window = 4*(p+q); korpusy bramki mają maksymalne okno 98 228 przy
# limicie 100 000, czyli zapas 1,8%, o którym nikt nie wiedział. Ziarno spoza
# bramki (`20260819`) wygenerowało `#` o ilorazie 2000/29841 (okno 127 364)
# i kampania zatrzymała się na błędzie aparatury.
#
# Limit jest teraz marginesem NAD oknem, nie zamiast niego: strażnik zachowuje
# swoją rolę (odwzorowanie, które nie rośnie, wciąż zatrzymuje przebieg), ale
# nie może już strzelić w planie, którego okno jest po prostu szerokie.
ORIGIN_LIMIT = 100_000


class OracleError(RuntimeError):
    """Awaria aparatury oracle'a — zatrzymuje iterację, nie jest wynikiem."""


def _floor(value):
    return value.numerator // value.denominator


def _ceil(value):
    return -((-value.numerator) // value.denominator)


def dependencies(node, children, n):
    """Zależności rekordu ``n``: lista (nazwa składowej, indeks, opóźnienie).

    Zwracane są **wszystkie** rekordy składowych, od których zależy rekord n,
    a nie tylko ten wiążący czasowo. Dla okna `@` to cały zakres rekordów
    pokrytych oknem: o dostępności decyduje najnowszy z nich, a o istnieniu —
    najstarszy, i oracle nie ma prawa przesądzać, który to który.

    Opóźnienie jest czasem fizycznym doklejanym do dostępności. Po
    przestemplowaniu z 2026-08-06 nie używa go już żaden operator: `>N` jest
    przesunięciem indeksu, nie opóźnieniem czasu. Pole zostaje, bo model
    dostępności jest od niego niezależny, a jego usunięcie zmieniłoby sygnaturę
    dzieloną z bramkami.
    """
    kind = node.kind
    if kind == PASS:
        return [(children[0].name, n, Fraction(0))]
    if kind == SHIFT:
        # tau_N jest OPÓŹNIENIEM: rekord n niesie treść rekordu n-N producenta.
        # Rekordy o indeksie mniejszym od N nie mają definicji — sięgałyby przed
        # początek producenta — więc `N` jest tu przesunięciem indeksu, czyli
        # składnikiem origin, a nie doklejonym czasem oczekiwania.
        return [(children[0].name, n - node.param, Fraction(0))]
    if kind == REDUCE:
        return [(children[0].name, n, Fraction(0))]
    if kind == HASH:
        left, right = children
        z = right.delta / (left.delta + right.delta)
        if _floor(z * n) == _floor(z * (n + 1)):
            return [(right.name, n - _floor(z * n), Fraction(0))]
        return [(left.name, _floor(z * n), Fraction(0))]
    if kind == ADD:
        left, right = children
        # ADD_MAPPING == "spec": odwzorowanie z Definicji sumy strumieni
        # (artykuł, eq. sum): c_n = (a_n, b_{floor(n*D_a/D_b)}).
        # ADD_MAPPING == "engine": odwzorowanie zaobserwowane w silniku,
        # b_{floor((n+1)*D_a/D_b)} — wariant WYŁĄCZNIE diagnostyczny, do
        # rozstrzygnięcia, ile z rozbieżności klasy `+` pochodzi z różnicy
        # definicji, a ile z rachunku ogona. Kampania używa "spec".
        offset = 1 if ADD_MAPPING == "engine" else 0
        if left.delta <= right.delta:
            return [(left.name, n, Fraction(0)),
                    (right.name, _floor(Fraction(n + offset) * left.delta / right.delta), Fraction(0))]
        return [(left.name, _floor(Fraction(n + offset) * right.delta / left.delta), Fraction(0)),
                (right.name, n, Fraction(0))]
    if kind == SUB:
        src = children[0]
        if node.delta == src.delta:
            return [(src.name, n, Fraction(0))]
        return [(src.name, _ceil(Fraction(n) * node.delta / src.delta), Fraction(0))]
    if kind == THETA:
        src = children[0]
        return [(src.name, n + _ceil(Fraction(n + 1) * node.delta / node.param), Fraction(0))]
    if kind == NTHETA:
        src = children[0]
        return [(src.name, n + _floor(Fraction(n) * node.delta / node.param), Fraction(0))]
    if kind == AGSE:
        src = children[0]
        first, last = agse_record_range(node, src, n)
        return [(src.name, index, Fraction(0)) for index in range(first, last + 1)]
    raise OracleError(f"oracle: brak modelu dla węzła {kind}")


def agse_window_positions(node, n):
    """Spłaszczone pozycje źródła objęte oknem rekordu ``n``, od najstarszej.

    Okno jest stemplowane KOŃCEM przedziału: rekord n obejmuje pozycje
    n*step-(|L|-1) ... n*step, więc jego najnowsze pole leży dokładnie
    w pozycji n*step, a indeks logiczny okna oznacza tę samą chwilę co indeks
    logiczny źródła. Ceną konwencji jest to, że dla małych n okno sięga przed
    początek źródła — te rekordy nie powstają, patrz origin.
    """
    step, length = node.param
    start = n * step - (abs(length) - 1)
    return [start + offset for offset in range(abs(length))]


def agse_record_range(node, src, n):
    """(najstarszy, najnowszy) rekord źródła pokryty oknem rekordu ``n``."""
    positions = agse_window_positions(node, n)
    # Dzielenie Pythona zaokrągla w dół także dla ujemnych liczników, więc
    # pozycja sprzed początku strumienia trafia do rekordu ujemnego, a nie do
    # rekordu 0 — dokładnie ta pomyłka, przed którą broni się origin.
    return positions[0] // src.width, positions[-1] // src.width


@dataclass(frozen=True)
class NodeResult:
    name: str
    kind: str
    delta: Fraction
    origin: int
    tail: int
    probe: int
    argmax_slot: int
    stable: bool


def _emission(delta, tail, index):
    return (Fraction(index) + 1 + tail) * delta


def _record_exists(node, children, origins, n):
    """Czy rekord ``n`` ma komplet istniejących zależności."""
    for child_name, index, _delay in dependencies(node, children, n):
        if index < origins[child_name]:
            return False
    return True


def _origin_over_scan(node, children, origins, window):
    """Najmniejszy indeks, OD KTÓREGO strumień jest ciągły.

    Nie jest to „pierwszy indeks o kompletnych zależnościach”. Przeplot pokazuje
    różnicę: przy składowych o różnych początkach rekord 0 może mieć komplet
    (bo w slocie 0 wypada element składowej o origin zerowym), a rekord 1 już
    nie (bo wypada element składowej przesuniętej). Strumień jest ciągiem
    rekordów, nie zbiorem z dziurami — zasada brzegu zabrania wypełnić dziurę
    NULL-em, a przesunięcie kolejnych rekordów zmieniłoby odwzorowanie indeksu.
    Początkiem logicznym jest więc pierwszy indeks, od którego nie ma już ani
    jednej luki.

    Skan idzie do przodu, dopóki nie zobaczy ``window`` kolejnych istniejących
    rekordów za ostatnią luką. Odwzorowania są niemalejące, więc taki ciąg
    domyka sprawę; szerokość okna jest tą samą, na której liczony jest ogon.
    """
    last_missing = -1
    n = 0
    limit = ORIGIN_LIMIT + window
    while n <= last_missing + window:
        if n > limit:
            raise OracleError(f"oracle: nie znaleziono początku logicznego dla {node.name} "
                              f"poniżej {limit} (okno {window})")
        if not _record_exists(node, children, origins, n):
            last_missing = n
        n += 1
    return last_missing + 1


def _tail_over_window(node, children, avail, origin, window, convention):
    """Ogon wymuszony przez istniejące sloty ``origin..origin+window-1``.

    Sloty przed origin nie są rekordami, więc nie stawiają żadnego wymagania —
    liczenie ogona od zera dokładałoby do niego wymagania rekordów, które nie
    powstają, i mieszałoby obie wielkości z powrotem w jedną.
    """
    best = 0
    best_slot = origin
    for n in range(origin, origin + window):
        required = None
        for child_name, index, delay in dependencies(node, children, n):
            if index < 0:
                raise OracleError(f"oracle: ujemny indeks składowej w {node.name} dla slotu {n}")
            moment = avail[child_name](index) + delay
            if required is None or moment > required:
                required = moment
        deficit = required / node.delta - (n + 1)
        if convention == C1:
            candidate = _ceil(deficit)
        else:
            candidate = _floor(deficit) + 1
        if candidate > best:
            best = candidate
            best_slot = n
    return max(best, 0), best_slot


def evaluate(plan, convention=C1, min_probe=MIN_PROBE, probe_factor=PROBE_FACTOR):
    """Początki logiczne i ogony wszystkich węzłów, w porządku topologicznym."""
    origins = {}
    avail = {}
    results = []

    for node in plan.nodes:
        if node.kind == SOURCE:
            origins[node.name] = 0
            avail[node.name] = (lambda delta: (lambda index: _emission(delta, 0, index)))(node.delta)
            results.append(NodeResult(node.name, node.kind, node.delta, 0, 0, 0, 0, True))
            continue

        children = [plan.by_name(name) for name in node.children]
        window = max(min_probe, probe_factor * period_hint(node, children))
        origin = _origin_over_scan(node, children, origins, window)
        tail, slot = _tail_over_window(node, children, avail, origin, window, convention)
        wide, _ = _tail_over_window(node, children, avail, origin, 2 * window, convention)
        if wide != tail:
            raise OracleError(
                f"oracle: okno sondowania {window} za wąskie dla {node.name} ({tail} != {wide})")

        origins[node.name] = origin
        avail[node.name] = (lambda delta, w: (lambda index: _emission(delta, w, index)))(node.delta, tail)
        results.append(NodeResult(node.name, node.kind, node.delta, origin, tail, window, slot, True))

    return results


def tails_by_name(plan, convention=C1):
    return {result.name: result.tail for result in evaluate(plan, convention=convention)}


def origins_by_name(plan, convention=C1):
    return {result.name: result.origin for result in evaluate(plan, convention=convention)}


def silence_by_name(plan, convention=C1):
    """Sloty milczenia = origin + ogon. Niezmiennik przestemplowania: ta suma
    jest tą samą wielkością przed i po zmianie z 2026-08-06, więc na niej —
    i tylko na niej — wolno porównywać obie kampanie."""
    return {result.name: result.origin + result.tail
            for result in evaluate(plan, convention=convention)}


# --- model treści rekordu -----------------------------------------------------
#
# Treść jest tym samym odwzorowaniem rekordów co dependencies(), tyle że
# przeniesionym na wartości. Służy bramce poprawności odwzorowania: dopóki
# silnik emituje dokładnie te wartości, które przewiduje oracle, rozbieżność
# ogona jest rozbieżnością ogona, a nie różnicą w definicji operatora.

def source_value(source_index, record_index, field_index):
    return (source_index + 1) * 1_000_000 + record_index * 10 + field_index


def source_record(source_index, record_index, width):
    return tuple(source_value(source_index, record_index, f) for f in range(width))


def content(plan, name, n, source_order=None):
    """Krotka wartości rekordu ``n`` strumienia ``name``.

    ``n`` jest indeksem LOGICZNYM. Rekord fizyczny 0 artefaktu nosi indeks
    logiczny równy origin strumienia, więc bramka odwzorowania musi przeliczyć
    pozycję w pliku na indeks logiczny, zanim tu zajrzy.
    """
    if n < 0:
        raise OracleError(f"oracle: żądanie treści rekordu {n} strumienia {name} — "
                          f"indeks przed początkiem logicznym nie jest rekordem")
    if source_order is None:
        source_order = [node.name for node in plan.nodes if node.kind == SOURCE]
    node = plan.by_name(name)
    if node.kind == SOURCE:
        return source_record(source_order.index(node.name), n, node.width)

    children = [plan.by_name(child) for child in node.children]
    deps = dependencies(node, children, n)

    if node.kind in (PASS, SHIFT, HASH, SUB, THETA, NTHETA):
        child_name, index, _ = deps[0]
        return content(plan, child_name, index, source_order)
    if node.kind == ADD:
        left = content(plan, deps[0][0], deps[0][1], source_order)
        right = content(plan, deps[1][0], deps[1][1], source_order)
        return left + right
    if node.kind == REDUCE:
        values = content(plan, deps[0][0], deps[0][1], source_order)
        # Reduktory zwracają pole RATIONAL — para (licznik, mianownik).
        if node.param == "sumc":
            return (sum(values), 1)
        if node.param == "min":
            return (min(values), 1)
        if node.param == "max":
            return (max(values), 1)
        average = Fraction(sum(values), len(values))
        return (average.numerator, average.denominator)
    if node.kind == AGSE:
        src = children[0]
        _step, length = node.param
        fields = []
        for flat in agse_window_positions(node, n):
            record = content(plan, src.name, flat // src.width, source_order)
            fields.append(record[flat % src.width])
        # Orientacja okna jest konwencją prezentacyjną ustaloną obserwacyjnie
        # na zbiorze kalibracyjnym: dodatnia długość daje pole najnowsze jako
        # pierwsze, ujemna odwraca. Pozycje w oknie (które rekordy wchodzą
        # do okna) pochodzą z definicji operatora, nie z obserwacji.
        if length > 0:
            fields.reverse()
        return tuple(fields)
    raise OracleError(f"oracle: brak modelu treści dla {node.kind}")
