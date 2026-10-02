#include <algorithm>
#include <cstdint>
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
  return HashStartupLatency(deltaA, deltaB, deltaA * deltaB / (deltaA + deltaB), wa, wb);
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
  dataModel proc(instance);
  pProc = &proc;  // dumpManager czyta model przez ten wskaznik
  CRationalStreamMath::TimeLine timeline(instance.getAvailableTimeIntervals());
  proc.processZeroStep();

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
    proc.processRows(dueMask, now);
    for (const auto &q : instance) {
      if (q.isDeclaration() || !due.contains(q.id)) continue;
      const auto count = proc.qSet.at(q.id)->outputPayload->getRecordsCount();
      if (count == before[q.id]) continue;
      const auto row  = proc.getRow(q.id, 0);
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
