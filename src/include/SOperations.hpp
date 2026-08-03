#pragma once

#include <cstdlib>
#include <iostream>
#include <numeric>

#include <boost/rational.hpp>

using namespace boost;

// https://stackoverflow.com/questions/35971827/c-boost-rational-class-floor-function

constexpr int floorR(boost::rational<int> const &num) { return static_cast<int>(num.numerator() / num.denominator()); }

constexpr int ceilR(boost::rational<int> const &num) {
  auto inum = static_cast<int>(num.numerator() / num.denominator());
  if (num == inum) return inum;
  return (num.numerator() > 0) ? ++inum : --inum;
}

// Przeplot (definicja formalna): dla i-tego rekordu strumienia C = A#B (i od 0)
//   c_i = b_{i-⌊iz⌋}  gdy ⌊iz⌋ == ⌊(i+1)z⌋   (zwraca true,  retPos = indeks w B)
//   c_i = a_{⌊iz⌋}    w przeciwnym razie      (zwraca false, retPos = indeks w A)
// gdzie z = deltaB/(deltaA+deltaB). retPos jest indeksem POSTĘPUJĄCYM (0-bazowym)
// elementu strumienia składowego.
inline bool Hash(const rational<int> &deltaA, const rational<int> &deltaB, const int i, int &retPos) {
  if (deltaA <= 0) {
    std::cerr << "\nFATAL: Hash: deltaA must be > 0\n";
    std::exit(EXIT_FAILURE);
  }
  if (deltaB <= 0) {
    std::cerr << "\nFATAL: Hash: deltaB must be > 0\n";
    std::exit(EXIT_FAILURE);
  }
  const rational<int> zet = deltaB / (deltaA + deltaB);
  bool ret                = floorR(zet * i) == floorR(zet * (i + 1));

  if (ret) {
    retPos = i - floorR(zet * i);  // B
  } else {
    retPos = floorR(zet * i);  // A
  }
  return ret;
}

// Rozplot (definicja formalna): pozycje elementów składowych w C = A#B, i od 0:
//   Θ  (lewa składowa):  a_i = c_{i+⌈(i+1)·deltaA/deltaB⌉}  -> Div(deltaA, deltaB, i)
//   ~Θ (prawa składowa): b_i = c_{i+⌊i·deltaB/deltaA⌋}      -> Mod(deltaA, deltaB, i)
// Wynik jest indeksem POSTĘPUJĄCYM (0-bazowym) w strumieniu C.

constexpr int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + ceilR((i + 1) * deltaA / deltaB);
}

constexpr int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + floorR(i * deltaB / deltaA);
}

// Różnica C-Delta wybiera z C składową o docelowym interwale Delta.
// Delta nie może być mniejsza od interwału C (wynik nie może wymagać
// rekordów szybszych niż źródło). Dla równych interwałów wybór jest
// tożsamością, a dla wolniejszego wyniku wybieramy indeks ceil(i*Delta/Dc).
constexpr int Subtract(const rational<int> &deltaSource, const rational<int> &deltaTarget, const int i) {
  if (deltaSource == deltaTarget) return i;
  return ceilR(i * deltaTarget / deltaSource);
}

// Suma strumieni (definicja formalna): dla C = A+B, gdzie Delta_c = min(Delta_a, Delta_b):
//   c_n = (a_n, b_{⌊n·Delta_a/Delta_b⌋})   gdy Delta_a <= Delta_b
//   c_n = (a_{⌊n·Delta_b/Delta_a⌋}, b_n)   w przeciwnym razie
// Oba przypadki mają jedną postać: indeks składowej o interwale deltaSrc to
// ⌊n·Delta_c/deltaSrc⌋ — dla składowej szybszej (deltaSrc == Delta_c) daje n.
// Wynik jest indeksem POSTĘPUJĄCYM (0-bazowym) w strumieniu składowej.
constexpr int Add(const rational<int> &deltaOut, const rational<int> &deltaSrc, const int n) {
  if (deltaOut == deltaSrc) return n;
  return floorR(n * deltaOut / deltaSrc);
}

constexpr int agse(int offset, int step) { return floorR(boost::rational<int>(offset) / boost::rational<int>(step)); }

// Dla kolejnych okien reszta (n*step) mod sourceWidth przebiega przez
// wielokrotności gcd(step, sourceWidth). Największe wyprzedzenie ostatniego
// pola okna względem jego slotu wynosi:
//   floor((abs(length)-1)/gcd(step, sourceWidth))*gcd(step, sourceWidth).
// Do tej fazy dochodzi ogon producenta przeliczony na pola. Odczyt historii
// odbywa się przed domknięciem wszystkich producentów aktywnych w tym samym
// takcie (również przy różnych interwałach), stąd nierówność ostra.
constexpr int AgseStartupLatency(const int sourceWidth, const int step, const int length, const int sourceLatency) {
  const int lengthAbs  = length < 0 ? -length : length;
  const int phaseUnit  = std::gcd(sourceWidth, step);
  const int phaseBound = ((lengthAbs - 1) / phaseUnit) * phaseUnit;
  const int numerator  = sourceLatency * sourceWidth + phaseBound;
  return floorR(boost::rational<int>(numerator, step)) + 1;
}

// C-Delta wybiera c_{ceil(n*Delta/Dc)}. Ułamkowa faza może wyprzedzać
// fizyczny slot najwyżej o (q-1)/q rekordu źródła, gdzie q jest mianownikiem
// Delta/Dc. Dla deklaracji również faza całkowita wymaga jednego slotu,
// ponieważ źródło jest publikowane po konsumentach w tym samym takcie.
constexpr int SubtractStartupLatency(const rational<int> &deltaSource, const rational<int> &deltaTarget, const int sourceLatency,
                                     const bool sourceDeclared) {
  const auto ratio = deltaTarget / deltaSource;
  const int q      = ratio.denominator();
  const rational<int> phase(q - 1, q);
  if (sourceDeclared) return floorR(phase / ratio) + 1;
  return ceilR((rational<int>(sourceLatency) + phase) / ratio);
}
