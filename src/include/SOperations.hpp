#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>

#include <boost/rational.hpp>

using namespace boost;

/// Funkcja floorR powinna:
/// - [soperations.floor-r.1~1] zwracać część całkowitą num, licząc numerator / denominator w int.
/// (!sr_problem:nazwa obiecuje podłogę, a dla ujemnego num wynik jest obcięciem w stronę zera, np. floorR(-1/2) == 0; wołający dziś podają wartości nieujemne.)
///
/// Wzorzec: https://stackoverflow.com/questions/35971827/c-boost-rational-class-floor-function
constexpr int floorR(boost::rational<int> const &num) { return static_cast<int>(num.numerator() / num.denominator()); }

/// Funkcja floorDiv powinna:
/// - [soperations.floor-div.1~1] zwracać iloraz numerator / denominator zaokrąglony w dół (w stronę minus nieskończoności), także dla ujemnego licznika lub mianownika, w arytmetyce int64.
///
/// Dzielenie całkowite z zaokrągleniem W DÓŁ. Wbudowane `/` zaokrągla w stronę zera, co dla
/// ujemnych liczników daje inny rekord niż model zdarzeniowy - a ujemne pozycje spłaszczone
/// pojawiają się naturalnie w oknie stemplowanym końcem przedziału (n*step-(|L|-1)).
/// Argumenty w int64: pozycja n*step rośnie jak liczba rekordów źródła razy jego szerokość
/// i w int pękała po ok. 2^31/F rekordach źródła.
constexpr std::int64_t floorDiv(const std::int64_t numerator, const std::int64_t denominator) {
  const std::int64_t quotient = numerator / denominator;
  return (numerator % denominator != 0 && ((numerator < 0) != (denominator < 0))) ? quotient - 1 : quotient;
}

/// Funkcja ceilR powinna:
/// - [soperations.ceil-r.1~1] zwracać num, gdy jest całkowite, a w przeciwnym razie zaokrąglać je od zera: część całkowitą obciętą w stronę zera powiększać o 1 dla dodatniego num i pomniejszać o 1 dla ujemnego.
constexpr int ceilR(boost::rational<int> const &num) {
  auto inum = static_cast<int>(num.numerator() / num.denominator());
  if (num == inum) return inum;
  return (num.numerator() > 0) ? ++inum : --inum;
}

/// Funkcja Hash powinna:
/// - [soperations.hash.1~1] dla rekordu i (od 0) przeplotu C = A#B przy z = deltaB/(deltaA+deltaB) zwracać true i ustawiać retPos = i - ⌊iz⌋ (indeks w B), gdy ⌊iz⌋ == ⌊(i+1)z⌋, a w przeciwnym razie zwracać false i ustawiać retPos = ⌊iz⌋ (indeks w A).
/// - [soperations.hash.2~1] zakładać dodatnie deltaA i deltaB, sprawdzane asercją wyłącznie w buildzie Debug.
///
/// Przeplot (definicja formalna): dla i-tego rekordu strumienia C = A#B (i od 0)
///   c_i = b_{i-⌊iz⌋}  gdy ⌊iz⌋ == ⌊(i+1)z⌋   (zwraca true,  retPos = indeks w B)
///   c_i = a_{⌊iz⌋}    w przeciwnym razie      (zwraca false, retPos = indeks w A)
/// gdzie z = deltaB/(deltaA+deltaB). retPos jest indeksem POSTĘPUJĄCYM (0-bazowym)
/// elementu strumienia składowego.
inline bool Hash(const rational<int> &deltaA, const rational<int> &deltaB, const int i, int &retPos) {
  // Kompilator sprawdza dodatniosc interwalow przed rachunkiem origin i ogona.
  // Parametry sa stale w planie, wiec w Release nie sprawdzamy ich dla kazdego rekordu.
  assert(deltaA > 0);
  assert(deltaB > 0);
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

/// Funkcja Div powinna:
/// - [soperations.div.1~1] zwracać indeks postępujący w przeplocie C elementu a_i lewej składowej (Θ): i + ceilR((i+1)*deltaA/deltaB).
constexpr int Div(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + ceilR((i + 1) * deltaA / deltaB);
}

/// Funkcja Mod powinna:
/// - [soperations.mod.1~1] zwracać indeks postępujący w przeplocie C elementu b_i prawej składowej (~Θ): i + floorR(i*deltaB/deltaA).
constexpr int Mod(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, const int i) {
  return i + floorR(i * deltaB / deltaA);
}

/// Funkcja Subtract powinna:
/// - [soperations.subtract.1~1] zwracać indeks rekordu C wybranego dla rekordu i różnicy C-Delta: i dla równych interwałów, a w przeciwnym razie ceilR(i*deltaTarget/deltaSource).
///
/// Różnica C-Delta wybiera z C składową o docelowym interwale Delta.
/// Delta nie może być mniejsza od interwału C (wynik nie może wymagać
/// rekordów szybszych niż źródło). Dla równych interwałów wybór jest
/// tożsamością, a dla wolniejszego wyniku wybieramy indeks ceil(i*Delta/Dc).
constexpr int Subtract(const rational<int> &deltaSource, const rational<int> &deltaTarget, const int i) {
  if (deltaSource == deltaTarget) return i;
  return ceilR(i * deltaTarget / deltaSource);
}

/// Funkcja Add powinna:
/// - [soperations.add.1~1] zwracać indeks postępujący rekordu składowej o interwale deltaSrc czytanego przez rekord n sumy: n, gdy deltaOut == deltaSrc, a w przeciwnym razie floorR(n*deltaOut/deltaSrc).
///
/// Suma strumieni (definicja formalna): dla C = A+B, gdzie Delta_c = min(Delta_a, Delta_b):
///   c_n = (a_n, b_{⌊n·Delta_a/Delta_b⌋})   gdy Delta_a <= Delta_b
///   c_n = (a_{⌊n·Delta_b/Delta_a⌋}, b_n)   w przeciwnym razie
/// Oba przypadki mają jedną postać: indeks składowej o interwale deltaSrc to
/// ⌊n·Delta_c/deltaSrc⌋ - dla składowej szybszej (deltaSrc == Delta_c) daje n.
/// Wynik jest indeksem POSTĘPUJĄCYM (0-bazowym) w strumieniu składowej.
constexpr int Add(const rational<int> &deltaOut, const rational<int> &deltaSrc, const int n) {
  if (deltaOut == deltaSrc) return n;
  return floorR(n * deltaOut / deltaSrc);
}

/// Funkcja roundedAwayFromZero powinna:
/// - [soperations.rounded-away-from-zero.1~1] zwracać iloraz numerator / denominator w int64: dokładny, gdy dzielenie jest bez reszty, a w przeciwnym razie zaokrąglony od zera (jak ceilR).
///
/// Rachunek w int64, bo (1+W_src)*F przekracza int juz dla trzech okien `@(1,65536)` w lancuchu
/// (A2 M11). Zaokraglenie jest to samo co w ceilR() - od zera - wiec w zakresie int wynik sie nie
/// zmienia; wynik spoza int odrzuca kompilator jako blad planu.
constexpr std::int64_t roundedAwayFromZero(const std::int64_t numerator, const std::int64_t denominator) {
  const std::int64_t quotient = numerator / denominator;
  if (numerator % denominator == 0) return quotient;
  return ((numerator < 0) == (denominator < 0)) ? quotient + 1 : quotient - 1;
}

/// Funkcja AgseStartupLatency powinna:
/// - [soperations.agse-startup-latency.1~1] zwracać ogon okna AGSE W = roundedAwayFromZero((1 + sourceLatency) * sourceWidth, step) - 1, liczony w int64.
///
/// Okno AGSE jest stemplowane KOŃCEM przedziału: rekord n obejmuje spłaszczone
/// pozycje źródła n*step-(abs(length)-1) ... n*step, więc najnowsze pole okna
/// leży dokładnie w pozycji n*step. Dzięki temu indeks logiczny rekordu okna
/// oznacza tę samą chwilę co indeks logiczny źródła i złączenie okna z jego
/// własnym źródłem (potok FIR) nie wyprzedza sygnału.
///
/// Cena konwencji: dla małych n okno sięga przed początek źródła. Te rekordy
/// nie powstają - patrz AgseLogicalOrigin() i query::logicalOrigin. NULL nie
/// jest tu rezerwacją miejsca (zasada brzegu), więc brakujące okna są nieobecne,
/// a nie wypełnione.
///
/// Ogon: najnowsze pole n*step leży w rekordzie źródła floor(n*step/F),
/// F = sourceWidth. Rekord ten jest określony w chwili
/// (floor(n*step/F)+1+W_src)*Delta_src, a slot n konsumenta kończy się w chwili
/// (n+1+W)*Delta_out, gdzie Delta_out = Delta_src*step/F. Podstawiając
/// floor(n*step/F) = (n*step - r_n)/F, r_n = (n*step) mod F, warunek dla każdego n:
///   W >= (F*(1+W_src) - r_n)/step - 1.
/// Reszty r_n przebiegają wielokrotności gcd(step,F) okresowo, więc minimum
/// r_n = 0 jest osiągane (n=0) niezależnie od origin. Stąd postać zamknięta
///   W = ceil( (1+W_src)*F / step ) - 1.
///
/// Wobec postaci sprzed przestemplowania (K24/H10a) znika człon fazowy
///   P = floor((abs(length)-1)/gcd(step,F))*gcd(step,F),
/// bo rozpiętość okna nie jest już czekaniem - przeszła do origin. Ogon zatem
/// maleje, ale nie zaniża: to samo czekanie liczy teraz origin + ogon.
constexpr std::int64_t AgseStartupLatency(const int sourceWidth, const int step, const int sourceLatency) {
  return roundedAwayFromZero((std::int64_t{1} + sourceLatency) * sourceWidth, step) - 1;
}

/// Funkcja AgseLogicalOrigin powinna:
/// - [soperations.agse-logical-origin.1~1] zwracać początek logiczny strumienia okna O = roundedAwayFromZero(sourceOrigin * sourceWidth + |length| - 1, step), czyli najmniejsze n, dla którego całe okno leży w istniejącej części źródła, liczony w int64.
///
/// Początek logiczny strumienia okna: najmniejsze n, dla którego całe okno
/// n*step-(abs(length)-1) ... n*step mieści się w istniejącej części źródła.
/// Źródło zaczyna się od pozycji spłaszczonej sourceOrigin*F, więc
///   n*step - (abs(length)-1) >= sourceOrigin*F
///   =>  O = ceil( (sourceOrigin*F + abs(length) - 1) / step ).
///
/// Rachunek w int64 z tego samego powodu co w AgseStartupLatency(): sourceOrigin*F dla dwoch okien
/// `@(1,65536)` w lancuchu to ok. 2^32.
constexpr std::int64_t AgseLogicalOrigin(const int sourceWidth, const int step, const int length, const int sourceOrigin) {
  const std::int64_t lengthAbs = length < 0 ? -std::int64_t{length} : length;
  return roundedAwayFromZero((std::int64_t{sourceOrigin} * sourceWidth) + lengthAbs - 1, step);
}

/// Funkcja AddStartupLatency powinna:
/// - [soperations.add-startup-latency.1~1] zwracać ogon sumy strumieni względem jednej składowej: ceilR((1 + sourceLatency) * deltaSource / deltaTarget) - 1; maksimum po składowych wyznacza wołający.
///
/// Suma strumieni czyta składową o interwale deltaSource po indeksie
/// floor(n*Delta_out/Delta_src) (patrz Add()). Rekord o tym indeksie jest
/// określony w chwili (floor(...)+1+W_src)*Delta_src, najpóźniej względem
/// slotu przy n=0, co po rozwiązaniu względem ogona daje
///   W = ceil( (1+W_src)*Delta_src/Delta_out ) - 1
/// dla każdej składowej z osobna; ogon węzła to maksimum po składowych.
///
/// K24/H10a: poprzednia postać ceil(W_src*Delta_src/Delta_out) zgadzała się
/// z granicą zdarzeniową dla 42,3% węzłów `+` w korpusie i zaniżała ogon dla
/// pozostałych - dla składowej o zerowym ogonie dawała zero niezależnie od
/// tego, jak wolna jest składowa. Postać powyższa zgadza się dla 2527 z 2527.
constexpr int AddStartupLatency(const rational<int> &deltaSource, const rational<int> &deltaTarget, const int sourceLatency) {
  return ceilR(rational<int>(1 + sourceLatency) * deltaSource / deltaTarget) - 1;
}

/// Funkcja PhaseStartupLatency powinna:
/// - [soperations.phase-startup-latency.1~1] zwracać ogon operatora o afinicznym odwzorowaniu indeksu: max(0, ceilR((phaseBound + sourceLatency + 1) / ratio) - 1), gdzie ratio = Delta_out/Delta_src, a phaseBound to kres części fazowej indeksu.
///
/// Ogon operatorów jednoargumentowych o afinicznym odwzorowaniu indeksu: `-`, `Θ` i `~Θ`.
/// Jedna postać, trzy stałe fazowe.
///
/// Rekord n czyta rekord idx(n) składowej, a odwzorowanie rozkłada się na część liniową
/// i fazową: idx(n) = n*r + e(n), gdzie r = Delta_out/Delta_src. Warunek dostępności
///   W >= ceil( (idx(n) + 1 + W_src) / r ) - 1 - n
/// po podstawieniu rozkładu upraszcza się, bo n*r/r = n jest całkowite i wychodzi przed
/// sufit, kasując -n:
///   W >= ceil( (e(n) + 1 + W_src) / r ) - 1.
/// Prawa strona zależy od n wyłącznie przez e(n), więc maksimum wypada tam, gdzie e(n)
/// osiąga kres. Kres jest OSIĄGANY - reszty przebiegają wszystkie klasy modulo mianownik,
/// bo po skróceniu gcd = 1 - i dlatego postać jest DOKŁADNA, a nie oszacowaniem z góry.
///
/// Do 2026-08-18 każdy z trzech operatorów miał tu własną regułę zawyżającą o slot
/// (`-` 19,1% zgodności z modelem zdarzeniowym, `Θ` 59,7%, `~Θ` 99,2%). Wspólną
/// przyczyną było rozbicie rachunku na "przeliczenie ogona składowej przez takt plus
/// stały człon własny": przeliczenie zakłada najgorsze wyrównanie fazowe niezależnie od
/// tego, który rekord konsument naprawdę czyta. Wyprowadzenie, dowód osiągalności kresu
/// i weryfikacja: rdb-experiment/investigation_K24H10/DERIVATION.md §5. Zgodność
/// z modelem zdarzeniowym na korpusie K24: 4329/4329, 2578/2578 i 2503/2503 węzłów
/// (ziarno 20260804), tyleż na 20260807, zero zaniżeń w żadnej z klas.
constexpr int PhaseStartupLatency(const rational<int> &phaseBound, const rational<int> &ratio, const int sourceLatency) {
  return std::max(0, ceilR((phaseBound + sourceLatency + 1) / ratio) - 1);
}

/// Funkcja SubtractStartupLatency powinna:
/// - [soperations.subtract-startup-latency.1~1] zwracać ogon różnicy C-Delta: PhaseStartupLatency((q-1)/q, deltaTarget/deltaSource, sourceLatency), gdzie q to mianownik skróconego deltaTarget/deltaSource.
///
/// C-Delta wybiera c_{ceil(n*Delta/Dc)}, czyli e(n) = (q - n*p mod q)/q przy p/q = Delta/Dc
/// po skróceniu. Kres (q-1)/q jest osiągany dla n spełniającego n*p = 1 (mod q).
constexpr int SubtractStartupLatency(const rational<int> &deltaSource, const rational<int> &deltaTarget,
                                     const int sourceLatency) {
  const auto ratio = deltaTarget / deltaSource;
  const int q      = ratio.denominator();
  return PhaseStartupLatency(rational<int>(q - 1, q), ratio, sourceLatency);
}

/// Funkcja ThetaStartupLatency powinna:
/// - [soperations.theta-startup-latency.1~1] zwracać ogon rozplotu Θ: PhaseStartupLatency((a+b-1)/b, deltaTarget/deltaSource, sourceLatency), gdzie a/b to skrócone deltaTarget/other.
///
/// Θ czyta pozycję i + ceil((i+1)*Da/Db) przeplotu (patrz Div()). Przy a/b = Delta_out/param
/// po skróceniu faza wynosi (a-t)/b dla t = 0 i (a+b-t)/b dla t > 0, gdzie t = (n+1)*a mod b,
/// więc kres to (a+b-1)/b - osiągany, bo gcd(a,b) = 1. Dla ilorazu całkowitego (b = 1) kres
/// wynosi a i po podzieleniu przez r daje ogon własny ZERO, nie jeden.
constexpr int ThetaStartupLatency(const rational<int> &deltaSource, const rational<int> &deltaTarget, const rational<int> &other,
                                  const int sourceLatency) {
  const auto span = deltaTarget / other;
  return PhaseStartupLatency(rational<int>(span.numerator() + span.denominator() - 1, span.denominator()),
                             deltaTarget / deltaSource, sourceLatency);
}

/// Funkcja NThetaStartupLatency powinna:
/// - [soperations.n-theta-startup-latency.1~1] zwracać ogon rozplotu ~Θ: PhaseStartupLatency(0, deltaTarget/deltaSource, sourceLatency).
///
/// ~Θ czyta pozycję i + floor(i*Db/Da) (patrz Mod()), więc e(n) = -(n*a mod b)/b <= 0
/// i kres wynosi 0, osiągany dla n = 0. Ogon jest zatem samym przeliczeniem dostępności
/// pierwszego rekordu - bez zaokrąglenia ogona składowej w górę, które zawyżało wynik przy
/// niezerowym W_src.
constexpr int NThetaStartupLatency(const rational<int> &deltaSource, const rational<int> &deltaTarget, const int sourceLatency) {
  return PhaseStartupLatency(rational<int>(0), deltaTarget / deltaSource, sourceLatency);
}

// Najdłuższy okres fazowy przeplotu, dla którego ogon liczy się DOKŁADNIE.
// Powyżej rachunek wraca do postaci O(1), która zawyża - koszt przeglądu rośnie
// liniowo z p+q, a p+q jest iloczynem liczników i mianowników delt składowych.
// Korpus kampanijny K24 ma maksimum 24 557, więc próg nie jest w nim osiągany.
constexpr std::int64_t kHashPhaseScanLimit = 100'000;

/// Funkcja HashStartupLatency powinna:
/// - [soperations.hash-startup-latency.1~1] dla okresu fazowego p+q nie większego niż kHashPhaseScanLimit (p/q to skrócone deltaA/deltaB) zwracać dokładny ogon przeplotu: maksimum po i z [0, p+q) wartości ceil((j(i) + 1 + W_src(i)) * Delta_src(i) / deltaOut) - 1 - i, gdzie składową i jej indeks j(i) wybiera Hash, liczone w int64.
/// - [soperations.hash-startup-latency.2~1] powyżej kHashPhaseScanLimit zwracać oszacowanie O(1) max(conv(latencyA), conv(latencyB) + ceil((p+q-1)/p)), gdzie conv(W) = ceilR(W * Delta_src / deltaOut) dla W > 0 i 0 w przeciwnym razie.
/// - [soperations.hash-startup-latency.3~1] nigdy nie zaniżać ogona: zaniżenie oznacza rekord wyemitowany przed określeniem zależności.
///
/// Ogon przeplotu (`#`). Rekord i strumienia C = A#B niesie treść rekordu j(i)
/// jednej ze składowych, wybranej przez Hash(); jest określony w chwili
/// (j(i)+1+W_src(i))*Delta_src(i), a slot i konsumenta kończy się w chwili
/// (i+1+W)*Delta_c. Warunek dostępności dla każdego i:
///   W >= ceil( (j(i)+1+W_src(i)) * Delta_src(i) / Delta_c ) - 1 - i.
/// Wybór składowej i reszta i*z powtarzają się z okresem p+q, gdzie
/// Delta_a/Delta_b = p/q po skróceniu, więc maksimum po JEDNYM okresie jest
/// maksimum po wszystkich rekordach. Przegląd zaczyna się od zera: origin
/// przesuwa i indeks konsumenta, i indeks składowej o tyle samo, a kampania K24p
/// potwierdziła, że okno [0, p+q) i [O_c, O_c+p+q) dają tę samą wartość.
///
/// Wynik jest DOKŁADNY: 5960/5960 i 5998/5998 węzłów `#` korpusu K24p wobec
/// niezależnego modelu zdarzeniowego, zero zaniżeń (dwa ziarna). Zastąpiona
/// postać O(1) - max(conv(W_A), conv(W_B) + ceil((p+q-1)/p)) - zgadzała się
/// z granicą zdarzeniową w 92,1% węzłów i zawyżała ogon o slot w pozostałych:
/// człon fazowy chroni najgorszą fazę odczytu drugiego argumentu, ale nie wie,
/// czy ta faza w ogóle wypada na rekord, który czeka najdłużej.
///
/// Powyżej kHashPhaseScanLimit wraca postać O(1) - zawyżająca, więc bezpieczna:
/// zaniżenie ogona oznacza rekord wyemitowany przed określeniem zależności,
/// zawyżenie tylko slot opóźnienia.
inline int HashStartupLatency(const rational<int> &deltaA, const rational<int> &deltaB, const rational<int> &deltaOut,
                              const int latencyA, const int latencyB) {
  const auto ratio          = deltaA / deltaB;
  const std::int64_t period = static_cast<std::int64_t>(ratio.numerator()) + ratio.denominator();

  if (period > kHashPhaseScanLimit) {
    const auto swapped = deltaB / deltaA;
    const auto denom   = static_cast<std::int64_t>(swapped.denominator());
    const auto advance = static_cast<std::int64_t>(swapped.numerator());
    const int own      = static_cast<int>(((denom + advance - 2) / denom) + 1);
    const auto toSlots = [](const int w, const rational<int> &dSrc, const rational<int> &dDst) {
      return w <= 0 ? 0 : ceilR(rational<int>(w) * dSrc / dDst);
    };
    return std::max(toSlots(latencyA, deltaA, deltaOut), toSlots(latencyB, deltaB, deltaOut) + own);
  }

  std::int64_t result = 0;
  for (int index = 0; index < static_cast<int>(period); ++index) {
    int position         = 0;
    const bool fromB     = Hash(deltaA, deltaB, index, position);
    const auto &deltaSrc = fromB ? deltaB : deltaA;
    const int latencySrc = fromB ? latencyB : latencyA;
    // Rachunek w 64 bitach: (position+1+W)*num*den przekracza zakres int już dla
    // umiarkowanych delt, a rational<int> mnoży przed skróceniem. Oba człony są
    // dodatnie, więc ceil(a/b) = (a+b-1)/b.
    const auto numerator = static_cast<std::int64_t>(position + 1 + latencySrc) * deltaSrc.numerator() * deltaOut.denominator();
    const auto denominator = static_cast<std::int64_t>(deltaSrc.denominator()) * deltaOut.numerator();
    const auto required    = ((numerator + denominator - 1) / denominator) - 1 - index;
    result                 = std::max(result, required);
  }
  return static_cast<int>(result);
}
