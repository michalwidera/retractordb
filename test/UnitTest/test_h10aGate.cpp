// Bramka H10a dla klas `@` (AGSE) i `>` (SHIFT) — przeniesiona do ctest z kampanii K24
// (rdb-experiment/results_20260804_K24r). Werdykt z 2026-08-04 mówił „dokładna" dla obu klas
// (AGSE 4309/4309, SHIFT 5484/5484); obie liczby dotyczyły semantyki sprzed przestemplowania
// (okno stemplowane POCZĄTKIEM przedziału, `>N` z opóźnieniem ukrytym w ogonie), więc nie da
// się ich zachować dosłownie.
// Zachowane jest to, co ma znaczenie: REŻIM (postać zamknięta równa granicy zdarzeniowej,
// nigdy zaniżona) oraz METODA (porównanie silnika z niezależnym modelem zdarzeniowym).
// Różnica wobec kampanii: bramka chodzi teraz przy każdym commicie, na mniejszym korpusie.
//
// Model zdarzeniowy poniżej jest NIEZALEŻNY od silnika. Nie wywołuje ani
// AgseStartupLatency(), ani AgseLogicalOrigin(), ani computeRequiredCapacities();
// wyprowadza wszystko z jednej reguły:
//
//   rekord n strumienia S jest emitowany w chwili (n + 1 + W_S) * Delta_S;
//   zależy od rekordów źródła o własnych chwilach dostępności;
//   W_S to najmniejsze W >= 0, przy którym emisja KAŻDEGO rekordu wypada
//   nie wcześniej niż dostępność wszystkich jego zależności.
//
// Konwencja dostępności C1 (nieostra, jak w PREDECLARATION.md kampanii): rekord jest
// dostępny w chwili swojej emisji, bo dataModel::processRows publikuje producentów
// przed konsumentami w takcie.
//
// ctest -R '^ut_h10aGate' -V

#include <array>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/qTree.hpp"

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

namespace {

using ratio = boost::rational<int>;

// --- model zdarzeniowy --------------------------------------------------------------

// Dzielenie w dół — świadomie własne, żeby model nie dzielił z silnikiem ani jednej
// linii kodu. Gdyby korzystał z floorDiv() z SOperations.hpp, wspólny błąd znaku
// przeszedłby przez bramkę niezauważony.
int floorOf(int numerator, int denominator) {
  const int quotient = numerator / denominator;
  return (numerator % denominator != 0 && ((numerator < 0) != (denominator < 0))) ? quotient - 1 : quotient;
}

int floorOf(const ratio &value) { return floorOf(value.numerator(), value.denominator()); }

int ceilOf(const ratio &value) { return -floorOf(-value.numerator(), value.denominator()); }

// Deklaracja wyprzedza konsumenta o rekord uzbrojony przy otwarciu storage i o zerowy
// prefetch, więc jej czoło jest dalej, niż wynika z samego rachunku czasowego.
constexpr int kDeclarationPrefetch = 2;

struct streamModel {
  ratio delta;
  int width;
  int tail;
  int origin;
};

// Okno stemplowane KOŃCEM przedziału: rekord n obejmuje spłaszczone pozycje źródła
// n*step-(|len|-1) ... n*step. Pozycja p leży w rekordzie źródła floor(p/F).
int oldestSourceRecord(int n, int step, int lengthAbs, int width) { return floorOf(n * step - lengthAbs + 1, width); }

int newestSourceRecord(int n, int step, int width) { return floorOf(n * step, width); }

// Okres fazowy w slotach wyjścia: reszty (n*step) mod F powtarzają się co F/gcd(F,step)
// slotów, więc na takim oknie widać każdy przypadek. Mnożnik i dolne ograniczenie
// jak w oracle'u kampanii; poprawność okna weryfikuje kontrola stabilności niżej.
int probeWindow(int step, int width) {
  const int period = width / std::gcd(width, step);
  return std::max(64, 4 * period);
}

struct agseOracle {
  int origin;
  int tail;
  int sourceCapacity;  // dystans wsteczny + 1, BEZ prefetchu deklaracji
  ratio delta;
};

// Ogon i origin liczone przeglądem slotów, nie postacią zamkniętą — o to chodzi
// w niezależności bramki.
agseOracle evaluate(const streamModel &source, int step, int length, int probeMultiplier = 1) {
  const int lengthAbs = length < 0 ? -length : length;
  agseOracle result{};
  result.delta = source.delta * ratio(step, source.width);

  // Origin: najmniejsze n, dla którego NAJSTARSZE pole okna trafia w istniejący rekord.
  // Odwzorowanie jest niemalejące, więc półprosta zaczyna się w pierwszym trafieniu.
  result.origin = 0;
  while (oldestSourceRecord(result.origin, step, lengthAbs, source.width) < source.origin)
    ++result.origin;

  const int window = probeWindow(step, source.width) * probeMultiplier;

  // Ogon: maksimum deficytu po slotach. Deficyt slotu n to
  // (chwila dostępności najpóźniejszej zależności)/Delta_out - (n+1).
  result.tail = 0;
  for (int n = result.origin; n < result.origin + window; ++n) {
    const int newest      = newestSourceRecord(n, step, source.width);
    const ratio available = ratio(newest + 1 + source.tail) * source.delta;
    const int deficit     = ceilOf(available / result.delta - ratio(n + 1));
    result.tail           = std::max(result.tail, deficit);
  }

  // Pojemność: w chwili emisji rekordu n źródło ma wydane rekordy do indeksu, którego
  // dostępność jeszcze mieści się w slocie n; najstarszy potrzebny to dolny koniec okna.
  // Bufor musi pomieścić oba końce zakresu, stąd +1.
  int distance = 0;
  for (int n = result.origin; n < result.origin + window; ++n) {
    const ratio emitted    = ratio(n + 1 + result.tail) * result.delta;
    const int newestOnHand = floorOf(emitted / source.delta) - 1 - source.tail;
    const int oldestWanted = oldestSourceRecord(n, step, lengthAbs, source.width);
    distance               = std::max(distance, newestOnHand - oldestWanted);
  }
  result.sourceCapacity = distance + 1;

  return result;
}

// Przesunięcie tau_N: rekord n niesie rekord n-N producenta, na tym samym interwale.
//
// Historia konwencji tej klasy jest trzystopniowa i warto ją tu mieć, bo dwa razy zmieniła
// się liczba, którą bramka sprawdza:
//   K24 (do 2026-08-06) — odwzorowanie tożsamościowe z opóźnieniem N*Delta doklejonym do
//     dostępności: W = W_src + N, opóźnienie NIEWIDOCZNE w złączeniu (rekord n brał rekord n);
//   K24p (2026-08-07)  — rekord n bierze rekord n-N, ale fetchBack adresował offsetem
//     WZGLĘDNYM, co wymuszało W = W_src; kampania zmierzyła to jako zawyżenie o min(W_src, N)
//     na 6,6% węzłów klasy;
//   dziś              — fetchForward adresuje indeksem LOGICZNYM, więc ogon jest wolny
//     i równy granicy zdarzeniowej.
struct shiftOracle {
  int origin;
  int tail;
  int sourceCapacity;
};

// Rekord n przesunięcia niesie treść rekordu n-N producenta, więc:
//   origin — rekordy poniżej O_src+N nie mają definicji;
//   ogon   — deficyt slotu n wynosi (n-N+1+W_src) - (n+1) = W_src - N i jest STAŁY,
//            stąd W = max(0, W_src - N);
//   pojemność — odległość wsteczna rev = W - W_src + N = N - min(N, W_src), plus jeden
//            na oba końce zakresu, plus wyprzedzenie czoła, jeżeli producent jest deklaracją.
shiftOracle evaluateShift(const streamModel &source, int offset, bool sourceDeclared) {
  const int tail = std::max(0, source.tail - offset);
  const int rev  = tail - source.tail + offset;
  return {
      .origin = source.origin + offset, .tail = tail, .sourceCapacity = rev + 1 + (sourceDeclared ? kDeclarationPrefetch : 0)};
}

// Przeplot `#`: rekord n niesie rekord jednej ze składowych, wybranej regułą Beatty'ego.
// Mapowanie wyprowadzone tu z DEFINICJI przeplotu, celowo bez wołania Hash() z SOperations.hpp
// — wspólny błąd w regule wyboru przeszedłby przez bramkę niezauważony.
struct hashPick {
  bool fromB;
  int position;
};

hashPick pickConstituent(const ratio &deltaA, const ratio &deltaB, int n) {
  const ratio zet = deltaB / (deltaA + deltaB);
  if (floorOf(zet * n) == floorOf(zet * (n + 1))) return {.fromB = true, .position = n - floorOf(zet * n)};
  return {.fromB = false, .position = floorOf(zet * n)};
}

struct hashOracle {
  int tail;
  ratio delta;
};

// Ogon przeglądem slotów: deficyt slotu n to (chwila dostępności wybranej składowej)/Delta_c
// minus (n+1). Okno sondowania jest WIELOKROTNOŚCIĄ okresu fazowego p+q, żeby test mówił
// również o tym, że jeden okres wystarcza — na tym stoi rachunek w HashStartupLatency().
hashOracle evaluateHash(const streamModel &left, const streamModel &right, int periodMultiplier = 4) {
  hashOracle result{};
  result.delta       = left.delta * right.delta / (left.delta + right.delta);
  const auto ratioAB = left.delta / right.delta;
  const int period   = ratioAB.numerator() + ratioAB.denominator();

  for (int n = 0; n < period * periodMultiplier; ++n) {
    const auto pick        = pickConstituent(left.delta, right.delta, n);
    const streamModel &src = pick.fromB ? right : left;
    const ratio available  = ratio(pick.position + 1 + src.tail) * src.delta;
    result.tail            = std::max(result.tail, ceilOf(available / result.delta - ratio(n + 1)));
  }
  return result;
}

// Początek logiczny przeplotu. Obie pozycje — floor(z*n) dla pierwszej składowej
// i n-floor(z*n) dla drugiej — są niemalejące, więc rekordy bez definicji tworzą
// prefiks. Szukamy go przeglądem, a nie maksimum dwóch progów, żeby model nie
// powtarzał rachunku silnika.
int evaluateHashOrigin(const streamModel &left, const streamModel &right, int probeMultiplier = 1) {
  // Origin to slot za OSTATNIM, w którym czytana składowa nie ma jeszcze rekordu.
  // Szukamy go przeglądem, a nie maksimum dwóch progów, żeby model nie powtarzał
  // rachunku silnika. Pozycja składowej o ilorazie p/q rośnie w tempie q/(p+q)
  // albo p/(p+q), więc próg origin wypada najpóźniej po (O_A+O_B)*(p+q) slotach;
  // zapas 64 slotów pokrywa fazę początkową.
  const auto ratioAB = left.delta / right.delta;
  const int span     = ((ratioAB.numerator() + ratioAB.denominator()) * (left.origin + right.origin + 2) + 64) * probeMultiplier;
  int origin         = 0;
  for (int n = 0; n < span; ++n) {
    const auto pick        = pickConstituent(left.delta, right.delta, n);
    const streamModel &src = pick.fromB ? right : left;
    if (pick.position < src.origin) origin = n + 1;
  }
  return origin;
}

// --- suma strumieni `+` -------------------------------------------------------------
//
// Rekord n sumy o interwale Delta_c = min(Delta_1, Delta_2) niesie krotkę złożoną
// z rekordów obu składowych: składowa o interwale Delta_src wnosi rekord
// floor(n*Delta_c/Delta_src) — szybsza dostaje n, wolniejsza swój bieżący.
// Odwzorowanie wyprowadzone z DEFINICJI sumy, celowo bez wołania Add()
// ani AddStartupLatency() z SOperations.hpp.
struct sumOracle {
  int origin;
  int tail;
  ratio delta;
};

// Reszta n*Delta_c/Delta_src wraca po mianowniku zredukowanego ilorazu, więc
// wielokrotność wspólnej wielokrotności obu mianowników pokazuje każdą fazę.
int sumProbeWindow(const ratio &deltaOut, const streamModel &left, const streamModel &right) {
  return std::max(64, 4 * std::lcm((deltaOut / left.delta).denominator(), (deltaOut / right.delta).denominator()));
}

sumOracle evaluateSum(const streamModel &left, const streamModel &right, int probeMultiplier = 1) {
  sumOracle result{};
  result.delta = std::min(left.delta, right.delta);

  auto indexIn = [&result](const streamModel &src, int n) { return floorOf(ratio(n) * result.delta / src.delta); };

  // Origin: najmniejsze n, w którym OBIE składowe trafiają w istniejący rekord.
  result.origin = 0;
  while (indexIn(left, result.origin) < left.origin || indexIn(right, result.origin) < right.origin)
    ++result.origin;

  const int window = sumProbeWindow(result.delta, left, right) * probeMultiplier;
  for (int n = result.origin; n < result.origin + window; ++n)
    for (const streamModel *src : {&left, &right}) {
      const ratio available = ratio(indexIn(*src, n) + 1 + src->tail) * src->delta;
      result.tail           = std::max(result.tail, ceilOf(available / result.delta - ratio(n + 1)));
    }
  return result;
}

// --- różnica `-` --------------------------------------------------------------------
//
// C-Delta wybiera z producenta rekord ceil(n*Delta/Delta_src); dla równych interwałów
// wybór jest tożsamością. Model daje ogon w konwencji C1, czyli DOLNE ograniczenie,
// i bramka wymaga tu bezpieczeństwa (EXPECT_GE), a nie równości — trybem porażki jest
// zaniżenie, bo ono oznacza rekord wyemitowany przed określeniem zależności.
//
// Do 2026-08-18 klasa `-` była w tab:tail-exactness zawyżająca: silnik dokładał
// deklaracji własny slot, uzasadniając to publikacją źródła po konsumentach w takcie.
// Ta gałąź zniknęła (K24/H10 faza 3) — dokładała slot ZAWSZE, stawiając `-` w konwencji
// dostępności innej niż siedem pozostałych klas silnika. Bramka zostaje kierunkowa:
// dokładność jest dziś osiągana, ale chronione jest to, żeby nigdy nie spaść poniżej.
struct subtractOracle {
  int origin;
  int tail;
};

subtractOracle evaluateSubtract(const streamModel &source, const ratio &target, int probeMultiplier = 1) {
  auto indexIn = [&](int n) { return source.delta == target ? n : ceilOf(ratio(n) * target / source.delta); };

  subtractOracle result{};
  result.origin = 0;
  while (indexIn(result.origin) < source.origin)
    ++result.origin;

  const int window = std::max(64, 4 * (target / source.delta).denominator()) * probeMultiplier;
  for (int n = result.origin; n < result.origin + window; ++n) {
    const ratio available = ratio(indexIn(n) + 1 + source.tail) * source.delta;
    result.tail           = std::max(result.tail, ceilOf(available / target - ratio(n + 1)));
  }
  return result;
}

// --- rozplot `&` (Theta) i `%` (~Theta) ---------------------------------------------
//
// Rekord i lewej składowej leży w przeplocie na pozycji i + ceil((i+1)*Da/Db), prawej
// — na pozycji i + floor(i*Db/Da). Oba odwzorowania wyprowadzone z DEFINICJI rozplotu,
// celowo bez wołania Div()/Mod() z SOperations.hpp: wspólny błąd odwzorowania
// przeszedłby przez bramkę niezauważony.
//
// Theta bywa o slot NIEPRZYCZYNOWA — jej pozycja w przeplocie potrafi wypaść po własnym
// slocie — i stąd bierze się jej ogon. Slot ten NIE jest jednak stały: przy ilorazie
// całkowitym kres fazy odczytu daje ogon zerowy, co silnik od 2026-08-18 liczy dokładnie
// (K24/H10). Bramka wykazuje, że tam, gdzie slot jest prawdziwy, nie da się go usunąć.
struct dehashOracle {
  int origin;
  int tail;
};

int thetaPosition(const ratio &deltaA, const ratio &deltaB, int i) { return i + ceilOf(ratio(i + 1) * deltaA / deltaB); }

int notThetaPosition(const ratio &deltaA, const ratio &deltaB, int i) { return i + floorOf(ratio(i) * deltaB / deltaA); }

dehashOracle evaluateDehash(const streamModel &source, const ratio &deltaA, const ratio &deltaB, bool leftConstituent,
                            int probeMultiplier = 1) {
  const ratio target = leftConstituent ? deltaA : deltaB;
  auto position      = [&](int i) {
    return leftConstituent ? thetaPosition(deltaA, deltaB, i) : notThetaPosition(deltaA, deltaB, i);
  };

  dehashOracle result{};
  result.origin = 0;
  while (position(result.origin) < source.origin)
    ++result.origin;

  const auto ratioAB = deltaA / deltaB;
  const int window   = std::max(64, 4 * (ratioAB.numerator() + ratioAB.denominator())) * probeMultiplier;
  for (int i = result.origin; i < result.origin + window; ++i) {
    const ratio available = ratio(position(i) + 1 + source.tail) * source.delta;
    result.tail           = std::max(result.tail, ceilOf(available / target - ratio(i + 1)));
  }
  return result;
}

// --- korpus -------------------------------------------------------------------------

std::string declareSource(const std::string &id, int width, const ratio &delta) {
  std::string text = "DECLARE ";
  for (int i = 0; i < width; ++i)
    text += (i ? ", f" : "f") + std::to_string(i) + " INTEGER";
  text += " STREAM " + id + ", " + std::to_string(delta.numerator()) + "/" + std::to_string(delta.denominator());
  text += " FILE 'a.txt'\n";
  return text;
}

std::string windowOf(const std::string &id, const std::string &src, int step, int length) {
  return "SELECT * STREAM " + id + " FROM " + src + "@(" + std::to_string(step) + "," + std::to_string(length) + ")\n";
}

std::string shiftOf(const std::string &id, const std::string &src, int offset) {
  return "SELECT * STREAM " + id + " FROM " + src + ">" + std::to_string(offset) + "\n";
}

std::string hashOf(const std::string &id, const std::string &left, const std::string &right) {
  return "SELECT * STREAM " + id + " FROM " + left + "#" + right + "\n";
}

std::string ratioText(const ratio &value) {
  return std::to_string(value.numerator()) + "/" + std::to_string(value.denominator());
}

std::string sumOf(const std::string &id, const std::string &left, const std::string &right) {
  return "SELECT * STREAM " + id + " FROM " + left + "+" + right + "\n";
}

std::string subtractOf(const std::string &id, const std::string &src, const ratio &target) {
  return "SELECT * STREAM " + id + " FROM " + src + "-" + ratioText(target) + "\n";
}

// Argumentem rozplotu jest interwał składowej USUWANEJ, nie odzyskiwanej.
std::string thetaOf(const std::string &id, const std::string &src, const ratio &removed) {
  return "SELECT * STREAM " + id + " FROM " + src + "&" + ratioText(removed) + "\n";
}

std::string notThetaOf(const std::string &id, const std::string &src, const ratio &removed) {
  return "SELECT * STREAM " + id + " FROM " + src + "%" + ratioText(removed) + "\n";
}

std::string projectionOf(const std::string &id, const std::string &src) {
  return "SELECT " + src + "[0] STREAM " + id + " FROM " + src + "\n";
}

std::string reductionOf(const std::string &id, const std::string &src) {
  return "SELECT * STREAM " + id + " FROM " + src + ".sumc\n";
}

const std::vector<int> kOffsets{1, 2, 3, 5, 8};

const std::vector<ratio> kDeltas{ratio(1, 1), ratio(1, 2), ratio(1, 3), ratio(2, 5), ratio(3, 10), ratio(1, 100)};
const std::vector<int> kWidths{1, 2, 3, 4};
const std::vector<int> kSteps{1, 2, 3, 4};
const std::vector<int> kLengths{1, 2, 3, 4, -2, -3, -4};

}  // namespace

// Korpus jednowęzłowy: okno nad deklaracją, pełna siatka (F, step, len, Delta).
// Deklaracja ma ogon 0 i origin 0, więc każdy przypadek testuje wyłącznie regułę węzła `@`.
TEST(h10aGate, closed_form_matches_event_model_over_single_window_corpus) {
  int checked = 0;
  for (const auto &delta : kDeltas)
    for (int width : kWidths)
      for (int step : kSteps)
        for (int length : kLengths) {
          qTree instance;
          const std::string rql             = declareSource("src", width, delta) + windowOf("win", "src", step, length);
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          // Plan odrzucony przez kompilator jest awarią aparatury, nie wynikiem —
          // ta sama zasada co w kampanii K24.
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
          const agseOracle expected = evaluate(source, step, length);

          const auto &win = instance.getQuery("win");
          EXPECT_EQ(win.rInterval, expected.delta) << rql;
          EXPECT_EQ(win.logicalOrigin, expected.origin) << rql;
          EXPECT_EQ(win.startupLatency, expected.tail) << rql;
          // Deklaracja ma dwa rekordy przed pierwszym wykonaniem konsumenta (uzbrojenie
          // storage i zerowy prefetch) — model zdarzeniowy tego nie widzi, bo to szczegół
          // realizacji źródła, nie semantyki operatora. Niezależnie wyprowadzony jest
          // dystans wsteczny i to on jest treścią bramki.
          EXPECT_EQ(instance.maxCapacity.at("src"), expected.sourceCapacity + kDeclarationPrefetch - 1) << rql;
          ++checked;
        }
  EXPECT_EQ(checked, static_cast<int>(kDeltas.size() * kWidths.size() * kSteps.size() * kLengths.size()));
}

// Korpus dwuwęzłowy: okno nad oknem. Dopiero tu producent ma NIEZEROWY ogon i NIEZEROWY
// origin, więc sprawdzana jest propagacja obu wielkości — a nie tylko przypadek brzegowy
// nad deklaracją.
TEST(h10aGate, closed_form_matches_event_model_over_stacked_windows) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(3, 10)};
  const std::vector<std::pair<int, int>> inner{{1, 3}, {2, 3}, {3, 2}};
  const std::vector<std::pair<int, int>> outer{{1, 2}, {2, 3}, {1, 4}};

  int checked = 0;
  for (const auto &delta : deltas)
    for (int width : {1, 2, 3})
      for (const auto &[step1, len1] : inner)
        for (const auto &[step2, len2] : outer) {
          qTree instance;
          const std::string rql = declareSource("src", width, delta) +  //
                                  windowOf("w1", "src", step1, len1) +  //
                                  windowOf("w2", "w1", step2, len2);
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
          const agseOracle first = evaluate(source, step1, len1);
          // Szerokość okna to liczba jego pól — tyle wnosi do spłaszczenia u konsumenta.
          const streamModel middle{
              .delta = first.delta, .width = len1 < 0 ? -len1 : len1, .tail = first.tail, .origin = first.origin};
          const agseOracle second = evaluate(middle, step2, len2);

          EXPECT_EQ(instance.getQuery("w1").logicalOrigin, first.origin) << rql;
          EXPECT_EQ(instance.getQuery("w1").startupLatency, first.tail) << rql;
          EXPECT_EQ(instance.getQuery("w2").rInterval, second.delta) << rql;
          EXPECT_EQ(instance.getQuery("w2").logicalOrigin, second.origin) << rql;
          EXPECT_EQ(instance.getQuery("w2").startupLatency, second.tail) << rql;
          // w1 jest producentem obliczanym, więc bez członu prefetchu deklaracji.
          EXPECT_EQ(instance.maxCapacity.at("w1"), second.sourceCapacity) << rql;
          ++checked;
        }
  EXPECT_EQ(checked, static_cast<int>(deltas.size() * 3 * inner.size() * outer.size()));
}

// Kontrola aparatury: okno sondowania musi być dobrane tak, żeby wynik już się nie zmieniał.
// Bez tej kontroli zbyt wąskie okno dawałoby zaniżony ogon i bramka przepuszczałaby błąd.
// (Odpowiednik testu stabilności z model.py kampanii.)
TEST(h10aGate, probe_window_is_wide_enough) {
  for (const auto &delta : kDeltas)
    for (int width : kWidths)
      for (int step : kSteps)
        for (int length : kLengths) {
          const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
          const agseOracle narrow = evaluate(source, step, length, 1);
          const agseOracle wide   = evaluate(source, step, length, 2);
          EXPECT_EQ(narrow.tail, wide.tail) << "F=" << width << " step=" << step << " len=" << length;
          EXPECT_EQ(narrow.sourceCapacity, wide.sourceCapacity) << "F=" << width << " step=" << step << " len=" << length;
        }
}

// Kontrola negatywna: model zdarzeniowy MUSI odrzucać ogon o jeden za mały. Bez niej
// bramka przechodziłaby również dla postaci zamkniętej, która systematycznie zaniża —
// a to jest dokładnie ta klasa defektu (rekord wydany, zanim jego zależności są określone),
// którą kampania K24 nazwała reżimem zaniżającym.
TEST(h10aGate, event_model_rejects_a_tail_one_slot_too_small) {
  int witnesses = 0;
  for (const auto &delta : kDeltas)
    for (int width : kWidths)
      for (int step : kSteps) {
        const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
        const agseOracle truth = evaluate(source, step, 3);
        if (truth.tail == 0) continue;  // nie ma czego zaniżać

        // Zaniżony ogon oznacza slot, w którym emisja wypada PRZED dostępnością
        // najnowszego pola okna. Szukamy takiego slotu wprost.
        const ratio deltaOut = source.delta * ratio(step, width);
        bool violated        = false;
        for (int n = truth.origin; n < truth.origin + probeWindow(step, width) && !violated; ++n) {
          const int newest      = newestSourceRecord(n, step, width);
          const ratio available = ratio(newest + 1 + source.tail) * source.delta;
          if (available > ratio(n + 1 + truth.tail - 1) * deltaOut) violated = true;
        }
        EXPECT_TRUE(violated) << "F=" << width << " step=" << step << " delta=" << delta;
        ++witnesses;
      }
  EXPECT_GT(witnesses, 0);
}

// Klasa SHIFT nad deklaracją: pełna siatka (F, N, Delta). Ogon musi być ZEROWY (producent
// o ogonie zerowym nie każe czekać na rekord STARSZY od bieżącego), a całe opóźnienie ma
// siedzieć w origin — przed przestemplowaniem było odwrotnie i dlatego `>N` nie było
// widoczne w złączeniu.
TEST(h10aGate, closed_form_matches_event_model_over_shift_corpus) {
  int checked = 0;
  for (const auto &delta : kDeltas)
    for (int width : kWidths)
      for (int offset : kOffsets) {
        qTree instance;
        const std::string rql             = declareSource("src", width, delta) + shiftOf("sh", "src", offset);
        auto [parseResult, keyword, name] = parserRQLString(instance, rql);
        ASSERT_EQ(parseResult, "OK") << rql;

        compiler compilerInstance(instance);
        ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

        const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
        const shiftOracle expected = evaluateShift(source, offset, true);

        const auto &sh = instance.getQuery("sh");
        EXPECT_EQ(sh.rInterval, delta) << rql;  // przesunięcie nie zmienia interwału
        EXPECT_EQ(sh.logicalOrigin, expected.origin) << rql;
        EXPECT_EQ(sh.startupLatency, expected.tail) << rql;
        EXPECT_EQ(instance.maxCapacity.at("src"), expected.sourceCapacity) << rql;
        ++checked;
      }
  EXPECT_EQ(checked, static_cast<int>(kDeltas.size() * kWidths.size() * kOffsets.size()));
}

// Przesunięcie NAD oknem: jedyne miejsce, w którym sprawdzane jest, że origin okna przechodzi
// przez `>N` i sumuje się z jego własnym przesunięciem, a ogon okna przechodzi nietknięty.
TEST(h10aGate, shift_over_window_composes_both_quantities) {
  int checked = 0;
  for (const auto &delta : {ratio(1, 1), ratio(1, 2), ratio(3, 10)})
    for (int width : {1, 2, 3})
      for (int step : kSteps)
        for (int length : {2, 3, -4})
          for (int offset : {1, 3, 8}) {
            qTree instance;
            const std::string rql = declareSource("src", width, delta) +    //
                                    windowOf("win", "src", step, length) +  //
                                    shiftOf("sh", "win", offset);
            auto [parseResult, keyword, name] = parserRQLString(instance, rql);
            ASSERT_EQ(parseResult, "OK") << rql;

            compiler compilerInstance(instance);
            ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

            const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
            const agseOracle window = evaluate(source, step, length);
            const streamModel middle{
                .delta = window.delta, .width = length < 0 ? -length : length, .tail = window.tail, .origin = window.origin};
            const shiftOracle expected = evaluateShift(middle, offset, false);

            EXPECT_EQ(instance.getQuery("sh").logicalOrigin, expected.origin) << rql;
            EXPECT_EQ(instance.getQuery("sh").startupLatency, expected.tail) << rql;
            EXPECT_EQ(instance.maxCapacity.at("win"), expected.sourceCapacity) << rql;
            ++checked;
          }
  EXPECT_EQ(checked, 3 * 3 * static_cast<int>(kSteps.size()) * 3 * 3);
}

// Okno NAD przesunięciem: producent o ogonie ZEROWYM i origin NIEZEROWYM. Korpus okien
// piętrowych tego nie obejmuje — tam origin i ogon producenta rosną razem — więc dopiero
// ten przypadek rozdziela wpływ obu wielkości na regułę węzła `@`.
TEST(h10aGate, window_over_shift_separates_origin_from_tail) {
  int checked = 0;
  for (const auto &delta : {ratio(1, 1), ratio(1, 2), ratio(3, 10)})
    for (int width : {1, 2, 3})
      for (int offset : {1, 3, 8})
        for (int step : kSteps)
          for (int length : {2, 3, -4}) {
            qTree instance;
            const std::string rql = declareSource("src", width, delta) +  //
                                    shiftOf("sh", "src", offset) +        //
                                    windowOf("win", "sh", step, length);
            auto [parseResult, keyword, name] = parserRQLString(instance, rql);
            ASSERT_EQ(parseResult, "OK") << rql;

            compiler compilerInstance(instance);
            ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

            const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
            const shiftOracle shifted = evaluateShift(source, offset, true);
            const streamModel middle{.delta = delta, .width = width, .tail = shifted.tail, .origin = shifted.origin};
            const agseOracle expected = evaluate(middle, step, length);

            EXPECT_EQ(instance.getQuery("win").logicalOrigin, expected.origin) << rql;
            EXPECT_EQ(instance.getQuery("win").startupLatency, expected.tail) << rql;
            EXPECT_EQ(instance.maxCapacity.at("sh"), expected.sourceCapacity) << rql;
            ++checked;
          }
  EXPECT_EQ(checked, 3 * 3 * 3 * static_cast<int>(kSteps.size()) * 3);
}

// Klasa HASH nad dwiema deklaracjami: siatka par delt. Obie składowe mają ogon zerowy,
// więc każdy przypadek testuje wyłącznie regułę węzła `#` — a ta od 2026-08-07 jest
// przeglądem okresu fazowego, nie postacią O(1) z członem ceil((p+q-1)/p).
TEST(h10aGate, closed_form_matches_event_model_over_hash_corpus) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(1, 3), ratio(2, 5), ratio(3, 10), ratio(5, 7)};
  int checked = 0;
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas) {
      qTree instance;
      const std::string rql = declareSource("a", 1, deltaA) +  //
                              declareSource("b", 1, deltaB) +  //
                              hashOf("c", "a", "b");
      auto [parseResult, keyword, name] = parserRQLString(instance, rql);
      ASSERT_EQ(parseResult, "OK") << rql;

      compiler compilerInstance(instance);
      ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

      const streamModel left{.delta = deltaA, .width = 1, .tail = 0, .origin = 0};
      const streamModel right{.delta = deltaB, .width = 1, .tail = 0, .origin = 0};
      const hashOracle expected = evaluateHash(left, right);

      const auto &node = instance.getQuery("c");
      EXPECT_EQ(node.rInterval, expected.delta) << rql;
      EXPECT_EQ(node.startupLatency, expected.tail) << rql;
      EXPECT_EQ(node.logicalOrigin, evaluateHashOrigin(left, right)) << rql;
      ++checked;
    }
  EXPECT_EQ(checked, static_cast<int>(deltas.size() * deltas.size()));
}

// Przeplot nad DWOMA oknami: obie składowe mają niezerowy ogon i niezerowy origin.
// To jest korpus, który faktycznie strzeże reguły. Siatka z ogonami zerowymi tego nie
// robi: dla zerowych ogonów składowych zastąpiona postać O(1) i przegląd okresu dają
// tę samą liczbę, więc bramka na deklaracjach przeszłaby również przed naprawą.
// Sprawdzone offline na tej siatce: 27 z 216 przypadków rozróżnia obie postacie,
// zawsze o slot w górę po stronie postaci O(1).
TEST(h10aGate, hash_over_two_windows_uses_the_constituent_selected_per_record) {
  const std::vector<ratio> leftDeltas{ratio(1, 1), ratio(1, 2)};
  const std::vector<ratio> rightDeltas{ratio(1, 1), ratio(1, 2), ratio(3, 10)};
  int checked = 0;
  for (const auto &deltaA : leftDeltas)
    for (int widthA : {2, 3})
      for (int stepA : {1, 2})
        for (const auto &deltaB : rightDeltas)
          for (int widthB : {2, 3, 4})
            for (int stepB : {1, 2, 3}) {
              qTree instance;
              // Przeplot wymaga zgodnych schematów, więc oba okna mają długość 2.
              const std::string rql = declareSource("a", widthA, deltaA) +  //
                                      declareSource("b", widthB, deltaB) +  //
                                      windowOf("wa", "a", stepA, 2) +       //
                                      windowOf("wb", "b", stepB, 2) +       //
                                      hashOf("c", "wa", "wb");
              auto [parseResult, keyword, name] = parserRQLString(instance, rql);
              ASSERT_EQ(parseResult, "OK") << rql;

              compiler compilerInstance(instance);
              ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

              const streamModel sourceA{.delta = deltaA, .width = widthA, .tail = 0, .origin = 0};
              const streamModel sourceB{.delta = deltaB, .width = widthB, .tail = 0, .origin = 0};
              const agseOracle windowA = evaluate(sourceA, stepA, 2);
              const agseOracle windowB = evaluate(sourceB, stepB, 2);
              const streamModel left{.delta = windowA.delta, .width = 2, .tail = windowA.tail, .origin = windowA.origin};
              const streamModel right{.delta = windowB.delta, .width = 2, .tail = windowB.tail, .origin = windowB.origin};
              const hashOracle expected = evaluateHash(left, right);

              EXPECT_EQ(instance.getQuery("c").rInterval, expected.delta) << rql;
              EXPECT_EQ(instance.getQuery("c").startupLatency, expected.tail) << rql;
              // Origin przeplotu nad składowymi o NIEZEROWYM origin — korpus nad deklaracjami
              // sprawdza tylko przypadek zerowy, więc dopiero tu widać, że obie pozycje
              // przeplotu przenoszą niedefiniowalność.
              EXPECT_EQ(instance.getQuery("c").logicalOrigin, evaluateHashOrigin(left, right)) << rql;
              ++checked;
            }
  EXPECT_EQ(checked, 2 * 2 * 2 * 3 * 3 * 3);
}

// Kontrola aparatury: jeden okres fazowy musi wystarczyć. Gdyby maksimum deficytu wypadało
// dalej niż p+q, rachunek w HashStartupLatency() zaniżałby ogon — a zaniżenie jest tą klasą
// defektu, której reżim bezpieczny miał nie dopuszczać.
TEST(h10aGate, one_phase_period_is_enough_for_the_hash_tail) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(1, 3), ratio(2, 5), ratio(3, 10), ratio(5, 7)};
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas)
      for (int tailA : {0, 1, 4}) {
        const streamModel left{.delta = deltaA, .width = 1, .tail = tailA, .origin = 0};
        const streamModel right{.delta = deltaB, .width = 1, .tail = 0, .origin = 0};
        EXPECT_EQ(evaluateHash(left, right, 1).tail, evaluateHash(left, right, 8).tail)
            << "dA=" << deltaA << " dB=" << deltaB << " W_A=" << tailA;
      }
}

// =====================================================================================
// Klasy dopisane 2026-08-07 (luka L1): `+`, `-`, `&`, `%`, projekcja i redukcje.
//
// Powód: tab:tail-exactness w artykule podaje reżim dla DZIEWIĘCIU klas, a bramka
// pilnowała trzech. Klasy `Θ` i `~Θ` nie miały żadnej kontroli, mimo że ich własny ogon
// jest w compiler::computeStartupLatency() pojedynczą instrukcją (`++result` i jej brak).
// Usunięcie tej instrukcji dawało reżim ZANIŻAJĄCY — rekord wydany przed określeniem
// zależności — i nie zapalało w ctest ani jednej lampki.
//
// Asercje są ASYMETRYCZNE, zgodnie z twierdzeniem: równość dla klas dokładnych,
// nierówność `silnik >= model` dla klas zawyżających. Origin jest wymagany dokładnie
// we wszystkich klasach — artykuł podaje dla niego 100% w dziewięciu na dziewięć.
// =====================================================================================

namespace {

const std::vector<ratio> kPairDeltas{ratio(1, 1), ratio(1, 2), ratio(3, 10)};
const std::vector<int> kPairWidths{2, 3};
const std::vector<int> kPairSteps{1, 2};

}  // namespace

// Klasa `+` nad DWOMA OKNAMI. Korpus nad deklaracjami byłby bezwartościowy: przy zerowych
// ogonach składowych AddStartupLatency() degeneruje się do ceil(Delta_src/Delta_out)-1
// i nie sprawdza członu (1+W_src). Dopiero producent o niezerowym ogonie odróżnia postać
// obowiązującą od postaci sprzed K24 (ceil(W_src*Delta_src/Delta_out)), która dla zerowego
// ogona składowej dawała zero niezależnie od tego, jak wolna jest ta składowa.
TEST(h10aGate, closed_form_matches_event_model_over_sum_corpus) {
  int checked        = 0;
  int distinguishing = 0;
  for (const auto &deltaA : kPairDeltas)
    for (int widthA : kPairWidths)
      for (int stepA : kPairSteps)
        for (const auto &deltaB : kPairDeltas)
          for (int widthB : kPairWidths)
            for (int stepB : kPairSteps) {
              qTree instance;
              const std::string rql = declareSource("a", widthA, deltaA) +  //
                                      declareSource("b", widthB, deltaB) +  //
                                      windowOf("wa", "a", stepA, 2) +       //
                                      windowOf("wb", "b", stepB, 3) +       //
                                      sumOf("s", "wa", "wb");
              auto [parseResult, keyword, name] = parserRQLString(instance, rql);
              ASSERT_EQ(parseResult, "OK") << rql;

              compiler compilerInstance(instance);
              ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

              const streamModel sourceA{.delta = deltaA, .width = widthA, .tail = 0, .origin = 0};
              const streamModel sourceB{.delta = deltaB, .width = widthB, .tail = 0, .origin = 0};
              const agseOracle windowA = evaluate(sourceA, stepA, 2);
              const agseOracle windowB = evaluate(sourceB, stepB, 3);
              const streamModel left{.delta = windowA.delta, .width = 2, .tail = windowA.tail, .origin = windowA.origin};
              const streamModel right{.delta = windowB.delta, .width = 3, .tail = windowB.tail, .origin = windowB.origin};
              const sumOracle expected = evaluateSum(left, right);

              const auto &node = instance.getQuery("s");
              EXPECT_EQ(node.rInterval, expected.delta) << rql;
              EXPECT_EQ(node.startupLatency, expected.tail) << rql;
              EXPECT_EQ(node.logicalOrigin, expected.origin) << rql;
              if (left.tail > 0 || right.tail > 0) ++distinguishing;
              ++checked;
            }
  EXPECT_EQ(checked, 3 * 2 * 2 * 3 * 2 * 2);
  // Moc detekcyjna: korpus musi zawierać przypadki z niezerowym ogonem składowej,
  // bo tylko one odróżniają postać obowiązującą od zastąpionej.
  EXPECT_GT(distinguishing, 0);
}

// Kontrola aparatury sumy: okno sondowania szerokie na tyle, że wynik już nie rośnie.
TEST(h10aGate, sum_probe_window_is_wide_enough) {
  for (const auto &deltaA : kPairDeltas)
    for (const auto &deltaB : kPairDeltas)
      for (int tailA : {0, 1, 3})
        for (int tailB : {0, 2}) {
          const streamModel left{.delta = deltaA, .width = 1, .tail = tailA, .origin = 1};
          const streamModel right{.delta = deltaB, .width = 1, .tail = tailB, .origin = 2};
          EXPECT_EQ(evaluateSum(left, right, 1).tail, evaluateSum(left, right, 3).tail)
              << "dA=" << deltaA << " dB=" << deltaB << " W_A=" << tailA << " W_B=" << tailB;
        }
}

// Klasa `-` nad oknem: producent ma niezerowy ogon i niezerowy origin, więc sprawdzana
// jest propagacja obu wielkości, a nie tylko przypadek brzegowy nad deklaracją (ten
// pokrywa ut_capacities). Reżim klasy jest ZAWYŻAJĄCY — w kampanii K24d 19,1% zgodności
// — więc asercją ogona jest nierówność. Origin musi być dokładny.
TEST(h10aGate, subtract_never_falls_below_the_event_model) {
  int checked     = 0;
  int tight       = 0;
  int originMoved = 0;
  // Okno pięciopolowe, a nie dwupolowe: przy origin producenta równym 1 odwzorowanie
  // ceil(n*Delta/Delta_src) zwraca dokładnie ten sam próg, więc korpus przechodziłby
  // także dla silnika PRZEPISUJĄCEGO origin producenta bez odwzorowania — sprawdzone
  // mutacyjnie 2026-08-07. Origin 4 (krok 1) i 2 (krok 2) rozdziela te dwie reguły.
  for (const auto &delta : kPairDeltas)
    for (int width : kPairWidths)
      for (int step : kPairSteps)
        for (const auto &factor : {ratio(1, 1), ratio(2, 1), ratio(3, 1), ratio(5, 2)}) {
          const ratio windowDelta = delta * ratio(step, width);
          const ratio target      = windowDelta * factor;

          qTree instance;
          const std::string rql = declareSource("src", width, delta) +  //
                                  windowOf("win", "src", step, 5) +     //
                                  subtractOf("res", "win", target);
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
          const agseOracle window = evaluate(source, step, 5);
          const streamModel producer{.delta = window.delta, .width = 5, .tail = window.tail, .origin = window.origin};
          const subtractOracle expected = evaluateSubtract(producer, target);

          const auto &node = instance.getQuery("res");
          EXPECT_EQ(node.rInterval, target) << rql;
          EXPECT_GE(node.startupLatency, expected.tail) << rql;
          EXPECT_EQ(node.logicalOrigin, expected.origin) << rql;
          if (node.startupLatency == expected.tail) ++tight;
          if (expected.origin != producer.origin) ++originMoved;
          ++checked;
        }
  EXPECT_EQ(checked, 3 * 2 * 2 * 4);
  // Moc detekcyjna, dwa świadki. Ogon: gdyby nierówność nigdzie nie była ciasna,
  // przechodziłaby też dla reguły zawyżającej dowolnie mocno, czyli nie strzegłaby
  // niczego poza znakiem. Origin: musi być w korpusie przypadek, w którym odwzorowanie
  // różnicy PRZESUWA origin producenta — inaczej sprawdzana jest tożsamość, nie reguła.
  EXPECT_GT(tight, 0);
  EXPECT_GT(originMoved, 0);
}

// Kontrola aparatury różnicy.
TEST(h10aGate, subtract_probe_window_is_wide_enough) {
  for (const auto &delta : kPairDeltas)
    for (const auto &factor : {ratio(1, 1), ratio(2, 1), ratio(5, 2), ratio(7, 3)})
      for (int tail : {0, 1, 4}) {
        const streamModel source{.delta = delta, .width = 1, .tail = tail, .origin = 2};
        const ratio target = delta * factor;
        EXPECT_EQ(evaluateSubtract(source, target, 1).tail, evaluateSubtract(source, target, 3).tail)
            << "d=" << delta << " target=" << target << " W=" << tail;
      }
}

// Klasy `&` i `%` nad przeplotem DWÓCH DEKLARACJI. Producent ma zerowy origin, więc każdy
// przypadek testuje wyłącznie własny wkład operatora rozplotu — a ten jest w silniku
// stałą: jeden slot dla Theta, zero dla ~Theta.
TEST(h10aGate, dehash_never_falls_below_the_event_model) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(2, 5), ratio(5, 7)};
  int checked    = 0;
  int thetaTight = 0;
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas) {
      qTree instance;
      const std::string rql = declareSource("a", 1, deltaA) +  //
                              declareSource("b", 1, deltaB) +  //
                              hashOf("c", "a", "b") +          //
                              thetaOf("left", "c", deltaB) +   //
                              notThetaOf("right", "c", deltaA);
      auto [parseResult, keyword, name] = parserRQLString(instance, rql);
      ASSERT_EQ(parseResult, "OK") << rql;

      compiler compilerInstance(instance);
      ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

      const streamModel left{.delta = deltaA, .width = 1, .tail = 0, .origin = 0};
      const streamModel right{.delta = deltaB, .width = 1, .tail = 0, .origin = 0};
      const hashOracle interleaved = evaluateHash(left, right);
      const streamModel producer{
          .delta = interleaved.delta, .width = 1, .tail = interleaved.tail, .origin = evaluateHashOrigin(left, right)};

      const dehashOracle theta    = evaluateDehash(producer, deltaA, deltaB, true);
      const dehashOracle notTheta = evaluateDehash(producer, deltaA, deltaB, false);

      const auto &leftNode  = instance.getQuery("left");
      const auto &rightNode = instance.getQuery("right");
      EXPECT_EQ(leftNode.rInterval, deltaA) << rql;
      EXPECT_EQ(rightNode.rInterval, deltaB) << rql;
      EXPECT_GE(leftNode.startupLatency, theta.tail) << rql;
      EXPECT_GE(rightNode.startupLatency, notTheta.tail) << rql;
      EXPECT_EQ(leftNode.logicalOrigin, theta.origin) << rql;
      EXPECT_EQ(rightNode.logicalOrigin, notTheta.origin) << rql;
      if (leftNode.startupLatency == theta.tail) ++thetaTight;
      ++checked;
    }
  EXPECT_EQ(checked, static_cast<int>(deltas.size() * deltas.size()));
  // Moc detekcyjna nie jest tu założeniem: własny slot Theta musi być w korpusie
  // WYMUSZONY przez model, inaczej bramka przeszłaby także po usunięciu `++result`
  // z compiler::computeStartupLatency(). Osobny test niżej mówi to wprost.
  EXPECT_GT(thetaTight, 0);
}

// Klasy `&` i `%` nad przeplotem DWÓCH OKIEN: producent ma niezerowy ogon ORAZ niezerowy
// origin. To jedyne miejsce, w którym sprawdzana jest propagacja niedefiniowalności przez
// odwzorowania rozplotu — a te rosną szybciej niż liniowo, więc origin nie przenosi się
// tu przez proste dodanie.
TEST(h10aGate, dehash_over_two_windows_propagates_origin_and_tail) {
  int checked = 0;
  for (const auto &deltaA : kPairDeltas)
    for (int widthA : kPairWidths)
      for (const auto &deltaB : kPairDeltas)
        for (int widthB : kPairWidths) {
          const ratio deltaWa = deltaA / widthA;  // okno @(1,2) nad szerokoscia F ma interwal D*1/F
          const ratio deltaWb = deltaB / widthB;

          qTree instance;
          const std::string rql = declareSource("a", widthA, deltaA) +  //
                                  declareSource("b", widthB, deltaB) +  //
                                  windowOf("wa", "a", 1, 2) +           //
                                  windowOf("wb", "b", 1, 2) +           //
                                  hashOf("c", "wa", "wb") +             //
                                  thetaOf("left", "c", deltaWb) +       //
                                  notThetaOf("right", "c", deltaWa);
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel sourceA{.delta = deltaA, .width = widthA, .tail = 0, .origin = 0};
          const streamModel sourceB{.delta = deltaB, .width = widthB, .tail = 0, .origin = 0};
          const agseOracle windowA = evaluate(sourceA, 1, 2);
          const agseOracle windowB = evaluate(sourceB, 1, 2);
          const streamModel left{.delta = windowA.delta, .width = 2, .tail = windowA.tail, .origin = windowA.origin};
          const streamModel right{.delta = windowB.delta, .width = 2, .tail = windowB.tail, .origin = windowB.origin};
          const hashOracle interleaved = evaluateHash(left, right);
          const streamModel producer{
              .delta = interleaved.delta, .width = 2, .tail = interleaved.tail, .origin = evaluateHashOrigin(left, right)};

          const dehashOracle theta    = evaluateDehash(producer, left.delta, right.delta, true);
          const dehashOracle notTheta = evaluateDehash(producer, left.delta, right.delta, false);

          EXPECT_GE(instance.getQuery("left").startupLatency, theta.tail) << rql;
          EXPECT_GE(instance.getQuery("right").startupLatency, notTheta.tail) << rql;
          EXPECT_EQ(instance.getQuery("left").logicalOrigin, theta.origin) << rql;
          EXPECT_EQ(instance.getQuery("right").logicalOrigin, notTheta.origin) << rql;
          ++checked;
        }
  EXPECT_EQ(checked, 3 * 2 * 3 * 2);
}

// Kontrola negatywna klasy Theta — dowód mocy detekcyjnej bramki, a nie jej założenie.
// Lekcja z §14.14/§14.15 planu badawczego: korpus, który przechodzi także dla reguły
// obalonej, niczego nie strzeże. Sprawdzamy WPROST, że wartość o slot mniejsza od
// deklarowanej przez silnik — czyli dokładnie wynik usunięcia `++result` — wypada PONIŻEJ
// granicy zdarzeniowej, więc dopuszczałaby emisję rekordu przed określeniem zależności.
TEST(h10aGate, event_model_rejects_theta_without_its_own_slot) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(2, 5), ratio(5, 7)};
  int witnesses = 0;
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas) {
      const streamModel left{.delta = deltaA, .width = 1, .tail = 0, .origin = 0};
      const streamModel right{.delta = deltaB, .width = 1, .tail = 0, .origin = 0};
      const hashOracle interleaved = evaluateHash(left, right);
      const streamModel producer{
          .delta = interleaved.delta, .width = 1, .tail = interleaved.tail, .origin = evaluateHashOrigin(left, right)};

      const dehashOracle theta = evaluateDehash(producer, deltaA, deltaB, true);
      if (theta.tail > 0) ++witnesses;
    }
  // Bez tej liczby test klasy Theta byłby spełniony przez ogon zerowy.
  EXPECT_EQ(witnesses, static_cast<int>(deltas.size() * deltas.size()));
}

// Kontrola aparatury rozplotu.
TEST(h10aGate, dehash_probe_window_is_wide_enough) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 2), ratio(2, 5), ratio(5, 7)};
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas)
      for (int tail : {0, 1, 3}) {
        const ratio deltaC = deltaA * deltaB / (deltaA + deltaB);
        const streamModel producer{.delta = deltaC, .width = 1, .tail = tail, .origin = 1};
        EXPECT_EQ(evaluateDehash(producer, deltaA, deltaB, true, 1).tail, evaluateDehash(producer, deltaA, deltaB, true, 3).tail)
            << "dA=" << deltaA << " dB=" << deltaB << " W=" << tail;
        EXPECT_EQ(evaluateDehash(producer, deltaA, deltaB, false, 1).tail,
                  evaluateDehash(producer, deltaA, deltaB, false, 3).tail)
            << "dA=" << deltaA << " dB=" << deltaB << " W=" << tail;
      }
}

// Projekcja i redukcje: klasy dokładne, bo NIE mają własnego wkładu — działają na bieżącej
// krotce producenta i przenoszą obie wielkości bez zmiany. Korpus ma producenta
// o niezerowym ogonie ORAZ niezerowym origin, żeby przenoszenie każdej z nich osobno było
// widoczne; dotąd sprawdzały to pojedyncze wartości wpisane wprost w ut_compiler.
TEST(h10aGate, projection_and_reduction_carry_both_quantities_unchanged) {
  int checked = 0;
  for (const auto &delta : kPairDeltas)
    for (int width : {2, 3, 4})
      for (int step : kPairSteps)
        for (int length : {2, 3}) {
          qTree instance;
          const std::string rql = declareSource("src", width, delta) +    //
                                  windowOf("win", "src", step, length) +  //
                                  projectionOf("proj", "win") +           //
                                  reductionOf("red", "win");
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel source{.delta = delta, .width = width, .tail = 0, .origin = 0};
          const agseOracle window = evaluate(source, step, length);

          const auto &win  = instance.getQuery("win");
          const auto &proj = instance.getQuery("proj");
          const auto &red  = instance.getQuery("red");
          ASSERT_EQ(win.startupLatency, window.tail) << rql;
          ASSERT_EQ(win.logicalOrigin, window.origin) << rql;
          EXPECT_EQ(proj.rInterval, window.delta) << rql;
          EXPECT_EQ(proj.startupLatency, window.tail) << rql;
          EXPECT_EQ(proj.logicalOrigin, window.origin) << rql;
          EXPECT_EQ(red.rInterval, window.delta) << rql;
          EXPECT_EQ(red.startupLatency, window.tail) << rql;
          EXPECT_EQ(red.logicalOrigin, window.origin) << rql;
          if (window.origin > 0) ++checked;
        }
  // Moc detekcyjna: korpus musi zawierać przypadki o niezerowym origin, inaczej
  // przenoszenie origin byłoby sprawdzane wyłącznie na wartości zerowej.
  EXPECT_GT(checked, 0);
}

// Origin przeplotu — korpus DEDYKOWANY, wymuszony przez kontrolę mutacyjną. Korpusy
// ogona (`closed_form_matches_event_model_over_hash_corpus` i `hash_over_two_windows_...`)
// mają origin składowych zawsze zdominowany przez pierwszą z nich, więc przechodzą także
// wtedy, gdy silnik liczy origin przeplotu WYŁĄCZNIE z pierwszej składowej — sprawdzone
// mutacyjnie 2026-08-07. Bramka, która przechodzi dla reguły obalonej, niczego nie strzeże.
//
// Tu obie składowe są oknami o RÓŻNYCH origin (krok okna rozstrzyga: ceil((L-1)/step)),
// a stosunek interwałów przesuwa dominację raz na jedną, raz na drugą stronę. Test wymaga
// obu rodzajów świadków wprost.
TEST(h10aGate, hash_origin_takes_the_later_of_both_constituents) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 5), ratio(3, 10)};
  const std::vector<int> steps{1, 2, 3};
  int leftDominates  = 0;
  int rightDominates = 0;
  int checked        = 0;

  for (const auto &deltaA : deltas)
    for (int stepA : steps)
      for (const auto &deltaB : deltas)
        for (int stepB : steps) {
          qTree instance;
          const std::string rql = declareSource("a", 1, deltaA) +  //
                                  declareSource("b", 1, deltaB) +  //
                                  windowOf("wa", "a", stepA, 5) +  //
                                  windowOf("wb", "b", stepB, 5) +  //
                                  hashOf("c", "wa", "wb");
          auto [parseResult, keyword, name] = parserRQLString(instance, rql);
          ASSERT_EQ(parseResult, "OK") << rql;

          compiler compilerInstance(instance);
          ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

          const streamModel sourceA{.delta = deltaA, .width = 1, .tail = 0, .origin = 0};
          const streamModel sourceB{.delta = deltaB, .width = 1, .tail = 0, .origin = 0};
          const agseOracle windowA = evaluate(sourceA, stepA, 5);
          const agseOracle windowB = evaluate(sourceB, stepB, 5);
          const streamModel left{.delta = windowA.delta, .width = 5, .tail = windowA.tail, .origin = windowA.origin};
          const streamModel right{.delta = windowB.delta, .width = 5, .tail = windowB.tail, .origin = windowB.origin};

          const int expected = evaluateHashOrigin(left, right);
          EXPECT_EQ(instance.getQuery("c").logicalOrigin, expected) << rql;

          // Świadek dominacji: origin liczony z JEDNEJ składowej jest ściśle mniejszy
          // od prawdziwego, czyli druga składowa naprawdę rozstrzyga.
          const streamModel leftOnly{.delta = left.delta, .width = 5, .tail = left.tail, .origin = 0};
          const streamModel rightOnly{.delta = right.delta, .width = 5, .tail = right.tail, .origin = 0};
          if (evaluateHashOrigin(left, rightOnly) < expected) ++rightDominates;
          if (evaluateHashOrigin(leftOnly, right) < expected) ++leftDominates;
          ++checked;
        }

  EXPECT_EQ(checked, 3 * 3 * 3 * 3);
  EXPECT_GT(leftDominates, 0);
  EXPECT_GT(rightDominates, 0);
}

// Kontrola aparatury origin przeplotu: zapas przeglądu musi być tak dobrany, żeby ostatni
// brak rekordu na pewno się w nim mieścił. Za wąskie okno dawałoby origin ZANIŻONY, czyli
// rekord bez definicji uznany za istniejący.
TEST(h10aGate, hash_origin_probe_span_is_wide_enough) {
  const std::vector<ratio> deltas{ratio(1, 1), ratio(1, 5), ratio(3, 10)};
  for (const auto &deltaA : deltas)
    for (const auto &deltaB : deltas)
      for (int originA : {0, 2, 4})
        for (int originB : {0, 2, 4}) {
          const streamModel left{.delta = deltaA, .width = 1, .tail = 0, .origin = originA};
          const streamModel right{.delta = deltaB, .width = 1, .tail = 0, .origin = originB};
          EXPECT_EQ(evaluateHashOrigin(left, right, 1), evaluateHashOrigin(left, right, 4))
              << "dA=" << deltaA << " dB=" << deltaB << " O_A=" << originA << " O_B=" << originB;
        }
}

// =====================================================================================
// L6 — miniatura kampanii K24 na KOMPOZYCJACH (2026-08-07).
//
// Bramki wyzej sprawdzaja dziewiec klas operatorow, ale KAZDA OSOBNO, na korpusach
// dobranych pod te jedna klase. Regresja w PROPAGACJI miedzy klasami — na przyklad
// origin przechodzacy przez roznice do okna — przejdzie przez wszystkie z nich, bo zaden
// nie sklada tych dwoch operatorow. Kampania K24d (rdb-experiment/results_20260807_K24d)
// robi to na 10 010 planach glebokosci 1-6, ale chodzi recznie i tylko przy przypinaniu SHA.
//
// Ta miniatura generuje plany MIESZANE o glebokosci 2-4 i sprawdza KAZDY wezel osobno.
// Trzy rzeczy sa tu istotne:
//
//  1. Generator jest DETERMINISTYCZNY, z ziarnem wpisanym w test — nie losowanym z zegara.
//     Korpus, ktory zmienia sie miedzy przebiegami, nie jest bramka, tylko loteria.
//  2. Oracle jest REKURENCYJNY nad drzewem planu i sklada sie wylacznie z modeli
//     zdarzeniowych zdefiniowanych wyzej w tym pliku. Zaden z nich nie wola funkcji
//     silnika (ani AgseStartupLatency, ani HashStartupLatency, ani Hash/Div/Mod/Add/
//     Subtract z SOperations.hpp). Ta niezaleznosc jest jedynym powodem, dla ktorego
//     test cokolwiek znaczy.
//  3. Plan odrzucony przez kompilator jest AWARIA APARATURY i zatrzymuje test — nie jest
//     cicho pomijany. Ta sama zasada co w kampanii K24. Generator dobiera wiec parametry
//     tak, zeby produkowac wylacznie plany poprawne (kontrola reprezentowalnosci
//     interwalu i szerokosci okna sondowania PRZED emisja wezla).
//
// Miniatura mieszka w tym pliku, a nie w osobnym, bo caly model zdarzeniowy dziewieciu
// klas jest tutaj i przeniesienie go do naglowka wspoldzielonego byloby przebudowa
// bramki L1, a nie dopisaniem bramki L6.
//
// KONTROLA MUTACYJNA (2026-08-07). Czternascie mutantow silnika, nanoszonych pojedynczo
// i cofanych; wszystkie WYKRYTE przez sama miniature (bez pozostalych bramek w pliku):
//   M1  ++result usuniete z galezi STREAM_DEHASH_DIV (wlasny slot Theta)
//   M2  AddStartupLatency w postaci sprzed K24: ceil(W_src*D_src/D_out)
//   M3  origin sumy liczony tylko z pierwszej skladowej
//   M4  origin przeplotu liczony tylko z pierwszej skladowej
//   M5  origin Theta bez odwzorowania Div
//   M6  origin ~Theta bez odwzorowania Mod
//   M7  SubtractStartupLatency zastapione zerem
//   M8  origin roznicy bez odwzorowania Subtract
//   M9  redukcja gubiaca origin producenta
//   M10 AgseStartupLatency: ceil -> floor
//   M11 AgseLogicalOrigin bez czlonu (lengthAbs - 1)
//   M12 origin przesuniecia bez czlonu N
//   M13 ogon przesuniecia bez odjecia N
//   M14 projekcja gubiaca ogon producenta
// M1-M9 to lista z planu L6; M10-M14 dopisane, zeby kazda z dziewieciu klas miala
// wlasnego mutanta zarowno dla ogona, jak i dla origin. M4 i M8 — te, ktore przeszly
// przez pierwsza wersje korpusow L1 — sa tu wykrywane od razu, bo korpus WYMAGA
// swiadkow rozrozniajacych obie sytuacje (asercje na koncu testu).
// =====================================================================================

namespace {

enum class opClass { window, shift, projection, reduction, subtract, sum, hash, theta, notTheta };
constexpr int kOpClassCount = 9;

int indexOf(opClass kind) { return static_cast<int>(kind); }

const char *nameOf(opClass kind) {
  switch (kind) {
    case opClass::window:
      return "@";
    case opClass::shift:
      return ">N";
    case opClass::projection:
      return "projekcja";
    case opClass::reduction:
      return "redukcja";
    case opClass::subtract:
      return "-";
    case opClass::sum:
      return "+";
    case opClass::hash:
      return "#";
    case opClass::theta:
      return "&";
    case opClass::notTheta:
      return "%";
  }
  return "?";
}

// Rezim klasy wg tab:tail-exactness: rownosc dla klas dokladnych, `silnik >= model`
// dla zawyzajacych. W ZADNEJ klasie nie wolno dopuscic zanizenia.
bool tailIsExact(opClass kind) { return kind != opClass::subtract && kind != opClass::theta && kind != opClass::notTheta; }

struct planNode {
  std::string id;
  opClass kind  = opClass::projection;
  bool declared = false;
  streamModel model{};

  // Czy ogon MODELU jest dla tego wezla granica dokladna, czy tylko dolna.
  //
  // Dokladnosc jest wlasnoscia wezla PRZY DOKLADNYCH WEJSCIACH. Klasa dokladna nad
  // producentem zawyzajacym dziedziczy jego luz: model liczy ogon z modelowego (nizszego)
  // ogona producenta, a silnik ze swojego (wyzszego), wiec rownosc nie obowiazuje.
  // Przyklad z korpusu: `(a#b)&D` ma ogon silnika 2 przy modelu 1 (Theta jest zawyzajaca),
  // a stojace nad nim `>1` dziedziczy te roznice. Skazenie idzie wiec w gore drzewa.
  bool tailExact = true;

  // Skladowe przeplotu — potrzebne do zbudowania nad nim rozplotu ORAZ do swiadkow
  // dominacji origin (mutant M4: origin przeplotu liczony z jednej skladowej).
  bool isHash = false;
  streamModel left{};
  streamModel right{};

  // Producent wezla jednoargumentowego — potrzebny do swiadka M8 (origin roznicy
  // bez odwzorowania Subtract jest tozsamoscia na origin producenta).
  int producerOrigin = 0;
};

// Interwal, ktorego licznik albo mianownik wychodzi poza ten prog, prowadzi w kompilatorze
// do bledu zakresu — plan bylby odrzucony, a odrzucony plan jest awaria aparatury.
constexpr int kMaxIntervalTerm = 20000;
// Gorne ograniczenia okien sondowania oracle'a: bez nich glebokie kompozycje przeplotow
// daja okresy fazowe rzedu 10^4 slotow i test przestaje miescic sie w budzecie.
constexpr int kMaxPhasePeriod         = 120;
constexpr int kMaxSubtractDenominator = 20;

bool representable(const ratio &value) {
  return std::abs(value.numerator()) <= kMaxIntervalTerm && value.denominator() <= kMaxIntervalTerm;
}

const std::vector<ratio> kMixDeltas{ratio(1, 1), ratio(1, 2), ratio(1, 3), ratio(1, 5), ratio(2, 5), ratio(3, 10)};
const std::vector<int> kMixWidths{1, 2, 3};
const std::vector<int> kMixSteps{1, 2, 3};
const std::vector<int> kMixLengths{2, 3, 4, 5, -2, -3};
const std::vector<int> kMixOffsets{1, 2, 3, 5};
const std::vector<ratio> kMixFactors{ratio(2, 1), ratio(3, 1), ratio(5, 2), ratio(4, 3)};

// Generator planow mieszanych. Ziarno jest argumentem konstruktora i w tescie jest STALA.
struct mixedPlanBuilder {
  explicit mixedPlanBuilder(std::uint32_t seed) : state_(seed) {}

  std::string rql;
  std::vector<planNode> derived;

  void reset() {
    rql.clear();
    derived.clear();
    counter_ = 0;
  }

  // Buduje poddrzewo o zadanej glebokosci i zwraca jego korzen.
  planNode build(int depth) {
    if (depth <= 0) return declaration();

    // Klasy probowane w losowej kolejnosci; pierwsza, ktora da sie zbudowac legalnie,
    // wygrywa. Projekcja jest zawsze legalna, wiec petla zawsze sie konczy.
    for (int attempt = 0; attempt < 24; ++attempt) {
      const auto kind = static_cast<opClass>(nextInt(kOpClassCount));
      if (auto node = tryBuild(kind, depth); node) return *node;
    }
    return unaryCarrier(opClass::projection, build(depth - 1));
  }

 private:
  std::uint32_t state_;
  int counter_ = 0;

  int nextInt(int bound) {
    state_ = state_ * 1103515245u + 12345u;
    return static_cast<int>((state_ >> 16) % static_cast<std::uint32_t>(bound));
  }

  template <typename T>
  const T &pick(const std::vector<T> &values) {
    return values[static_cast<size_t>(nextInt(static_cast<int>(values.size())))];
  }

  std::string freshId(const char *prefix) { return prefix + std::to_string(counter_++); }

  planNode declaration() {
    const ratio delta = pick(kMixDeltas);
    const int width   = pick(kMixWidths);
    planNode node{.id = freshId("d"), .kind = opClass::projection, .declared = true};
    node.model = streamModel{.delta = delta, .width = width, .tail = 0, .origin = 0};
    rql += declareSource(node.id, width, delta);
    return node;
  }

  planNode record(planNode node) {
    derived.push_back(node);
    return node;
  }

  // Projekcja i redukcja: klasy przenoszace obie wielkosci bez zmiany, szerokosc wyniku 1.
  planNode unaryCarrier(opClass kind, const planNode &src) {
    planNode node{.id = freshId(kind == opClass::projection ? "p" : "r"), .kind = kind};
    node.model          = streamModel{.delta = src.model.delta, .width = 1, .tail = src.model.tail, .origin = src.model.origin};
    node.tailExact      = src.tailExact && tailIsExact(kind);
    node.producerOrigin = src.model.origin;
    rql += (kind == opClass::projection) ? projectionOf(node.id, src.id) : reductionOf(node.id, src.id);
    return record(node);
  }

  // Przeplot wymaga zgodnych schematow, wiec obie skladowe sprowadzamy projekcja
  // do jednego pola. Projekcja jest klasa dokladna i sama tez jest sprawdzana.
  planNode narrowToOneField(const planNode &node) {
    if (node.model.width == 1) return node;
    return unaryCarrier(opClass::projection, node);
  }

  std::optional<planNode> makeHash(int depth) {
    const planNode rawLeft  = build(depth - 1);
    const planNode rawRight = build(nextInt(depth));
    const planNode left     = narrowToOneField(rawLeft);
    const planNode right    = narrowToOneField(rawRight);

    const ratio ratioAB = left.model.delta / right.model.delta;
    if (ratioAB.numerator() + ratioAB.denominator() > kMaxPhasePeriod) return std::nullopt;
    const ratio deltaC = left.model.delta * right.model.delta / (left.model.delta + right.model.delta);
    if (!representable(deltaC)) return std::nullopt;

    const hashOracle expected = evaluateHash(left.model, right.model);
    planNode node{.id = freshId("h"), .kind = opClass::hash};
    node.model = streamModel{
        .delta = expected.delta, .width = 1, .tail = expected.tail, .origin = evaluateHashOrigin(left.model, right.model)};
    node.isHash    = true;
    node.tailExact = left.tailExact && right.tailExact && tailIsExact(opClass::hash);
    node.left      = left.model;
    node.right     = right.model;
    rql += hashOf(node.id, left.id, right.id);
    return record(node);
  }

  std::optional<planNode> tryBuild(opClass kind, int depth) {
    switch (kind) {
      case opClass::projection:
      case opClass::reduction:
        return unaryCarrier(kind, build(depth - 1));

      case opClass::window: {
        const planNode src = build(depth - 1);
        const int step     = pick(kMixSteps);
        const int length   = pick(kMixLengths);
        const ratio delta  = src.model.delta * ratio(step, src.model.width);
        if (!representable(delta)) return std::nullopt;

        const agseOracle expected = evaluate(src.model, step, length);
        planNode node{.id = freshId("w"), .kind = kind};
        node.model = streamModel{
            .delta = expected.delta, .width = length < 0 ? -length : length, .tail = expected.tail, .origin = expected.origin};
        node.tailExact      = src.tailExact && tailIsExact(kind);
        node.producerOrigin = src.model.origin;
        rql += windowOf(node.id, src.id, step, length);
        return record(node);
      }

      case opClass::shift: {
        const planNode src         = build(depth - 1);
        const int offset           = pick(kMixOffsets);
        const shiftOracle expected = evaluateShift(src.model, offset, src.declared);
        planNode node{.id = freshId("s"), .kind = kind};
        node.model =
            streamModel{.delta = src.model.delta, .width = src.model.width, .tail = expected.tail, .origin = expected.origin};
        node.tailExact      = src.tailExact && tailIsExact(kind);
        node.producerOrigin = src.model.origin;
        rql += shiftOf(node.id, src.id, offset);
        return record(node);
      }

      case opClass::subtract: {
        const planNode src = build(depth - 1);
        const ratio target = src.model.delta * pick(kMixFactors);
        if (!representable(target)) return std::nullopt;
        if ((target / src.model.delta).denominator() > kMaxSubtractDenominator) return std::nullopt;

        const subtractOracle expected = evaluateSubtract(src.model, target);
        planNode node{.id = freshId("m"), .kind = kind};
        node.model = streamModel{.delta = target, .width = src.model.width, .tail = expected.tail, .origin = expected.origin};
        node.tailExact      = false;  // klasa zawyzajaca
        node.producerOrigin = src.model.origin;
        rql += subtractOf(node.id, src.id, target);
        return record(node);
      }

      case opClass::sum: {
        const planNode left  = build(depth - 1);
        const planNode right = build(nextInt(depth));
        const ratio deltaOut = std::min(left.model.delta, right.model.delta);
        if (!representable(deltaOut)) return std::nullopt;
        const int span = std::lcm((deltaOut / left.model.delta).denominator(), (deltaOut / right.model.delta).denominator());
        if (span > kMaxPhasePeriod) return std::nullopt;

        const sumOracle expected = evaluateSum(left.model, right.model);
        planNode node{.id = freshId("a"), .kind = kind};
        node.model     = streamModel{.delta  = expected.delta,
                                     .width  = left.model.width + right.model.width,
                                     .tail   = expected.tail,
                                     .origin = expected.origin};
        node.tailExact = left.tailExact && right.tailExact && tailIsExact(kind);
        node.left      = left.model;
        node.right     = right.model;
        rql += sumOf(node.id, left.id, right.id);
        return record(node);
      }

      case opClass::hash:
        return makeHash(depth);

      case opClass::theta:
      case opClass::notTheta: {
        const auto source = makeHash(depth);
        if (!source) return std::nullopt;
        const bool leftConstituent = (kind == opClass::theta);
        // Argumentem rozplotu jest interwal skladowej USUWANEJ.
        const ratio removed   = leftConstituent ? source->right.delta : source->left.delta;
        const ratio recovered = leftConstituent ? source->left.delta : source->right.delta;

        const dehashOracle expected = evaluateDehash(source->model, source->left.delta, source->right.delta, leftConstituent);
        planNode node{.id = freshId(leftConstituent ? "t" : "n"), .kind = kind};
        node.model =
            streamModel{.delta = recovered, .width = source->model.width, .tail = expected.tail, .origin = expected.origin};
        node.tailExact      = false;  // klasa zawyzajaca
        node.producerOrigin = source->model.origin;
        rql += leftConstituent ? thetaOf(node.id, source->id, removed) : notThetaOf(node.id, source->id, removed);
        return record(node);
      }
    }
    return std::nullopt;
  }
};

}  // namespace

// Miniatura K24: 240 planow mieszanych o glebokosci 2-4, asercje PER WEZEL.
TEST(h10aGate, mixed_plan_corpus_matches_the_recursive_event_model) {
  constexpr std::uint32_t kSeed = 20260807u;  // ziarno WPISANE W TEST
  constexpr int kPlans          = 400;
  constexpr int kMinPerClass    = 60;

  mixedPlanBuilder builder(kSeed);
  std::array<int, kOpClassCount> classCount{};
  // Osobno liczone asercje ROWNOSCI ogona. Asercja `>=` jest istotnie slabsza, wiec
  // stratyfikacja po samej liczbie wystapien klasy nie wystarcza: klasa dokladna, ktora
  // w calym korpusie stoi wylacznie nad producentem zawyzajacym, bylaby sprawdzana
  // wylacznie nierownoscia.
  std::array<int, kOpClassCount> exactTailChecks{};
  int nodesChecked = 0;

  // Swiadkowie mocy detekcyjnej — patrz komentarz przy asercjach na koncu testu.
  int hashLeftDominates = 0, hashRightDominates = 0;
  int subtractOriginMoved = 0, subtractTight = 0, thetaTight = 0;

  for (int planIndex = 0; planIndex < kPlans; ++planIndex) {
    builder.reset();
    builder.build(2 + planIndex % 3);

    qTree instance;
    auto [parseResult, keyword, name] = parserRQLString(instance, builder.rql);
    ASSERT_EQ(parseResult, "OK") << builder.rql;

    compiler compilerInstance(instance);
    ASSERT_EQ(compilerInstance.compile(), "OK") << builder.rql;

    for (const auto &node : builder.derived) {
      // Wezel, ktory znika z planu, jest awaria aparatury tak samo jak plan odrzucony:
      // bez niego nie wiadomo, czy regula byla sprawdzona.
      ASSERT_TRUE(instance.exists(node.id)) << builder.rql;
      const auto &q = instance.getQuery(node.id);

      EXPECT_EQ(q.rInterval, node.model.delta) << nameOf(node.kind) << " " << node.id << "\n" << builder.rql;
      // Origin: rownosc we WSZYSTKICH dziewieciu klasach.
      EXPECT_EQ(q.logicalOrigin, node.model.origin) << nameOf(node.kind) << " " << node.id << "\n" << builder.rql;
      // Ogon: rownosc dla klas dokladnych NAD DOKLADNYMI PRODUCENTAMI, `silnik >= model`
      // gdziekolwiek w poddrzewie stoi klasa zawyzajaca. Zanizenie jest niedopuszczalne
      // w kazdej klasie i w kazdym polozeniu.
      if (node.tailExact) {
        EXPECT_EQ(q.startupLatency, node.model.tail) << nameOf(node.kind) << " " << node.id << "\n" << builder.rql;
        ++exactTailChecks[indexOf(node.kind)];
      } else
        EXPECT_GE(q.startupLatency, node.model.tail) << nameOf(node.kind) << " " << node.id << "\n" << builder.rql;

      ++classCount[indexOf(node.kind)];
      ++nodesChecked;

      if (node.kind == opClass::hash) {
        // Swiadek mutanta M4 (origin przeplotu liczony tylko z pierwszej skladowej):
        // korpus musi zawierac przypadki, w ktorych rozstrzyga raz jedna, raz druga.
        const streamModel leftOnly{.delta = node.left.delta, .width = node.left.width, .tail = node.left.tail, .origin = 0};
        const streamModel rightOnly{.delta = node.right.delta, .width = node.right.width, .tail = node.right.tail, .origin = 0};
        if (evaluateHashOrigin(node.left, rightOnly) < node.model.origin) ++hashRightDominates;
        if (evaluateHashOrigin(leftOnly, node.right) < node.model.origin) ++hashLeftDominates;
      }
      if (node.kind == opClass::subtract) {
        // Swiadek mutanta M8 (origin roznicy bez odwzorowania Subtract): odwzorowanie
        // musi w korpusie NAPRAWDE przesuwac origin producenta, inaczej sprawdzana
        // jest tozsamosc, a nie regula.
        if (node.model.origin != node.producerOrigin) ++subtractOriginMoved;
        if (q.startupLatency == node.model.tail) ++subtractTight;
      }
      if (node.kind == opClass::theta && q.startupLatency == node.model.tail) ++thetaTight;
    }
  }

  // --- stratyfikacja ---------------------------------------------------------------
  // Bez tej asercji generator moglby wylosowac korpus, w ktorym trzech klas nie ma wcale,
  // a test i tak by przeszedl.
  for (int i = 0; i < kOpClassCount; ++i)
    EXPECT_GE(classCount[i], kMinPerClass) << "klasa " << nameOf(static_cast<opClass>(i)) << " wystapila " << classCount[i]
                                           << " razy";
  // Szesc klas dokladnych musi byc sprawdzone ROWNOSCIA, a nie tylko nierownoscia.
  for (int i = 0; i < kOpClassCount; ++i) {
    const auto kind = static_cast<opClass>(i);
    if (!tailIsExact(kind)) continue;
    EXPECT_GE(exactTailChecks[i], kMinPerClass)
        << "klasa " << nameOf(kind) << " sprawdzona rownoscia ogona " << exactTailChecks[i] << " razy";
  }
  EXPECT_GT(nodesChecked, 0);

  // --- moc detekcyjna --------------------------------------------------------------
  // Lekcja z §14.14/§14.15 planu badawczego, potwierdzona przy L1: mutanty M4 i M8
  // przeszly przez pierwsza wersje korpusow, bo korpus przypadkiem nie zawieral
  // przypadku rozrozniajacego. Tu obie sytuacje sa wymagane WPROST.
  EXPECT_GT(hashLeftDominates, 0);
  EXPECT_GT(hashRightDominates, 0);
  EXPECT_GT(subtractOriginMoved, 0);
  // Nierownosci musza byc gdzies CIASNE — inaczej przechodzilyby takze dla reguly
  // zawyzajacej dowolnie mocno, czyli nie strzeglyby niczego poza znakiem.
  EXPECT_GT(subtractTight, 0);
  EXPECT_GT(thetaTight, 0);
}

// Kontrola aparatury miniatury: oracle rekurencyjny na planach JEDNOWEZLOWYCH musi dawac
// to samo co bramki jednoklasowe wyzej. Plan glebokosci 1 to dokladnie ich ksztalt —
// jeden operator nad deklaracjami — wiec ten test sprawdza maszynerie skladania, a nie
// same reguly. Bez niej blad w rekurencji (na przyklad zla szerokosc przekazana w gore)
// mogby udawac blad silnika albo, gorzej, kompensowac sie z nim.
TEST(h10aGate, recursive_oracle_reduces_to_the_single_node_gates) {
  constexpr std::uint32_t kSeed = 20260808u;
  constexpr int kPlans          = 120;

  mixedPlanBuilder builder(kSeed);
  std::array<int, kOpClassCount> classCount{};

  for (int planIndex = 0; planIndex < kPlans; ++planIndex) {
    builder.reset();
    builder.build(1);

    qTree instance;
    auto [parseResult, keyword, name] = parserRQLString(instance, builder.rql);
    ASSERT_EQ(parseResult, "OK") << builder.rql;

    compiler compilerInstance(instance);
    ASSERT_EQ(compilerInstance.compile(), "OK") << builder.rql;

    for (const auto &node : builder.derived) {
      ASSERT_TRUE(instance.exists(node.id)) << builder.rql;
      const auto &q = instance.getQuery(node.id);
      EXPECT_EQ(q.rInterval, node.model.delta) << nameOf(node.kind) << "\n" << builder.rql;
      EXPECT_EQ(q.logicalOrigin, node.model.origin) << nameOf(node.kind) << "\n" << builder.rql;
      if (node.tailExact)
        EXPECT_EQ(q.startupLatency, node.model.tail) << nameOf(node.kind) << "\n" << builder.rql;
      else
        EXPECT_GE(q.startupLatency, node.model.tail) << nameOf(node.kind) << "\n" << builder.rql;
      ++classCount[indexOf(node.kind)];
    }
  }

  // Kontrola aparatury tez musi byc stratyfikowana — inaczej moglaby nie dotknac
  // polowy klas i milczaco potwierdzac maszynerie, ktorej nie uruchomila.
  for (int i = 0; i < kOpClassCount; ++i)
    EXPECT_GT(classCount[i], 0) << "klasa " << nameOf(static_cast<opClass>(i)) << " nieobecna w kontroli aparatury";
}
