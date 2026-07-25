#!/usr/bin/env python3
"""Niezależny oracle semantyki przeplotu strumieni o wymiernych interwałach.

Definicja odniesienia NIE korzysta ze wzoru Beatty'ego użytego w silniku.
Przeplot jest tu zdefiniowany jako scalenie dwóch arytmetycznych siatek
terminów:

    termin A[k] = (k+1) * delta_a
    termin B[j] =  j    * delta_b

scalanych rosnąco, z remisami rozstrzyganymi na korzyść A.

Ta definicja jest operacyjna ("kto ma wcześniejszy termin, ten idzie
pierwszy") i nie zawiera żadnej podłogi ani sufitu. Jeżeli zgadza się ze
wzorem podłogowym silnika, jest to zgodność dwóch niezależnych konstrukcji,
a nie zgodność kodu z jego własnym przepisaniem.

Przesunięcie o jeden interwał po stronie A odpowiada jednoslotowemu
opóźnieniu Theta opisanemu w dokumentacji algebry.
"""

from fractions import Fraction

SOURCE_A = "A"
SOURCE_B = "B"


def interleave_trace(delta_a, delta_b, count, tie_to_a=True):
    """Zwraca listę par (source-id, source-index) długości `count`.

    `tie_to_a` jest parametrem, a nie wbudowaną decyzją: reguła remisu to
    jedyne miejsce, w którym dwie poprawne implementacje mogą się rozejść,
    więc musi dać się ją zmienić i przetestować.
    """
    delta_a = Fraction(delta_a)
    delta_b = Fraction(delta_b)
    if delta_a <= 0 or delta_b <= 0:
        raise ValueError("interwały muszą być dodatnie")

    trace = []
    next_a = 0
    next_b = 0
    while len(trace) < count:
        time_a = (next_a + 1) * delta_a
        time_b = next_b * delta_b
        if time_a < time_b or (time_a == time_b and tie_to_a):
            trace.append((SOURCE_A, next_a))
            next_a += 1
        else:
            trace.append((SOURCE_B, next_b))
            next_b += 1
    return trace


def output_interval(delta_a, delta_b):
    """Interwał wyjściowy przeplotu: 1/delta_c = 1/delta_a + 1/delta_b."""
    delta_a = Fraction(delta_a)
    delta_b = Fraction(delta_b)
    return (delta_a * delta_b) / (delta_a + delta_b)


def ratio_terms(delta_a, delta_b):
    """Zwraca (a, b, g, P) dla stosunku interwałów delta_a/delta_b = a/b.

    P = (a+b)/g jest kandydatem na minimalny okres słowa wyboru.
    """
    ratio = Fraction(delta_a) / Fraction(delta_b)
    a = ratio.numerator
    b = ratio.denominator
    from math import gcd

    g = gcd(a, b)
    return a, b, g, (a + b) // g


def labels(trace):
    """Samo słowo wyboru, bez indeksów źródłowych."""
    return [source for source, _ in trace]


def minimal_period(word):
    """Najmniejszy okres słowa, sprawdzany na dostarczonym prefiksie.

    Zwraca None, jeżeli prefiks jest za krótki, by potwierdzić okres.
    """
    n = len(word)
    for period in range(1, n // 2 + 1):
        if all(word[i] == word[i % period] for i in range(n)):
            return period
    return None
