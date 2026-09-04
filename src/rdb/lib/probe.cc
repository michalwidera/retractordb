#include "rdb/probe.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <print>
#include <set>
#include <string>

#include "rdb/descriptor.hpp"

namespace rdb::probe {

namespace {
// Stan sondy planu — zimny (raz na kompilację), więc trzymany tutaj, a nie w nagłówku:
// std::set w nagłówku oznaczałby dynamiczną inicjalizację w każdej jednostce kompilacji,
// która dołącza probe.hpp. Węzły R2 liczone są jako zbiór, bo ta sama reguła może odpalić
// w jednym węźle wielokrotnie, a metryka pyta o liczbę przepisanych węzłów.
std::size_t rewriteR1{};
std::set<std::string> rewriteR2Nodes{};
std::size_t rewriteR3{};

constexpr long kNanosecondsPerSecond{1'000'000'000L};
constexpr long kNanosecondsPerMillisecond{1'000'000L};
constexpr std::size_t kBitsPerByte{8U};
constexpr std::size_t kCanonicalWideNumericBytes{kBitsPerByte};
constexpr std::size_t kCanonicalFloatBytes{4U};
constexpr std::size_t kCanonicalPairBytes{2U * kCanonicalWideNumericBytes};

/// Jedno źródło czasu dla wszystkich sond: zegar monotoniczny, ten sam, którym posługuje
/// się planowanie snu slotu (executor_rt). Mieszanie zegarów dawałoby w wake_lag stałe
/// przesunięcie nie do odróżnienia od jitteru planisty.
long nowNs() {
  std::timespec ts{};
  ::clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * kNanosecondsPerSecond) + ts.tv_nsec;
}

long toNs(const std::timespec &ts) { return (ts.tv_sec * kNanosecondsPerSecond) + ts.tv_nsec; }

/// Kanoniczna szerokość jednego pola — odwzorowanie NIEZALEŻNE od reprezentacji
/// RetractorDB i Flinka, bo metryka pierwotna K23 porównuje oba systemy.
///
/// Odwzorowanie: liczby całkowite i podwójnej precyzji = 8 B, pojedyncza precyzja = 4 B
/// (IEEE-754 binary32/binary64), bajt = 1 B, para liczb (RATIONAL, INTPAIR) = 16 B, napis =
/// szerokość zadeklarowana w deskryptorze. Pola konfiguracyjne (TYPE, REF, RETENTION,
/// RETMEMORY) i NULLTYPE nie są danymi, więc mają szerokość zero — tak samo jak
/// w Descriptor::fieldSize().
///
/// Świadomie NIE jest to `rlen * rarray`: natywne `rlen` opisuje format RetractorDB, a §10
/// degraduje takie liczby do metryk drugorzędnych właśnie dlatego, że „ich reprezentacje nie
/// są bezpośrednio porównywalne". Baseline dopasowany do naszego formatu byłby pierwszą
/// rzeczą, którą zakwestionuje recenzent.
std::size_t canonicalFieldWidth(const rdb::rField &field) {
  const auto count = static_cast<std::size_t>(field.rarray);
  switch (field.rtype) {
    case rdb::BYTE:
      return count;
    case rdb::INTEGER:
    case rdb::UINT:
    case rdb::DOUBLE:
      return kCanonicalWideNumericBytes * count;
    case rdb::FLOAT:
      return kCanonicalFloatBytes * count;
    case rdb::RATIONAL:
    case rdb::INTPAIR:
      return kCanonicalPairBytes * count;
    case rdb::IDXPAIR:
      return (static_cast<std::size_t>(field.rlen) * count) + kCanonicalWideNumericBytes;
    case rdb::STRING:
      return static_cast<std::size_t>(field.rlen) * count;
    default:
      return 0U;  // NULLTYPE oraz pola konfiguracyjne
  }
}
}  // namespace

//
// ─── E4 + K6: liczniki runtime ──────────────────────────────────────────────────
//

workCounters workReport() { return detail::work; }

void workReset() { detail::work = workCounters{}; }

materializationCounters materializationReport() { return detail::materialization; }

void materializationReset() { detail::materialization = materializationCounters{}; }

logicalWriteCounters logicalWriteReport() { return detail::logicalWrite; }

void logicalWriteReset() { detail::logicalWrite = logicalWriteCounters{}; }

std::size_t canonicalRecordBytes(const Descriptor &descriptor) {
  std::size_t bytes = 0;
  for (const auto &field : descriptor)
    bytes += canonicalFieldWidth(field);

  // Kanoniczna mapa NULL/luk: jeden bit na wartość spłaszczonego widoku pól, zaokrąglony
  // w górę do bajtu. Szerokość mapy jest stała dla deskryptora — nie zależy od tego, ile
  // wartości jest w danym rekordzie puste, bo metryka ma być deterministyczna.
  const auto values = static_cast<std::size_t>(descriptor.flatElementCount());
  return bytes + ((values + (kBitsPerByte - 1U)) / kBitsPerByte);
}

void detail::countRewriteR1() { ++rewriteR1; }

void detail::countRewriteR2(const std::string &node) { rewriteR2Nodes.insert(node); }

void detail::countRewriteR3(std::size_t applied) { rewriteR3 += applied; }

void detail::printRuntimeCounters() {
  if constexpr (rdb_probe_materialize) {
    if (std::getenv("RDB_BENCH_MATERIALIZE") != nullptr) {
      const auto m = materializationReport();
      std::println(stderr,
                   "MATERIALIZED trwale: dopisania={} nadpisania={} bajty={} pamieciowe: dopisania={} nadpisania={} bajty={}",
                   m.appends, m.overwrites, m.bytes, m.memoryAppends, m.memoryOverwrites, m.memoryBytes);
    }

    // Osobna zmienna, bo to inna wielkość niż wiersz powyżej: tam objętość magazynu
    // w reprezentacji natywnej, tu kanoniczne bajty zapisów z rozdziałem na role.
    if (std::getenv("RDB_BENCH_LOGICAL") != nullptr) {
      const auto l = logicalWriteReport();
      std::println(
          stderr, "LOGICAL substrat: dopisania={} nadpisania={} bajty={} publiczne: dopisania={} nadpisania={} bajty={}",
          l.substrateAppends, l.substrateOverwrites, l.substrateBytes, l.publicAppends, l.publicOverwrites, l.publicBytes);
    }
  }

  if constexpr (rdb_probe_work) {
    if (std::getenv("RDB_BENCH_WORK") != nullptr) {
      const auto w = workReport();
      std::println(stderr,
                   "WORK agse: okna={} elementy={} odczyty={} eval: wywolania={} tokeny={} hash: wybory={} add: scalenia={}",
                   w.agseWindows, w.agseElements, w.agseReads, w.evalCalls, w.evalTokens, w.hashPicks, w.addMerges);
    }
  }
}

//
// ─── E1/E2E: budżet czasowy slotu ───────────────────────────────────────────────
//

void slotProbe::openCsv() {
  const char *path = std::getenv("RDB_BENCH_CSV");
  if (path == nullptr) return;  // sonda wkompilowana, ale nieuzbrojona — normalne działanie usługi
  csv_ = std::fopen(path, "w");
  if (csv_ != nullptr) std::println(csv_, "iter,compute_ns,wake_lag_ns,e2e_ns");
}

void slotProbe::closeCsv() {
  if (csv_ == nullptr) return;
  std::fclose(csv_);
  csv_ = nullptr;
}

void slotProbe::setAnchor(const std::timespec &origin) { anchorNs_ = toNs(origin); }

void slotProbe::markWake(long intervalMs) {
  if (csv_ == nullptr) return;
  wakeNs_     = nowNs();
  deadlineNs_ = anchorNs_ + (intervalMs * kNanosecondsPerMillisecond);  // ms -> ns
}

void slotProbe::markComputeBegin() {
  if (csv_ == nullptr) return;
  computeBeginNs_ = nowNs();
}

void slotProbe::markComputeEnd() {
  if (csv_ == nullptr) return;
  computeEndNs_ = nowNs();
}

void slotProbe::writeRow() {
  if (csv_ == nullptr) return;
  const long emitNs = nowNs();  // koniec emisji wyniku (E2E)
  std::println(csv_, "{},{},{},{}", slot_++, computeEndNs_ - computeBeginNs_, wakeNs_ - deadlineNs_, emitNs - deadlineNs_);
}

//
// ─── E3: rozmiar planu i czas kompilacji ────────────────────────────────────────
//

long planProbe::monotonicNs() { return nowNs(); }

void planProbe::begin() {
  active_   = std::getenv("RDB_BENCH_PLAN") != nullptr;
  rewriteR1 = 0;
  rewriteR2Nodes.clear();
  rewriteR3 = 0;
  startNs_  = nowNs();
}

planShape &planProbe::stage(planStage which) { return stages_[static_cast<std::size_t>(which)]; }

void planProbe::print(const capacityShape &capacities, bool dedupEnabled) const {
  const long compileNs = nowNs() - startNs_;

  const auto &atEntry   = stages_[static_cast<std::size_t>(planStage::entry)];
  const auto &preDedup  = stages_[static_cast<std::size_t>(planStage::preDedup)];
  const auto &postDedup = stages_[static_cast<std::size_t>(planStage::postDedup)];
  const auto &atExit    = stages_[static_cast<std::size_t>(planStage::exit)];

  std::println(stderr,
               "PLAN bench (publiczne/substraty/tokeny-from/tokeny-pol, dedup={}): wejscie={}/{}/{}/{} przed-dedup={}/{}/{}/{} "
               "po-dedup={}/{}/{}/{} wyjscie={}/{}/{}/{}",
               dedupEnabled ? "ON" : "OFF", atEntry.publicStreams, atEntry.substrates, atEntry.fromTokens, atEntry.fieldTokens,
               preDedup.publicStreams, preDedup.substrates, preDedup.fromTokens, preDedup.fieldTokens, postDedup.publicStreams,
               postDedup.substrates, postDedup.fromTokens, postDedup.fieldTokens, atExit.publicStreams, atExit.substrates,
               atExit.fromTokens, atExit.fieldTokens);
  std::println(stderr, "REWRITE_APPLIED r1={} r2={} r3={}", rewriteR1, rewriteR2Nodes.size(), rewriteR3);

  // Czas kompilacji (K6, §9.2). Mierzone jest WYŁĄCZNIE compile(), bez parsowania RQL
  // i bez startu procesu — te są niezależne od profilu ablacyjnego i dla planów rzędu
  // kilkudziesięciu węzłów całkowicie zdominowałyby różnicę, której kampania szuka.
  std::println(stderr, "COMPILE_NS {} sonda={}", compileNs - overheadNs_, overheadNs_);
  std::println(stderr, "PLAN capacity: strumieni={} suma={} maks={}", capacities.streams, capacities.total, capacities.max);
}

}  // namespace rdb::probe
