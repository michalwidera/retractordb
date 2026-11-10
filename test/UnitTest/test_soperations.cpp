#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <tuple>
#include <vector>

#include "SOperations.hpp"

// ctest -R '^ut-soperations' -V

TEST(xSOperations, rounding_helpers_obey_signed_and_64_bit_contracts) {
  // covers: soperations.floor-r.1~1, soperations.floor-div.1~1, soperations.ceil-r.1~1, soperations.rounded-away-from-zero.1~1
  EXPECT_EQ(floorR(boost::rational<int>(-3, 2)), -1);
  EXPECT_EQ(floorR(boost::rational<int>(3, 2)), 1);
  EXPECT_EQ(ceilR(boost::rational<int>(-3, 2)), -2);
  EXPECT_EQ(ceilR(boost::rational<int>(3, 2)), 2);
  EXPECT_EQ(ceilR(boost::rational<int>(-2)), -2);
  EXPECT_EQ(ceilR(boost::rational<int>(0)), 0);
  EXPECT_EQ(floorDiv(-3, 2), -2);
  EXPECT_EQ(floorDiv(3, -2), -2);
  EXPECT_EQ(floorDiv(-3, -2), 1);
  EXPECT_EQ(floorDiv(4, 2), 2);
  EXPECT_EQ(floorDiv(INT64_C(6000000001), 2), INT64_C(3000000000));
  EXPECT_EQ(roundedAwayFromZero(-3, 2), -2);
  EXPECT_EQ(roundedAwayFromZero(3, 2), 2);
  EXPECT_EQ(roundedAwayFromZero(-4, 2), -2);
  EXPECT_EQ(roundedAwayFromZero(INT64_C(6000000001), 2), INT64_C(3000000001));
}

#ifndef NDEBUG
// Hash dostaje tylko dodatnie interwaly sprawdzone przez kompilator. Bezposrednie naruszenie
// tego kontraktu strzeze asercja debug; nie jest to kanal odmowy blednego planu.
TEST(xSOperations, hash_asserts_positive_delta_in_debug) {
  // covers: soperations.hash.2~1
  EXPECT_DEATH(
      {
        int retPos = 0;
        Hash(0, 1, 0, retPos);
      },
      "deltaA > 0");
  for (const int deltaB : {0, -1}) {
    EXPECT_DEATH(
        {
          int retPos = 0;
          Hash(1, deltaB, 0, retPos);
        },
        "deltaB > 0");
  }
}
#endif

TEST(xSOperations, hash_operations) {
  // covers: soperations.hash.1~1
  std::vector<std::vector<int>> testdata{
      {0, 1, 0},  // B[0]
      {1, 1, 1},  // B[1]
      {2, 0, 0},  //      A[0]
      {3, 1, 2},  // B[2]
      {4, 1, 3},  // B[3]
      {5, 0, 1},  //      A[1]
      {6, 1, 4},  // B[4]
      {7, 1, 5},  // B[5]
      {8, 0, 2},  //      A[2]
      {9, 1, 6},  // B[6]
  };

  auto deltaA = boost::rational<int>(1, 1);
  auto deltaB = boost::rational<int>(1, 2);

  int retPos{0};
  for (int i = 0; i < 10; i++) {
    auto leftOrRight = Hash(deltaA, deltaB, i, retPos);
    EXPECT_TRUE(testdata[i][1] == leftOrRight ? 1 : 0);
    EXPECT_TRUE(testdata[i][2] == retPos);
  }
}

// Wzorzec wyprowadzony z definicji przeplotu (def:interleave):
// z = deltaB/(deltaA+deltaB) = 2/3 dla pary (1/16, 1/8) - wzorzec B,A,A,...
// Stary kod używał delta_c zamiast z w retPos, co było maskowane w teście
// wyżej przez deltaA == 1 (wtedy delta_c == z liczbowo).
TEST(xSOperations, hash_operations_nonunit_delta) {
  // covers: soperations.hash.1~1
  std::vector<std::vector<int>> testdata{
      {0, 1, 0},  // B[0]
      {1, 0, 0},  //      A[0]
      {2, 0, 1},  //      A[1]
      {3, 1, 1},  // B[1]
      {4, 0, 2},  //      A[2]
      {5, 0, 3},  //      A[3]
      {6, 1, 2},  // B[2]
      {7, 0, 4},  //      A[4]
      {8, 0, 5},  //      A[5]
      {9, 1, 3},  // B[3]
  };

  auto deltaA = boost::rational<int>(1, 16);
  auto deltaB = boost::rational<int>(1, 8);

  int retPos{0};
  for (int i = 0; i < 10; i++) {
    auto leftOrRight = Hash(deltaA, deltaB, i, retPos);
    EXPECT_EQ(testdata[i][1], leftOrRight ? 1 : 0) << "i=" << i;
    EXPECT_EQ(testdata[i][2], retPos) << "i=" << i;
  }
}

// Równe delty (perfect shuffle): z = 1/2 - wzorzec B,A,B,A,...
TEST(xSOperations, hash_operations_equal_delta) {
  // covers: soperations.hash.1~1
  auto delta = boost::rational<int>(1, 360);

  int retPos{0};
  for (int i = 0; i < 20; i++) {
    auto leftOrRight = Hash(delta, delta, i, retPos);
    EXPECT_EQ(i % 2 == 0, leftOrRight) << "i=" << i;
    EXPECT_EQ(i / 2, retPos) << "i=" << i;
  }
}

// Dopasowanie przesunięć przeplotu:
// (A > i) # (B > k) == (A # B) > (i + k), gdy i*deltaA == k*deltaB.
// Test sprawdza niezależnie od kompilatora zarówno prefiks opóźnienia, jak i
// późniejszy wybór tej samej gałęzi oraz tego samego indeksu źródła.
TEST(xSOperations, hash_matched_time_moves) {
  struct TestCase {
    boost::rational<int> deltaA;
    boost::rational<int> deltaB;
    int shiftA;
    int shiftB;
  };
  const std::vector<TestCase> cases{
      {{1, 10}, {1, 5}, 2, 1},
      {{1, 10}, {1, 10}, 3, 3},
      {{3, 10}, {1, 10}, 1, 3},
  };

  for (const auto &[deltaA, deltaB, shiftA, shiftB] : cases) {
    ASSERT_EQ(deltaA * shiftA, deltaB * shiftB);
    const int outputShift = shiftA + shiftB;

    for (int n = 0; n < 200; ++n) {
      int shiftedSourceIndex = 0;
      const bool takeB       = Hash(deltaA, deltaB, n, shiftedSourceIndex);
      const int sourceShift  = takeB ? shiftB : shiftA;

      if (n < outputShift) {
        EXPECT_LT(shiftedSourceIndex, sourceShift) << "n=" << n;
        continue;
      }

      int originalSourceIndex  = 0;
      const bool originalTakeB = Hash(deltaA, deltaB, n - outputShift, originalSourceIndex);
      EXPECT_EQ(takeB, originalTakeB) << "n=" << n;
      EXPECT_EQ(shiftedSourceIndex - sourceShift, originalSourceIndex) << "n=" << n;
    }
  }
}

TEST(xSOperations, divmod_operations) {
  // covers: soperations.div.1~1, soperations.mod.1~1
  std::vector<std::vector<int>> testdata{
      {0, 2, 0},    //
      {1, 5, 1},    //
      {2, 8, 3},    //
      {3, 11, 4},   //
      {4, 14, 6},   //
      {5, 17, 7},   //
      {6, 20, 9},   //
      {7, 23, 10},  //
      {8, 26, 12},  //
      {9, 29, 13},  //
  };

  auto deltaA = boost::rational<int>(1, 1);
  auto deltaB = boost::rational<int>(1, 2);

  for (int i = 0; i < 10; i++) {
    auto retPosDiv = Div(deltaA, deltaB, i);
    auto retPosMod = Mod(deltaA, deltaB, i);
    EXPECT_TRUE(testdata[i][1] == retPosDiv);
    EXPECT_TRUE(testdata[i][2] == retPosMod);
  }
}

// Spójność rozplotu z przeplotem (cor:exact): pozycja z Div/Mod musi
// wskazywać w strumieniu przeplecionym element WŁAŚCIWEJ składowej o tym
// samym indeksie - Θ (Div) trafia w gałąź A, ~Θ (Mod) w gałąź B.
TEST(xSOperations, divmod_inverts_hash) {
  std::vector<std::pair<boost::rational<int>, boost::rational<int>>> pairs{
      {{1, 1}, {1, 2}},      //
      {{1, 16}, {1, 8}},     //
      {{1, 360}, {1, 360}},  //
      {{3, 10}, {1, 10}},    //
  };

  for (const auto &[deltaA, deltaB] : pairs) {
    for (int i = 0; i < 200; i++) {
      int retPos{0};
      EXPECT_FALSE(Hash(deltaA, deltaB, Div(deltaA, deltaB, i), retPos));
      EXPECT_EQ(i, retPos);
      EXPECT_TRUE(Hash(deltaA, deltaB, Mod(deltaA, deltaB, i), retPos));
      EXPECT_EQ(i, retPos);
    }
  }
}

TEST(xSOperations, subtract_uses_forward_target_index) {
  // covers: soperations.subtract.1~1
  const boost::rational<int> source{1, 2};

  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(i, Subtract(source, source, i));
    EXPECT_EQ(2 * i, Subtract(source, boost::rational<int>{1}, i));
  }

  const std::vector<int> expected{0, 2, 3, 5, 6, 8};
  for (int i = 0; i < static_cast<int>(expected.size()); ++i)
    EXPECT_EQ(expected[i], Subtract(source, boost::rational<int>{3, 4}, i));
}

// Definicja sumy strumieni (def:sum): c_n = (a_n, b_{⌊n·Δa/Δb⌋}) dla Δa <= Δb.
// Silnik odwzorowywał wcześniej b_{⌊(n+1)·Δa/Δb⌋} (bieżący payload składowej) -
// stąd wartości oczekiwane wypisane wprost, a nie wyprowadzone drugą kopią wzoru.
TEST(xSOperations, add_pairs_slower_component_by_floor_index) {
  // covers: soperations.add.1~1
  const boost::rational<int> fast{1, 2};

  // Składowa o interwale wyjścia jest indeksowana wprost n.
  for (int n = 0; n < 20; ++n)
    EXPECT_EQ(n, Add(fast, fast, n));

  // Iloraz całkowity: Δsrc = 2·Δout - b_0, b_0, b_1, b_1, ...
  const std::vector<int> ratioTwo{0, 0, 1, 1, 2, 2, 3, 3};
  for (int n = 0; n < static_cast<int>(ratioTwo.size()); ++n)
    EXPECT_EQ(ratioTwo[n], Add(fast, boost::rational<int>{1}, n));

  // Iloraz niecałkowity: Δsrc = 3/4, Δout = 1/2 - ⌊2n/3⌋.
  const std::vector<int> ratioThreeQuarters{0, 0, 1, 2, 2, 3, 4, 4};
  for (int n = 0; n < static_cast<int>(ratioThreeQuarters.size()); ++n)
    EXPECT_EQ(ratioThreeQuarters[n], Add(fast, boost::rational<int>{3, 4}, n));
}

// Okno stemplowane KOŃCEM przedziału: rekord n obejmuje pozycje n*step-(|len|-1) ... n*step.
// Ogon zabezpiecza wyłącznie NAJNOWSZE pole (n*step), bo dolny koniec zakresu jest domeną
// origin - stąd brak zależności od długości okna i brak członu fazowego. Wartości poniżej
// wyprowadzone z warunku dostępności: rekord floor(n*step/F) jest określony w chwili
// (floor(n*step/F)+1+W_src)*Delta_src, a slot n kończy się w (n+1+W)*Delta_out.
TEST(xSOperations, agse_startup_latency_covers_newest_field) {
  // covers: soperations.agse-startup-latency.1~1
  // F=1, step=1: rekord n potrzebuje wyłącznie źródła n, dostępnego w jego własnym slocie.
  // Czekanie na komplet okna przeszło do origin, więc ogon jest zerowy niezależnie od długości.
  EXPECT_EQ(0, AgseStartupLatency(1, 1, 0));
  // F=5, step=1: slot wyjścia to 1/5 rekordu źródła, więc na rekord 0 źródła czeka się
  // 5 slotów; W = ceil(5/1)-1 = 4.
  EXPECT_EQ(4, AgseStartupLatency(5, 1, 0));
  // Producent obliczany o własnym ogonie 2: pierwszy rekord źródła dopiero w chwili 3*D_src.
  EXPECT_EQ(1, AgseStartupLatency(2, 3, 2));
  // Krok większy od szerokości źródła - slot wyjścia dłuższy niż rekord źródła, ogon zerowy.
  EXPECT_EQ(0, AgseStartupLatency(2, 3, 0));
  EXPECT_EQ(1, AgseStartupLatency(6, 4, 0));
}

// Origin jest jedynym miejscem, w którym rozpiętość okna wpływa na start strumienia.
// O = ceil((O_src*F + |len| - 1) / step).
TEST(xSOperations, agse_logical_origin_skips_incomplete_windows) {
  // covers: soperations.agse-logical-origin.1~1
  // F=1, step=1, len=4: pierwsze pełne okno kończy się na pozycji 3.
  EXPECT_EQ(3, AgseLogicalOrigin(1, 1, 4, 0));
  // Znak długości jest wyłącznie konwencją kolejności pól - origin jest ten sam.
  EXPECT_EQ(3, AgseLogicalOrigin(1, 1, -4, 0));
  // Krok 2: okno kończące się na pozycji 2*n musi sięgnąć 2*n-1 >= 0, czyli n >= 1.
  EXPECT_EQ(1, AgseLogicalOrigin(1, 2, 2, 0));
  // Okno o długości 1 nigdy nie sięga wstecz - strumień zaczyna się od rekordu 0.
  EXPECT_EQ(0, AgseLogicalOrigin(1, 1, 1, 0));
  // Origin źródła przesuwa jego pozycje spłaszczone o O_src*F i propaguje się w górę:
  // ceil((2*3 + 4 - 1) / 2) = ceil(9/2) = 5.
  EXPECT_EQ(5, AgseLogicalOrigin(3, 2, 4, 2));
}

// Dwa okna `@(1,65536)` w lancuchu: O_src = 65535 i F = 65536, wiec O_src*F przekracza int. Do
// 2026-09-27 obie postaci liczyly w int i przepelnienie bylo UB; teraz wynik jest dokladny w int64,
// a wartosc spoza int odrzuca kompilator jako blad planu (A2 M11).
TEST(xSOperations, agse_origin_and_latency_do_not_overflow_int) {
  // covers: soperations.agse-startup-latency.1~1, soperations.agse-logical-origin.1~1
  EXPECT_EQ(std::int64_t{4294967295}, AgseLogicalOrigin(65536, 1, 65536, 65535));
  EXPECT_EQ(std::int64_t{4294967295}, AgseStartupLatency(65536, 1, 65535));
  // Duzy krok sprowadza ten sam iloczyn posredni z powrotem do int.
  EXPECT_EQ(std::int64_t{65535}, AgseLogicalOrigin(65536, 65536, 1, 65535));
}

// Ogon sumy strumieni musi objąć dostępność rekordu KAŻDEJ składowej pod
// indeksem floor(n*D_out/D_src), a nie tylko przeliczyć ogon składowej.
TEST(xSOperations, add_startup_latency_covers_slower_component) {
  // covers: soperations.add-startup-latency.1~1
  const boost::rational<int> fast{1, 2};

  // Składowa o interwale wyjścia i zerowym ogonie nie wnosi opóźnienia.
  EXPECT_EQ(0, AddStartupLatency(fast, fast, 0));
  // Składowa dwa razy wolniejsza: jej rekord 0 powstaje w chwili 1*D_src,
  // czyli po dwóch slotach wyjścia - ogon 1. Poprzednia postać dawała 0.
  EXPECT_EQ(1, AddStartupLatency(boost::rational<int>{1}, fast, 0));
  // Ta sama składowa z własnym ogonem 2: rekord 0 dopiero w chwili 3*D_src.
  EXPECT_EQ(5, AddStartupLatency(boost::rational<int>{1}, fast, 2));
  // Składowa szybsza od wyjścia nie może wystąpić (D_out = min), ale wzór
  // pozostaje zdefiniowany i nie schodzi poniżej zera.
  EXPECT_EQ(0, AddStartupLatency(boost::rational<int>{1, 4}, fast, 0));
}

// Ogon `-`, `Θ` i `~Θ` liczony z kresu fazy odczytu. Wartości oczekiwane pochodzą
// z modelu zdarzeniowego (investigation_K24H10, weryfikacja per węzeł), nie z tej samej
// postaci zamkniętej - inaczej test potwierdzałby wyłącznie sam siebie.
TEST(xSOperations, subtract_startup_latency_covers_fractional_phase) {
  // covers: soperations.phase-startup-latency.1~1, soperations.subtract-startup-latency.1~1
  const boost::rational<int> source{1, 2};

  // Składowa dwa razy szybsza od wyniku: pierwszy potrzebny rekord istnieje w slocie 0.
  // Do 2026-08-18 gałąź deklaracyjna dokładała tu slot, choć deklaracja ma ogon zerowy.
  EXPECT_EQ(0, SubtractStartupLatency(source, boost::rational<int>{1}, 0));
  // Faza ułamkowa: r = 3/2, kres fazy 1/2, więc ceil((1/2+1)/(3/2))-1 = 0.
  // Dawna postać dawała 1 - to jest ten slot zawyżenia, który mierzyła kampania K24.
  EXPECT_EQ(0, SubtractStartupLatency(source, boost::rational<int>{3, 4}, 0));
  // Ogon składowej wchodzi liniowo: ceil((1/2+2)/(3/2))-1 = 1.
  EXPECT_EQ(1, SubtractStartupLatency(source, boost::rational<int>{3, 4}, 1));
  // Ten sam takt na wejściu i wyjściu przenosi ogon składowej bez zmiany.
  EXPECT_EQ(3, SubtractStartupLatency(source, source, 3));
}

TEST(xSOperations, dehash_startup_latency_follows_the_phase_bound) {
  // covers: soperations.phase-startup-latency.1~1, soperations.theta-startup-latency.1~1
  // covers: soperations.n-theta-startup-latency.1~1
  const boost::rational<int> source{1, 2};

  // Iloraz całkowity: kres fazy Θ wynosi a, po podzieleniu przez r daje ogon ZERO.
  // Dawne bezwarunkowe ++result dawało tu 1 - 40,3% węzłów `Θ` korpusu K24.
  EXPECT_EQ(0, ThetaStartupLatency(source, boost::rational<int>{1}, boost::rational<int>{1}, 0));
  // Iloraz ułamkowy: a/b = (3/4)/(3/2) = 1/2, kres (1+2-1)/2 = 1, r = 3/2,
  // więc ceil((1+1)/(3/2))-1 = 1. Tu slot jest prawdziwy i zostaje.
  EXPECT_EQ(1, ThetaStartupLatency(source, boost::rational<int>{3, 4}, boost::rational<int>{3, 2}, 0));

  // ~Θ nie ma własnego członu: kres fazy jest zerowy, bo pozycja wypada najpóźniej
  // w bieżącym slocie.
  EXPECT_EQ(0, NThetaStartupLatency(source, boost::rational<int>{1}, 0));
  EXPECT_EQ(0, NThetaStartupLatency(source, boost::rational<int>{3, 4}, 0));
  // Ogon składowej 1 przy takcie wyjścia dwa razy wolniejszym: ceil(2/2)-1 = 0.
  // Dawna reguła zaokrąglała ogon składowej w górę OSOBNO i dawała 1.
  EXPECT_EQ(0, NThetaStartupLatency(boost::rational<int>{1, 10}, boost::rational<int>{1, 5}, 1));
}

// =====================================================================================
// Rachunek ogona przeplotu: postać zamknięta wobec przeglądu okresu fazowego (#616).
//
// HashStartupLatency() liczy ogon postacią zamkniętą, udowodnioną w Lean
// (exactInterleaveTail_closed). Do 2026-11-10 liczył go przegląd jednego okresu do progu
// p+q = 100 000, a powyżej postać O(1), która zawyżała. Przegląd wołał Hash() w
// rational<int> i przy q*(p+q) > 2^31 ZANIŻAŁ ogon - dlatego przegląd testowy niżej nie
// woła Hash(), tylko liczy wybór składowej z definicji w __int128.
// =====================================================================================

namespace {

// Niezależny przegląd jednego okresu fazowego w __int128. Powtarza WARUNEK DOSTĘPNOŚCI
// z definicji: rekord i niesie rekord j(i) składowej wybranej przez zet = Db/(Da+Db),
// określony w chwili (j+1+W_src)*D_src, a slot i kończy się w (i+1+W)*D_c, gdzie
// D_c = Da*Db/(Da+Db). Wystarcza jeden okres p+q - okresowość wymagań dowodzi
// tailRequirement_period w math_proofs/Profs/InterleaveTailExact.lean.
std::int64_t hashTailByScan(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, int latencyA, int latencyB) {
  using wide        = __int128;
  const wide an     = deltaA.numerator();
  const wide ad     = deltaA.denominator();
  const wide bn     = deltaB.numerator();
  const wide bd     = deltaB.denominator();
  const wide sum    = an * bd + bn * ad;  // (Da+Db) * ad * bd
  const wide zetN   = bn * ad;            // zet = zetN / sum
  const auto ratio  = deltaA / deltaB;
  const wide period = static_cast<wide>(ratio.numerator()) + ratio.denominator();
  wide result       = 0;
  for (wide index = 0; index < period; ++index) {
    const wide before = zetN * index / sum;
    const bool fromB  = before == zetN * (index + 1) / sum;
    const wide pos    = fromB ? index - before : before;
    // D_src / D_c = sum / (ad*bn) dla A i sum / (bd*an) dla B.
    const wide num  = (pos + 1 + (fromB ? latencyB : latencyA)) * sum;
    const wide den  = fromB ? bd * an : ad * bn;
    const wide need = (num + den - 1) / den - 1 - index;
    result          = std::max(result, need);
  }
  return static_cast<std::int64_t>(result);
}

// Zastąpiona 2026-08-07 postać O(1): max(conv(W_A), conv(W_B) + ceil((p+q-1)/p)), do #616
// gałąź powyżej progu przeglądu. Liczona tu wprost, w 64 bitach.
std::int64_t replacedO1Tail(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB, int latencyA, int latencyB) {
  const auto ratio     = deltaA / deltaB;
  const std::int64_t p = ratio.numerator();
  const std::int64_t q = ratio.denominator();
  const auto toSlots = [](std::int64_t w, std::int64_t num, std::int64_t den) { return w <= 0 ? 0 : (w * num + den - 1) / den; };
  const std::int64_t ownPhaseTerm = (q + p - 2) / p + 1;
  // D_A/D_c = (p+q)/q, D_B/D_c = (p+q)/p.
  return std::max(toSlots(latencyA, p + q, q), toSlots(latencyB, p + q, p) + ownPhaseTerm);
}

}  // namespace

// Zgodność z przeglądem na siatce: p, q <= 30 względnie pierwsze, trzy skale interwałów,
// ogony składowych do 5 - siatka modelu K1.3 (#615), który znalazł postać, z trzecią skalą.
TEST(xSOperations, hash_startup_latency_matches_phase_scan_on_grid) {
  // covers: soperations.hash-startup-latency.1~2, soperations.hash-startup-latency.3~1
  int checked = 0;
  for (int p = 1; p <= 30; ++p)
    for (int q = 1; q <= 30; ++q) {
      if (std::gcd(p, q) != 1) continue;
      for (const boost::rational<int> scale : {boost::rational<int>(1), boost::rational<int>(1, 7), boost::rational<int>(5, 2)})
        for (int latencyA = 0; latencyA <= 5; ++latencyA)
          for (int latencyB = 0; latencyB <= 5; ++latencyB) {
            const boost::rational<int> deltaA = scale * p;
            const boost::rational<int> deltaB = scale * q;
            ASSERT_EQ(HashStartupLatency(deltaA, deltaB, latencyA, latencyB), hashTailByScan(deltaA, deltaB, latencyA, latencyB))
                << "p=" << p << " q=" << q << " scale=" << scale << " W_A=" << latencyA << " W_B=" << latencyB;
            ++checked;
          }
    }
  EXPECT_EQ(checked, 555 * 3 * 36);  // 555 par wzglednie pierwszych, 3 skale, 6 x 6 ogonow
}

// Powyżej dawnego progu przeglądu (p+q > 100 000) wynik jest dokładny i nie większy niż
// dawna gałąź zastępcza. Para 2/99997 jest poniżej progu, ale tam dawny przegląd zaniżał
// ogon przez przepełnienie w Hash() - 99998 zamiast 99999 przy W_B = 1.
TEST(xSOperations, hash_startup_latency_is_exact_above_former_scan_limit) {
  // covers: soperations.hash-startup-latency.1~2, soperations.hash-startup-latency.3~1
  for (const auto &[a, b] : {std::pair{1, 100001}, std::pair{100001, 1}, std::pair{99999, 2}, std::pair{2, 99999},
                             std::pair{100003, 7}, std::pair{2, 99997}, std::pair{3, 99997}})
    for (int latencyA : {0, 1, 5})
      for (int latencyB : {0, 3}) {
        const boost::rational<int> deltaA{a, 3};
        const boost::rational<int> deltaB{b, 3};
        const auto tail = HashStartupLatency(deltaA, deltaB, latencyA, latencyB);
        EXPECT_EQ(tail, hashTailByScan(deltaA, deltaB, latencyA, latencyB))
            << a << "/" << b << " W_A=" << latencyA << " W_B=" << latencyB;
        EXPECT_LE(tail, replacedO1Tail(deltaA, deltaB, latencyA, latencyB))
            << a << "/" << b << " W_A=" << latencyA << " W_B=" << latencyB;
      }
  EXPECT_EQ(HashStartupLatency(boost::rational<int>(2), boost::rational<int>(99997), 0, 1), 99999);
}

// Arytmetyka pośrednia musi być 64-bitowa. Świadek wyszukany wyczerpująco 2026-08-07:
// dla Da = Db = 99991/100000 i ogona składowej 30 000 rachunek w rational<int> zwracał
// 17 047 zamiast 60 000 - czyli ZANIŻAŁ ogon o ponad 42 000 slotów.
//
// Zaniżenie jest tą klasą defektu, której cały reżim bezpieczny ma nie dopuszczać:
// oznacza rekord wydany, zanim jego zależności są określone.
TEST(xSOperations, hash_startup_latency_uses_64bit_intermediate_arithmetic) {
  // covers: soperations.hash-startup-latency.1~2, soperations.hash-startup-latency.2~2
  const boost::rational<int> deltaA{99991, 100000};
  const boost::rational<int> deltaB{99991, 100000};
  const int latencyA  = 30000;
  const auto expected = hashTailByScan(deltaA, deltaB, latencyA, 0);
  ASSERT_GT(expected, 20000) << "przypadek przestal byc swiadkiem przepelnienia";
  EXPECT_EQ(HashStartupLatency(deltaA, deltaB, latencyA, 0), expected);
}

// Kraniec zakresu: p, q i ogony równe INT_MAX. Wartości wyliczone ręcznie, L = 2^31:
//   Da/Db = INT_MAX/1: kres A = INT_MAX * 2^31 = 2^62 - 2^31, kres B = 2^31 + 1;
//   Da/Db = 1/INT_MAX: kres A = 2^31, kres B = 2^31 * 2^31 - 1 = 2^62 - 1.
// Przepełnienie członu (1+W)*L w int albo w int64 dałoby tu wartość ujemną lub obciętą.
TEST(xSOperations, hash_startup_latency_handles_the_int_range_edge) {
  // covers: soperations.hash-startup-latency.2~2
  constexpr int big            = std::numeric_limits<int>::max();
  constexpr std::int64_t two31 = std::int64_t{1} << 31;
  EXPECT_EQ(HashStartupLatency(boost::rational<int>(big), boost::rational<int>(1), big, big), (two31 << 31) - two31);
  EXPECT_EQ(HashStartupLatency(boost::rational<int>(1), boost::rational<int>(big), big, big), (two31 << 31) - 1);
  EXPECT_GT(HashStartupLatency(boost::rational<int>(1), boost::rational<int>(big), 0, 1), std::int64_t{big});
}

// Świadek, w którym postać zamknięta i zastąpiona postać O(1) DAJĄ RÓŻNE liczby: Da=1,
// Db=2/5, W_B=1 - przegląd okresu daje 3, postać O(1) daje 4. Test wymaga wartości
// dokładnej i osobno sprawdza, że postać O(1) jest tu naprawdę wyższa; bez drugiej połowy
// pierwsza sprawdzałaby zgodność dwóch identycznych liczb.
TEST(xSOperations, hash_startup_latency_differs_from_the_replaced_o1_form) {
  // covers: soperations.hash-startup-latency.1~2
  const boost::rational<int> deltaA{1, 1};
  const boost::rational<int> deltaB{2, 5};
  const int latencyB = 1;
  const auto exact   = hashTailByScan(deltaA, deltaB, 0, latencyB);
  EXPECT_EQ(exact, 3);
  EXPECT_EQ(HashStartupLatency(deltaA, deltaB, 0, latencyB), exact);
  EXPECT_GT(replacedO1Tail(deltaA, deltaB, 0, latencyB), exact) << "przypadek przestal odrozniac obie postacie";
}

TEST(xSOperations, hash_former_scan_limit_boundary_is_exact) {
  // covers: soperations.hash-startup-latency.1~2
  // Niezalezna tablica zdarzen: W_A=0, W_B=1. Dla kazdej pary minimalny ogon wynosi 3.
  // Ostatni okres przekracza dawny prog przegladu; dawna galaz zastepcza dawala tam 4.
  for (const auto &[a, b, expected] : {std::tuple{99997, 2, 3}, std::tuple{99997, 3, 3}, std::tuple{99999, 2, 3}}) {
    SCOPED_TRACE(a + b);
    EXPECT_EQ(HashStartupLatency(boost::rational<int>(a), boost::rational<int>(b), 0, 1), expected);
  }
}
