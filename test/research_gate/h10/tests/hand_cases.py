#!/usr/bin/env python3
"""Bramka oracle'a — przypadki o ręcznie wyprowadzonej odpowiedzi.

Każdy początek logiczny i każdy ogon poniżej wyprowadzono z definicji, nie
odczytano z silnika ani z jego rachunku. Reguły wyprowadzania są dwie:

    origin O_S = najmniejsze n, dla którego wszystkie rekordy składowych
                 potrzebne przez rekord n już istnieją (mają indeks >= origin
                 swojej składowej);
    ogon  W_S  = max(0, max_{n >= O_S} ceil( avail(n)/Delta_S - (n+1) )),
                 gdzie avail(n) jest najpóźniejszą chwilą dostępności
                 składowych rekordu n.

Wyprowadzenia rodzinami (pełne rachunki w REPORT.md §2):

* przeplot A#B, oba źródła W=0, O=0: odwzorowanie indeksu jest nieujemne od
  n=0, więc origin jest zerowy; deficyt maksymalizuje się na slocie, w którym
  wypada element B o najgorszej fazie — dla 1#1 daje 1, dla 1#2 daje 2,
  dla 2#1 daje 1, dla 1/3#1/2 daje 2;
* przesunięcie >N: rekord n niesie rekord n-N producenta, więc rekordy
  o indeksie < N nie istnieją i O = O_src + N. Deficyt slotu n wynosi
  (n-N+1+W_src) - (n+1) = W_src - N, jest STAŁY, więc
  W = max(0, W_src - N). Nad źródłem (W_src=0) daje ogon zerowy — całe
  milczenie siedzi w origin;
* różnica o całkowitym ilorazie r: rekord n czyta indeks r*n, dostępny
  w chwili (rn+1)*Delta_zrodla, a slot n kończy się w (n+1)*r*Delta_zrodla;
  deficyt wynosi 1/r - 1 < 0, więc ogon jest zerowy dla każdego r >= 1;
* różnica o ilorazie niecałkowitym 8/3: maksymalny deficyt wypada ujemny,
  ogon zerowy;
* AGSE @(k,L) nad źródłem o F polach, okno stemplowane KOŃCEM przedziału:
  rekord n obejmuje pozycje n*k-(|L|-1) ... n*k. Warunek mieszczenia się
  w źródle daje O = ceil((O_src*F + |L|-1)/k). O dostępności decyduje pole
  najnowsze, w rekordzie floor(n*k/F); podstawienie r_n = (n*k) mod F daje
  deficyt (F*(1+W_src) - r_n)/k - 1, a reszta r_n = 0 jest osiągana, więc
  W = ceil(F*(1+W_src)/k) - 1;
* Theta i ~Theta: indeks składowej wyprowadzony z definicji rozplotu, nieujemny
  od n=0 (origin zerowy); dla badanych trójek deficyt wypada dokładnie zero,
  więc ogon zerowy przy C1 i jeden przy C2;
* redukcje i projekcja: odwzorowanie tożsamościowe, origin i ogon równe
  wartościom źródła;
* suma A+B wg definicji z artykułu (b_{floor(n*Delta_a/Delta_b)}): rekord 0
  potrzebuje b_0, dostępnego dopiero w chwili Delta_b > Delta_a, stąd ogon
  dodatni, gdy strumień wolniejszy nie jest wielokrotnością szybszego.

Kolumny ``C2`` są wartością tej samej reguły przy konwencji ostrej
(kandydat floor(deficyt)+1 zamiast ceil(deficyt)).
"""

import sys
from fractions import Fraction
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "oracle"))

import plan as P  # noqa: E402


def _plan(nodes):
    return P.Plan(nodes=tuple(nodes))


def _src(name, delta, width=1):
    return P.make_source(name, delta, width)


def hand_cases():
    """Lista (etykieta, plan, {nazwa: ogon C1}, {nazwa: ogon C2}, {nazwa: origin})."""
    out = []

    def add(label, nodes, c1, c2, origin):
        out.append((label, _plan(nodes), c1, c2, origin))

    # --- przeplot ---
    for da, db, tail1, tail2 in [(1, 1, 1, 2), (1, 2, 2, 3), (2, 1, 1, 2),
                                 (Fraction(1, 3), Fraction(1, 2), 2, 3),
                                 (Fraction(1, 10), Fraction(1, 5), 2, 3),
                                 (Fraction(3, 10), Fraction(1, 5), 2, 2)]:
        a, b = _src("s0", da), _src("s1", db)
        add(f"hash {da}#{db}", [a, b, P.make_hash("n0", a, b)],
            {"n0": tail1}, {"n0": tail2}, {"n0": 0})

    # --- przesunięcie ---
    # Nad źródłem W_src = 0, więc ogon max(0, 0-N) = 0 w obu konwencjach,
    # a całe milczenie N slotów jest początkiem logicznym.
    for offset in (1, 2, 5):
        a = _src("s0", Fraction(1, 10))
        add(f"shift >{offset}", [a, P.make_shift("n0", a, offset)],
            {"n0": 0}, {"n0": 0}, {"n0": offset})

    # --- projekcja i redukcje ---
    a = _src("s0", Fraction(1, 10), 3)
    add("pass", [a, P.make_pass("n0", a)], {"n0": 0}, {"n0": 1}, {"n0": 0})
    for reducer in P.REDUCERS:
        a = _src("s0", Fraction(1, 10), 3)
        add(f"reduce {reducer}", [a, P.make_reduce("n0", a, reducer)],
            {"n0": 0}, {"n0": 1}, {"n0": 0})

    # --- różnica ---
    for ratio in (1, 2, 3, 5):
        a = _src("s0", Fraction(1, 100))
        add(f"sub ratio {ratio}", [a, P.make_sub("n0", a, Fraction(ratio, 100))],
            {"n0": 0}, {"n0": 1 if ratio == 1 else 0}, {"n0": 0})
    a = _src("s0", Fraction(1, 8))
    add("sub 8/3", [a, P.make_sub("n0", a, Fraction(1, 3))], {"n0": 0}, {"n0": 0}, {"n0": 0})

    # --- AGSE ---
    # O = ceil((|L|-1)/k) nad źródłem; W = ceil(F/k) - 1; C2 = max(0, floor(F/k)).
    for width, step, length, tail1, tail2, origin in [(1, 1, 3, 0, 1, 2), (1, 1, -3, 0, 1, 2),
                                                      (1, 2, 4, 0, 0, 2), (2, 1, 4, 1, 2, 3),
                                                      (3, 2, 2, 1, 1, 1), (4, 3, 5, 1, 1, 2)]:
        a = _src("s0", Fraction(1, 10), width)
        add(f"agse F={width} @({step},{length})", [a, P.make_agse("n0", a, step, length)],
            {"n0": tail1}, {"n0": tail2}, {"n0": origin})

    # --- rozplot ---
    for delta, other, theta1, theta2, ntheta1, ntheta2 in [
            (Fraction(1, 2), 1, 0, 1, 0, 0),
            (Fraction(2, 3), 1, 0, 1, 0, 0),
            (Fraction(2, 3), 2, 1, 1, 0, 0)]:
        a = _src("s0", delta)
        add(f"theta {delta}&{other}", [a, P.make_theta("n0", a, other)],
            {"n0": theta1}, {"n0": theta2}, {"n0": 0})
        a = _src("s0", delta)
        add(f"ntheta {delta}%{other}", [a, P.make_ntheta("n0", a, other)],
            {"n0": ntheta1}, {"n0": ntheta2}, {"n0": 0})

    # ~Theta nad skladowa o NIEZEROWYM ogonie. Bez tego przypadku korpus nie
    # rozroznia reguly dokladnej od samego przeliczenia ogona skladowej przez
    # takt: rozjazd wymaga W_src > 0 (w kampanii K24 tylko 0,8% wezlow `~Theta`).
    #
    # s0 = 1/16; n0 = s0&1/6 ma Delta = (1/16*1/6)/(1/6-1/16) = 1/10.
    #   idx(n) = n + ceil((n+1)*(1/10)/(1/6)) = n + ceil(3(n+1)/5),
    #   deficyt ceil((idx(n)+1)*5/8) - 1 - n wynosi 1 dla n = 0..3 i 0 od n = 4,
    #   wiec W(n0) = 1, O(n0) = 0 (odwzorowanie nieujemne od zera).
    # n1 = n0%1/5 ma Delta = (1/10*1/5)/(1/5-1/10) = 1/5, a idx(n) = n + floor(n) = 2n:
    #   deficyt = ceil((2n+1+W_src)*(1/10)/(1/5)) - 1 - n = ceil((2n+2)/2) - 1 - n = 0,
    #   wiec W(n1) = 0 mimo ogona skladowej 1. Przy C2 deficyt podnosi sie do 1.
    a = _src("s0", Fraction(1, 16))
    th = P.make_theta("n0", a, Fraction(1, 6))
    add("ntheta over theta (skladowa z ogonem)", [a, th, P.make_ntheta("n1", th, Fraction(1, 5))],
        {"n0": 1, "n1": 0}, {"n0": 1, "n1": 1}, {"n0": 0, "n1": 0})

    # --- suma ---
    a, b = _src("s0", 1), _src("s1", 1)
    add("add 1+1", [a, b, P.make_add("n0", a, b)], {"n0": 0}, {"n0": 1}, {"n0": 0})
    a, b = _src("s0", Fraction(1, 3)), _src("s1", Fraction(1, 2))
    add("add 1/3+1/2", [a, b, P.make_add("n0", a, b)], {"n0": 1}, {"n0": 1}, {"n0": 0})
    a, b = _src("s0", Fraction(1, 10)), _src("s1", Fraction(1, 4))
    add("add 1/10+1/4", [a, b, P.make_add("n0", a, b)], {"n0": 2}, {"n0": 2}, {"n0": 0})

    # --- kompozycje ---
    # h = 1#2: O=0, W=2 (C2 3). n1 = h>2: O = 0+2 = 2, W = max(0, 2-2) = 0;
    # C2: deficyt (n-2+1+3) - (n+1) = 1, więc floor(1)+1 = 2.
    a, b = _src("s0", 1), _src("s1", 2)
    h = P.make_hash("n0", a, b)
    add("hash then shift", [a, b, h, P.make_shift("n1", h, 2)],
        {"n0": 2, "n1": 0}, {"n0": 3, "n1": 2}, {"n0": 0, "n1": 2})

    # n0 = a>1: O=1, W=0. n1 = n0#b, z = 2/3: lewa składowa czytana pod
    # indeksem floor(2n/3) >= 1 dopiero od n=2, prawa od n=0, więc O=2.
    # Spacer po slotach 2..9 (okres 3) daje maksimum deficytu na slotach,
    # w których wypada element b: W = 2 (C2 3).
    a, b = _src("s0", 1), _src("s1", 2)
    shifted = P.make_shift("n0", a, 1)
    add("shift under hash", [a, b, shifted, P.make_hash("n1", shifted, b)],
        {"n0": 0, "n1": 2}, {"n0": 0, "n1": 3}, {"n0": 1, "n1": 2})

    # Przesunięcie pod PRAWĄ składową przeplotu. Rekord 0 wypada na składową
    # prawą (indeks 0), której jeszcze nie ma, a rekord 1 na lewą, która już
    # jest — brakujące rekordy nie tworzą prefiksu, więc origin jest pierwszym
    # indeksem, od którego strumień jest ciągły: 1. Ogon: maksimum deficytu
    # wypada na slotach z elementem składowej prawej (n=3, 6), i wynosi 2.
    a, b = _src("s0", 1), _src("s1", 2)
    moved_right = P.make_shift("n0", b, 1)
    add("shift under hash (prawa)", [a, b, moved_right, P.make_hash("n1", a, moved_right)],
        {"n0": 0, "n1": 2}, {"n0": 0, "n1": 3}, {"n0": 1, "n1": 1})

    a, b, c = _src("s0", 1), _src("s1", 2), _src("s2", 3)
    h1 = P.make_hash("n0", a, b)
    add("hash of hash", [a, b, c, h1, P.make_hash("n1", h1, c)],
        {"n0": 2, "n1": 5}, {"n0": 3, "n1": 6}, {"n0": 0, "n1": 0})

    # --- kompozycje wprowadzone w K24p: propagacja początku logicznego ---
    # Dwa przesunięcia: origin sumuje się, ogon zostaje zerowy.
    a = _src("s0", Fraction(1, 10))
    first = P.make_shift("n0", a, 2)
    add("shift then shift", [a, first, P.make_shift("n1", first, 3)],
        {"n0": 0, "n1": 0}, {"n0": 0, "n1": 0}, {"n0": 2, "n1": 5})

    # Okno, potem projekcja: origin i ogon przechodzą bez zmian.
    a = _src("s0", Fraction(1, 10), 2)
    window = P.make_agse("n0", a, 1, 4)
    add("agse then pass", [a, window, P.make_pass("n1", window)],
        {"n0": 1, "n1": 1}, {"n0": 2, "n1": 3}, {"n0": 3, "n1": 3})

    # Różnica nad przesunięciem: origin przechodzi przez odwzorowanie
    # ceil(n*Delta_out/Delta_src) = 3n, więc próg 3n >= 2 daje O = 1.
    a = _src("s0", Fraction(1, 100))
    moved = P.make_shift("n0", a, 2)
    add("sub over shift", [a, moved, P.make_sub("n1", moved, Fraction(3, 100))],
        {"n0": 0, "n1": 0}, {"n0": 0, "n1": 0}, {"n0": 2, "n1": 1})

    # Suma okna z jego własnym źródłem: origin okna (3) przechodzi na sumę,
    # bo składowa szybsza jest adresowana tożsamościowo. Deficyt stały = 1.
    a = _src("s0", Fraction(1, 10), 2)
    window2 = P.make_agse("n0", a, 1, 4)
    add("add over agse", [a, window2, P.make_add("n1", window2, a)],
        {"n0": 1, "n1": 1}, {"n0": 2, "n1": 3}, {"n0": 3, "n1": 3})

    # --- kompozycje wprowadzone w K24d: przeplot o OBU składowych z ogonem ---
    #
    # Powód dołożenia jest wynikiem sam w sobie. Korpus K24p nie miał ani
    # jednego węzła `#`, na którym zastąpiona postać O(1) różni się od reguły
    # dokładnej: różnica wymaga, żeby OBIE składowe miały niezerowy ogon,
    # a wszystkie przypadki `#` korpusu miały co najwyżej jedną taką składową.
    # Bramka mutantów przechodziła więc również dla postaci obalonej w K24 —
    # ta sama pułapka, którą wykryto w bramce `ctest` silnika (§14.14 planu).
    #
    # (1#1) # (1#1/2). Składowe: n0 = 1#1 -> Delta 1/2, W = 1 (jak w korpusie
    # wyżej); n1 = 1#(1/2) -> Delta 1/3, W = 1 (slot 0 czyta s3[0] dostępny
    # w 1/2, co daje ceil((1/2)/(1/3))-1 = 1; slot 2 czyta s2[0] i daje 0).
    # Węzeł n2: z = 2/5, Delta = 1/5, okres p+q = 5, spacer po slotach
    #   n=0 -> n1[0]: ceil(2*(1/3)/(1/5))-1-0 = 4-1 = 3
    #   n=1 -> n1[1]: ceil(3*(1/3)/(1/5))-1-1 = 5-2 = 3
    #   n=2 -> n0[0]: ceil(2*(1/2)/(1/5))-1-2 = 5-3 = 2
    #   n=3 -> n1[2]: ceil(4*(1/3)/(1/5))-1-3 = 7-4 = 3
    #   n=4 -> n0[1]: ceil(3*(1/2)/(1/5))-1-4 = 8-5 = 3
    # daje W = 3. Postać O(1) dałaby max(3, 2+2) = 4 — o slot za dużo.
    s0, s1 = _src("s0", 1), _src("s1", 1)
    s2, s3 = _src("s2", 1), _src("s3", Fraction(1, 2))
    left = P.make_hash("n0", s0, s1)
    right = P.make_hash("n1", s2, s3)
    add("hash of two hashes (obie składowe z ogonem)",
        [s0, s1, s2, s3, left, right, P.make_hash("n2", left, right)],
        {"n0": 1, "n1": 1, "n2": 3}, {"n0": 2, "n1": 2, "n2": 6},
        {"n0": 0, "n1": 0, "n2": 0})

    # Przeplot, w którym maksimum deficytu wypada w DRUGIEJ połowie okresu
    # fazowego — kontrola tego, że przegląd musi objąć cały okres.
    # n0 = s0@(1,1), F = 3, Delta_src = 3: Delta = 3*1/3 = 1, O = 0,
    #   W = ceil(3*(1+0)/1) - 1 = 2.
    # n1 = n0 # s1, Delta_a = 1, Delta_b = 2: z = 2/3, Delta = 2/3,
    #   okres p+q = 3, spacer:
    #   n=0 -> s1[0]:  ceil(1*2/(2/3))-1-0 = 3-1 = 2
    #   n=1 -> n0[0]:  ceil(3*1/(2/3))-1-1 = 5-2 = 3
    #   n=2 -> n0[1]:  ceil(4*1/(2/3))-1-2 = 6-3 = 3
    # daje W = 3, ale maksimum pojawia się dopiero na slocie 1 — przegląd
    # skrócony do połowy okresu (slot 0) zwróciłby 2.
    a = _src("s0", 3, 3)
    b = _src("s1", 2)
    window3 = P.make_agse("n0", a, 1, 1)
    add("hash with max in the second half of the period",
        [a, b, window3, P.make_hash("n1", window3, b)],
        {"n0": 2, "n1": 3}, {"n0": 3, "n1": 5}, {"n0": 0, "n1": 0})

    return out
