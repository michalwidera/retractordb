#pragma once

#include <unistd.h>  // for ::lseek, ::write, ::close

#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <string_view>
#include <vector>

#include "rdb/error.hpp"
#include "rdb/payload.hpp"

/// Obiekt klasy dumpTask powinien:
/// - przechowywać nazwę reguły, zakres zrzutu (od, do), liczbę zachowywanych plików RETENTION oraz stan wykonania: liczbę rekordów pozostałych do zapisu, liczbę rekordów opóźnienia, nazwę pliku i deskryptor.
///   Kontrola strukturalna: pola struktury dumpTask; samo przechowywanie danych nie ma osobnego testu.
/// - przy konstrukcji przyjmować nazwę, zakres i RETENTION, z deskryptorem równym -1 i zerowymi licznikami.
///   Test: `dumpTask.constructor_keeps_rule_and_starts_closed_with_zero_counters`.
/// - uniemożliwiać kopiowanie i przy przenoszeniu przekazywać deskryptor nowemu obiektowi, ustawiając go w źródle na -1, a przy przypisaniu przenoszącym najpierw zamykać własny otwarty deskryptor.
///   Sprawdzenie kompilacji: static_assert w test_dumpManager.cpp (brak kopiowania, przenoszenie noexcept). Test: `dumpTask.move_constructor_transfers_fd`, `dumpTask.move_assignment_closes_old_fd_and_transfers`.
/// - zamykać otwarty deskryptor przy zniszczeniu.
///   Test: `dumpTask.destructor_closes_fd`, `dumpTask.destructor_with_negative_fd_does_not_crash`.
struct dumpTask {
  // from rule definition
  std::string taskName;                       // name of task
  std::pair<long int, long int> range{0, 0};  // first - from, second - to
  size_t retentionSize{0};                    // How many dumps to retain

  // configuration
  int dumpedRecordsToGo{0};     // How many records left to dump - 0 close task
  std::string dumpFilename;     // name of dump file
  int fd{-1};                   // file descriptor Linux posix file handle
  int delayDumpRecordsToGo{0};  // How many records to delay the dump ( for range starting in future )

  explicit dumpTask(std::string name, std::pair<long int, long int> rangeParam, size_t retention)
      : taskName(std::move(name)),
        range(rangeParam),
        retentionSize(retention) {}

  dumpTask(const dumpTask &)            = delete;
  dumpTask &operator=(const dumpTask &) = delete;
  dumpTask(dumpTask &&other) noexcept;
  dumpTask &operator=(dumpTask &&other) noexcept;
  ~dumpTask();
};

/// Obiekt klasy dumpManager powinien:
/// - tworzyć pliki zrzutu w katalogu ustawionym przez setDumpStorage, pod nazwą <strumień>_<reguła>_dump.tmp dla reguły bez RETENTION albo <strumień>_<reguła>_dump_<k>.tmp dla RETENTION N, gdzie k to kolejne wyzwolenia tej reguły modulo N.
///   Test: `DumpManagerFileTest.setDumpStorage_sets_path`, `DumpManagerFileTest.createDumpFile_without_retention_creates_tmp_suffix`, `DumpManagerFileTest.createDumpFile_with_retention_creates_numbered_files`, `DumpManagerTaskTest.registerTask_retains_completed_dump_files_and_contents`, `DumpManagerFileTest.retention_counters_are_kept_per_stream_and_rule`.
/// - tworzyć plik zrzutu wyłącznie jako nowy i-węzeł: usuwać poprzedni plik pod tą nazwą, otwierać z O_CREAT, O_EXCL, O_NOFOLLOW i O_CLOEXEC z trybem 0644 i ponawiać po wyścigu z innym twórcą, nie więcej niż kMaxRecreates razy.
///   Test: `DumpManagerFileTest.createDumpFile_replaces_previous_file_with_a_fresh_inode`, `DumpManagerFileTest.createDumpFile_retries_a_competing_creator_at_most_100_times`, `it_dump_nofollow-run`.
/// - przy rejestracji zadania usuwać z księgi strumienia zadania piszące do tego samego pliku, tak aby każda reguła miała w księdze najwyżej tyle zadań, ile ma nazw plików.
///   Test: `DumpManagerTaskTest.unretained_replacement_closes_only_same_rule_tasks`, `DumpManagerTaskTest.retention_wrap_closes_task_of_reused_slot`, `DumpManagerTaskTest.unretained_rules_do_not_evict_each_other`.
/// - traktowac zakres zrzutu jako polotwarty [poczatek, koniec); dla ujemnego poczatku zapisywac przy rejestracji
///   min(koniec, 0) - poczatek rekordow historii od najstarszego do najnowszego, a czesc nieujemna po jednym rekordzie
///   na slot, zaczynajac od rekordu biezacego w slocie wyzwolenia.
///   DUMP -5 TO -2 obejmuje wylacznie rekordy -5, -4 i -3.
///   Test: `DumpManagerPastRangeTest.past_only_range_dumps_exactly_its_history_records`, `DumpManagerHistoryTest.compiled_rule_dumps_history_and_current_record_bytes`, `DumpManagerTaskTest.unretained_replacement_closes_only_same_rule_tasks`.
/// - dla zakresu o nieujemnym początku pomijać tyle kolejnych slotów, ile wynosi początek, a następnie zapisywać po jednym bieżącym rekordzie strumienia w slocie do wyczerpania zakresu.
///   Test: `dumpManager.buildDumpChunk_delay_decrements_returns_false`, `dumpManager.buildDumpChunk_delay_exhausted_then_writes`, `DumpManagerModelTest.processStreamChunk_does_no_work_without_tasks_or_records`, `DumpManagerTaskTest.unretained_rules_do_not_evict_each_other`.
/// - w processStreamChunk nie wykonywać żadnej pracy dla strumienia bez zadań ani dla strumienia bez rekordów, a po wyczerpaniu zakresu zamykać plik zadania i usuwać zadanie z księgi.
///   Test: `DumpManagerModelTest.processStreamChunk_does_no_work_without_tasks_or_records`, `DumpManagerTaskTest.registerTask_retains_completed_dump_files_and_contents`.
/// - dopisywac payload do konca pliku pojedynczym write z zadanym rozmiarem deskryptora strumienia; kazdy dodatni wynik
///   uznawac za zapis rekordu, bez sprawdzania pelnej liczby zapisanych bajtow i bez ponawiania niepelnego zapisu.
///   Test: `dumpManager.buildDumpChunk_counts_any_positive_write_and_reports_failures`, `dumpManager.buildDumpChunk_accepts_fd_zero_as_valid`, `dumpManager.buildDumpChunk_multiple_records_not_done_until_last`.
/// - zwracać Errc::IO z kodem systemowym przy błędzie usunięcia, utworzenia, pozycjonowania lub zapisu pliku zrzutu.
///   Test: `DumpManagerFileTest.createDumpFile_reports_remove_and_create_failures_with_errno`, `dumpManager.buildDumpChunk_counts_any_positive_write_and_reports_failures`, `DumpManagerModelTest.registerTask_history_dump_reports_seek_and_write_failures`, `DumpManagerFileTest.createDumpFile_retries_a_competing_creator_at_most_100_times`.
class dumpManager {
 public:
  dumpManager()  = default;
  ~dumpManager() = default;

  [[nodiscard]] rdb::Result<> registerTask(const std::string &streamName, dumpTask task);  // Register a dump function
  [[nodiscard]] rdb::Result<> processStreamChunk(const std::string &streamName);           // Call all registered dump functions
  void setDumpStorage(std::string storagePathParam);                                       // Set storage path for dump files

 private:
  std::map<std::string, int> retentionCounter;  // first - streamName_taskName, second - counter
  std::map<std::string, int> retentionSize;     // first - streamName_taskName, second - retention size
  std::string storagePath;
  std::map<std::string, std::vector<dumpTask>> bookOfTasks;  // streamName -> list of tasks

  [[nodiscard]] rdb::Result<bool> buildDumpChunk(
      dumpTask &task,
      std::unique_ptr<rdb::payload>::pointer payload);  // Execute dump task - return true if task is completed
  [[nodiscard]] rdb::Result<std::pair<std::string, int>> createDumpFile(std::string_view streamName, std::string_view taskName);
};
