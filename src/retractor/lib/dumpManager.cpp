#include "dumpManager.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>  // std::min
#include <cstdlib>    // std::abs
#include <cstring>    // strerror
#include <filesystem>

#include <spdlog/spdlog.h>

#include "dataModel.hpp"
#include "executorsmState.hpp"
#include "fatalError.hpp"

namespace {
constexpr mode_t kDefaultDumpFileMode = 0644;
}

dumpTask::dumpTask(dumpTask &&other) noexcept
    : taskName(std::move(other.taskName)),
      range(other.range),
      retentionSize(other.retentionSize),
      dumpedRecordsToGo(other.dumpedRecordsToGo),
      dumpFilename(std::move(other.dumpFilename)),
      fd(other.fd),
      delayDumpRecordsToGo(other.delayDumpRecordsToGo) {
  other.fd = -1;
}

dumpTask &dumpTask::operator=(dumpTask &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (fd >= 0) {
    ::close(fd);
  }

  taskName             = std::move(other.taskName);
  range                = other.range;
  retentionSize        = other.retentionSize;
  dumpedRecordsToGo    = other.dumpedRecordsToGo;
  dumpFilename         = std::move(other.dumpFilename);
  fd                   = other.fd;
  delayDumpRecordsToGo = other.delayDumpRecordsToGo;

  other.fd = -1;
  return *this;
}

dumpTask::~dumpTask() {
  if (fd >= 0) {
    ::close(fd);
  }
}

void dumpManager::registerTask(const std::string &streamName, dumpTask task) {
  if (pProc == nullptr) FatalError("dumpManager::registerTask: dataModel pointer is null");
  if (!pProc->qSet.contains(streamName)) {
    FatalError("dumpManager::registerTask: stream '{}' not found in dataModel", streamName);
  }
  if (task.range.first > task.range.second) {
    FatalError("dumpManager::registerTask: range.first {} > range.second {} for stream '{}'", task.range.first,
               task.range.second, streamName);
  }

  // createDumpFile wybiera nazwe po retentionSize, wiec wpis musi byc przed nim. Do #379 stal po
  // nim i pierwsze wyzwolenie reguly z RETENTION trafialo do _dump.tmp zamiast _dump_0.tmp.
  retentionSize[streamName + "_" + task.taskName] = static_cast<int>(task.retentionSize);
  std::tie(task.dumpFilename, task.fd)            = createDumpFile(streamName, task.taskName);
  // Nowy plik zastapil poprzedni pod ta sama nazwa: regula bez RETENTION odtwarza go przy kazdym
  // wyzwoleniu, regula z RETENTION N przy zawinieciu licznika slotow. Zadania tego pliku pisalyby
  // dalej do odlaczonego i-wezla, wiec je usuwamy; move/destruktor dumpTask zamyka ich deskryptory.
  // Kazda regula ma wiec w ksiedze najwyzej tyle zadan, ile ma nazw plikow (1 albo N), i ksiega nie
  // potrzebuje wlasnej pojemnosci. Do #380 byl nia circular_buffer o pojemnosci najwiekszego
  // RETENTION na strumieniu, a jego pelnosc wypychala najstarsze zadanie dowolnej reguly - takze
  // cudze, ucinajac jego widoczny zrzut (np. dwie reguly bez RETENTION przy pojemnosci 1).
  auto &tasks = bookOfTasks[streamName];
  auto removed =
      std::ranges::remove_if(tasks, [&](const dumpTask &previous) { return previous.dumpFilename == task.dumpFilename; });
  tasks.erase(removed.begin(), removed.end());
  task.dumpedRecordsToGo = static_cast<int>(abs(task.range.second - task.range.first));

  if (task.range.first < 0) {
    // Filling dump with data already in stream history
    // we need to dump abs(range.first) records from history
    // if range.first is 0 or positive - no history dump needed
    // CHECK IF WE HAVE ENOUGH HISTORY
    // CHECK SEQUENCE IF THIS IS IN REVERSE ORDER
    int dumpHistoryCount = static_cast<int>(abs(task.range.first));
    for (auto i = 0; i < dumpHistoryCount; ++i) {
      auto *payLoadPtr = pProc->getPayload(streamName, dumpHistoryCount - i);
      auto resultSeek  = ::lseek(task.fd, 0, SEEK_END);
      if (resultSeek == -1) FatalError("dumpManager::registerTask: lseek failed during history dump");
      ssize_t write_count_result = ::write(task.fd, payLoadPtr->span().data(), payLoadPtr->descriptor.getSizeInBytes());
      if (write_count_result <= 0) FatalError("dumpManager::registerTask: write failed during history dump");
    }
    if (task.dumpedRecordsToGo < dumpHistoryCount) {
      FatalError("dumpManager::registerTask: dumpedRecordsToGo {} < dumpHistoryCount {}", task.dumpedRecordsToGo,
                 dumpHistoryCount);
    }
    task.dumpedRecordsToGo -= dumpHistoryCount;
  } else {
    task.delayDumpRecordsToGo = static_cast<int>(task.range.first);
  }

  tasks.push_back(std::move(task));
}

void dumpManager::setDumpStorage(std::string storagePathParam) { storagePath = std::move(storagePathParam); }

void dumpManager::processStreamChunk(const std::string &streamName) {
  if (pProc == nullptr) FatalError("dumpManager::processStreamChunk: dataModel pointer is null");
  if (!pProc->qSet.contains(streamName)) {
    FatalError("dumpManager::processStreamChunk: stream '{}' not found in dataModel", streamName);
  }
  if (!bookOfTasks.contains(streamName)) return;

  auto currentStreamCount = pProc->getStreamCount(streamName);
  if (currentStreamCount == 0) return;  // nothing to dump

  auto *payLoadPtr = pProc->getPayload(streamName);

  if (payLoadPtr->descriptor.getSizeInBytes() == 0)
    FatalError("dumpManager::processStreamChunk: payload descriptor size is zero");
  if (payLoadPtr->span().empty()) FatalError("dumpManager::processStreamChunk: payload data span is empty");

  // enumerate all tasks for this stream
  for (auto &task : bookOfTasks[streamName]) {
    if (task.dumpedRecordsToGo == 0) continue;  // already completed task - will be removed later

    if (task.fd < 0) {
      SPDLOG_ERROR("dumpManager::processStreamChunk file descriptor is not set for stream: {}", streamName);
      continue;
    }
    auto dumpTaskCompleted = buildDumpChunk(task, payLoadPtr);

    if (dumpTaskCompleted) {
      ::close(task.fd);
      task.fd = -1;  // mark fd in task as closed
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
}

bool dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  if (task.dumpedRecordsToGo < 0) FatalError("dumpManager::buildDumpChunk: dumpedRecordsToGo is negative");
  if (task.delayDumpRecordsToGo < 0) FatalError("dumpManager::buildDumpChunk: delayDumpRecordsToGo is negative");
  if (task.fd < 0) FatalError("dumpManager::buildDumpChunk: file descriptor is not set");

  // tutaj trzeba będzie opóźnić zrzut danych do pliku jeśli range określa tylko zrzut w przyszłości np. range 2 to 4
  if (task.delayDumpRecordsToGo != 0) {
    task.delayDumpRecordsToGo--;
    return false;
  }

  auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
  if (resultSeek == -1) FatalError("dumpManager::buildDumpChunk: lseek to end failed");

  ssize_t write_count_result = ::write(task.fd, payload->span().data(), payload->descriptor.getSizeInBytes());
  if (write_count_result <= 0) FatalError("dumpManager::buildDumpChunk: write failed");

  if (task.dumpedRecordsToGo > 0) {
    task.dumpedRecordsToGo--;
  }

  return (task.dumpedRecordsToGo == 0);
}

std::pair<std::string, int> dumpManager::createDumpFile(const std::string_view streamName, const std::string_view taskName) {
  // Klucz to rdzen nazwy pliku. Bez separatora pary a/bc i ab/c dzielily licznik slotow i RETENTION,
  // choc ich pliki sa rozne; rowny rdzen dwoch roznych par odrzuca juz parser.
  std::string key = std::string(streamName) + "_" + std::string(taskName);
  auto filename =
      std::filesystem::path(storagePath) / std::filesystem::path(std::string(streamName) + "_" + std::string(taskName));
  if (retentionSize[key] == 0) {
    filename += "_dump.tmp";
  } else {
    auto ret = (retentionCounter[key]++) % retentionSize[key];
    filename += "_dump_" + std::to_string(ret) + ".tmp";
    if (ret >= retentionSize[key]) {
      FatalError("dumpManager::createDumpFile: retention counter out of bounds: {} >= {} for key '{}'", ret, retentionSize[key],
                 key);
    }
  }
  // Sciezka zrzutu jest w calosci przewidywalna z tekstu planu (STORAGE, strumien, regula), wiec
  // ktos z prawem zapisu do katalogu moze ja zajac przed nami. O_TRUNC obcina to, na co nazwa
  // WSKAZUJE: cel dowiazania symbolicznego albo i-wezel wspoldzielony przez dowiazanie twarde -
  // takze z O_NOFOLLOW, ktore zatrzymuje tylko to pierwsze. Dlatego poprzedni plik kasujemy
  // (unlink nie idzie za dowiazaniem) i tworzymy nowy przez O_EXCL: otwarcie udaje sie tylko na
  // i-wezle, ktory sami wlasnie utworzylismy. Kontrola stillLinked (jak w lockFile.cpp) nie jest
  // potrzebna: przez zrzut nikt sie nie synchronizuje, a podmiana nazwy po otwarciu kosztuje
  // tylko nasz wlasny zrzut, nie cudzy plik. O_CLOEXEC - deskryptor nie trafia do polecen
  // uruchamianych przez DO SYSTEM.
  constexpr int kMaxRecreates = 100;
  for (int attempt = 0; attempt < kMaxRecreates; ++attempt) {
    if (::unlink(filename.c_str()) != 0 && errno != ENOENT) {
      FatalError("dumpManager::createDumpFile: failed to remove '{}': {}", filename.string(), strerror(errno));
    }
    int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, kDefaultDumpFileMode);
    if (fd >= 0) return std::make_pair(filename, fd);
    if (errno != EEXIST) break;
    // Ktos utworzyl plik miedzy naszym unlink() a open(); kasujemy go jeszcze raz.
  }
  FatalError("dumpManager::createDumpFile: failed to create '{}': {}", filename.string(), strerror(errno));
}
