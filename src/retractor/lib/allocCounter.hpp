#pragma once
// =============================================================================
// allocCounter.hpp — interfejs licznika alokacji (diagnostyka speed_improvement).
//
// Cały interfejs jest pod #ifdef RDB_ALLOC_COUNTER; bez flagi znika bez śladu, a
// makro RDB_ALLOC_SCOPE rozwija się do (void)0 (zero kosztu w produkcji).
//
// Lokalizacja alokacji do call-site: opakuj fazę makrem RDB_ALLOC_SCOPE(bucket) —
// RAII zlicza (allocCount na wyjściu − na wejściu) do kubełka. Kubełki rejestruje
// strona instrumentowana (registerBucket), a Reporter w allocCounter.cpp wypisuje
// je na stderr przy zakończeniu procesu. Per-interwał liczony metodą różnicy
// dwóch przebiegów N1<N2 (kubełki nie mają narzutu startowego — akumulują tylko
// w pętli — ale różnica i tak kasuje warmup). Patrz run_alloc.sh.
// =============================================================================

#ifdef RDB_ALLOC_COUNTER

#include <atomic>

namespace ralloc {

// Globalny licznik alokacji (definicja w allocCounter.cpp). Constant-init.
extern std::atomic<long> allocCount;

// Rejestracja kubełka do raportu Reportera (woła się raz, przy static init).
void registerBucket(const char *name, std::atomic<long> *counter);

// RAII: dodaje deltę allocCount w czasie życia obiektu do wskazanego kubełka.
struct ScopeAccum {
  std::atomic<long> &bucket_;
  long start_;
  explicit ScopeAccum(std::atomic<long> &b) : bucket_(b), start_(allocCount.load(std::memory_order_relaxed)) {}
  ~ScopeAccum() { bucket_.fetch_add(allocCount.load(std::memory_order_relaxed) - start_, std::memory_order_relaxed); }
  ScopeAccum(const ScopeAccum &)            = delete;
  ScopeAccum &operator=(const ScopeAccum &) = delete;
};

}  // namespace ralloc

// Nazwa obiektu z __LINE__ — pozwala umieścić kilka scope'ów w jednym bloku.
#define RDB_ALLOC_SCOPE_CAT2(a, b) a##b
#define RDB_ALLOC_SCOPE_CAT(a, b)  RDB_ALLOC_SCOPE_CAT2(a, b)
#define RDB_ALLOC_SCOPE(bucket)    ralloc::ScopeAccum RDB_ALLOC_SCOPE_CAT(_rdbAcc_, __LINE__)(bucket)

#else  // RDB_ALLOC_COUNTER

#define RDB_ALLOC_SCOPE(bucket) ((void)0)

#endif  // RDB_ALLOC_COUNTER
