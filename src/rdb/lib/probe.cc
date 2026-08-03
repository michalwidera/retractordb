#include "rdb/probe.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <set>
#include <string>

namespace rdb::probe {

namespace {
// Stan sondy planu — zimny (raz na kompilację), więc trzymany tutaj, a nie w nagłówku:
// std::set w nagłówku oznaczałby dynamiczną inicjalizację w każdej jednostce kompilacji,
// która dołącza probe.hpp. Węzły R2 liczone są jako zbiór, bo ta sama reguła może odpalić
// w jednym węźle wielokrotnie, a metryka pyta o liczbę przepisanych węzłów.
std::size_t rewriteR1{};
std::set<std::string> rewriteR2Nodes{};

/// Jedno źródło czasu dla wszystkich sond: zegar monotoniczny, ten sam, którym posługuje
/// się planowanie snu slotu (executor_rt). Mieszanie zegarów dawałoby w wake_lag stałe
/// przesunięcie nie do odróżnienia od jitteru planisty.
long nowNs() {
  std::timespec ts{};
  ::clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1'000'000'000L + ts.tv_nsec;
}

long toNs(const std::timespec &ts) { return ts.tv_sec * 1'000'000'000L + ts.tv_nsec; }
}  // namespace

//
// ─── E4 + K6: liczniki runtime ──────────────────────────────────────────────────
//

workCounters workReport() { return detail::work; }

void workReset() { detail::work = workCounters{}; }

materializationCounters materializationReport() { return detail::materialization; }

void materializationReset() { detail::materialization = materializationCounters{}; }

void detail::countRewriteR1() { ++rewriteR1; }

void detail::countRewriteR2(const std::string &node) { rewriteR2Nodes.insert(node); }

void detail::printRuntimeCounters() {
  if constexpr (rdb_probe_materialize) {
    if (std::getenv("RDB_BENCH_MATERIALIZE")) {
      const auto m = materializationReport();
      std::fprintf(stderr,
                   "MATERIALIZED trwale: dopisania=%llu nadpisania=%llu bajty=%llu  "
                   "pamieciowe: dopisania=%llu nadpisania=%llu bajty=%llu\n",
                   m.appends, m.overwrites, m.bytes, m.memoryAppends, m.memoryOverwrites, m.memoryBytes);
    }
  }

  if constexpr (rdb_probe_work) {
    if (std::getenv("RDB_BENCH_WORK")) {
      const auto w = workReport();
      std::fprintf(stderr,
                   "WORK agse: okna=%llu elementy=%llu odczyty=%llu  eval: wywolania=%llu tokeny=%llu  "
                   "hash: wybory=%llu  add: scalenia=%llu\n",
                   w.agseWindows, w.agseElements, w.agseReads, w.evalCalls, w.evalTokens, w.hashPicks, w.addMerges);
    }
  }
}

//
// ─── E1/E2E: budżet czasowy slotu ───────────────────────────────────────────────
//

void slotProbe::openCsv() {
  const char *path = std::getenv("RDB_BENCH_CSV");
  if (!path) return;  // sonda wkompilowana, ale nieuzbrojona — normalne działanie usługi
  csv_ = std::fopen(path, "w");
  if (csv_) std::fprintf(csv_, "iter,compute_ns,wake_lag_ns,e2e_ns\n");
}

void slotProbe::closeCsv() {
  if (!csv_) return;
  std::fclose(csv_);
  csv_ = nullptr;
}

void slotProbe::setAnchor(const std::timespec &origin) { anchorNs_ = toNs(origin); }

void slotProbe::markWake(long intervalMs) {
  if (!csv_) return;
  wakeNs_     = nowNs();
  deadlineNs_ = anchorNs_ + intervalMs * 1'000'000L;  // ms -> ns
}

void slotProbe::markComputeBegin() {
  if (!csv_) return;
  computeBeginNs_ = nowNs();
}

void slotProbe::markComputeEnd() {
  if (!csv_) return;
  computeEndNs_ = nowNs();
}

void slotProbe::writeRow() {
  if (!csv_) return;
  const long emitNs = nowNs();  // koniec emisji wyniku (E2E)
  std::fprintf(csv_, "%ld,%ld,%ld,%ld\n", slot_++, computeEndNs_ - computeBeginNs_, wakeNs_ - deadlineNs_, emitNs - deadlineNs_);
}

//
// ─── E3: rozmiar planu i czas kompilacji ────────────────────────────────────────
//

long planProbe::monotonicNs() { return nowNs(); }

void planProbe::begin() {
  active_   = std::getenv("RDB_BENCH_PLAN") != nullptr;
  rewriteR1 = 0;
  rewriteR2Nodes.clear();
  startNs_ = nowNs();
}

planShape &planProbe::stage(planStage which) { return stages_[static_cast<std::size_t>(which)]; }

void planProbe::print(const capacityShape &capacities, bool dedupEnabled) const {
  const long compileNs = nowNs() - startNs_;

  const auto &atEntry   = stages_[static_cast<std::size_t>(planStage::entry)];
  const auto &preDedup  = stages_[static_cast<std::size_t>(planStage::preDedup)];
  const auto &postDedup = stages_[static_cast<std::size_t>(planStage::postDedup)];
  const auto &atExit    = stages_[static_cast<std::size_t>(planStage::exit)];

  std::fprintf(stderr,
               "PLAN bench (publiczne/substraty/tokeny-from/tokeny-pol, dedup=%s): "
               "wejscie=%zu/%zu/%zu/%zu  przed-dedup=%zu/%zu/%zu/%zu  "
               "po-dedup=%zu/%zu/%zu/%zu  wyjscie=%zu/%zu/%zu/%zu\n",
               dedupEnabled ? "ON" : "OFF", atEntry.publicStreams, atEntry.substrates, atEntry.fromTokens, atEntry.fieldTokens,
               preDedup.publicStreams, preDedup.substrates, preDedup.fromTokens, preDedup.fieldTokens, postDedup.publicStreams,
               postDedup.substrates, postDedup.fromTokens, postDedup.fieldTokens, atExit.publicStreams, atExit.substrates,
               atExit.fromTokens, atExit.fieldTokens);
  std::fprintf(stderr, "REWRITE_APPLIED r1=%zu r2=%zu\n", rewriteR1, rewriteR2Nodes.size());

  // Czas kompilacji (K6, §9.2). Mierzone jest WYŁĄCZNIE compile(), bez parsowania RQL
  // i bez startu procesu — te są niezależne od profilu ablacyjnego i dla planów rzędu
  // kilkudziesięciu węzłów całkowicie zdominowałyby różnicę, której kampania szuka.
  std::fprintf(stderr, "COMPILE_NS %ld sonda=%ld\n", compileNs - overheadNs_, overheadNs_);
  std::fprintf(stderr, "PLAN capacity: strumieni=%zu suma=%zu maks=%d\n", capacities.streams, capacities.total, capacities.max);
}

}  // namespace rdb::probe
