#include "dumpManager.h"

#include <assert.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>  // std::min
#include <cstdlib>    // std::abs
#include <filesystem>
#include <iostream>

#include "dataModel.h"

/** WORK IN PROGRESS - DO NOT USE **/

/*
std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }
std::string removeSpc(std::string input) { return std::regex_replace(input, std::regex(R"(\s+)"), " "); }
*/

// dumpManager.cpp
// dumpManager.h

// musi dojść do wywołania dump managera - lub wysłania do niego zadania
// musi nastąpić zrzut danych ze struminia do pliku (materializacja)
// tworzone pliki zrzutów jeśli retencja > 0 muszą podlegać retencji
// w obiekcie r.dump_retention jest informacja o retencji - ile plików zrzutów ma być zachowanych
// w obiekcie r.dumpRange jest informacja o zakresie zrzutu
// r.dumpRange.first - ile rekordów wstecz
// r.dumpRange.second - ile rekordów w przód
// etap kompilacji zapewnia że first <= second

// dump manager - osobna klasa - singleton
// dump manager - zarządza plikami zrzutów
// dump manager - zarządza retencją plików zrzutów
// dump manager - zapewnia unikalne nazwy plików zrzutów
// dump manager - zapewnia miejsce na dysku dla plików zrzutów
// dump manager - zapewnia format plików zrzutów (np. csv, json, binarny)
// dump manager - zapewnia metadane plików zrzutów (np. nagłówki, typy danych)
// dump manager - zapewnia indeksowanie plików zrzutów (np. po czasie, po kluczu)
// dump manager - zapewnia wyszukiwanie plików zrzutów
// dump manager - zapewnia usuwanie starych plików zrzutów
// dump manager - zapewnia raportowanie stanu plików zrzutów
// dump manager - zapewnia konfigurację (np. katalog zrzutów, format plików, retencja)
// dump manager - zapewnia logowanie operacji zrzutów
// dump manager - zapewnia obsługę błędów (np. brak miejsca na dysku, błędy zapisu)
// dump manager - zapewnia testowanie (np. jednostkowe, integracyjne)
// dump manager - zapewnia dokumentację
// dump manager - zapewnia bezpieczeństwo (np. uprawnienia, szyfrowanie)
// dump manager - zapewnia wydajność (np. buforowanie, asynchroniczność)
// dump manager - zapewnia integrację (np. z innymi systemami, z chmurą)
// dump manager - zapewnia rozwój (np. nowe funkcje, poprawki błędów)
// dump manager - zapewnia wsparcie (np. techniczne, społecznościowe)
// dump manager - zapewnia monitoring (np. metryki, alerty)

/* szkicowy algorytm - do poprawy i optymalizacji
auto recordsCount = outputPayload->getRecordsCount();
auto left = r.dumpRange.first < 0 ? 0 : r.dumpRange.first;
auto right = r.dumpRange.second < 0 ? 0 : r.dumpRange.second;
left = std::min(left, recordsCount - 1);
right = std::min(right, recordsCount - 1);
if (left > right) {
  SPDLOG_ERROR("Rule/Dump: Dump left range cannot be greater than dump right range");
  abort();
}
SPDLOG_INFO("Rule/Dump: Dumping from {} to {} of stream {}", left, right, qry.id);
for (auto i = left; i <= right; ++i) {
  outputPayload->read(i);
  // TODO - send to dump manager
  // TODO - retention of dump files
  std::cout << *(outputPayload->getPayload()) << std::endl;
}
*/

std::map<std::string, int> retentionCounter;  // first - streamName+taskName, second - counter
std::map<std::string, int> retentionSize;     // first - streamName+taskName, second - retention size

extern dataModel *pProc;

void dumpManager::registerTask(const std::string streamName, dumpTask task) {
  assert(pProc != nullptr && "dumpManager::registerTask dataModel is not set");
  assert(pProc->qSet.find(streamName) != pProc->qSet.end() && "dumpManager::registerTask streamName not found in dataModel");
  assert(task.range.first <= task.range.second && "dumpManager::registerTask range.first must be <= range.second");

  std::tie(task.dumpFilename, task.fd)      = createDumpFile(streamName, task.taskName);
  task.dumpedRecordsToGo                    = abs(task.range.second - task.range.first);
  retentionSize[streamName + task.taskName] = task.retentionSize;
  if (bookOfTasks[streamName].capacity() == 0) {
    bookOfTasks[streamName].set_capacity(task.retentionSize > 0 ? task.retentionSize : 1);
  }
  bookOfTasks[streamName].push_back(task);
  // This push_back will overwrite oldest task if retentionSize is exceeded
  // Task destructor will close file descriptor if still open

  if (task.range.first < 0) {
    // Filling dump with data already in stream history
    // we need to dump abs(range.first) records from history
    // if range.first is 0 or positive - no history dump needed
    size_t dumpHistoryCount   = abs(task.range.first);
    size_t currentStreamCount = pProc->getStreamCount(streamName);
    for (auto revOffset = std::min(dumpHistoryCount, currentStreamCount); revOffset >= 0; --revOffset) {
      auto payLoadPtr = pProc->getPayload(streamName, revOffset);
      auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
      assert(resultSeek != -1);
      ssize_t write_count_result = ::write(task.fd, payLoadPtr->get(), payLoadPtr->getDescriptor().getSizeInBytes());
      assert(write_count_result > 0);
    }
    assert(task.dumpedRecordsToGo >= dumpHistoryCount);
    task.dumpedRecordsToGo -= dumpHistoryCount;
  } else {
    task.delayDumpRecordsToGo = task.range.first;
  }
}

void dumpManager::setDumpStorage(const std::string storagePathParam) {
  storagePath = storagePathParam;
}

void dumpManager::processStreamChunk(const std::string streamName) {
  assert(pProc != nullptr && "dumpManager::processStreamChunk dataModel is not set");
  assert(pProc->qSet.find(streamName) != pProc->qSet.end() &&
         "dumpManager::processStreamChunk streamName not found in dataModel");
  assert(bookOfTasks.find(streamName) != bookOfTasks.end() &&
         "dumpManager::processStreamChunk streamName not found in bookOfTasks");

  auto currentStreamCount = pProc->getStreamCount(streamName);
  if (currentStreamCount == 0) return;  // nothing to dump

  auto payLoadPtr = pProc->getPayload(streamName);

  // enumerate all tasks for this stream
  for (auto &task : bookOfTasks[streamName]) {
    // Here we would implement the actual dump logic
    // TODO: Implement dump logic
    // For now, just print the task details
    std::cout << "Dumping chunk stream: " << task.taskName  //
              << " Range: [" << task.range.first            //
              << ", " << task.range.second << "]"           //
              << " Retention: " << task.retentionSize       //
              << std::endl;

    auto status = buildDumpChunk(task, payLoadPtr);

    if (status) {
      std::cout << "Dump completed successfully for stream: " << task.taskName << std::endl;
    } else {
      std::cerr << "Dump still in process: " << task.taskName << std::endl;
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
}

bool dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  assert(payload != nullptr && "dumpManager::buildDumpChunk payload is null");
  assert(task.fd != 0 && "dumpManager::buildDumpChunk file descriptor is not set");
  assert(task.dumpedRecordsToGo >= 0 && "dumpManager::buildDumpChunk dumpedRecordsToGo is negative");
  assert(task.delayDumpRecordsToGo >= 0 && "dumpManager::buildDumpChunk delayDumpRecordsToGo is negative");

  // tutaj trzeba będzie opóźnić zrzut danych do pliku jeśli range określa tylko zrzut w przyszłości np. range 2 to 4
  if (task.delayDumpRecordsToGo != 0) {
    SPDLOG_INFO("DumpManager: delaying dump for task {} by {} records", task.taskName, task.delayDumpRecordsToGo);
    task.delayDumpRecordsToGo--;
    return false;
  }

  auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
  assert(resultSeek != -1);
  ssize_t write_count_result = ::write(task.fd, payload->get(), payload->getDescriptor().getSizeInBytes());
  assert(write_count_result > 0);

  if (task.dumpedRecordsToGo > 0) {
    task.dumpedRecordsToGo--;
  }

  if (task.dumpedRecordsToGo == 0) {  // remove task form bookOfTasks in processStreamChunk
    ::close(task.fd);
    task.fd = 0;
    return true;  // task completed
  }
  return false;
}

std::pair<std::string, int> dumpManager::createDumpFile(std::string streamName, std::string taskName) {
  auto filename = std::filesystem::path(storagePath) / std::filesystem::path(streamName + "_" + taskName);
  SPDLOG_DEBUG("Creating dump file: {}", filename);
  if (retentionSize[streamName + taskName] == 0) {
    filename += "_dump.tmp";
  } else {
    auto ret = (retentionCounter[streamName + taskName]++) % retentionSize[streamName + taskName];
    filename += "_dump_" + std::to_string(ret) + ".tmp";
    assert(ret < retentionSize[streamName + taskName]);
  }
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_TRUNC, 0644);
  assert(fd >= 0);
  return std::make_pair(filename, fd);
}