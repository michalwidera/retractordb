#pragma once

/// @brief Sondy pomiarowe kampanii benchmarkowych - jedno miejsce dla całej instrumentacji.
///
/// Sondy nie są funkcją produktu: mierzą go na potrzeby eksperymentów (E1/E2E czas slotu,
/// E3 rozmiar planu, E4 praca na slot, K6 objętość materializacji). Dlatego ich kod jest
/// zebrany w dwóch plikach (`probe.hpp` + `probe.cc`), a w miejscach pomiaru zostaje
/// wyłącznie wywołanie o nazwie mówiącej, co jest liczone.
///
/// Włączanie: opcja CMake `RDB_BENCH_PROBE` (scripts/buildrdb.sh probe) ustawia stałe
/// `rdb_probe_*` w `probeConfig.h`. Gdy stała jest `false`, `if constexpr` odrzuca treść sondy
/// - w wyniku nie zostaje po niej ani instrukcja. Kod sondy mimo to przechodzi przez
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
/// napisy), `if constexpr` musi objąć całe wywołanie w miejscu pomiaru - wrapper wycina
/// swoje ciało, ale argumenty wylicza wywołujący.
///
/// Wątkowość: wszystkie liczniki są procesowe i NIE są atomowe. Inkrementacje zachodzą
/// wyłącznie na wątku przetwarzania (`dataModel::processRows` i to, co woła) albo na wątku
/// kompilacji; wątek komunikacyjny IPC ich nie dotyka. Atomik w pętli po elementach okna
/// obciążałby dokładnie ten budżet slotu, który sonda mierzy.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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

/// Czy jakakolwiek sonda jest wkompilowana - do banerów ostrzegawczych.
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

/// @requirements-exempt workCounters: Zestaw licznikow pracy; pomiary i aktualizacje wykonuja funkcje sondy.
struct workCounters {
  unsigned long long agseWindows  = 0;  ///< konstrukcje okna agregatu (wywołania constructAgsePayload)
  unsigned long long agseElements = 0;  ///< odwiedziny elementów okna - praca rosnąca z długością okna
  unsigned long long agseReads    = 0;  ///< odczyty rekordu źródła (revRead); mniej niż elementów, gdy cache trafia
  unsigned long long evalCalls    = 0;  ///< wywołania ewaluatora wyrażeń
  unsigned long long evalTokens   = 0;  ///< wykonania tokenów RPN - praca arytmetyczna
  unsigned long long hashPicks    = 0;  ///< wybory składowej przeplotu (STREAM_HASH)
  unsigned long long addMerges    = 0;  ///< scalenia payloadów sumy strumieni (STREAM_ADD)
};

/// Liczniki materializacji: ile rekordów i bajtów trafiło do magazynu (K6, §9.2).
///
/// Liczone są wyłącznie zapisy, które faktycznie dotarły do accessora; rekord all-null
/// pochłonięty przez detekcję gap nie jest materializacją. Nadpisanie nie zwiększa
/// objętości magazynu, tylko koszt zapisu - stąd rozdział dopisań i nadpisań.
///
/// Magazyn trwały i pamięciowy są liczone ROZDZIELNIE. Bez tego plan z `SUBSTRAT 'memory'`
/// raportowałby objętość materializacji, której nigdy nie zapisał na dysk - a to jest
/// dokładnie ta liczba, którą artykuł nazwałby „objętością materializacji".
/// @requirements-exempt materializationCounters: Zestaw licznikow fizycznych zapisow i bajtow materializacji.
struct materializationCounters {
  unsigned long long appends          = 0;
  unsigned long long overwrites       = 0;
  unsigned long long bytes            = 0;
  unsigned long long memoryAppends    = 0;
  unsigned long long memoryOverwrites = 0;
  unsigned long long memoryBytes      = 0;
};

/// Kanoniczne zapisy logiczne (K23/H9) - metryka PIERWOTNA kampanii K23, obok
/// `materializationCounters`, nie zamiast nich.
///
/// Trzy różnice wobec licznika natywnego wynikają wprost ze specyfikacji metryki:
///
/// 1. **bajty są kanoniczne, nie natywne** - liczone przez canonicalRecordBytes() wg
///    odwzorowania niezależnego od reprezentacji obu porównywanych systemów, wraz
///    z kanoniczną mapą NULL/luk. Natywne `mat_bytes` są nieporównywalne między
///    RetractorDB a Flinkiem, bo opisują dwa różne formaty rekordu.
/// 2. **rola strumienia jest rozdzielona** - metryka pyta o zapisy do materializowanego
///    podplanu (substraty), a mianownikiem jest liczba publicznych rekordów wyjściowych.
///    Licznik natywny jest globalny, więc jednej ani drugiej wielkości nie daje.
/// 3. **nadpisanie wnosi bajty** - jednostką jest RZECZYWISTY ZAPIS rekordu pośredniego,
///    nie przyrost objętości magazynu. Substrat na buforze kołowym (`SUBSTRAT 'memory'`)
///    po zawinięciu idzie ścieżką nadpisania; licznik natywny pokazałby tam zero bajtów,
///    czyli bramka mechanizmu przeszłaby, a metryka pierwotna skłamała. Dopisania
///    i nadpisania zostają rozdzielne, żeby ten reżim był widoczny w raporcie.
///
/// Zapis pochłonięty przez detekcję gap nie jest liczony - wynika to z położenia sondy
/// za `metaData_->absorbAppend()`, nie z osobnej reguły tutaj.
/// @requirements-exempt logicalWriteCounters: Zestaw licznikow logicznych zapisow strumieni i substratow.
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
// inkrementacja musi się inline'ować - skok do innej jednostki kompilacji byłby w pomiarze
// widoczny. Przy wyłączonej sondzie te obiekty nadal istnieją (kilkadziesiąt bajtów
// w .bss), ale nikt ich nie dotyka. Stan sondy planu jest zamknięty w probe.cc: jest zimny
// (raz na kompilację), a std::set w nagłówku oznaczałby dynamiczną inicjalizację w każdej
// jednostce kompilacji dołączającej ten plik - także tam, gdzie sond nie ma.
inline workCounters work{};
inline materializationCounters materialization{};
inline logicalWriteCounters logicalWrite{};

/// Funkcja printRuntimeCounters powinna:
/// - wypisywać na stderr wiersz MATERIALIZED przy ustawionym RDB_BENCH_MATERIALIZE i wiersz LOGICAL przy ustawionym RDB_BENCH_LOGICAL (oba tylko w buildzie z sondą materializacji) oraz wiersz WORK przy ustawionym RDB_BENCH_WORK (tylko w buildzie z sondą pracy).
///   Test: `probeReport.materialization_line_printed_when_armed`, `probeReport.logical_line_is_armed_by_its_own_variable`, `probeReport.work_line_printed_when_armed`, `probeReport.runtime_counters_are_silent_without_environment`; wiersze sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
void printRuntimeCounters();

/// Funkcja countRewriteR1 powinna:
/// - zwiększać o jeden licznik zastosowań reguły przepisującej R1.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - licznik czyta wylacznie raport planProbe.
void countRewriteR1();

/// Funkcja countRewriteR2 powinna:
/// - dodawać węzeł do zbioru węzłów, w których zadziałała reguła R2, licząc każdy węzeł raz.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - zbior czyta wylacznie raport planProbe.
void countRewriteR2(const std::string &node);

/// Funkcja countRewriteR3 powinna:
/// - zwiększać licznik przepisań reguły R3 o liczbę zastosowań.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - licznik czyta wylacznie raport planProbe.
void countRewriteR3(std::size_t applied);
}  // namespace detail

/// Funkcja onAgseWindow powinna:
/// - w buildzie z sondą pracy zwiększać o jeden licznik okien agregatu i o elements licznik odwiedzonych elementów, a w pozostałych nie robić nic.
///   Test: `probeWork.counters_accumulate_per_event`, `probeWork.counters_accumulate_across_slots`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
///
/// @note Okno odwiedza `elements` elementow w KAZDYM slocie, niezaleznie od tego, ile trafi w dostepny zakres.
[[gnu::always_inline]] inline void onAgseWindow(std::size_t elements) noexcept {
  if constexpr (rdb_probe_work) {
    ++detail::work.agseWindows;
    detail::work.agseElements += elements;
  }
}

/// Funkcja onAgseRead powinna:
/// - w buildzie z sondą pracy zwiększać o jeden licznik odczytów rekordów źródła w oknie, osobny od licznika odwiedzin, a w pozostałych nie robić nic.
///   Test: `probeWork.counters_accumulate_per_event`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
///
/// @note Cache `lastReadPosition` czesc odwiedzin omija, dlatego odczyty i odwiedziny sa rozdzielne.
[[gnu::always_inline]] inline void onAgseRead() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.agseReads;
}

/// Funkcja onEval powinna:
/// - w buildzie z sondą pracy zwiększać o jeden licznik wykonań programu wyrażenia i o tokens licznik wykonanych tokenów, a w pozostałych nie robić nic.
///   Test: `probeWork.counters_accumulate_per_event`, `probeWork.counters_accumulate_across_slots`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
///
/// @note Praca arytmetyczna slotu to liczba WYKONANYCH tokenow, nie rozmiar programu w planie.
[[gnu::always_inline]] inline void onEval(std::size_t tokens) noexcept {
  if constexpr (rdb_probe_work) {
    ++detail::work.evalCalls;
    detail::work.evalTokens += tokens;
  }
}

/// Funkcja onHashPick powinna:
/// - w buildzie z sondą pracy zwiększać o jeden licznik wyborów składowej przeplotu (STREAM_HASH), a w pozostałych nie robić nic.
///   Test: `probeWork.counters_accumulate_per_event`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
[[gnu::always_inline]] inline void onHashPick() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.hashPicks;
}

/// Funkcja onAddMerge powinna:
/// - w buildzie z sondą pracy zwiększać o jeden licznik scaleń payloadów sumy strumieni (STREAM_ADD), a w pozostałych nie robić nic.
///   Test: `probeWork.counters_accumulate_per_event`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
[[gnu::always_inline]] inline void onAddMerge() noexcept {
  if constexpr (rdb_probe_work) ++detail::work.addMerges;
}

/// Funkcja workReport powinna:
/// - zwracać kopię bieżących liczników pracy.
///   Test: `probeCounters.work_report_is_a_snapshot_and_reset_clears_seeded_state`, `probeWork.report_is_a_snapshot_not_a_view`.
workCounters workReport();

/// Funkcja workReset powinna:
/// - zerować wszystkie liczniki pracy.
///   Test: `probeCounters.work_report_is_a_snapshot_and_reset_clears_seeded_state`, `probeWork.reset_zeroes_every_counter`.
void workReset();

//
// ─── K6: objętość materializacji (rdb_probe_materialize) ────────────────────────
//

/// Funkcja onMaterializedAppend powinna:
/// - w buildzie z sondą materializacji zwiększać o jeden liczbę dopisań i o bytes liczbę bajtów magazynów pamięciowych albo trwałych, zależnie od memoryBacked, a w pozostałych nie robić nic.
///   Test: `probeMaterialization.persistent_and_memory_storage_counted_separately`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
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

/// Funkcja onMaterializedOverwrite powinna:
/// - w buildzie z sondą materializacji zwiększać o jeden liczbę nadpisań magazynów pamięciowych albo trwałych, zależnie od memoryBacked, a w pozostałych nie robić nic.
///   Test: `probeMaterialization.overwrite_costs_a_write_but_no_volume`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
[[gnu::always_inline]] inline void onMaterializedOverwrite(bool memoryBacked) noexcept {
  if constexpr (rdb_probe_materialize) {
    if (memoryBacked)
      ++detail::materialization.memoryOverwrites;
    else
      ++detail::materialization.overwrites;
  }
}

/// Funkcja materializationReport powinna:
/// - zwracać kopię bieżących liczników materializacji.
///   Test: `probeCounters.materialization_report_is_a_snapshot_and_reset_clears_seeded_state`.
materializationCounters materializationReport();

/// Funkcja materializationReset powinna:
/// - zerować wszystkie liczniki materializacji.
///   Test: `probeCounters.materialization_report_is_a_snapshot_and_reset_clears_seeded_state`, `probeMaterialization.reset_zeroes_every_counter`.
void materializationReset();

/// Funkcja canonicalRecordBytes powinna:
/// - zwracać kanoniczną szerokość rekordu opisanego deskryptorem: sumę kanonicznych szerokości pól i kanoniczną mapę NULL/luk o jednym bicie na slot płaski, zaokrągloną w górę do bajtu.
///   Test: `probeCanonicalRecord.integer_field_is_eight_bytes_plus_one_map_byte`, `probeCanonicalRecord.array_multiplies_the_canonical_width`, `probeCanonicalRecord.null_map_rounds_up_to_whole_bytes`, `probeCanonicalRecord.empty_descriptor_has_only_an_empty_null_map`.
/// - liczyć szerokość pola jako rarray razy: 1 dla BYTE, kCanonicalWideNumericBytes dla INTEGER, UINT i DOUBLE, kCanonicalFloatBytes dla FLOAT, kCanonicalPairBytes dla RATIONAL i INTPAIR oraz rlen dla STRING; dla IDXPAIR rlen razy rarray plus kCanonicalWideNumericBytes, a dla NULLTYPE i pól konfiguracyjnych 0.
///   Test: `probeCanonicalRecord.width_per_type_follows_the_frozen_mapping`, `probeCanonicalRecord.uint_and_pair_types_follow_the_frozen_mapping`, `probeCanonicalRecord.string_keeps_its_declared_width`, `probeCanonicalRecord.configuration_fields_and_nulltype_carry_no_width`; mnozenie przez rarray sprawdzone dla INTEGER i IDXPAIR, pola RETENTION i RETMEMORY bez przypadku.
/// - zależeć wyłącznie od deskryptora, nie od zawartości rekordu.
///   Test: `probeCanonicalRecord.result_is_independent_of_record_contents`, `probeLogicalGate.null_values_do_not_change_the_canonical_width`; ten drugi rozni zawartosc rekordu tylko w buildzie RDB_BENCH_PROBE=ON.
///
/// @note To jest serializer metryki pierwotnej K23 i jedyne miejsce, w ktorym wolno go zmienic; ta sama
///       definicja obowiazuje job Flinka, a predeklaracja kampanii ja zamraza.
std::size_t canonicalRecordBytes(const Descriptor &descriptor);

/// Funkcja onLogicalWrite powinna:
/// - w buildzie z sondą materializacji zwiększać o jeden licznik dopisań albo nadpisań (append) substratu albo wyniku publicznego (substrate) i o canonicalBytes licznik bajtów tej roli, a w pozostałych nie robić nic.
///   Test: `probeLogicalWrite.substrate_and_public_roles_counted_apart`, `probeLogicalWrite.overwrite_carries_bytes_unlike_the_native_counter`, `probeLogicalGate.overwrite_keeps_the_role_of_its_stream`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
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

/// Funkcja logicalWriteReport powinna:
/// - zwracać kopię bieżących liczników zapisów logicznych.
///   Test: `probeCounters.logical_write_report_is_a_snapshot_and_reset_clears_seeded_state`.
logicalWriteCounters logicalWriteReport();

/// Funkcja logicalWriteReset powinna:
/// - zerować wszystkie liczniki zapisów logicznych.
///   Test: `probeCounters.logical_write_report_is_a_snapshot_and_reset_clears_seeded_state`, `probeLogicalWrite.reset_zeroes_every_counter`.
void logicalWriteReset();

/// Funkcja reportRuntimeCounters powinna:
/// - w buildzie z sondą materializacji albo pracy wypisywać raport liczników przez printRuntimeCounters, a w pozostałych nie robić nic.
///   Test: `probeReport.work_line_printed_when_armed`, `probeReport.materialization_line_printed_when_armed`, `probeReport.logical_line_is_armed_by_its_own_variable`; galaz z sonda sprawdza tylko build RDB_BENCH_PROBE=ON (job CI ablation-probe-on).
///
/// @note Raport (K6 + K23 + E4) wolany po zakonczeniu mierzonej petli, zeby nie obciazal budzetu slotu.
///       Osobne zmienne srodowiskowe, bo wiersze opisuja rozne wielkosci.
[[gnu::always_inline]] inline void reportRuntimeCounters() {
  if constexpr (rdb_probe_materialize || rdb_probe_work) detail::printRuntimeCounters();
}

//
// ─── E1/E2E: budżet czasowy slotu (rdb_probe_slot) ──────────────────────────────
//

/// Sonda slotu: CSV z czasem obliczeń i latencją end-to-end, analizowany przez
/// examples/ecg/e1_stats.py. Kolumny:
///   compute_ns  - czas processRows() (E1, czysty rdzeń obliczeń),
///   wake_lag_ns - spóźnienie pobudki względem deadline'u slotu (kotwica + termin slotu;
///                 ten sam cel, do którego dąży rtAbsoluteSleep) - jitter planisty,
///   e2e_ns      - od deadline'u (nominalny moment pojawienia się krotki wejściowej
///                 w modelu czasowym) do końca emisji wyniku do kolejek IPC.
/// Aktywna dopiero, gdy `RDB_BENCH_CSV` wskazuje plik wyjściowy. Pętla śpi do tego samego
/// deadline'u w każdym trybie taktowanym, więc wake_lag/e2e nie kumulują dryfu także bez `-t`;
/// `-t` zmienia tylko szeregowanie (SCHED_FIFO, mlockall, powinowactwo CPU).
/// Obiekt klasy slotProbe powinien:
/// - w buildzie bez sondy slotu (rdb_probe_slot równe false) nie wykonywać żadnej operacji w open, anchor, beginSlot, beginCompute, endCompute, endSlot ani w destruktorze.
///   Test: `probeSlot.csv_has_header_and_one_row_per_slot`, `probeSlot.measured_intervals_keep_their_order`; jedynym obserwowalnym efektem jest plik CSV, ktorego build bez sondy nie tworzy mimo ustawionej zmiennej.
/// - w open otwierać do zapisu plik wskazany przez zmienną RDB_BENCH_CSV i zapisywać nagłówek iter,compute_ns,wake_lag_ns,e2e_ns, a bez tej zmiennej lub przy nieudanym otwarciu pozostawać nieuzbrojoną bez wpływu na pracę usługi.
///   Test: `probeSlot.csv_has_header_and_one_row_per_slot`, `probeSlot.disarmed_probe_writes_nothing_and_survives_use`, `probeSlot.unopenable_or_unopened_csv_leaves_the_probe_disarmed`; naglowek sprawdza tylko build RDB_BENCH_PROBE=ON.
/// - w anchor zapamiętywać kotwicę osi czasu, względem której wołający planuje sen slotu.
///   Test: `probeSlot.deadline_is_anchor_plus_offset_and_compute_spans_its_markers`; tylko build RDB_BENCH_PROBE=ON.
/// - w beginSlot zapamiętywać chwilę pobudki i termin slotu równy kotwicy powiększonej o przekazane przesunięcie w milisekundach.
///   Test: `probeSlot.deadline_is_anchor_plus_offset_and_compute_spans_its_markers`; tylko build RDB_BENCH_PROBE=ON.
/// - w beginCompute i endCompute zapamiętywać granice czasu obliczeń slotu.
///   Test: `probeSlot.deadline_is_anchor_plus_offset_and_compute_spans_its_markers`; tylko build RDB_BENCH_PROBE=ON.
/// - w endSlot dopisywać wiersz z kolejnym numerem slotu od zera, czasem obliczeń, spóźnieniem pobudki względem terminu oraz czasem od terminu do końca emisji wyniku, wszystkie w nanosekundach.
///   Test: `probeSlot.csv_has_header_and_one_row_per_slot`, `probeSlot.measured_intervals_keep_their_order`, `probeSlot.deadline_is_anchor_plus_offset_and_compute_spans_its_markers`; tylko build RDB_BENCH_PROBE=ON.
/// - w uzbrojonej sondzie nie zapisywać niczego, gdy plik nie został otwarty.
///   Test: `probeSlot.unopenable_or_unopened_csv_leaves_the_probe_disarmed`; uzbrojenie bez pliku ma skutek tylko w buildzie RDB_BENCH_PROBE=ON.
/// - zamykać plik CSV przy zniszczeniu i uniemożliwiać kopiowanie sondy.
///   Test: `probeSlot.csv_has_header_and_one_row_per_slot` (domkniecie pliku w destruktorze, tylko build RDB_BENCH_PROBE=ON); zakaz kopiowania: static_assert w test/UnitTest/test_probe.cpp.
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

  /// Kotwica osi czasu - dokładnie ta sama, względem której planowany jest sen slotu.
  [[gnu::always_inline]] void anchor(const std::timespec &origin) {
    if constexpr (rdb_probe_slot) setAnchor(origin);
  }

  /// Początek slotu: moment pobudki i deadline = kotwica + @p deadlineMs (termin slotu w ms
  /// od kotwicy - ten sam, który dostaje rtAbsoluteSleep).
  [[gnu::always_inline]] void beginSlot(long deadlineMs) {
    if constexpr (rdb_probe_slot) markWake(deadlineMs);
  }

  [[gnu::always_inline]] void beginCompute() {
    if constexpr (rdb_probe_slot) markComputeBegin();
  }

  [[gnu::always_inline]] void endCompute() {
    if constexpr (rdb_probe_slot) markComputeEnd();
  }

  /// Koniec slotu - po emisji wyniku; dopisuje wiersz CSV.
  [[gnu::always_inline]] void endSlot() {
    if constexpr (rdb_probe_slot) writeRow();
  }

 private:
  void openCsv();
  void closeCsv();
  void setAnchor(const std::timespec &origin);
  void markWake(long deadlineMs);
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
///   * strumienie publiczne i substraty osobno - substrat nie ma tożsamości obserwowalnej,
///     więc to on jest właściwą jednostką redukcji strukturalnej;
///   * tokeny drzewa FROM (query::lProgram) - tam widać efekt R1 i deduplikacji;
///   * tokeny programów pól (field::lProgram) - tam i TYLKO tam widać efekt R2.
/// Ostatni składnik jest konieczny: współdzielenie równoważnych SELECT przenosi kosztowny
/// program pól do jednego substratu, nie zmieniając ani jednego tokenu w lProgram.
/// @requirements-exempt planShape: Dane pomiaru rozmiaru planu, bez logiki pomiarowej.
struct planShape {
  std::size_t publicStreams = 0;
  std::size_t substrates    = 0;
  std::size_t fromTokens    = 0;
  std::size_t fieldTokens   = 0;
};

/// Rozmiar buforów planu (K6, §9.2) - wynik computeRequiredCapacities(). Suma jest
/// proporcjonalna do zajętości pamięci planu, maksimum wskazuje najgłębszy bufor,
/// czyli ten decydujący o najgorszym przypadku.
/// @requirements-exempt capacityShape: Dane pomiaru pojemnosci planu, bez logiki pomiarowej.
struct capacityShape {
  std::size_t streams = 0;
  std::size_t total   = 0;
  int max             = 0;
};

/// Etapy, na których zdejmowana jest migawka planu. Spadek liczby tokenów/strumieni
/// (zwłaszcza na etapie deduplikacji) to mierzalny efekt optymalizacji z sekcji E3.
enum class planStage : std::uint8_t {
  entry,      ///< surowy plan po parsowaniu
  preDedup,   ///< po kanonizacji do postaci pośredniej (dekompozycja)
  postDedup,  ///< po eliminacji zdublowanych substratów - właściwa redukcja planu
  exit        ///< końcowy, zoptymalizowany plan
};

/// Funkcja shapeOf powinna:
/// - zwracać kształt planu z pominięciem dyrektyw kompilatora: liczbę strumieni publicznych, liczbę substratów, łączną liczbę tokenów programów FROM i łączną liczbę tokenów programów pól.
///   Test: `probePlanShape.directives_are_not_part_of_the_plan`, `probePlanShape.substrates_counted_apart_from_public_streams`, `probePlanShape.field_tokens_counted_separately_from_from_tokens`, `probePlanShape.empty_plan_has_zero_shape`.
///
/// @note Szablon, bo typy planu naleza do biblioteki retractor, a sonda lezy poziom nizej, w rdb.
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

/// Funkcja shapeOfCapacities powinna:
/// - zwracać dla mapy pojemności historii liczbę strumieni, sumę pojemności i największą pojemność.
///   Test: `probePlanShape.capacities_summed_and_maximum_reported`, `probePlanShape.empty_capacity_map_has_zero_shape`.
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

/// Funkcja onRewriteR1 powinna:
/// - w buildzie z sondą planu liczyć zastosowanie reguły R1 (faktoryzacja przesunięć przez przeplot), a w pozostałych nie robić nic.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - bez sondy licznika nie czyta nic, wiec polowa "nie robic nic" nie ma odczytu.
[[gnu::always_inline]] inline void onRewriteR1() {
  if constexpr (rdb_probe_plan) detail::countRewriteR1();
}

/// Funkcja onRewriteR2 powinna:
/// - w buildzie z sondą planu liczyć węzeł, w którym zadziałała reguła R2 (przemienność ADD przy odciskach), raz na węzeł, a w pozostałych nie robić nic.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - bez sondy zbioru nie czyta nic, wiec polowa "nie robic nic" nie ma odczytu.
[[gnu::always_inline]] inline void onRewriteR2(const std::string &node) {
  if constexpr (rdb_probe_plan) detail::countRewriteR2(node);
}

/// Funkcja onRewriteR3 powinna:
/// - w buildzie z sondą planu doliczać liczbę przepisań reguły R3 (uproszczenia algebraiczne), a w pozostałych nie robić nic.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`; tylko build RDB_BENCH_PROBE=ON - bez sondy licznika nie czyta nic, wiec polowa "nie robic nic" nie ma odczytu.
///
/// @note Liczona suma przepisan, bo jedno wyrazenie moze zwinac wiele stalych naraz.
[[gnu::always_inline]] inline void onRewriteR3(std::size_t applied) {
  if constexpr (rdb_probe_plan) detail::countRewriteR3(applied);
}

/// Sonda planu, obiekt o czasie życia jednej kompilacji. Aktywna dopiero, gdy ustawiona
/// jest zmienna `RDB_BENCH_PLAN`; bez niej nie zdejmuje migawek i nie ma efektów ubocznych.
///
/// Czas instrumentacji jest ODEJMOWANY od czasu kompilacji: migawka iteruje cały plan,
/// więc jej koszt zależy od rozmiaru planu, a ten różni się między profilami ablacyjnymi
/// - bez odjęcia profil o większym planie wyglądałby na wolniejszy częściowo z powodu
/// własnego pomiaru.
/// Obiekt klasy planProbe powinien:
/// - w buildzie bez sondy planu (rdb_probe_plan równe false) nie wykonywać żadnej operacji w konstruktorze, capture ani report.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`, `probePlanReport.dedup_switch_is_visible_in_the_report`; jedynym obserwowalnym efektem jest raport, ktorego build bez sondy nie wypisuje mimo RDB_BENCH_PLAN.
/// - przy konstrukcji uzbrajać się wyłącznie, gdy ustawiona jest zmienna RDB_BENCH_PLAN, zerować liczniki przepisań R1, R2 i R3 oraz zapamiętywać początek pomiaru kompilacji.
///   Test: `probePlanReport.silent_without_environment`, `probePlanReport.construction_resets_rewrite_counters`, `probePlanReport.compile_time_runs_from_construction_less_the_probe_cost`; tylko build RDB_BENCH_PROBE=ON.
/// - w nieuzbrojonej sondzie nie zdejmować migawek i nie wypisywać raportu.
///   Test: `probePlanReport.silent_without_environment`; pominiecie samej migawki nie ma odczytu poza raportem.
/// - przez capture zapamiętywać dla wskazanego etapu (entry, preDedup, postDedup, exit) liczbę strumieni publicznych, substratów, tokenów FROM i tokenów programów pól, z pominięciem dyrektyw kompilatora, i doliczać czas migawki do kosztu sondy.
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`, `probePlanReport.compile_time_runs_from_construction_less_the_probe_cost`; tylko build RDB_BENCH_PROBE=ON, pomijanie dyrektyw sprawdza `probePlanShape.directives_are_not_part_of_the_plan` na shapeOf.
/// - przez report zdejmować migawkę etapu exit, a następnie wypisywać na stderr wiersze PLAN bench (cztery migawki i stan deduplikacji), REWRITE_APPLIED (r1, liczba węzłów z R2, r3), COMPILE_NS z czasem kompilacji pomniejszonym o koszt sondy oraz PLAN capacity (liczba strumieni, suma i maksimum pojemności).
///   Test: `probePlanReport.stages_and_rewrites_reported_when_armed`, `probePlanReport.dedup_switch_is_visible_in_the_report`, `probePlanReport.compile_time_runs_from_construction_less_the_probe_cost`; tylko build RDB_BENCH_PROBE=ON.
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
  std::array<planShape, 4> stages_{};
};

}  // namespace rdb::probe
