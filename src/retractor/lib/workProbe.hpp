#pragma once

/// @brief Sonda pracy na slot (E4, issue_219): ile elementów odwiedza slot, per klasa operatora.
///
/// Powód powstania — kampania K6c i nieudany model kosztu slotu (K20 etap 1). Istniejące
/// liczniki opisują STRUKTURĘ planu (`tokens_from`, `tokens_fields`, `nodes_*`) albo jego
/// WYNIK (`MATERIALIZED`, `PLAN capacity`). Żaden nie mówi, ile pracy plan wykonuje w JEDNYM
/// slocie: okno `@(1,30)` odwiedza 30 elementów w każdym slocie, a w licznikach planu jest
/// jednym tokenem. Skutkiem był rozrzut kosztu tokena 12,4x między rodzinami workloadów
/// i model kosztu z błędem 258% na rodzinach niewidzianych przy dopasowaniu.
///
/// Liczniki są procesowe (jak `materializationCounters`), bo pytanie dotyczy pracy CAŁEGO
/// planu, nie pojedynczego strumienia. Analiza dzieli je przez liczbę slotów przebiegu.
///
/// Wątkowość: inkrementacje zachodzą wyłącznie na wątku przetwarzania
/// (`dataModel::processRows` i to, co woła), nigdy na wątku komunikacyjnym IPC. Odczyt
/// następuje po zakończeniu mierzonej pętli. Dlatego liczniki NIE są atomowe — atomik
/// w pętli po elementach okna obciążałby dokładnie ten budżet slotu, który mierzymy.
///
/// Bez `RDB_BENCH_PROBE` makro rozwija się do `((void)0)` i w binarce nie zostaje ślad.

#ifdef RDB_BENCH_PROBE

namespace rdb::probe {

struct workCounters {
  unsigned long long agseWindows  = 0;  ///< konstrukcje okna agregatu (wywołania constructAgsePayload)
  unsigned long long agseElements = 0;  ///< odwiedziny elementów okna — praca rosnąca z długością okna
  unsigned long long agseReads    = 0;  ///< odczyty rekordu źródła (revRead); mniej niż elementów, gdy cache trafia
  unsigned long long evalCalls    = 0;  ///< wywołania ewaluatora wyrażeń
  unsigned long long evalTokens   = 0;  ///< wykonania tokenów RPN — praca arytmetyczna
  unsigned long long hashPicks    = 0;  ///< wybory składowej przeplotu (STREAM_HASH)
  unsigned long long addMerges    = 0;  ///< scalenia payloadów sumy strumieni (STREAM_ADD)
};

inline workCounters work{};

inline workCounters workReport() { return work; }

inline void workReset() { work = workCounters{}; }

}  // namespace rdb::probe

#define RDB_BENCH_WORK_ADD(field, n) (rdb::probe::work.field += static_cast<unsigned long long>(n))

#else

#define RDB_BENCH_WORK_ADD(field, n) ((void)0)

#endif
