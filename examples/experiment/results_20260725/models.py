#!/usr/bin/env python3
"""Cztery realizacje tej samej semantyki przeplotu.

1. `beatty_online`  — wzór podłogowy używany przez silnik RetractorDB.
2. `csdf_explicit`  — aktor CSDF o P jawnych fazach; tablica faz wygenerowana
                      z oracle'a (scalanie terminów).
3. `csdf_lookup`    — ten sam aktor CSDF, ale tablica faz wygenerowana ze
                      wzoru Beatty'ego; wariant kontrolny rozdzielający koszt
                      reprezentacji od kosztu konstrukcji tablicy.
4. `sdf_block`      — klasyczny aktor SDF: jedno odpalenie konsumuje b/g
                      tokenów z A i a/g z B, produkując blok P tokenów.

Wszystkie zwracają ciąg par (source-id, source-index) — obserwowalny ślad
zdefiniowany w `reference.py`.
"""

from fractions import Fraction

from reference import SOURCE_A, SOURCE_B, interleave_trace, ratio_terms


def beatty_online(delta_a, delta_b, count):
    """Wzór z silnika: d_n rozstrzygane różnicą podłóg n*z i (n+1)*z."""
    delta_a = Fraction(delta_a)
    delta_b = Fraction(delta_b)
    z = delta_b / (delta_a + delta_b)

    trace = []
    for n in range(count):
        floor_n = (n * z).numerator // (n * z).denominator
        step = (n + 1) * z
        floor_n1 = step.numerator // step.denominator
        if floor_n == floor_n1:
            trace.append((SOURCE_B, n - floor_n1))
        else:
            trace.append((SOURCE_A, floor_n))
    return trace


def _phase_word_from_oracle(delta_a, delta_b, period):
    return [source for source, _ in interleave_trace(delta_a, delta_b, period)]


def _phase_word_from_beatty(delta_a, delta_b, period):
    return [source for source, _ in beatty_online(delta_a, delta_b, period)]


def _run_phase_actor(phase_word, count):
    """Aktor cyklostatyczny: faza n mod P konsumuje jeden token ze
    wskazanego wejścia i produkuje jeden token wyjściowy."""
    period = len(phase_word)
    next_index = {SOURCE_A: 0, SOURCE_B: 0}
    trace = []
    for n in range(count):
        source = phase_word[n % period]
        trace.append((source, next_index[source]))
        next_index[source] += 1
    return trace


def csdf_explicit(delta_a, delta_b, count):
    _, _, _, period = ratio_terms(delta_a, delta_b)
    return _run_phase_actor(_phase_word_from_oracle(delta_a, delta_b, period), count)


def csdf_lookup(delta_a, delta_b, count):
    _, _, _, period = ratio_terms(delta_a, delta_b)
    return _run_phase_actor(_phase_word_from_beatty(delta_a, delta_b, period), count)


def sdf_block(delta_a, delta_b, count):
    """Aktor blokowy o stałych rate'ach.

    Rate'y wyprowadzone arytmetycznie ze stosunku interwałów: jedno odpalenie
    konsumuje b/g tokenów z A oraz a/g z B i produkuje P = (a+b)/g tokenów.
    Kolejność wewnątrz bloku pochodzi ze wzoru Beatty'ego, a nie z oracle'a.
    """
    a, b, g, period = ratio_terms(delta_a, delta_b)
    block_order = _phase_word_from_beatty(delta_a, delta_b, period)

    consume_a = b // g
    consume_b = a // g
    if block_order.count(SOURCE_A) != consume_a or block_order.count(SOURCE_B) != consume_b:
        raise AssertionError(
            "rate'y aktora SDF nie zgadzają się z zawartością bloku: "
            f"delta_a/delta_b = {a}/{b}, blok = {block_order}"
        )

    next_index = {SOURCE_A: 0, SOURCE_B: 0}
    trace = []
    while len(trace) < count:
        for source in block_order:
            trace.append((source, next_index[source]))
            next_index[source] += 1
    return trace[:count]


MODELS = {
    "beatty_online": beatty_online,
    "csdf_explicit": csdf_explicit,
    "csdf_lookup": csdf_lookup,
    "sdf_block": sdf_block,
}


def representation_cost(delta_a, delta_b):
    """Strukturalne koszty reprezentacji — wielkości wyprowadzone, nie mierzone
    czasowo.

    * `phases`          — liczba jawnych faz w statycznym opisie;
    * `static_words`    — rozmiar statycznego opisu w słowach;
    * `startup_tokens`  — ile tokenów wejściowych musi być dostępnych, zanim
                          realizacja wyprodukuje pierwszy token wyjściowy;
    * `input_highwater` — maksymalna liczba tokenów wejściowych trzymanych
                          jednocześnie przez realizację.
    """
    a, b, g, period = ratio_terms(delta_a, delta_b)
    consume_a = b // g
    consume_b = a // g
    block_tokens = consume_a + consume_b

    return {
        "P": period,
        "beatty_online": {
            "phases": 1,
            "static_words": 3,  # licznik n oraz para (licznik, mianownik) z
            "startup_tokens": 1,
            "input_highwater": 1,
        },
        "csdf_explicit": {
            "phases": period,
            "static_words": period,
            "startup_tokens": 1,
            "input_highwater": 1,
        },
        "csdf_lookup": {
            "phases": period,
            "static_words": period,
            "startup_tokens": 1,
            "input_highwater": 1,
        },
        "sdf_block": {
            "phases": 1,
            "static_words": period + 2,  # kolejność w bloku + dwa rate'y
            "startup_tokens": block_tokens,
            "input_highwater": block_tokens,
        },
    }
