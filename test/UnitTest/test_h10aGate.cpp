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
  return {.origin = source.origin + offset,
          .tail   = tail,
          .sourceCapacity = rev + 1 + (sourceDeclared ? kDeclarationPrefetch : 0)};
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
