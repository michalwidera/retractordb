#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

#include "SOperations.hpp"

// ctest -R '^ut-soperations' -V

TEST(xSOperations, hash_operations) {
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
// z = deltaB/(deltaA+deltaB) = 2/3 dla pary (1/16, 1/8) — wzorzec B,A,A,...
// Stary kod używał delta_c zamiast z w retPos, co było maskowane w teście
// wyżej przez deltaA == 1 (wtedy delta_c == z liczbowo).
TEST(xSOperations, hash_operations_nonunit_delta) {
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

// Równe delty (perfect shuffle): z = 1/2 — wzorzec B,A,B,A,...
TEST(xSOperations, hash_operations_equal_delta) {
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
// samym indeksie — Θ (Div) trafia w gałąź A, ~Θ (Mod) w gałąź B.
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
// Silnik odwzorowywał wcześniej b_{⌊(n+1)·Δa/Δb⌋} (bieżący payload składowej) —
// stąd wartości oczekiwane wypisane wprost, a nie wyprowadzone drugą kopią wzoru.
TEST(xSOperations, add_pairs_slower_component_by_floor_index) {
  const boost::rational<int> fast{1, 2};

  // Składowa o interwale wyjścia jest indeksowana wprost n.
  for (int n = 0; n < 20; ++n)
    EXPECT_EQ(n, Add(fast, fast, n));

  // Iloraz całkowity: Δsrc = 2·Δout — b_0, b_0, b_1, b_1, ...
  const std::vector<int> ratioTwo{0, 0, 1, 1, 2, 2, 3, 3};
  for (int n = 0; n < static_cast<int>(ratioTwo.size()); ++n)
    EXPECT_EQ(ratioTwo[n], Add(fast, boost::rational<int>{1}, n));

  // Iloraz niecałkowity: Δsrc = 3/4, Δout = 1/2 — ⌊2n/3⌋.
  const std::vector<int> ratioThreeQuarters{0, 0, 1, 2, 2, 3, 4, 4};
  for (int n = 0; n < static_cast<int>(ratioThreeQuarters.size()); ++n)
    EXPECT_EQ(ratioThreeQuarters[n], Add(fast, boost::rational<int>{3, 4}, n));
}

// Okno stemplowane KOŃCEM przedziału: rekord n obejmuje pozycje n*step-(|len|-1) ... n*step.
// Ogon zabezpiecza wyłącznie NAJNOWSZE pole (n*step), bo dolny koniec zakresu jest domeną
// origin — stąd brak zależności od długości okna i brak członu fazowego. Wartości poniżej
// wyprowadzone z warunku dostępności: rekord floor(n*step/F) jest określony w chwili
// (floor(n*step/F)+1+W_src)*Delta_src, a slot n kończy się w (n+1+W)*Delta_out.
TEST(xSOperations, agse_startup_latency_covers_newest_field) {
  // F=1, step=1: rekord n potrzebuje wyłącznie źródła n, dostępnego w jego własnym slocie.
  // Czekanie na komplet okna przeszło do origin, więc ogon jest zerowy niezależnie od długości.
  EXPECT_EQ(0, AgseStartupLatency(1, 1, 0));
  // F=5, step=1: slot wyjścia to 1/5 rekordu źródła, więc na rekord 0 źródła czeka się
  // 5 slotów; W = ceil(5/1)-1 = 4.
  EXPECT_EQ(4, AgseStartupLatency(5, 1, 0));
  // Producent obliczany o własnym ogonie 2: pierwszy rekord źródła dopiero w chwili 3*D_src.
  EXPECT_EQ(1, AgseStartupLatency(2, 3, 2));
  // Krok większy od szerokości źródła — slot wyjścia dłuższy niż rekord źródła, ogon zerowy.
  EXPECT_EQ(0, AgseStartupLatency(2, 3, 0));
  EXPECT_EQ(1, AgseStartupLatency(6, 4, 0));
}

// Origin jest jedynym miejscem, w którym rozpiętość okna wpływa na start strumienia.
// O = ceil((O_src*F + |len| - 1) / step).
TEST(xSOperations, agse_logical_origin_skips_incomplete_windows) {
  // F=1, step=1, len=4: pierwsze pełne okno kończy się na pozycji 3.
  EXPECT_EQ(3, AgseLogicalOrigin(1, 1, 4, 0));
  // Znak długości jest wyłącznie konwencją kolejności pól — origin jest ten sam.
  EXPECT_EQ(3, AgseLogicalOrigin(1, 1, -4, 0));
  // Krok 2: okno kończące się na pozycji 2*n musi sięgnąć 2*n-1 >= 0, czyli n >= 1.
  EXPECT_EQ(1, AgseLogicalOrigin(1, 2, 2, 0));
  // Okno o długości 1 nigdy nie sięga wstecz — strumień zaczyna się od rekordu 0.
  EXPECT_EQ(0, AgseLogicalOrigin(1, 1, 1, 0));
  // Origin źródła przesuwa jego pozycje spłaszczone o O_src*F i propaguje się w górę:
  // ceil((2*3 + 4 - 1) / 2) = ceil(9/2) = 5.
  EXPECT_EQ(5, AgseLogicalOrigin(3, 2, 4, 2));
}

// Ogon sumy strumieni musi objąć dostępność rekordu KAŻDEJ składowej pod
// indeksem floor(n*D_out/D_src), a nie tylko przeliczyć ogon składowej.
TEST(xSOperations, add_startup_latency_covers_slower_component) {
  const boost::rational<int> fast{1, 2};

  // Składowa o interwale wyjścia i zerowym ogonie nie wnosi opóźnienia.
  EXPECT_EQ(0, AddStartupLatency(fast, fast, 0));
  // Składowa dwa razy wolniejsza: jej rekord 0 powstaje w chwili 1*D_src,
  // czyli po dwóch slotach wyjścia — ogon 1. Poprzednia postać dawała 0.
  EXPECT_EQ(1, AddStartupLatency(boost::rational<int>{1}, fast, 0));
  // Ta sama składowa z własnym ogonem 2: rekord 0 dopiero w chwili 3*D_src.
  EXPECT_EQ(5, AddStartupLatency(boost::rational<int>{1}, fast, 2));
  // Składowa szybsza od wyjścia nie może wystąpić (D_out = min), ale wzór
  // pozostaje zdefiniowany i nie schodzi poniżej zera.
  EXPECT_EQ(0, AddStartupLatency(boost::rational<int>{1, 4}, fast, 0));
}

// Ogon `-`, `Θ` i `~Θ` liczony z kresu fazy odczytu. Wartości oczekiwane pochodzą
// z modelu zdarzeniowego (investigation_K24H10, weryfikacja per węzeł), nie z tej samej
// postaci zamkniętej — inaczej test potwierdzałby wyłącznie sam siebie.
TEST(xSOperations, subtract_startup_latency_covers_fractional_phase) {
  const boost::rational<int> source{1, 2};

  // Składowa dwa razy szybsza od wyniku: pierwszy potrzebny rekord istnieje w slocie 0.
  // Do 2026-08-18 gałąź deklaracyjna dokładała tu slot, choć deklaracja ma ogon zerowy.
  EXPECT_EQ(0, SubtractStartupLatency(source, boost::rational<int>{1}, 0));
  // Faza ułamkowa: r = 3/2, kres fazy 1/2, więc ceil((1/2+1)/(3/2))-1 = 0.
  // Dawna postać dawała 1 — to jest ten slot zawyżenia, który mierzyła kampania K24.
  EXPECT_EQ(0, SubtractStartupLatency(source, boost::rational<int>{3, 4}, 0));
  // Ogon składowej wchodzi liniowo: ceil((1/2+2)/(3/2))-1 = 1.
  EXPECT_EQ(1, SubtractStartupLatency(source, boost::rational<int>{3, 4}, 1));
  // Ten sam takt na wejściu i wyjściu przenosi ogon składowej bez zmiany.
  EXPECT_EQ(3, SubtractStartupLatency(source, source, 3));
}

TEST(xSOperations, dehash_startup_latency_follows_the_phase_bound) {
  const boost::rational<int> source{1, 2};

  // Iloraz całkowity: kres fazy Θ wynosi a, po podzieleniu przez r daje ogon ZERO.
  // Dawne bezwarunkowe ++result dawało tu 1 — 40,3% węzłów `Θ` korpusu K24.
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
// Rachunek ogona przeplotu: gałąź awaryjna i arytmetyka 64-bitowa (luka L3).
//
// Obie były w ctest NIEOSIĄGALNE. Postać O(1) powyżej kHashPhaseScanLimit nie wykonuje
// się nigdy w korpusie kampanijnym (maksimum okresu 24 557 wobec progu 100 000), a
// rachunek w int64 wprowadzono świadomie — komentarz przy HashStartupLatency() mówi, że
// rational<int> mnoży przed skróceniem i wychodzi poza zakres int. Powrót do rational<int>
// nie zapalał dotąd w ctest ani jednej lampki.
// =====================================================================================

namespace {

// Niezależny przegląd okresu fazowego w 64 bitach. Powtarza WARUNEK DOSTĘPNOŚCI
// z definicji, nie implementację: wybór składowej bierze z Hash() (tu chodzi
// o arytmetykę, nie o regułę wyboru — tę strzeże ut_h10aGate).
std::int64_t hashTailByScan(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB,
                            const boost::rational<int> &deltaOut, int latencyA, int latencyB, std::int64_t period) {
  std::int64_t result = 0;
  for (std::int64_t index = 0; index < period; ++index) {
    int position         = 0;
    const bool fromB     = Hash(deltaA, deltaB, static_cast<int>(index), position);
    const auto &deltaSrc = fromB ? deltaB : deltaA;
    const int latencySrc = fromB ? latencyB : latencyA;
    const auto numerator = static_cast<std::int64_t>(position + 1 + latencySrc) * deltaSrc.numerator() * deltaOut.denominator();
    const auto denominator = static_cast<std::int64_t>(deltaSrc.denominator()) * deltaOut.numerator();
    result                 = std::max(result, (numerator + denominator - 1) / denominator - 1 - index);
  }
  return result;
}

boost::rational<int> interleavedInterval(const boost::rational<int> &deltaA, const boost::rational<int> &deltaB) {
  // Delta_c = Da*Db/(Da+Db), liczone w 64 bitach — tak jak robi to kompilator przez
  // widen()/narrowInterval(). Bez tego świadkiem byłby błąd aparatury testu.
  const auto numerator   = static_cast<std::int64_t>(deltaA.numerator()) * deltaB.numerator();
  const auto denominator = static_cast<std::int64_t>(deltaA.numerator()) * deltaB.denominator() +
                           static_cast<std::int64_t>(deltaB.numerator()) * deltaA.denominator();
  const auto divisor = std::gcd(numerator, denominator);
  return {static_cast<int>(numerator / divisor), static_cast<int>(denominator / divisor)};
}

}  // namespace

// Arytmetyka pośrednia musi być 64-bitowa. Świadek wyszukany wyczerpująco 2026-08-07:
// dla Da = Db = 99991/100000 i ogona składowej 30 000 iloczyn (pos+1+W)*num_src*den_out
// wynosi ~6e14, a już pierwszy człon (1+W)*99991 przekracza zakres int mimo skracania,
// które boost::rational wykonuje przed mnożeniem. Postać rational<int> zwraca w tym
// przypadku 17 047 zamiast 60 000 — czyli ZANIŻA ogon o ponad 42 000 slotów.
//
// Zaniżenie jest tą klasą defektu, której cały reżim bezpieczny ma nie dopuszczać:
// oznacza rekord wydany, zanim jego zależności są określone.
TEST(xSOperations, hash_startup_latency_uses_64bit_intermediate_arithmetic) {
  const boost::rational<int> deltaA{99991, 100000};
  const boost::rational<int> deltaB{99991, 100000};
  const auto deltaOut = interleavedInterval(deltaA, deltaB);
  ASSERT_EQ(deltaOut, boost::rational<int>(99991, 200000));

  const auto ratio          = deltaA / deltaB;
  const std::int64_t period = static_cast<std::int64_t>(ratio.numerator()) + ratio.denominator();
  ASSERT_LE(period, kHashPhaseScanLimit) << "swiadek ma badac galaz DOKLADNA, nie awaryjna";

  const int latencyA  = 30000;
  const auto expected = hashTailByScan(deltaA, deltaB, deltaOut, latencyA, 0, period);
  // Świadek jest świadkiem tylko wtedy, gdy iloczyn naprawdę wychodzi poza int.
  ASSERT_GT(expected, 20000) << "przypadek przestal byc swiadkiem przepelnienia";
  EXPECT_EQ(HashStartupLatency(deltaA, deltaB, deltaOut, latencyA, 0), static_cast<int>(expected));
}

// Gałąź awaryjna powyżej progu przeglądu. Reżim deklarowany w komentarzu przy
// kHashPhaseScanLimit: postać O(1) ZAWYŻA, więc jest bezpieczna. Test wymaga tego wprost
// — wartość zwrócona nie może spaść poniżej granicy zdarzeniowej wyliczonej przeglądem
// całego okresu. Bez tego testu gałąź nie wykonuje się w ctest ani razu.
TEST(xSOperations, hash_startup_latency_above_scan_limit_stays_safe) {
  const boost::rational<int> deltaA{1, 1};
  const boost::rational<int> deltaB{1, 100001};
  const auto deltaOut = interleavedInterval(deltaA, deltaB);
  ASSERT_EQ(deltaOut, boost::rational<int>(1, 100002));

  const auto ratio          = deltaA / deltaB;
  const std::int64_t period = static_cast<std::int64_t>(ratio.numerator()) + ratio.denominator();
  ASSERT_GT(period, kHashPhaseScanLimit) << "przypadek nie wchodzi w galaz awaryjna";

  for (int latencyA : {0, 1, 5})
    for (int latencyB : {0, 3}) {
      const auto exact    = hashTailByScan(deltaA, deltaB, deltaOut, latencyA, latencyB, period);
      const int fromLimit = HashStartupLatency(deltaA, deltaB, deltaOut, latencyA, latencyB);
      EXPECT_GE(fromLimit, static_cast<int>(exact)) << "W_A=" << latencyA << " W_B=" << latencyB;
    }
}

// Kontrola progu. Sam próg nie jest strzeżony ani przez test dokładności, ani przez test
// bezpieczeństwa: gałąź awaryjna ZAWYŻA, więc obniżenie kHashPhaseScanLimit do zera nie
// złamałoby żadnego z nich — po cichu cofnęłoby tylko klasę `#` z reżimu dokładnego do
// zawyżającego, czyli o krok 3c z 2026-08-07.
//
// Świadkiem jest przypadek, w którym obie postacie DAJĄ RÓŻNE liczby: Da=1, Db=2/5,
// W_B=1 — przegląd okresu daje 3, zastąpiona postać O(1) daje 4. Test wymaga od silnika
// wartości dokładnej i osobno sprawdza, że postać O(1) jest tu naprawdę wyższa; bez
// drugiej połowy pierwsza sprawdzałaby zgodność dwóch identycznych liczb.
TEST(xSOperations, hash_scan_limit_keeps_the_exact_branch_in_use) {
  const boost::rational<int> deltaA{1, 1};
  const boost::rational<int> deltaB{2, 5};
  const auto deltaOut = interleavedInterval(deltaA, deltaB);
  ASSERT_EQ(deltaOut, boost::rational<int>(2, 7));

  const auto ratio          = deltaA / deltaB;
  const std::int64_t period = static_cast<std::int64_t>(ratio.numerator()) + ratio.denominator();
  ASSERT_LE(period, kHashPhaseScanLimit);

  const int latencyB = 1;
  const auto exact   = hashTailByScan(deltaA, deltaB, deltaOut, 0, latencyB, period);
  EXPECT_EQ(HashStartupLatency(deltaA, deltaB, deltaOut, 0, latencyB), static_cast<int>(exact));

  // Zastąpiona postać O(1): max(conv(W_A), conv(W_B) + ceil((p+q-1)/p)). Liczona tu wprost,
  // żeby świadek nie opierał się na komentarzu, tylko na liczbie.
  const auto swapped     = deltaB / deltaA;
  const int ownPhaseTerm = (swapped.denominator() + swapped.numerator() - 2) / swapped.denominator() + 1;
  const auto toSlots     = [](int latency, const boost::rational<int> &source, const boost::rational<int> &target) {
    return latency <= 0 ? 0 : ceilR(boost::rational<int>(latency) * source / target);
  };
  const int replaced = std::max(toSlots(0, deltaA, deltaOut), toSlots(latencyB, deltaB, deltaOut) + ownPhaseTerm);
  EXPECT_GT(replaced, static_cast<int>(exact)) << "przypadek przestal odrozniac obie postacie";
}
