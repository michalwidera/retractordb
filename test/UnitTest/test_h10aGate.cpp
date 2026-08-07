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

#include <numeric>
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
