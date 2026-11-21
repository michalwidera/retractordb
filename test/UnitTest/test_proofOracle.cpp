#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "proofOracle.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"
#include "SOperations.hpp"

// ctest -R '^ut_proofOracle' -V
//
// Zgodnosc SOperations.hpp z dowodami Lean w math_proofs. Twierdzenia o poczatku logicznym, ogonie
// i interwale sa sprawdzane na planie skompilowanym z RQL, bo te wielkosci liczy dopiero compiler
// (computeLogicalOrigin, computeStartupLatency). Nazwa kazdego testu jest nazwa
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
  return static_cast<int>(HashStartupLatency(deltaA, deltaB, wa, wb));
}

void compilePlan(qTree &instance, const std::string &rql) {
  const auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, rql);
  ASSERT_EQ(parseResult, "OK") << rql;
  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK") << rql;
}

// Pary interwalow a/100 i b/100 w planach przeplotu.
constexpr std::pair<int, int> kPeriodPairs[] = {{1, 1}, {1, 2}, {2, 1}, {3, 5}, {3, 2}, {7, 11}};

// Plan z deklaracjami pa<a>_<b> (interwal a/100) i pb<a>_<b> (interwal b/100) dla kazdej pary
// oraz strumieniami przesunietymi o zadana liczbe rekordow. Przesuniecia sa nazwanymi strumieniami
// SELECT, a nie substratami, wiec factorMatchedHashTimeMoves nie przepisuje przeplotu nad nimi.
class PlanText {
 public:
  PlanText() {
    for (const auto [a, b] : kPeriodPairs)
      text += std::format(
          "DECLARE value INTEGER STREAM pa{0}_{1}, {0}/100 FILE 'a.txt'\n"
          "DECLARE value INTEGER STREAM pb{0}_{1}, {1}/100 FILE 'b.txt'\n",
          a, b);
  }

  std::string shifted(const std::string &source, int by) {
    if (by == 0) return source;
    const std::string name = std::format("{}_sh{}", source, by);
    if (emitted.insert(name).second) text += std::format("SELECT * STREAM {} FROM {}>{}\n", name, source, by);
    return name;
  }

  std::string text;

 private:
  std::set<std::string> emitted;
};

// indexA, indexB (InterleaveCovering.lean) i originReady (CausalShift.lean).
std::int64_t indexA(std::int64_t a, std::int64_t b, std::int64_t n) { return n * b / (a + b); }
std::int64_t indexB(std::int64_t a, std::int64_t b, std::int64_t n) { return n - indexA(a, b, n); }
bool originReady(std::int64_t a, std::int64_t b, std::int64_t oa, std::int64_t ob, std::int64_t n) {
  return oa <= indexA(a, b, n) && ob <= indexB(a, b, n);
}

// Wyemitowany rekord: wartosc pierwszego pola (-1 dla null) i chwila slotu, w ktorym powstal.
struct Emission {
  int value;
  rational<int> time;
};

// strumien -> indeks logiczny -> rekord.
using Trace = std::map<std::string, std::map<int, Emission>>;

// Wykonanie planu slot po slocie, tak jak petla executorsm, do chwili horizon. Fizyczny rekord k
// strumienia obliczanego nosi indeks logiczny logicalOrigin + k (dataModel::processRows).
Trace runPlan(qTree &instance, const rational<int> &horizon) {
  auto procOwner  = rdbtest::ok(dataModel::create(instance));
  dataModel &proc = *procOwner;
  pProc           = &proc;  // dumpManager czyta model przez ten wskaznik
  CRationalStreamMath::TimeLine timeline(rdbtest::ok(instance.getAvailableTimeIntervals()));
  rdbtest::ok(proc.processZeroStep());

  Trace trace;
  for (auto now = timeline.getNextTimeSlot(); now <= horizon; now = timeline.getNextTimeSlot()) {
    // Zbior nazw sluzy filtrowaniu sladu ponizej; processRows bierze maske pozycyjna.
    std::set<std::string> due;
    std::vector<char> dueMask(instance.size(), 0);
    std::map<std::string, size_t> before;
    std::size_t position = 0;
    for (const auto &q : instance) {
      if (timeline.isThisDeltaAwaitCurrentTimeSlot(q.rInterval)) {
        due.insert(q.id);
        dueMask[position] = 1;
        before[q.id]      = proc.qSet.at(q.id)->outputPayload->getRecordsCount();
      }
      ++position;
    }
    rdbtest::ok(proc.processRows(dueMask, now));
    for (const auto &q : instance) {
      if (q.isDeclaration() || !due.contains(q.id)) continue;
      const auto count = proc.qSet.at(q.id)->outputPayload->getRecordsCount();
      if (count == before[q.id]) continue;
      const auto row  = rdbtest::ok(proc.getRow(q.id, 0));
      const int value = std::holds_alternative<int>(row.at(0)) ? std::get<int>(row.at(0)) : -1;
      trace[q.id][q.logicalOrigin + static_cast<int>(count) - 1] = {value, now};
    }
  }
  pProc = nullptr;
  return trace;
}

// Katalog na zrodla i artefakty jednego wykonania; usuwany razem z zawartoscia.
constexpr const char *kExecDir = "proofOracle_exec";

// Wykonanie planu z opisu CausalShift.lean dla interwalow a/10 i b/10 oraz przesuniec
// dopasowanych do tempa (i*a = k*b): srcA ma wartosci 1000+n, srcB 2000+n, copyA to srcA
// przepisane, lhs = (srcA>i)#(srcB>k), rhs = (srcA#srcB)>(i+k).
struct ShiftMatchingRun {
  int a, b, i, k;
  qTree instance;
  Trace trace;

  ShiftMatchingRun(int a_, int b_, int m) : a(a_), b(b_), i(m * b_ / std::gcd(a_, b_)), k(m * a_ / std::gcd(a_, b_)) {
    std::filesystem::remove_all(kExecDir);
    std::filesystem::create_directories(kExecDir);
    for (const auto [name, base] : {std::pair{"a.txt", 1000}, std::pair{"b.txt", 2000}}) {
      std::ofstream data(std::filesystem::path(kExecDir) / name);
      for (int n = 0; n < 900; ++n)
        data << base + n << '\n';
    }
    compilePlan(instance, std::format("STORAGE '{0}/'\n"
                                      "DECLARE v INTEGER STREAM srcA, {1}/10 FILE '{0}/a.txt'\n"
                                      "DECLARE v INTEGER STREAM srcB, {2}/10 FILE '{0}/b.txt'\n"
                                      "SELECT * STREAM copyA FROM srcA\n"
                                      "SELECT * STREAM shiftA FROM srcA>{3}\n"
                                      "SELECT * STREAM shiftB FROM srcB>{4}\n"
                                      "SELECT * STREAM lhs FROM shiftA#shiftB\n"
                                      "SELECT * STREAM plain FROM srcA#srcB\n"
                                      "SELECT * STREAM rhs FROM plain>{5}\n",
                                      kExecDir, a, b, i, k, i + k));
    if (!::testing::Test::HasFatalFailure()) trace = runPlan(instance, rational<int>(6 * std::max(a, b)));
    std::filesystem::remove_all(kExecDir);
  }

  std::string describe() const { return std::format("a={} b={} i={} k={}", a, b, i, k); }
};

// Przypadki: (1,2) z ostrym ogonem (tail_can_be_strict), (3,2) i (1,1) z m=2.
constexpr int kShiftMatchingCases[][3] = {{1, 2, 1}, {3, 2, 1}, {1, 1, 2}};

// Warunek dostepnosci affineReady (StartupBoundaries.lean) w liczbach calkowitych: interwal wyjscia
// to p*tick, interwal wejscia q*tick, a rekord n wyjscia czyta rekord read wejscia o ogonie ws.
bool affineReady(std::int64_t p, std::int64_t q, std::int64_t read, std::int64_t ws, std::int64_t w, std::int64_t n) {
  return q * (read + 1 + ws) <= p * (n + 1 + w);
}

// Ogon jest dokladna granica dostepnosci (affineTail_exact): przy W = tail warunek zachodzi dla kazdego
// rekordu od progow N0 = 0 i 7, a przy W = tail-1 pada na ktoryms z nich. Reszta (n*p + c) mod q
// powtarza sie co q rekordow, wiec wystarczy jeden okres od progu.
template <typename Read>
::testing::AssertionResult tailIsExact(int p, int q, int ws, int tail, const Read &read) {
  for (int origin : {0, 7}) {
    bool tailReady    = true;
    bool shorterReady = tail > 0;
    for (int n = origin; n < origin + q; ++n) {
      tailReady    = tailReady && affineReady(p, q, read(n), ws, tail, n);
      shorterReady = shorterReady && affineReady(p, q, read(n), ws, tail - 1, n);
    }
    if (!tailReady) return ::testing::AssertionFailure() << "W=" << tail << " nie wystarcza od N0=" << origin;
    if (shorterReady) return ::testing::AssertionFailure() << "W=" << tail - 1 << " wystarcza od N0=" << origin;
  }
  return ::testing::AssertionSuccess();
}

// affineRead p q c n z tablicy Lean; -1, gdy wiersza nie ma.
int oracleRead(int p, int q, int c, int n) {
  static const auto table = [] {
    std::map<std::array<int, 4>, int> rows;
    for (const auto &row : kAffineReadRows)
      rows[{row.p, row.q, row.c, row.n}] = row.read;
    return rows;
  }();
  const auto it = table.find({p, q, c, n});
  return it == table.end() ? -1 : it->second;
}

// Zredukowany iloraz wyjscia do wejscia p/q, w ktorym liczy sie warunek dostepnosci.
std::pair<int, int> ratioOf(const rational<int> &out, const rational<int> &src) {
  const auto ratio = out / src;
  return {ratio.numerator(), ratio.denominator()};
}

}  // namespace

// Definicje przeplotu i rozplotu (interleaveAt, deinterleaveLeft/Right) wobec Hash, Div i Mod.
TEST(ProofOracle, interleaveAt_table) {
  // covers: soperations.hash.1~1
  for (int d : kScales)
    for (const auto &row : kInterleaveRows) {
      int pos         = -1;
      const bool from = Hash(rational<int>(row.a, d), rational<int>(row.b, d), row.n, pos);
      ASSERT_EQ(from, row.fromB) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
      ASSERT_EQ(pos, row.pos) << "a=" << row.a << " b=" << row.b << " n=" << row.n << " d=" << d;
    }
}

TEST(ProofOracle, deinterleave_table) {
  // covers: soperations.div.1~1, soperations.mod.1~1
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
  // covers: soperations.subtract.1~1
  for (int d : kScales)
    for (const auto &row : kSubtractRows)
      ASSERT_EQ(Subtract(rational<int>(row.period, 4 * d), rational<int>(row.target, 4 * d), row.n), row.index)
          << "period=" << row.period << "/4 target=" << row.target << "/4 n=" << row.n << " d=" << d;
}

// Definicja dokladnego ogona przeplotu (exactInterleaveTail) wobec HashStartupLatency.
TEST(ProofOracle, exactInterleaveTail_table) {
  // covers: soperations.hash-startup-latency.1~2
  for (int d : kScales)
    for (const auto &row : kTailRows)
      ASSERT_EQ(hashTail(row.a, row.b, row.wa, row.wb, d), row.tail)
          << "a=" << row.a << " b=" << row.b << " WA=" << row.wa << " WB=" << row.wb << " d=" << d;
}

// Postac zamknieta ogona (exactInterleaveTail_closed): dla wzglednie pierwszych a, b ogon z tablicy
// Lean jest wiekszym z kresow ceil(W_A*(a+b)/b) i ceil(((1+W_B)*(a+b) + a - 1)/a) - 1. Wzor jest tu
// przepisany niezaleznie od SOperations.hpp, wiec test sprawdza wypowiedz twierdzenia, nie silnik.
TEST(ProofOracle, exactInterleaveTail_closed) {
  int checked = 0;
  for (const auto &row : kTailRows) {
    if (std::gcd(row.a, row.b) != 1) continue;
    const int boundA = (row.wa * (row.a + row.b) + row.b - 1) / row.b;
    const int boundB = ((1 + row.wb) * (row.a + row.b) + row.a - 2) / row.a;
    ASSERT_EQ(std::max(boundA, boundB), row.tail) << "a=" << row.a << " b=" << row.b << " WA=" << row.wa << " WB=" << row.wb;
    ++checked;
  }
  EXPECT_GT(checked, 0);
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
  // covers: soperations.hash-startup-latency.1~2, soperations.hash-startup-latency.3~1
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

// Ogon przeplotu dwoch zrodel deklarowanych (ogon 0) wynosi co najmniej 1, w kazdej skali.
TEST(ProofOracle, exactInterleaveTail_declared_pos) {
  for (int a = 1; a <= 40; ++a)
    for (int b = 1; b <= 40; ++b)
      for (int d : kScales)
        ASSERT_GE(hashTail(a, b, 0, 0, d), 1) << "a=" << a << " b=" << b << " d=" << d;
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
  // covers: compiler.compiler.10~1
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

// Roznica o interwale s wybiera z wolniejszego o <= s rekord ceil(n*s/o) (Subtract), a suma czyta
// go z powrotem pod indeksem floor(m*o/s) (Add = sampleIndex): wraca rekord n.
TEST(ProofOracle, resample_roundtrip) {
  for (int d : kScales)
    for (int s = 1; s <= 24; ++s)
      for (int o = 1; o <= s; ++o) {
        const rational<int> source(o, d);
        const rational<int> target(s, d);
        for (int n = 0; n < 50; ++n)
          ASSERT_EQ(Add(source, target, Subtract(source, target, n)), n) << "s=" << s << " o=" << o << " n=" << n << " d=" << d;
      }
}

// Skladowa o interwale sumy jest czytana pod wlasnym indeksem: sampleIndex d d n = n.
TEST(ProofOracle, sample_self) {
  // covers: soperations.add.1~1
  for (int d : kScales)
    for (int p = 1; p <= 24; ++p)
      for (int n = 0; n < 50; ++n)
        ASSERT_EQ(Add(rational<int>(p, d), rational<int>(p, d), n), n) << "p=" << p << " n=" << n << " d=" << d;
}

// A+B i B+A maja ten sam interwal, a kazda skladowa jest czytana pod tym samym indeksem.
TEST(ProofOracle, sum_commutativity) {
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    plan.text += std::format(
        "SELECT * STREAM sab{0}_{1} FROM pa{0}_{1}+pb{0}_{1}\n"
        "SELECT * STREAM sba{0}_{1} FROM pb{0}_{1}+pa{0}_{1}\n",
        a, b);
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs) {
    const auto periodA = instance.getQuery(std::format("pa{}_{}", a, b)).rInterval;
    const auto periodB = instance.getQuery(std::format("pb{}_{}", a, b)).rInterval;
    const auto sumAB   = instance.getQuery(std::format("sab{}_{}", a, b)).rInterval;
    const auto sumBA   = instance.getQuery(std::format("sba{}_{}", a, b)).rInterval;
    ASSERT_EQ(sumAB, sumBA) << "a=" << a << " b=" << b;
    for (int n = 0; n < 50; ++n) {
      ASSERT_EQ(Add(sumAB, periodA, n), Add(sumBA, periodA, n)) << "a=" << a << " b=" << b << " n=" << n;
      ASSERT_EQ(Add(sumAB, periodB, n), Add(sumBA, periodB, n)) << "a=" << a << " b=" << b << " n=" << n;
    }
  }
}

// Przy DeltaA <= DeltaB suma ma interwal DeltaA, czyta A[n] i B[floor(n*DeltaA/DeltaB)].
TEST(ProofOracle, sum_fast_left) {
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    if (a <= b) plan.text += std::format("SELECT * STREAM sab{0}_{1} FROM pa{0}_{1}+pb{0}_{1}\n", a, b);
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs) {
    if (a > b) continue;
    const auto periodA = instance.getQuery(std::format("pa{}_{}", a, b)).rInterval;
    const auto periodB = instance.getQuery(std::format("pb{}_{}", a, b)).rInterval;
    const auto sum     = instance.getQuery(std::format("sab{}_{}", a, b)).rInterval;
    ASSERT_EQ(sum, periodA) << "a=" << a << " b=" << b;
    for (int n = 0; n < 50; ++n) {
      ASSERT_EQ(Add(sum, periodA, n), n) << "a=" << a << " b=" << b << " n=" << n;
      ASSERT_EQ(Add(sum, periodB, n), n * a / b) << "a=" << a << " b=" << b << " n=" << n;
    }
  }
}

// (A+B)-DeltaA ma interwal DeltaA i niesie A[n]; tak samo dla B.
TEST(ProofOracle, diff_recovers_sum) {
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    plan.text += std::format(
        "SELECT * STREAM sum{0}_{1} FROM pa{0}_{1}+pb{0}_{1}\n"
        "SELECT * STREAM ra{0}_{1} FROM sum{0}_{1}-{0}/100\n"
        "SELECT * STREAM rb{0}_{1} FROM sum{0}_{1}-{1}/100\n",
        a, b);
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs) {
    const auto sum     = instance.getQuery(std::format("sum{}_{}", a, b)).rInterval;
    const auto periodA = instance.getQuery(std::format("pa{}_{}", a, b)).rInterval;
    const auto periodB = instance.getQuery(std::format("pb{}_{}", a, b)).rInterval;
    ASSERT_EQ(sum, std::min(periodA, periodB)) << "a=" << a << " b=" << b;
    ASSERT_EQ(instance.getQuery(std::format("ra{}_{}", a, b)).rInterval, periodA) << "a=" << a << " b=" << b;
    ASSERT_EQ(instance.getQuery(std::format("rb{}_{}", a, b)).rInterval, periodB) << "a=" << a << " b=" << b;
    for (int n = 0; n < 50; ++n) {
      ASSERT_EQ(Add(sum, periodA, Subtract(sum, periodA, n)), n) << "a=" << a << " b=" << b << " n=" << n;
      ASSERT_EQ(Add(sum, periodB, Subtract(sum, periodB, n)), n) << "a=" << a << " b=" << b << " n=" << n;
    }
  }
}

// Interwal przeplotu to DeltaA*DeltaB/(DeltaA+DeltaB) i jest dodatni.
TEST(ProofOracle, causalInterleave_interval) {
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    plan.text += std::format("SELECT * STREAM z{0}_{1} FROM pa{0}_{1}#pb{0}_{1}\n", a, b);
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs) {
    const auto periodA  = instance.getQuery(std::format("pa{}_{}", a, b)).rInterval;
    const auto periodB  = instance.getQuery(std::format("pb{}_{}", a, b)).rInterval;
    const auto interval = instance.getQuery(std::format("z{}_{}", a, b)).rInterval;
    ASSERT_EQ(interval, periodA * periodB / (periodA + periodB)) << "a=" << a << " b=" << b;
    ASSERT_GT(interval, rational<int>(0)) << "a=" << a << " b=" << b;
  }
}

// Przesuniecie >N producenta o origin O i ogonie W: origin O+N, ogon max(0, W-N), najmniejszy
// pokrywajacy dostepnosc starszego rekordu n-N (causalShift_tail_exact); dostepnosc rekordu
// wyniku pociaga dostepnosc czytanego rekordu zrodla (causalShift_available_source).
TEST(ProofOracle, causalShift_tail_exact) {
  qTree instance;
  compilePlan(instance, R"(
        DECLARE value INTEGER STREAM a35, 3/100 FILE 'a.txt'
        DECLARE value INTEGER STREAM b35, 1/20  FILE 'b.txt'
        DECLARE x INTEGER, y INTEGER, z INTEGER STREAM wide, 1/10 FILE 'a.txt'
        SELECT * STREAM mix FROM a35#b35
        SELECT * STREAM wide_shifted FROM wide>1
        SELECT * STREAM win FROM wide_shifted@(1,2)
        SELECT * STREAM mix_1 FROM mix>1
        SELECT * STREAM mix_2 FROM mix>2
        SELECT * STREAM mix_3 FROM mix>3
        SELECT * STREAM mix_5 FROM mix>5
        SELECT * STREAM win_1 FROM win>1
        SELECT * STREAM win_2 FROM win>2
        SELECT * STREAM win_4 FROM win>4
      )");
  ASSERT_FALSE(HasFatalFailure());

  const std::pair<const char *, int> shifts[] = {{"mix", 1}, {"mix", 2}, {"mix", 3}, {"mix", 5},
                                                 {"win", 1}, {"win", 2}, {"win", 4}};
  for (const auto [source, by] : shifts) {
    const auto &src    = instance.getQuery(source);
    const auto &result = instance.getQuery(std::format("{}_{}", source, by));
    const int w        = src.startupLatency;
    ASSERT_GT(w, 0) << source;
    ASSERT_EQ(result.logicalOrigin, src.logicalOrigin + by) << source << ">" << by;
    ASSERT_EQ(result.startupLatency, std::max(0, w - by)) << source << ">" << by;

    int smallest = 0;
    for (int n = result.logicalOrigin; n < result.logicalOrigin + 20; ++n)
      smallest = std::max(smallest, (n - by + 1 + w) - (n + 1));
    ASSERT_EQ(result.startupLatency, smallest) << source << ">" << by;

    for (int n = result.logicalOrigin; n < result.logicalOrigin + 20; ++n)
      for (int slot = n + 1 + result.startupLatency; slot < n + 5 + result.startupLatency; ++slot) {
        ASSERT_LE(src.logicalOrigin, n - by) << source << ">" << by << " n=" << n;
        ASSERT_LE(n - by + 1 + w, slot) << source << ">" << by << " n=" << n << " slot=" << slot;
      }
  }
}

// Origin przeplotu to najmniejszy slot n, od ktorego obie skladowe czytaja rekordy istniejace
// (interleaveOrigin_le_iff); taki slot istnieje (originReady_exists), a od niego w gore kazdy
// slot czyta rekordy zdefiniowane (causalInterleave_inputs_defined).
TEST(ProofOracle, interleaveOrigin_le_iff) {
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    for (int oa = 0; oa <= 3; ++oa)
      for (int ob = 0; ob <= 3; ++ob) {
        const auto left  = plan.shifted(std::format("pa{}_{}", a, b), oa);
        const auto right = plan.shifted(std::format("pb{}_{}", a, b), ob);
        plan.text += std::format("SELECT * STREAM z{}_{}_{}_{} FROM {}#{}\n", a, b, oa, ob, left, right);
      }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs)
    for (int oa = 0; oa <= 3; ++oa)
      for (int ob = 0; ob <= 3; ++ob) {
        const int origin = instance.getQuery(std::format("z{}_{}_{}_{}", a, b, oa, ob)).logicalOrigin;
        for (int n = 0; n < origin + 3 * (a + b); ++n)
          ASSERT_EQ(originReady(a, b, oa, ob, n), origin <= n)
              << "a=" << a << " b=" << b << " OA=" << oa << " OB=" << ob << " n=" << n << " origin=" << origin;
      }
}

// Przesuniecia dopasowane do tempa (i*a = k*b) przed przeplotem przesuwaja jego origin o i+k
// (interleaveOrigin_shift), bo gotowosc przesunietych skladowych w slocie n to gotowosc
// nieprzesunietych w slocie n-(i+k) (originReady_shift).
TEST(ProofOracle, interleaveOrigin_shift) {
  struct Case {
    int a, b, oa, ob, i, k;
  };
  std::vector<Case> cases;
  PlanText plan;
  for (const auto [a, b] : kPeriodPairs)
    for (int m = 1; m <= 2; ++m)
      for (int oa = 0; oa <= 1; ++oa)
        for (int ob = 0; ob <= 1; ++ob) {
          const Case c{a, b, oa, ob, m * b / std::gcd(a, b), m * a / std::gcd(a, b)};
          const std::string pa = std::format("pa{}_{}", a, b);
          const std::string pb = std::format("pb{}_{}", a, b);
          const auto plainA    = plan.shifted(pa, c.oa);
          const auto plainB    = plan.shifted(pb, c.ob);
          const auto delayedA  = plan.shifted(pa, c.oa + c.i);
          const auto delayedB  = plan.shifted(pb, c.ob + c.k);
          plan.text += std::format("SELECT * STREAM plain{}_{}_{}_{}_{} FROM {}#{}\n", a, b, m, oa, ob, plainA, plainB);
          plan.text += std::format("SELECT * STREAM delayed{}_{}_{}_{}_{} FROM {}#{}\n", a, b, m, oa, ob, delayedA, delayedB);
          cases.push_back(c);
        }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto &c : cases) {
    const int m         = c.i * std::gcd(c.a, c.b) / c.b;
    const auto suffix   = std::format("{}_{}_{}_{}_{}", c.a, c.b, m, c.oa, c.ob);
    const int plain     = instance.getQuery("plain" + suffix).logicalOrigin;
    const int delayed   = instance.getQuery("delayed" + suffix).logicalOrigin;
    const auto describe = std::format("a={} b={} OA={} OB={} i={} k={}", c.a, c.b, c.oa, c.ob, c.i, c.k);
    ASSERT_EQ(c.i * c.a, c.k * c.b) << describe;
    ASSERT_EQ(delayed, plain + c.i + c.k) << describe;
    for (int n = 0; n < delayed + 3 * (c.a + c.b); ++n)
      ASSERT_EQ(delayed <= n, c.i + c.k <= n && plain <= n - (c.i + c.k)) << describe << " n=" << n;
  }
}

// Wykonany plan: rekord n przesuniecia >i istnieje dokladnie dla n >= i i niesie rekord n-i zrodla.
TEST(ProofOracle, causalShift_read) {
  for (const auto [a, b, m] : kShiftMatchingCases) {
    const ShiftMatchingRun run(a, b, m);
    ASSERT_FALSE(HasFatalFailure());
    const auto &copyA  = run.trace.at("copyA");
    const auto &shiftA = run.trace.at("shiftA");
    ASSERT_EQ(copyA.begin()->first, 0) << run.describe();
    for (const auto &[n, record] : copyA)
      ASSERT_EQ(record.value, 1000 + n) << run.describe() << " n=" << n;
    ASSERT_GE(shiftA.size(), 10U) << run.describe();
    ASSERT_EQ(shiftA.begin()->first, run.i) << run.describe();
    for (const auto &[n, record] : shiftA)
      ASSERT_EQ(record.value, copyA.at(n - run.i).value) << run.describe() << " n=" << n;
  }
}

// Wykonany plan: (A>i)#(B>k) i (A#B)>(i+k) zaczynaja od tego samego indeksu i niosa te same rekordy.
TEST(ProofOracle, causal_shift_matching_read) {
  // covers: compiler.compiler.10~1
  for (const auto [a, b, m] : kShiftMatchingCases) {
    const ShiftMatchingRun run(a, b, m);
    ASSERT_FALSE(HasFatalFailure());
    const auto &lhs = run.trace.at("lhs");
    const auto &rhs = run.trace.at("rhs");
    ASSERT_GE(lhs.size(), 10U) << run.describe();
    ASSERT_EQ(lhs.begin()->first, rhs.begin()->first) << run.describe();
    int common = 0;
    for (const auto &[n, record] : lhs) {
      if (!rhs.contains(n)) continue;
      ASSERT_NE(record.value, -1) << run.describe() << " n=" << n;
      ASSERT_EQ(record.value, rhs.at(n).value) << run.describe() << " n=" << n;
      ++common;
    }
    ASSERT_GE(common, 10) << run.describe();
  }
}

// Wykonany plan: obie strony maja ten sam interwal, origin i rekordy, prawa ma ogon nie dluzszy,
// a kazdy rekord dostepny po lewej jest juz dostepny po prawej.
TEST(ProofOracle, causal_shift_matching) {
  for (const auto [a, b, m] : kShiftMatchingCases) {
    ShiftMatchingRun run(a, b, m);
    ASSERT_FALSE(HasFatalFailure());
    const auto &lhsQry = run.instance.getQuery("lhs");
    const auto &rhsQry = run.instance.getQuery("rhs");
    ASSERT_EQ(lhsQry.rInterval, rhsQry.rInterval) << run.describe();
    ASSERT_EQ(lhsQry.logicalOrigin, rhsQry.logicalOrigin) << run.describe();
    ASSERT_LE(rhsQry.startupLatency, lhsQry.startupLatency) << run.describe();

    const auto &lhs = run.trace.at("lhs");
    const auto &rhs = run.trace.at("rhs");
    ASSERT_GE(lhs.size(), 10U) << run.describe();
    ASSERT_EQ(lhs.begin()->first, lhsQry.logicalOrigin) << run.describe();
    ASSERT_EQ(rhs.begin()->first, rhsQry.logicalOrigin) << run.describe();
    for (const auto &[n, record] : lhs) {
      ASSERT_TRUE(rhs.contains(n)) << run.describe() << " n=" << n;
      ASSERT_EQ(record.value, rhs.at(n).value) << run.describe() << " n=" << n;
      ASSERT_LE(rhs.at(n).time, record.time) << run.describe() << " n=" << n;
    }
  }
}

// Nad zrodlami deklarowanymi lhs niesie caly ogon przeplotu W, a rhs max(0, W - L). Cisza rozni sie
// o min(W, L) i o tyle slotow wyjscia wczesniej pojawia sie pierwszy rekord rhs (most G3, #353).
TEST(ProofOracle, causal_shift_matching_declared) {
  for (const auto [a, b, m] : kShiftMatchingCases) {
    ShiftMatchingRun run(a, b, m);
    ASSERT_FALSE(HasFatalFailure());
    const auto &lhsQry = run.instance.getQuery("lhs");
    const auto &rhsQry = run.instance.getQuery("rhs");
    const int w        = run.instance.getQuery("plain").startupLatency;
    const int shift    = run.i + run.k;
    ASSERT_GE(w, 1) << run.describe();
    ASSERT_EQ(lhsQry.startupLatency, w) << run.describe();
    ASSERT_EQ(rhsQry.startupLatency, std::max(0, w - shift)) << run.describe();
    ASSERT_EQ(lhsQry.logicalOrigin + lhsQry.startupLatency, rhsQry.logicalOrigin + rhsQry.startupLatency + std::min(w, shift))
        << run.describe();

    const auto &lhs = run.trace.at("lhs");
    const auto &rhs = run.trace.at("rhs");
    ASSERT_FALSE(lhs.empty()) << run.describe();
    ASSERT_FALSE(rhs.empty()) << run.describe();
    ASSERT_EQ(lhs.begin()->second.time - rhs.begin()->second.time, std::min(w, shift) * lhsQry.rInterval) << run.describe();
  }
}

// ===== Granice rozruchowe operatorow afinicznych (StartupBoundaries.lean) =====
//
// Rekord n wyjscia czyta rekord affineRead p q c n = (n*p + c)/q wejscia, a p/q to iloraz interwalu
// wyjscia do interwalu wejscia. Podstawienia map silnika sa w naglowku StartupBoundaries.lean.

namespace {

// Plan z deklaracjami TEXTFILE dodawanymi na zadanie i przesunieciami jako nazwanymi strumieniami
// SELECT, jak w PlanText.
class SourcePlan {
 public:
  std::string source(const std::string &name, const std::string &interval, const std::string &fields = "value INTEGER") {
    if (emitted.insert(name).second) text += std::format("DECLARE {} STREAM {}, {} TEXTFILE 'a.txt'\n", fields, name, interval);
    return name;
  }

  std::string shifted(const std::string &source, int by) {
    if (by == 0) return source;
    const std::string name = std::format("{}_sh{}", source, by);
    if (emitted.insert(name).second) text += std::format("SELECT * STREAM {} FROM {}>{}\n", name, source, by);
    return name;
  }

  std::string text;

 private:
  std::set<std::string> emitted;
};

// Pary (u, v) z u + v <= 8, czyli w zakresie kAffineReadRows dla Div i Mod.
constexpr std::pair<int, int> kSplitPairs[] = {{1, 1}, {1, 2}, {2, 1}, {2, 3}, {3, 2}, {1, 7}, {5, 3}, {3, 4}};

}  // namespace

// Mapa sumy: skladowa o interwale q czytana przez wyjscie o interwale p, c = 0.
TEST(ProofOracle, addRead_eq) {
  // covers: soperations.add.1~1
  for (int d : kScales)
    for (const auto &row : kAffineReadRows)
      if (row.c == 0)
        ASSERT_EQ(Add(rational<int>(row.p, d), rational<int>(row.q, d), row.n), row.read)
            << "p=" << row.p << " q=" << row.q << " n=" << row.n << " d=" << d;
}

// Mapa roznicy: zrodlo o interwale q, wynik o interwale p, c = q-1.
TEST(ProofOracle, subtractRead_eq) {
  // covers: soperations.subtract.1~1
  for (int d : kScales)
    for (const auto &row : kAffineReadRows)
      if (row.c == row.q - 1)
        ASSERT_EQ(Subtract(rational<int>(row.q, d), rational<int>(row.p, d), row.n), row.read)
            << "p=" << row.p << " q=" << row.q << " n=" << row.n << " d=" << d;
}

// Mapa Theta: Div(u, v, n) przy p = u + v, q = v, c = u + v - 1.
TEST(ProofOracle, thetaRead_eq) {
  // covers: soperations.div.1~1
  for (int d : kScales)
    for (const auto &row : kAffineReadRows)
      if (row.p > row.q && row.c == row.p - 1)
        ASSERT_EQ(Div(rational<int>(row.p - row.q, d), rational<int>(row.q, d), row.n), row.read)
            << "u=" << row.p - row.q << " v=" << row.q << " n=" << row.n << " d=" << d;
}

// Mapa ~Theta: Mod(v, u, n) przy p = u + v, q = v, c = 0 (u/v = Delta/Delta_a).
TEST(ProofOracle, nthetaRead_eq) {
  // covers: soperations.mod.1~1
  for (int d : kScales)
    for (const auto &row : kAffineReadRows)
      if (row.p > row.q && row.c == 0)
        ASSERT_EQ(Mod(rational<int>(row.q, d), rational<int>(row.p - row.q, d), row.n), row.read)
            << "u=" << row.p - row.q << " v=" << row.q << " n=" << row.n << " d=" << d;
}

// Postac zamknieta ogona (affineTail_eq_phase, phaseStartupLatency_eq_engine): affineTail p q c Ws to
// PhaseStartupLatency((c - c mod g)/q, p/q, Ws) silnika, g = gcd(p, q).
TEST(ProofOracle, affineTail_table) {
  // covers: soperations.phase-startup-latency.1~1
  for (const auto &row : kAffineTailRows) {
    const int g = std::gcd(row.p, row.q);
    ASSERT_EQ(PhaseStartupLatency(rational<int>(row.c - row.c % g, row.q), rational<int>(row.p, row.q), row.ws), row.tail)
        << "p=" << row.p << " q=" << row.q << " c=" << row.c << " Ws=" << row.ws;
  }
}

// PhaseStartupLatency jest dokladna granica dostepnosci dla dowolnej stalej c, takze poza tablica.
TEST(ProofOracle, affineTail_exact) {
  for (int p = 1; p <= 16; ++p)
    for (int q = 1; q <= 16; ++q)
      for (int c = 0; c <= p + q; ++c)
        for (int ws = 0; ws <= 3; ++ws) {
          const int g    = std::gcd(p, q);
          const int tail = PhaseStartupLatency(rational<int>(c - c % g, q), rational<int>(p, q), ws);
          ASSERT_TRUE(tailIsExact(p, q, ws, tail, [&](int n) { return (n * p + c) / q; }))
              << "p=" << p << " q=" << q << " c=" << c << " Ws=" << ws;
        }
}

// Ogon sumy wzgledem skladowej: AddStartupLatency(q, p, Ws) = affineTail p q 0 Ws.
TEST(ProofOracle, addTail_eq) {
  // covers: soperations.add-startup-latency.1~1
  for (int d : kScales)
    for (const auto &row : kAffineTailRows)
      if (row.c == 0)
        ASSERT_EQ(AddStartupLatency(rational<int>(row.q, d), rational<int>(row.p, d), row.ws), row.tail)
            << "p=" << row.p << " q=" << row.q << " Ws=" << row.ws << " d=" << d;
}

// Ogon roznicy: SubtractStartupLatency jest affineTail p q (q-1) Ws i dokladna granica dostepnosci.
TEST(ProofOracle, subtractTail_exact) {
  // covers: soperations.subtract-startup-latency.1~1
  for (int d : kScales)
    for (const auto &row : kAffineTailRows)
      if (row.c == row.q - 1)
        ASSERT_EQ(SubtractStartupLatency(rational<int>(row.q, d), rational<int>(row.p, d), row.ws), row.tail)
            << "p=" << row.p << " q=" << row.q << " Ws=" << row.ws << " d=" << d;
  for (int p = 1; p <= 24; ++p)
    for (int q = 1; q <= 24; ++q)
      for (int ws = 0; ws <= 3; ++ws) {
        const rational<int> source(q, 5);
        const rational<int> target(p, 5);
        const int tail = SubtractStartupLatency(source, target, ws);
        ASSERT_TRUE(tailIsExact(p, q, ws, tail, [&](int n) { return Subtract(source, target, n); }))
            << "p=" << p << " q=" << q << " Ws=" << ws;
      }
}

// Ogon Theta nad przeplotem (Delta_s = Delta*Delta_b/(Delta + Delta_b)): ThetaStartupLatency jest
// affineTail (u+v) v (u+v-1) Ws i dokladna granica dostepnosci przy odczycie Div.
TEST(ProofOracle, thetaTail_exact) {
  // covers: soperations.theta-startup-latency.1~1
  for (int d : kScales)
    for (const auto &row : kAffineTailRows)
      if (row.p > row.q && row.c == row.p - 1) {
        const rational<int> delta(row.p - row.q, d);
        const rational<int> deltaB(row.q, d);
        ASSERT_EQ(ThetaStartupLatency(delta * deltaB / (delta + deltaB), delta, deltaB, row.ws), row.tail)
            << "u=" << row.p - row.q << " v=" << row.q << " Ws=" << row.ws << " d=" << d;
      }
  for (int u = 1; u <= 16; ++u)
    for (int v = 1; v <= 16; ++v)
      for (int ws = 0; ws <= 3; ++ws) {
        const rational<int> delta(u, 5);
        const rational<int> deltaB(v, 5);
        const int tail = ThetaStartupLatency(delta * deltaB / (delta + deltaB), delta, deltaB, ws);
        ASSERT_TRUE(tailIsExact(u + v, v, ws, tail, [&](int n) { return Div(delta, deltaB, n); }))
            << "u=" << u << " v=" << v << " Ws=" << ws;
      }
}

// Ogon ~Theta nad przeplotem: NThetaStartupLatency jest affineTail (u+v) v 0 Ws i dokladna granica
// dostepnosci przy odczycie Mod(Delta_a, Delta, n), u/v = Delta/Delta_a.
TEST(ProofOracle, nthetaTail_exact) {
  // covers: soperations.n-theta-startup-latency.1~1
  for (int d : kScales)
    for (const auto &row : kAffineTailRows)
      if (row.p > row.q && row.c == 0) {
        const rational<int> delta(row.p - row.q, d);
        const rational<int> deltaA(row.q, d);
        ASSERT_EQ(NThetaStartupLatency(delta * deltaA / (delta + deltaA), delta, row.ws), row.tail)
            << "u=" << row.p - row.q << " v=" << row.q << " Ws=" << row.ws << " d=" << d;
      }
  for (int u = 1; u <= 16; ++u)
    for (int v = 1; v <= 16; ++v)
      for (int ws = 0; ws <= 3; ++ws) {
        const rational<int> delta(u, 5);
        const rational<int> deltaA(v, 5);
        const int tail = NThetaStartupLatency(delta * deltaA / (delta + deltaA), delta, ws);
        ASSERT_TRUE(tailIsExact(u + v, v, ws, tail, [&](int n) { return Mod(deltaA, delta, n); }))
            << "u=" << u << " v=" << v << " Ws=" << ws;
      }
}

// Ogon okna AGSE @(h, w) nad zrodlem o F polach: AgseStartupLatency(F, h, Ws) = affineTail h F 0 Ws,
// dokladna granica dostepnosci przy odczycie floor(n*h/F) najnowszego pola okna.
TEST(ProofOracle, agseTail_exact) {
  // covers: soperations.agse-startup-latency.1~1
  for (const auto &row : kAffineTailRows)
    if (row.c == 0)
      ASSERT_EQ(AgseStartupLatency(row.q, row.p, row.ws), row.tail) << "F=" << row.q << " h=" << row.p << " Ws=" << row.ws;
  for (int f = 1; f <= 16; ++f)
    for (int h = 1; h <= 16; ++h)
      for (int ws = 0; ws <= 3; ++ws) {
        const int tail = static_cast<int>(AgseStartupLatency(f, h, ws));
        ASSERT_TRUE(tailIsExact(h, f, ws, tail, [&](int n) { return n * h / f; })) << "F=" << f << " h=" << h << " Ws=" << ws;
      }
}

// Theta nad przeplotem a#b z argumentem b/100 ma interwal a/100, a iloraz interwalu wyniku do interwalu
// wejscia wynosi (u+v)/v przy u/v = Delta/Delta_b po skroceniu.
TEST(ProofOracle, theta_ratio) {
  SourcePlan plan;
  for (const auto [a, b] : kPeriodPairs) {
    const auto pa = plan.source(std::format("pa{}_{}", a, b), std::format("{}/100", a));
    const auto pb = plan.source(std::format("pb{}_{}", a, b), std::format("{}/100", b));
    plan.text += std::format("SELECT * STREAM z{0}_{1} FROM {2}#{3}\n", a, b, pa, pb);
    plan.text += std::format("SELECT * STREAM t{0}_{1} FROM z{0}_{1}&{1}/100\n", a, b);
  }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : kPeriodPairs) {
    const auto source = instance.getQuery(std::format("z{}_{}", a, b)).rInterval;
    const auto result = instance.getQuery(std::format("t{}_{}", a, b)).rInterval;
    const rational<int> deltaB(b, 100);
    ASSERT_EQ(result, rational<int>(a, 100)) << "a=" << a << " b=" << b;
    const auto span = result / deltaB;
    ASSERT_EQ(result / source, rational<int>(span.numerator() + span.denominator(), span.denominator()))
        << "a=" << a << " b=" << b;
  }
}

// Ogon sumy w skompilowanym planie to maksimum ogonow wzgledem skladowych i dokladna granica
// dostepnosci obu skladowych jednoczesnie (sumTail_exact). Skladowe przeplotu maja ogon >= 1.
TEST(ProofOracle, sumTail_exact) {
  SourcePlan plan;
  std::vector<std::string> sums;
  for (const auto [a, b] : kPeriodPairs) {
    const auto pa  = plan.source(std::format("pa{}_{}", a, b), std::format("{}/100", a));
    const auto pb  = plan.source(std::format("pb{}_{}", a, b), std::format("{}/100", b));
    const auto mix = std::format("h{}_{}", a, b);
    plan.text += std::format("SELECT * STREAM {} FROM {}#{}\n", mix, pa, pb);
    const auto mixShifted                             = plan.shifted(mix, 1);
    const std::pair<std::string, std::string> parts[] = {{pa, pb}, {mix, pa}, {pb, mix}, {mixShifted, pb}};
    for (const auto &[left, right] : parts) {
      sums.push_back(std::format("s{}", sums.size()));
      plan.text += std::format("SELECT * STREAM {} FROM {}+{}\n", sums.back(), left, right);
    }
  }
  // Obie skladowe z ogonem.
  for (const auto &[left, right] : {std::pair{"h1_2", "h3_2"}, std::pair{"h7_11", "h3_5"}}) {
    sums.push_back(std::format("s{}", sums.size()));
    plan.text += std::format("SELECT * STREAM {} FROM {}+{}\n", sums.back(), left, right);
  }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  int withTail = 0;
  for (const auto &id : sums) {
    const auto &sum = instance.getQuery(id);
    ASSERT_EQ(sum.lProgram.back().getCommandID(), STREAM_ADD) << id;
    const auto &first  = instance.getQuery(sum.lProgram.begin()->getStr_());
    const auto &second = instance.getQuery(std::next(sum.lProgram.begin())->getStr_());
    const int w        = sum.startupLatency;
    withTail += first.startupLatency > 0 || second.startupLatency > 0;
    for (int origin : {sum.logicalOrigin, sum.logicalOrigin + 7}) {
      bool tailReady    = true;
      bool shorterReady = w > 0;
      for (const query *part : {&first, &second}) {
        const auto [p, q] = ratioOf(sum.rInterval, part->rInterval);
        for (int n = origin; n < origin + q; ++n) {
          const int read = Add(sum.rInterval, part->rInterval, n);
          tailReady      = tailReady && affineReady(p, q, read, part->startupLatency, w, n);
          shorterReady   = shorterReady && affineReady(p, q, read, part->startupLatency, w - 1, n);
        }
      }
      ASSERT_TRUE(tailReady) << id << " W=" << w << " N0=" << origin;
      ASSERT_FALSE(shorterReady) << id << " W=" << w << " N0=" << origin;
    }
  }
  EXPECT_GT(withTail, 0);
}

// Poczatek roznicy, Theta i ~Theta nad zrodlem o poczatku o to najmniejsze n z o <= affineRead n
// (affineOrigin_le_iff, firstIndexReaching w computeLogicalOrigin); mapy z tablicy Lean.
TEST(ProofOracle, affineOrigin_le_iff) {
  struct Case {
    std::string id;
    int p, q, c, o;
  };
  std::vector<Case> cases;
  SourcePlan plan;
  // Interwaly zrodla q/100 i roznicy p/100, q <= p.
  constexpr std::pair<int, int> kSubtractPairs[] = {{1, 2}, {2, 3}, {3, 3}, {3, 5}, {2, 7}, {4, 6}, {3, 8}};
  for (int o = 0; o <= 3; ++o) {
    for (const auto [q, p] : kSubtractPairs) {
      const auto src = plan.shifted(plan.source(std::format("src{}", q), std::format("{}/100", q)), o);
      cases.push_back({std::format("d{}_{}_{}", q, p, o), p, q, q - 1, o});
      plan.text += std::format("SELECT * STREAM {} FROM {}-{}/100\n", cases.back().id, src, p);
    }
    for (const auto [u, v] : kSplitPairs) {
      const auto mix = plan.shifted(plan.source(std::format("c{}_{}", u, v), std::format("{}/{}", u * v, 100 * (u + v))), o);
      cases.push_back({std::format("t{}_{}_{}", u, v, o), u + v, v, u + v - 1, o});
      plan.text += std::format("SELECT * STREAM {} FROM {}&{}/100\n", cases.back().id, mix, v);
      cases.push_back({std::format("m{}_{}_{}", u, v, o), u + v, v, 0, o});
      plan.text += std::format("SELECT * STREAM {} FROM {}%{}/100\n", cases.back().id, mix, v);
    }
  }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto &c : cases) {
    const auto &node = instance.getQuery(c.id);
    const auto &src  = instance.getQuery(node.lProgram.begin()->getStr_());
    ASSERT_EQ(src.logicalOrigin, c.o) << c.id;
    ASSERT_EQ(node.rInterval / src.rInterval, rational<int>(c.p, c.q)) << c.id;
    ASSERT_LT(node.logicalOrigin, 24) << c.id;
    for (int n = 0; n < 24; ++n) {
      const int read = oracleRead(c.p, c.q, c.c, n);
      ASSERT_NE(read, -1) << c.id << " n=" << n;
      ASSERT_EQ(node.logicalOrigin <= n, c.o <= read) << c.id << " n=" << n << " origin=" << node.logicalOrigin;
    }
  }
}

// Poczatek sumy to maksimum progow skladowych (sumOrigin_le_iff); mapy Add z tablicy Lean.
TEST(ProofOracle, sumOrigin_le_iff) {
  constexpr std::pair<int, int> pairs[] = {{1, 1}, {1, 2}, {2, 1}, {3, 5}, {3, 2}, {4, 6}};
  SourcePlan plan;
  for (const auto [a, b] : pairs)
    for (int oa = 0; oa <= 3; ++oa)
      for (int ob = 0; ob <= 3; ++ob) {
        const auto left  = plan.shifted(plan.source(std::format("pa{}_{}", a, b), std::format("{}/100", a)), oa);
        const auto right = plan.shifted(plan.source(std::format("pb{}_{}", a, b), std::format("{}/100", b)), ob);
        plan.text += std::format("SELECT * STREAM s{}_{}_{}_{} FROM {}+{}\n", a, b, oa, ob, left, right);
      }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (const auto [a, b] : pairs)
    for (int oa = 0; oa <= 3; ++oa)
      for (int ob = 0; ob <= 3; ++ob) {
        const auto id   = std::format("s{}_{}_{}_{}", a, b, oa, ob);
        const int fast  = std::min(a, b);
        const int start = instance.getQuery(id).logicalOrigin;
        ASSERT_EQ(instance.getQuery(id).rInterval, rational<int>(fast, 100)) << id;
        ASSERT_LT(start, 24) << id;
        for (int n = 0; n < 24; ++n) {
          const int readA = oracleRead(fast, a, 0, n);
          const int readB = oracleRead(fast, b, 0, n);
          ASSERT_NE(readA, -1) << id << " n=" << n;
          ASSERT_NE(readB, -1) << id << " n=" << n;
          ASSERT_EQ(start <= n, oa <= readA && ob <= readB) << id << " n=" << n << " origin=" << start;
        }
      }
}

// agseOrigin F h w Os z tablicy Lean to AgseLogicalOrigin silnika (sufit z agseOrigin_eq_ceil), dla
// dodatniej i ujemnej dlugosci okna.
TEST(ProofOracle, agseOrigin_eq_ceil) {
  // covers: soperations.agse-logical-origin.1~1
  for (const auto &row : kAgseOriginRows) {
    ASSERT_EQ(AgseLogicalOrigin(row.f, row.h, row.w, row.os), row.origin)
        << "F=" << row.f << " h=" << row.h << " w=" << row.w << " Os=" << row.os;
    ASSERT_EQ(AgseLogicalOrigin(row.f, row.h, -row.w, row.os), row.origin)
        << "F=" << row.f << " h=" << row.h << " w=-" << row.w << " Os=" << row.os;
  }
}

// Poczatek okna w skompilowanym planie: O <= n dokladnie wtedy, gdy Os*F + |w| - 1 <= n*h
// (agseOrigin_le_iff), i O jest wartoscia agseOrigin z tablicy Lean.
TEST(ProofOracle, agseOrigin_le_iff) {
  constexpr const char *kFields[] = {"x INTEGER", "x INTEGER, y INTEGER", "x INTEGER, y INTEGER, z INTEGER"};
  constexpr int kSteps[]          = {1, 2, 3, 5};
  constexpr int kLengths[]        = {1, 2, 4, -3};
  SourcePlan plan;
  for (int f = 1; f <= 3; ++f)
    for (int os = 0; os <= 3; ++os) {
      const auto src = plan.shifted(plan.source(std::format("wide{}", f), "1/10", kFields[f - 1]), os);
      for (int h : kSteps)
        for (std::size_t l = 0; l < std::size(kLengths); ++l)
          plan.text += std::format("SELECT * STREAM w{}_{}_{}_{} FROM {}@({},{})\n", f, os, h, l, src, h, kLengths[l]);
    }
  qTree instance;
  compilePlan(instance, plan.text);
  ASSERT_FALSE(HasFatalFailure());

  for (int f = 1; f <= 3; ++f)
    for (int os = 0; os <= 3; ++os)
      for (int h : kSteps)
        for (std::size_t l = 0; l < std::size(kLengths); ++l) {
          const auto id     = std::format("w{}_{}_{}_{}", f, os, h, l);
          const auto &node  = instance.getQuery(id);
          const int width   = std::abs(kLengths[l]);
          const int sourceO = instance.getQuery(node.lProgram.begin()->getStr_()).logicalOrigin;
          ASSERT_EQ(sourceO, os) << id;
          const auto row = std::ranges::find_if(
              kAgseOriginRows, [&](const AgseOriginRow &r) { return r.f == f && r.h == h && r.w == width && r.os == os; });
          ASSERT_NE(row, std::end(kAgseOriginRows)) << id;
          ASSERT_EQ(node.logicalOrigin, row->origin) << id;
          for (int n = 0; n < node.logicalOrigin + 8; ++n)
            ASSERT_EQ(node.logicalOrigin <= n, os * f + width - 1 <= n * h) << id << " n=" << n;
        }
}

namespace {

// Projekcja i redukcje krotek nad zrodlami o niezerowym poczatku i ogonie: okno win (poczatek 4,
// ogon 2) i przesuniety przeplot mix_sh1. Pary: wezel, jego zrodlo.
constexpr std::pair<const char *, const char *> kProjections[] = {
    {"win_copy", "win"},     {"win_expr", "win"},     {"win_min", "win"},     {"win_sum", "win"},
    {"mix_copy", "mix_sh1"}, {"mix_expr", "mix_sh1"}, {"mix_max", "mix_sh1"}, {"mix_avg", "mix_sh1"}};

void compileProjections(qTree &instance) {
  compilePlan(instance, R"(
        DECLARE x INTEGER, y INTEGER, z INTEGER STREAM wide, 1/10 TEXTFILE 'a.txt'
        DECLARE value INTEGER STREAM pa, 3/100 TEXTFILE 'a.txt'
        DECLARE value INTEGER STREAM pb, 1/20 TEXTFILE 'b.txt'
        SELECT * STREAM wide_sh1 FROM wide>1
        SELECT * STREAM win FROM wide_sh1@(1,2)
        SELECT * STREAM mix FROM pa#pb
        SELECT * STREAM mix_sh1 FROM mix>1
        SELECT * STREAM win_copy FROM win
        SELECT win[0]+win[1] STREAM win_expr FROM win
        SELECT * STREAM win_min FROM MIN(win)
        SELECT * STREAM win_sum FROM SUMC(win)
        SELECT * STREAM mix_copy FROM mix_sh1
        SELECT mix_sh1[0]*2 STREAM mix_expr FROM mix_sh1
        SELECT * STREAM mix_max FROM MAX(mix_sh1)
        SELECT * STREAM mix_avg FROM AVG(mix_sh1)
      )");
}

}  // namespace

// Projekcja i redukcje krotek przenosza poczatek zrodla bez zmian (projectionOrigin_eq).
TEST(ProofOracle, projectionOrigin_eq) {
  qTree instance;
  compileProjections(instance);
  ASSERT_FALSE(HasFatalFailure());
  for (const auto [id, source] : kProjections) {
    const auto &src = instance.getQuery(source);
    ASSERT_GT(src.logicalOrigin, 0) << source;
    ASSERT_EQ(instance.getQuery(id).logicalOrigin, src.logicalOrigin) << id;
  }
}

// Ogon projekcji i redukcji krotek to ogon zrodla, dokladna granica dostepnosci przy p = q = 1
// (projectionTail_exact).
TEST(ProofOracle, projectionTail_exact) {
  qTree instance;
  compileProjections(instance);
  ASSERT_FALSE(HasFatalFailure());
  for (const auto [id, source] : kProjections) {
    const auto &node = instance.getQuery(id);
    const int ws     = instance.getQuery(source).startupLatency;
    ASSERT_GT(ws, 0) << source;
    ASSERT_EQ(node.rInterval, instance.getQuery(source).rInterval) << id;
    ASSERT_EQ(node.startupLatency, ws) << id;
    ASSERT_TRUE(tailIsExact(1, 1, ws, node.startupLatency, [](int n) { return n; })) << id;
  }
}
