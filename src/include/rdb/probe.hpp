#pragma once

/// @brief Sondy pomiarowe kampanii benchmarkowych — jedno miejsce dla całej instrumentacji.
///
/// Sondy nie są funkcją produktu: mierzą go na potrzeby eksperymentów (E1/E2E czas slotu,
/// E3 rozmiar planu, E4 praca na slot, K6 objętość materializacji). Dlatego ich kod jest
/// zebrany w dwóch plikach (`probe.hpp` + `probe.cc`), a w miejscach pomiaru zostaje
/// wyłącznie wywołanie o nazwie mówiącej, co jest liczone.
///
/// Włączanie: opcja CMake `RDB_BENCH_PROBE` (scripts/buildrdb.sh probe) ustawia stałe
/// `rdb_probe_*` w `probeConfig.h`. Gdy stała jest `false`, `if constexpr` odrzuca treść sondy
/// — w wyniku nie zostaje po niej ani instrukcja. Kod sondy mimo to przechodzi przez
/// kompilator w KAŻDYM buildzie, więc nie może zgnić niezauważony; poprzednia wersja
/// (`#ifdef RDB_BENCH_PROBE`) tej własności nie miała.
///
/// Publiczne funkcje sond są cienkimi wrapperami nad implementacją z `probe.cc`: `if
/// constexpr` w nagłówku wycina treść, a ciało (formatowanie raportów, obsługa pliku CSV)
/// nie zaśmieca ścieżki mierzonej. Wrappery mają `[[gnu::always_inline]]`, bo samo `inline`
/// NIE wystarcza: przy `-O0` (build Debug) kompilator nie wciąga pustej funkcji i w miejscu
/// pomiaru zostaje wywołanie. Sprawdzone: bez tego atrybutu `nm` pokazuje w
/// `streamInstance.cpp.o` symbole sondy mimo `rdb_probe_work == false`.
///
/// Gdy wyliczenie ARGUMENTU samo kosztuje (np. storage::isMemoryBackedStorage() porównuje
/// napisy), `if constexpr` musi objąć całe wywołanie w miejscu pomiaru — wrapper wycina
/// swoje ciało, ale argumenty wylicza wywołujący.
///
/// Wątkowość: wszystkie liczniki są procesowe i NIE są atomowe. Inkrementacje zachodzą
/// wyłącznie na wątku przetwarzania (`dataModel::processRows` i to, co woła) albo na wątku
/// kompilacji; wątek komunikacyjny IPC ich nie dotyka. Atomik w pętli po elementach okna
/// obciążałby dokładnie ten budżet slotu, który sonda mierzy.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>

#include "probeConfig.h"

namespace rdb {
// Deklaracja wyprzedzająca, nie include: canonicalRecordBytes() potrzebuje deskryptora, ale
// probe.hpp dołączany jest w gorących jednostkach kompilacji, a descriptor.hpp ciągnie za sobą
// magic_enum i boost::rational. Definicja siedzi w probe.cc, które include ma.
class Descriptor;
}  // namespace rdb

namespace rdb::probe {

/// Czy jakakolwiek sonda jest wkompilowana — do banerów ostrzegawczych.
constexpr bool enabled = rdb_probe_slot || rdb_probe_plan || rdb_probe_materialize || rdb_probe_work;

//
// ─── E4: praca na slot (rdb_probe_work) ─────────────────────────────────────────
//
// Liczniki planu opisują jego STRUKTURĘ (`tokens_from`, `nodes_*`) albo WYNIK
// (`MATERIALIZED`, `PLAN capacity`). Żaden nie mówi, ile pracy plan wykonuje w JEDNYM
// slocie: okno `@(1,30)` odwiedza 30 elementów w każdym slocie, a w planie jest jednym
// tokenem. Skutkiem był rozrzut kosztu tokena 12,4x między rodzinami workloadów i model
// kosztu z błędem 258% na rodzinach niewidzianych przy dopasowaniu (K20 etap 1).
//

struct workCounters {
  unsigned long long agseWindows  = 0;  ///< konstrukcje okna agregatu (wywołania constructAgsePayload)
  unsigned long long agseElements = 0;  ///< odwiedziny elementów okna — praca rosnąca z długością okna
  unsigned long long agseReads    = 0;  ///< odczyty rekordu źródła (revRead); mniej niż elementów, gdy cache trafia
  unsigned long long evalCalls    = 0;  ///< wywołania ewaluatora wyrażeń
  unsigned long long evalTokens   = 0;  ///< wykonania tokenów RPN — praca arytmetyczna
  unsigned long long hashPicks    = 0;  ///< wybory składowej przeplotu (STREAM_HASH)
  unsigned long long addMerges    = 0;  ///< scalenia payloadów sumy strumieni (STREAM_ADD)
};

/// Liczniki materializacji: ile rekordów i bajtów trafiło do magazynu (K6, §9.2).
///
/// Liczone są wyłącznie zapisy, które faktycznie dotarły do accessora; rekord all-null
/// pochłonięty przez detekcję gap nie jest materializacją. Nadpisanie nie zwiększa
/// objętości magazynu, tylko koszt zapisu — stąd rozdział dopisań i nadpisań.
///
/// Magazyn trwały i pamięciowy są liczone ROZDZIELNIE. Bez tego plan z `SUBSTRAT 'memory'`
/// raportowałby objętość materializacji, której nigdy nie zapisał na dysk — a to jest
/// dokładnie ta liczba, którą artykuł nazwałby „objętością materializacji".
struct materializationCounters {
  unsigned long long appends          = 0;
  unsigned long long overwrites       = 0;
  unsigned long long bytes            = 0;
  unsigned long long memoryAppends    = 0;
  unsigned long long memoryOverwrites = 0;
  unsigned long long memoryBytes      = 0;
};

/// Kanoniczne zapisy logiczne (K23/H9) — metryka PIERWOTNA kampanii K23, obok
/// `materializationCounters`, nie zamiast nich.
///
/// Trzy różnice wobec licznika natywnego wynikają wprost ze specyfikacji metryki:
///
/// 1. **bajty są kanoniczne, nie natywne** — liczone przez canonicalRecordBytes() wg
///    odwzorowania niezależnego od reprezentacji obu porównywanych systemów, wraz
///    z kanoniczną mapą NULL/luk. Natywne `mat_bytes` są nieporównywalne między
///    RetractorDB a Flinkiem, bo opisują dwa różne formaty rekordu.
/// 2. **rola strumienia jest rozdzielona** — metryka pyta o zapisy do materializowanego
///    podplanu (substraty), a mianownikiem jest liczba publicznych rekordów wyjściowych.
///    Licznik natywny jest globalny, więc jednej ani drugiej wielkości nie daje.
/// 3. **nadpisanie wnosi bajty** — jednostką jest RZECZYWISTY ZAPIS rekordu pośredniego,
///    nie przyrost objętości magazynu. Substrat na buforze kołowym (`SUBSTRAT 'memory'`)
///    po zawinięciu idzie ścieżką nadpisania; licznik natywny pokazałby tam zero bajtów,
///    czyli bramka mechanizmu przeszłaby, a metryka pierwotna skłamała. Dopisania
///    i nadpisania zostają rozdzielne, żeby ten reżim był widoczny w raporcie.
///
/// Zapis pochłonięty przez detekcję gap nie jest liczony — wynika to z położenia sondy
/// za `metaData_->absorbAppend()`, nie z osobnej reguły tutaj.
struct logicalWriteCounters {
  unsigned long long substrateAppends    = 0;
  unsigned long long substrateOverwrites = 0;
  unsigned long long substrateBytes      = 0;
  unsigned long long publicAppends       = 0;
  unsigned long long publicOverwrites    = 0;
  unsigned long long publicBytes         = 0;
};

namespace detail {
// Stan sond ścieżki gorącej (okno agregatu, ewaluator, zapis rekordu) leży w nagłówku, bo
// inkrementacja musi się inline'ować — skok do innej jednostki kompilacji byłby w pomiarze
// widoczny. Przy wyłączonej sondzie te obiekty nadal istnieją (kilkadziesiąt bajtów
// w .bss), ale nikt ich nie dotyka. Stan sondy planu jest zamknięty w probe.cc: jest zimny
// (raz na kompilację), a std::set w nagłówku oznaczałby dynamiczną inicjalizację w każdej
// jednostce kompilacji dołączającej ten plik — także tam, gdzie sond nie ma.
inline workCounters work{};
inline materializationCounters materialization{};
inline logicalWriteCounters logicalWrite{};

void printRuntimeCounters();
void countRewriteR1();
void countRewriteR2(const std::string &node);
}  // namespace detail

/// Konstrukcja okna agregatu: okno odwiedza `elements` elementów w KAŻDYM slocie,
/// niezależnie od tego, ile z nich trafi w dostępny zakres.
[[gnu::always_inline]] inline void onAgseWindow(std::size_t elements) noexcept {
  if constexpr (rdb_probe_work) {
    ++detail::work.agseWindows;
    detail::work.agseElements += elements;
  }
}

/// Odczyt rekordu źródła w oknie; cache `lastReadPosition` część odwiedzin omija,
/// dlatego odczyty i odwiedziny liczone są rozdzielnie.
[[gnu::always_inline]] inline void onAgseRead() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.agseReads;
}

/// Wykonanie programu wyrażenia: praca arytmetyczna slotu to LICZBA WYKONANYCH tokenów,
/// nie rozmiar programu w planie — ten sam program wykonuje się raz na slot na każdy
/// strumień, który go używa.
[[gnu::always_inline]] inline void onEval(std::size_t tokens) noexcept {
  if constexpr (rdb_probe_work) {
    ++detail::work.evalCalls;
    detail::work.evalTokens += tokens;
  }
}

/// Wybór składowej przeplotu w tym slocie (STREAM_HASH).
[[gnu::always_inline]] inline void onHashPick() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.hashPicks;
}

/// Scalenie payloadów sumy strumieni (STREAM_ADD).
[[gnu::always_inline]] inline void onAddMerge() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.addMerges;
}

workCounters workReport();
void workReset();

//
// ─── K6: objętość materializacji (rdb_probe_materialize) ────────────────────────
//

[[gnu::always_inline]] inline void onMaterializedAppend(bool memoryBacked, std::size_t bytes) noexcept {
  if constexpr (rdb_probe_materialize) {
    if (memoryBacked) {
      ++detail::materialization.memoryAppends;
      detail::materialization.memoryBytes += bytes;
    } else {
      ++detail::materialization.appends;
      detail::materialization.bytes += bytes;
    }
  }
}

[[gnu::always_inline]] inline void onMaterializedOverwrite(bool memoryBacked) noexcept {
  if constexpr (rdb_probe_materialize) {
    if (memoryBacked)
      ++detail::materialization.memoryOverwrites;
    else
      ++detail::materialization.overwrites;
  }
}

materializationCounters materializationReport();
void materializationReset();

/// Kanoniczna szerokość rekordu opisanego deskryptorem, wraz z kanoniczną mapą NULL/luk.
///
/// **To jest serializer metryki pierwotnej K23 i jedyne miejsce, w którym wolno go zmienić.**
/// Ta sama definicja obowiązuje job Flinka; predeklaracja kampanii ją zamraża. Wynik zależy
/// wyłącznie od deskryptora, nie od zawartości rekordu — metryka bajtowa jest
/// deterministycznym wynikiem mechanizmu, więc nie może się zmieniać z danymi.
std::size_t canonicalRecordBytes(const Descriptor &descriptor);

/// Zapis rekordu do magazynu strumienia: `substrate` rozdziela materializowany podplan od
/// wyniku publicznego, `append` — dopisanie od nadpisania.
[[gnu::always_inline]] inline void onLogicalWrite(bool substrate, bool append, std::size_t canonicalBytes) noexcept {
  if constexpr (rdb_probe_materialize) {
    auto &counters = detail::logicalWrite;
    if (substrate) {
      if (append)
        ++counters.substrateAppends;
      else
        ++counters.substrateOverwrites;
      counters.substrateBytes += canonicalBytes;
    } else {
      if (append)
        ++counters.publicAppends;
      else
        ++counters.publicOverwrites;
      counters.publicBytes += canonicalBytes;
    }
  }
}

logicalWriteCounters logicalWriteReport();
void logicalWriteReset();

/// Raport liczników runtime (K6 + K23 + E4) na stderr, po zakończeniu mierzonej pętli — żeby
/// zliczanie nie obciążało budżetu slotu. Każdy wiersz sterowany osobną zmienną
/// środowiskową (`RDB_BENCH_MATERIALIZE`, `RDB_BENCH_LOGICAL`, `RDB_BENCH_WORK`), bo to różne
/// wielkości: tam objętość zapisów w reprezentacji natywnej, obok kanoniczne bajty zapisów
/// z rozdziałem na role, tu liczba odwiedzin elementów.
[[gnu::always_inline]] inline void reportRuntimeCounters() {
  if constexpr (rdb_probe_materialize || rdb_probe_work) detail::printRuntimeCounters();
}

//
// ─── E1/E2E: budżet czasowy slotu (rdb_probe_slot) ──────────────────────────────
//

/// Sonda slotu: CSV z czasem obliczeń i latencją end-to-end, analizowany przez
/// examples/ecg/e1_stats.py. Kolumny:
///   compute_ns  — czas processRows() (E1, czysty rdzeń obliczeń),
///   wake_lag_ns — spóźnienie pobudki względem deadline'u interwału (kotwica + interwał;
///                 ten sam cel, do którego dąży rtAbsoluteSleep) — jitter planisty,
///   e2e_ns      — od deadline'u (nominalny moment pojawienia się krotki wejściowej
///                 w modelu czasowym) do końca emisji wyniku do kolejek IPC.
/// Aktywna dopiero, gdy `RDB_BENCH_CSV` wskazuje plik wyjściowy. Uwaga: bez `-t` pętla
/// śpi względnie, więc dryf kumuluje się w wake_lag/e2e — do CDF E2E miarodajny jest
/// przebieg z `-t`.
class slotProbe {
 public:
  slotProbe() = default;
  [[gnu::always_inline]] ~slotProbe() {
    if constexpr (rdb_probe_slot) closeCsv();
  }
  slotProbe(const slotProbe &)            = delete;
  slotProbe &operator=(const slotProbe &) = delete;

  /// Otwarcie pliku CSV. Wołane PRZED ustawieniem kotwicy osi czasu: koszt otwarcia
  /// nie może obciążyć budżetu pierwszych slotów (transjent startowy).
  [[gnu::always_inline]] void open() {
    if constexpr (rdb_probe_slot) openCsv();
  }

  /// Kotwica osi czasu — dokładnie ta sama, względem której planowany jest sen slotu.
  [[gnu::always_inline]] void anchor(const std::timespec &origin) {
    if constexpr (rdb_probe_slot) setAnchor(origin);
  }

  /// Początek slotu: moment pobudki i deadline wyliczony z kotwicy oraz interwału [ms].
  [[gnu::always_inline]] void beginSlot(long intervalMs) {
    if constexpr (rdb_probe_slot) markWake(intervalMs);
  }

  [[gnu::always_inline]] void beginCompute() {
    if constexpr (rdb_probe_slot) markComputeBegin();
  }

  [[gnu::always_inline]] void endCompute() {
    if constexpr (rdb_probe_slot) markComputeEnd();
  }

  /// Koniec slotu — po emisji wyniku; dopisuje wiersz CSV.
  [[gnu::always_inline]] void endSlot() {
    if constexpr (rdb_probe_slot) writeRow();
  }

 private:
  void openCsv();
  void closeCsv();
  void setAnchor(const std::timespec &origin);
  void markWake(long intervalMs);
  void markComputeBegin();
  void markComputeEnd();
  void writeRow();

  std::FILE *csv_      = nullptr;
  long anchorNs_       = 0;
  long deadlineNs_     = 0;
  long wakeNs_         = 0;
  long computeBeginNs_ = 0;
  long computeEndNs_   = 0;
  long slot_           = 0;
};

//
// ─── E3: rozmiar planu i czas kompilacji (rdb_probe_plan) ───────────────────────
//

/// Plan opisany czwórką liczb, z pominięciem dyrektyw kompilatora:
///   * strumienie publiczne i substraty osobno — substrat nie ma tożsamości obserwowalnej,
///     więc to on jest właściwą jednostką redukcji strukturalnej;
///   * tokeny drzewa FROM (query::lProgram) — tam widać efekt R1 i deduplikacji;
///   * tokeny programów pól (field::lProgram) — tam i TYLKO tam widać efekt R2.
/// Ostatni składnik jest konieczny: współdzielenie równoważnych SELECT przenosi kosztowny
/// program pól do jednego substratu, nie zmieniając ani jednego tokenu w lProgram.
struct planShape {
  std::size_t publicStreams = 0;
  std::size_t substrates    = 0;
  std::size_t fromTokens    = 0;
  std::size_t fieldTokens   = 0;
};

/// Rozmiar buforów planu (K6, §9.2) — wynik computeRequiredCapacities(). Suma jest
/// proporcjonalna do zajętości pamięci planu, maksimum wskazuje najgłębszy bufor,
/// czyli ten decydujący o najgorszym przypadku.
struct capacityShape {
  std::size_t streams = 0;
  std::size_t total   = 0;
  int max             = 0;
};

/// Etapy, na których zdejmowana jest migawka planu. Spadek liczby tokenów/strumieni
/// (zwłaszcza na etapie deduplikacji) to mierzalny efekt optymalizacji z sekcji E3.
enum class planStage {
  entry,      ///< surowy plan po parsowaniu
  preDedup,   ///< po kanonizacji do postaci pośredniej (dekompozycja)
  postDedup,  ///< po eliminacji zdublowanych substratów — właściwa redukcja planu
  exit        ///< końcowy, zoptymalizowany plan
};

/// Kształt planu. Szablon, bo typy planu (qTree/query/field) należą do biblioteki
/// retractor, a sonda leży poziom niżej — w rdb, gdzie sięga po nią także storage.
template <class Plan>
planShape shapeOf(const Plan &plan) {
  planShape acc;
  for (const auto &q : plan) {
    if (q.isCompilerDirective()) continue;
    ++(q.isSubstrat ? acc.substrates : acc.publicStreams);
    acc.fromTokens += q.lProgram.size();
    for (const auto &f : q.lSchema)
      acc.fieldTokens += f.lProgram.size();
  }
  return acc;
}

template <class CapacityMap>
capacityShape shapeOfCapacities(const CapacityMap &capacities) {
  capacityShape acc;
  acc.streams = capacities.size();
  for (const auto &[name, capacity] : capacities) {
    acc.total += static_cast<std::size_t>(capacity);
    acc.max = std::max(acc.max, capacity);
  }
  return acc;
}

/// Zastosowanie reguły przepisującej R1 (faktoryzacja przesunięć przez przeplot).
[[gnu::always_inline]] inline void onRewriteR1() {
  if constexpr (rdb_probe_plan) detail::countRewriteR1();
}

/// Zastosowanie reguły R2 (przemienność ADD przy odciskach) w danym węźle — liczone
/// po węzłach, bo ta sama reguła może odpalić w jednym węźle wielokrotnie.
[[gnu::always_inline]] inline void onRewriteR2(const std::string &node) {
  if constexpr (rdb_probe_plan) detail::countRewriteR2(node);
}

/// Sonda planu, obiekt o czasie życia jednej kompilacji. Aktywna dopiero, gdy ustawiona
/// jest zmienna `RDB_BENCH_PLAN`; bez niej nie zdejmuje migawek i nie ma efektów ubocznych.
///
/// Czas instrumentacji jest ODEJMOWANY od czasu kompilacji: migawka iteruje cały plan,
/// więc jej koszt zależy od rozmiaru planu, a ten różni się między profilami ablacyjnymi
/// — bez odjęcia profil o większym planie wyglądałby na wolniejszy częściowo z powodu
/// własnego pomiaru.
class planProbe {
 public:
  [[gnu::always_inline]] planProbe() {
    if constexpr (rdb_probe_plan) begin();
  }

  /// Migawka planu na danym etapie; koszt migawki trafia do budżetu sondy, nie kompilacji.
  template <class Plan>
  [[gnu::always_inline]] void capture(planStage which, const Plan &plan) {
    if constexpr (rdb_probe_plan) {
      if (!active_) return;
      const long begin = monotonicNs();
      stage(which)     = shapeOf(plan);
      overheadNs_ += monotonicNs() - begin;
    }
  }

  /// Migawka końcowa + raport E3 na stderr. Zegar zatrzymywany PO ostatniej migawce,
  /// żeby jej koszt też został odjęty.
  template <class Plan, class CapacityMap>
  [[gnu::always_inline]] void report(const Plan &plan, const CapacityMap &capacities, bool dedupEnabled) {
    if constexpr (rdb_probe_plan) {
      if (!active_) return;
      capture(planStage::exit, plan);
      print(shapeOfCapacities(capacities), dedupEnabled);
    }
  }

 private:
  void begin();
  planShape &stage(planStage which);
  void print(const capacityShape &capacities, bool dedupEnabled) const;
  static long monotonicNs();

  bool active_     = false;
  long startNs_    = 0;
  long overheadNs_ = 0;
  planShape stages_[4]{};
};

}  // namespace rdb::probe
