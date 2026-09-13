#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "proofOracle.hpp"
#include "SOperations.hpp"

// ctest -R '^ut_proofOracle' -V
//
// Zgodnosc SOperations.hpp z dowodami Lean w math_proofs. Nazwa kazdego testu jest nazwa
// twierdzenia (albo definicji z sufiksem _table), ktore sprawdza; powiazanie twierdzen z testami
// trzyma test/proof_manifest.tsv, a jego kompletnosc i swiezosc tablic sprawdza test proof_drift.
//
// Dowody licza na liczbach naturalnych a, b, silnik na interwalach wymiernych. Tablice sa wiec
// sprawdzane takze po przeskalowaniu obu interwalow przez 1/d: wzory zaleza tylko od ilorazu.

using boost::rational;

namespace {

constexpr int kScales[] = {1, 7, 16};

// Dostepnosc rekordu wyjscia n przy ogonie W, liczona wprost z warunku zdarzeniowego
// interleaveReady (InterleaveTailExact.lean), w liczbach calkowitych: po pomnozeniu obu stron
// przez (a+b)/tick interwal A to a(a+b), interwal B to b(a+b), a interwal wyjscia to ab.
bool interleaveReady(std::int64_t a, std::int64_t b, std::int64_t wa, std::int64_t wb, std::int64_t w, std::int64_t n) {
  const std::int64_t progress = n * b / (a + b);
  const bool fromA            = progress != (n + 1) * b / (a + b);
  const std::int64_t index    = fromA ? progress : n - progress;
  const std::int64_t lhs      = (index + 1 + (fromA ? wa : wb)) * (fromA ? a : b) * (a + b);
  return lhs <= (n + 1 + w) * a * b;
}

int hashTail(int a, int b, int wa, int wb, int d = 1) {
  const rational<int> deltaA(a, d);
  const rational<int> deltaB(b, d);
  return HashStartupLatency(deltaA, deltaB, deltaA * deltaB / (deltaA + deltaB), wa, wb);
}

}  // namespace

// Definicje przeplotu i rozplotu (interleaveAt, deinterleaveLeft/Right) wobec Hash, Div i Mod.
TEST(ProofOracle, interleaveAt_table) {
  for (int d : kScales)
    for (const auto &row : kInterleaveRows) {
      int pos         = -1;
      const bool from = Hash(rational<int>(row.a, d), rational<int>(row.b, d), row.n, pos);
      ASSERT_EQ(from, row.fromB) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
      ASSERT_EQ(pos, row.pos) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
    }
}

TEST(ProofOracle, deinterleave_table) {
  for (int d : kScales)
    for (const auto &row : kInterleaveRows) {
      const rational<int> deltaA(row.a, d);
      const rational<int> deltaB(row.b, d);
      ASSERT_EQ(Div(deltaA, deltaB, row.n), row.left) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
      ASSERT_EQ(Mod(deltaA, deltaB, row.n), row.right) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
    }
}

// Definicja roznicy (rateDiff) wobec Subtract; interwaly w tablicy sa w cwiartkach.
TEST(ProofOracle, rateDiff_table) {
  for (int d : kScales)
    for (const auto &row : kSubtractRows)
      ASSERT_EQ(Subtract(rational<int>(row.period, 4 * d), rational<int>(row.target, 4 * d), row.n), row.index)
          << "period=" << row.period << "/4 target=" << row.target << "/4 n=" << row.n << " d=" << d;
}

// Definicja dokladnego ogona przeplotu (exactInterleaveTail) wobec HashStartupLatency.
TEST(ProofOracle, exactInterleaveTail_table) {
  for (int d : kScales)
    for (const auto &row : kTailRows)
      ASSERT_EQ(hashTail(row.a, row.b, row.wa, row.wb, d), row.tail)
          << "a=" << row.a << " b=" << row.b << " WA=" << row.wa << " WB=" << row.wb << " d=" << d;
}

// Kazdy rekord A i kazdy rekord B pojawia sie w przeplocie dokladnie raz, w kolejnosci.
TEST(ProofOracle, interleave_sequential_covering) {
  for (int a = 1; a <= 40; ++a)
    for (int b = 1; b <= 40; ++b) {
      int nextA = 0;
      int nextB = 0;
      for (int n = 0; n < 4 * (a + b); ++n) {
        int pos         = -1;
        const bool from = Hash(rational<int>(a, 3), rational<int>(b, 3), n, pos);
        ASSERT_EQ(pos, from ? nextB++ : nextA++) << "a=" << a << " b=" << b << " n=" << n;
      }
      ASSERT_EQ(nextA, 4 * b) << "a=" << a << " b=" << b;
      ASSERT_EQ(nextB, 4 * a) << "a=" << a << " b=" << b;
    }
}

// Selektory rozplotu razem trafiaja w kazdy slot przeplotu dokladnie raz.
TEST(ProofOracle, selectors_bijective) {
  for (int a = 1; a <= 40; ++a)
    for (int b = 1; b <= 40; ++b) {
      const rational<int> deltaA(a, 3);
      const rational<int> deltaB(b, 3);
      const int slots = 4 * (a + b);
      std::vector<int> hits(slots, 0);
      for (int n = 0; Div(deltaA, deltaB, n) < slots; ++n)
        ++hits[Div(deltaA, deltaB, n)];
      for (int n = 0; Mod(deltaA, deltaB, n) < slots; ++n)
        ++hits[Mod(deltaA, deltaB, n)];
      for (int slot = 0; slot < slots; ++slot)
        ASSERT_EQ(hits[slot], 1) << "a=" << a << " b=" << b << " slot=" << slot;
    }
}

// Rozplot odwraca przeplot: slot wskazany przez Div niesie A[n], a slot wskazany przez Mod niesie B[n].
TEST(ProofOracle, deinterleave_inverts_interleave) {
  for (int a = 1; a <= 40; ++a)
    for (int b = 1; b <= 40; ++b) {
      const rational<int> deltaA(a, 3);
      const rational<int> deltaB(b, 3);
      for (int n = 0; n < 3 * (a + b); ++n) {
        int pos = -1;
        ASSERT_FALSE(Hash(deltaA, deltaB, Div(deltaA, deltaB, n), pos)) << "a=" << a << " b=" << b << " n=" << n;
        ASSERT_EQ(pos, n) << "a=" << a << " b=" << b << " n=" << n;
        ASSERT_TRUE(Hash(deltaA, deltaB, Mod(deltaA, deltaB, n), pos)) << "a=" << a << " b=" << b << " n=" << n;
        ASSERT_EQ(pos, n) << "a=" << a << " b=" << b << " n=" << n;
      }
    }
}

// Poczatek przeplotu 3#2 z dowodu: B0 B1 A0 B2 A1 B3 B4 A2 B5 A3.
TEST(ProofOracle, tau_prefix) {
  const std::vector<std::pair<bool, int>> expected{{true, 0}, {true, 1}, {false, 0}, {true, 2}, {false, 1},
                                                   {true, 3}, {true, 4}, {false, 2}, {true, 5}, {false, 3}};
  for (int n = 0; n < static_cast<int>(expected.size()); ++n) {
    int pos = -1;
    EXPECT_EQ(Hash(rational<int>(3), rational<int>(2), n, pos), expected[n].first) << "n=" << n;
    EXPECT_EQ(pos, expected[n].second) << "n=" << n;
  }
}

// Zdarzenia co 3 s (A) i co 2 s (B): slot 6 niesie B4 (chwila 10), a pozniejszy slot 7 niesie
// wczesniejsze zdarzenie A2 (chwila 9).
TEST(ProofOracle, event_order_counterexample) {
  int pos = -1;
  ASSERT_TRUE(Hash(rational<int>(3), rational<int>(2), 6, pos));
  const int eventAt6 = (pos + 1) * 2;
  ASSERT_FALSE(Hash(rational<int>(3), rational<int>(2), 7, pos));
  const int eventAt7 = (pos + 1) * 3;
  EXPECT_EQ(eventAt6, 10);
  EXPECT_EQ(eventAt7, 9);
  EXPECT_LT(eventAt7, eventAt6);
}

// Ogon jest dokladna granica dostepnosci: przy W = ogon warunek zachodzi dla kazdego rekordu
// od dowolnego progu N0, a przy W = ogon-1 pada na ktoryms z nich. Z okresowosci wymagan
// (tailRequirement_period) wystarczy jeden okres a+b od progu.
TEST(ProofOracle, exactInterleaveTail_exact) {
  for (int a = 1; a <= 40; ++a)
    for (int b = 1; b <= 40; ++b)
      for (int wa = 0; wa <= 3; ++wa)
        for (int wb = 0; wb <= 3; ++wb) {
          const int tail = hashTail(a, b, wa, wb, 5);
          for (int origin : {0, 7}) {
            bool tailReady    = true;
            bool shorterReady = tail > 0;
            for (int n = origin; n < origin + a + b; ++n) {
              tailReady    = tailReady && interleaveReady(a, b, wa, wb, tail, n);
              shorterReady = shorterReady && interleaveReady(a, b, wa, wb, tail - 1, n);
            }
            ASSERT_TRUE(tailReady) << "a=" << a << " b=" << b << " WA=" << wa << " WB=" << wb << " N0=" << origin;
            ASSERT_FALSE(shorterReady) << "a=" << a << " b=" << b << " WA=" << wa << " WB=" << wb << " N0=" << origin;
          }
        }
}

// Przesuniecie dopasowane do tempa (i*a = k*b): slot n przeplotu przesunietych skladowych czyta
// ten sam rekord co slot n+i+k przeplotu nieprzesunietego.
TEST(ProofOracle, shift_matching_values) {
  for (int a = 1; a <= 30; ++a)
    for (int b = 1; b <= 30; ++b)
      for (int m = 1; m <= 3; ++m) {
        const rational<int> deltaA(a, 3);
        const rational<int> deltaB(b, 3);
        const int g = std::gcd(a, b);
        const int i = m * b / g;
        const int k = m * a / g;
        for (int n = 0; n < 3 * (a + b); ++n) {
          int shiftedPos    = -1;
          int plainPos      = -1;
          const bool fromB  = Hash(deltaA, deltaB, n, shiftedPos);
          const bool plainB = Hash(deltaA, deltaB, n + i + k, plainPos);
          ASSERT_EQ(fromB, plainB) << "a=" << a << " b=" << b << " i=" << i << " k=" << k << " n=" << n;
          ASSERT_EQ(shiftedPos + (fromB ? k : i), plainPos) << "a=" << a << " b=" << b << " i=" << i << " k=" << k << " n=" << n;
        }
      }
}

// Przesuniecie dopasowane do tempa (i*a = k*b) wyciagniete przed przeplot nie zmniejsza ogona
// ponizej przesuniecia zastosowanego do skladowych.
TEST(ProofOracle, exact_tail_rewrite) {
  for (int a = 1; a <= 20; ++a)
    for (int b = 1; b <= 20; ++b)
      for (int m = 1; m <= 3; ++m) {
        const int g = std::gcd(a, b);
        const int i = m * b / g;
        const int k = m * a / g;
        for (int wa = 0; wa <= 5; ++wa)
          for (int wb = 0; wb <= 5; ++wb)
            ASSERT_LE(std::max(0, hashTail(a, b, wa, wb) - (i + k)), hashTail(a, b, std::max(0, wa - i), std::max(0, wb - k)))
                << "a=" << a << " b=" << b << " i=" << i << " k=" << k << " WA=" << wa << " WB=" << wb;
      }
}

// Nierownosc w exact_tail_rewrite bywa ostra: (A>2)#(B>1) ma ogon 2, a (A#B)>3 ogon 0.
TEST(ProofOracle, tail_can_be_strict) {
  EXPECT_EQ(hashTail(1, 2, 0, 0), 2);
  EXPECT_EQ(std::max(0, hashTail(1, 2, 0, 0) - (2 + 1)), 0);
}
