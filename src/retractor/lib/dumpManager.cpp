#include "dumpManager.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>  // std::min
#include <cerrno>     // errno
#include <cstdlib>    // std::abs
#include <cstring>    // strerror
#include <filesystem>
#include <format>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "dataModel.hpp"
#include "executorsmState.hpp"
#include "rdb/error.hpp"

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

rdb::Result<> dumpManager::registerTask(const std::string &streamName, dumpTask task) {
  // Model publikuje petla demona; silnik osadzony odrzuca reguly DUMP przy compile() - oba to niezmienniki.
  RDB_ASSERT(pProc != nullptr, "dumpManager::registerTask: dataModel pointer is null");
  RDB_ASSERT(pProc->qSet.contains(streamName), "dumpManager::registerTask: stream '{}' not found in dataModel", streamName);
  // Niezmiennik, nie wejscie: zakres jest bramkowany DWUKROTNIE, w parserze (RQLParser.cpp,
  // komunikat dla uzytkownika) i w compiler::computeRequiredCapacities. Zaden tekst RQL nie
  // dociera tu z pustym zakresem.
  RDB_ASSERT(task.range.first <= task.range.second,
             "dumpManager::registerTask: range.first {} > range.second {} for stream '{}'", task.range.first, task.range.second,
             streamName);

  // createDumpFile wybiera nazwe po retentionSize, wiec wpis musi byc przed nim. Do #379 stal po
  // nim i pierwsze wyzwolenie reguly z RETENTION trafialo do _dump.tmp zamiast _dump_0.tmp.
  retentionSize[streamName + "_" + task.taskName] = static_cast<int>(task.retentionSize);
  RDB_TRY_ASSIGN(std::tie(task.dumpFilename, task.fd), createDumpFile(streamName, task.taskName));
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
    //
    // Zakres jest polotwarty [first, second) wzgledem rekordu biezacego (0). Prawa granica ujemna
    // (`DUMP -5 TO -2` = rekordy -5, -4, -3) lezy w calosci w historii: zrzut konczy sie tutaj, a
    // zadanie z dumpedRecordsToGo == 0 usuwa najblizszy processStreamChunk. Do 2026-10 petla
    // zrzucala zawsze abs(first) rekordow, czyli do -1, i tak zakres konczyl sie rdb::fatal.
    const int dumpHistoryCount = static_cast<int>(abs(task.range.first));
    const int historyEnd       = task.range.second < 0 ? static_cast<int>(abs(task.range.second)) : 0;
    for (auto i = 0; i < dumpHistoryCount - historyEnd; ++i) {
      RDB_TRY_ASSIGN(auto *payLoadPtr, pProc->getPayload(streamName, dumpHistoryCount - i));
      auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
      if (resultSeek == -1)
        return rdb::fail(rdb::Errc::IO,
                         fmt::format("dumpManager::registerTask: lseek on '{}' failed during history dump: {}",
                                     task.dumpFilename, strerror(errno)),
                         errno != 0 ? errno : EIO);
      ssize_t write_count_result = ::write(task.fd, payLoadPtr->span().data(), payLoadPtr->descriptor.getSizeInBytes());
      if (write_count_result <= 0)
        return rdb::fail(rdb::Errc::IO,
                         fmt::format("dumpManager::registerTask: write to '{}' failed during history dump: {}",
                                     task.dumpFilename, strerror(errno)),
                         errno != 0 ? errno : EIO);
    }
    // Niezmiennik, nie wejscie: zakres niepusty (first < second) gwarantuje parser, wiec zrzucona
    // historia nigdy nie przekracza dlugosci zakresu.
    RDB_ASSERT(task.dumpedRecordsToGo >= dumpHistoryCount - historyEnd,
               "dumpManager::registerTask: dumpedRecordsToGo {} < history records dumped {}", task.dumpedRecordsToGo,
               dumpHistoryCount - historyEnd);
    task.dumpedRecordsToGo -= dumpHistoryCount - historyEnd;
  } else {
    task.delayDumpRecordsToGo = static_cast<int>(task.range.first);
  }

  tasks.push_back(std::move(task));
  return {};
}

void dumpManager::setDumpStorage(std::string storagePathParam) { storagePath = std::move(storagePathParam); }

rdb::Result<> dumpManager::processStreamChunk(const std::string &streamName) {
  // Brak zadan PRZED siegnieciem po pProc: streamInstance wola te funkcje w kazdym slocie dla
  // kazdego strumienia, a globalny pProc publikuje wylacznie petla demona. Silnik osadzony
  // (rdb::embed::Engine) go nie publikuje i odrzuca reguly DUMP przy compile(), wiec dla
  // niego ta ksiega jest zawsze pusta - i wtedy wskaznik nie jest do niczego potrzebny.
  if (!bookOfTasks.contains(streamName)) return {};
  RDB_ASSERT(pProc != nullptr, "dumpManager::processStreamChunk: dataModel pointer is null");
  RDB_ASSERT(pProc->qSet.contains(streamName), "dumpManager::processStreamChunk: stream '{}' not found in dataModel",
             streamName);

  auto currentStreamCount = pProc->getStreamCount(streamName);
  if (currentStreamCount == 0) return {};  // nothing to dump

  RDB_TRY_ASSIGN(auto *payLoadPtr, pProc->getPayload(streamName));

  RDB_ASSERT(payLoadPtr->descriptor.getSizeInBytes() != 0, "dumpManager::processStreamChunk: payload descriptor size is zero");
  RDB_ASSERT(!payLoadPtr->span().empty(), "dumpManager::processStreamChunk: payload data span is empty");

  // enumerate all tasks for this stream
  for (auto &task : bookOfTasks[streamName]) {
    if (task.dumpedRecordsToGo == 0) continue;  // already completed task - will be removed later

    if (task.fd < 0) {
      SPDLOG_ERROR("dumpManager::processStreamChunk file descriptor is not set for stream: {}", streamName);
      continue;
    }
    RDB_TRY_ASSIGN(const bool dumpTaskCompleted, buildDumpChunk(task, payLoadPtr));

    if (dumpTaskCompleted) {
      ::close(task.fd);
      task.fd = -1;  // mark fd in task as closed
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
  return {};
}

rdb::Result<bool> dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  RDB_ASSERT(task.dumpedRecordsToGo >= 0, "dumpManager::buildDumpChunk: dumpedRecordsToGo is negative");
  RDB_ASSERT(task.delayDumpRecordsToGo >= 0, "dumpManager::buildDumpChunk: delayDumpRecordsToGo is negative");
  RDB_ASSERT(task.fd >= 0, "dumpManager::buildDumpChunk: file descriptor is not set");

  // tutaj trzeba będzie opóźnić zrzut danych do pliku jeśli range określa tylko zrzut w przyszłości np. range 2 to 4
  if (task.delayDumpRecordsToGo != 0) {
    task.delayDumpRecordsToGo--;
    return false;
  }

  auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
  if (resultSeek == -1)
    return rdb::fail(
        rdb::Errc::IO,
        fmt::format("dumpManager::buildDumpChunk: lseek to end of '{}' failed: {}", task.dumpFilename, strerror(errno)),
        errno != 0 ? errno : EIO);

  ssize_t write_count_result = ::write(task.fd, payload->span().data(), payload->descriptor.getSizeInBytes());
  if (write_count_result <= 0)
    return rdb::fail(rdb::Errc::IO,
                     fmt::format("dumpManager::buildDumpChunk: write to '{}' failed: {}", task.dumpFilename, strerror(errno)),
                     errno != 0 ? errno : EIO);

  if (task.dumpedRecordsToGo > 0) {
    task.dumpedRecordsToGo--;
  }

  return (task.dumpedRecordsToGo == 0);
}

rdb::Result<std::pair<std::string, int>> dumpManager::createDumpFile(const std::string_view streamName,
                                                                     const std::string_view taskName) {
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
    // Warunek nie moze byc falszywy: 'ret' powstaje jako '% retentionSize[key]', wiec jest
    // z definicji mniejsze od dzielnika. Zostaje jako asercja tego faktu - kosztuje jedno
    // porownanie i nazywa zalozenie, na ktorym stoi nazwa pliku zrzutu.
    RDB_ASSERT(ret < retentionSize[key], "dumpManager::createDumpFile: retention counter out of bounds: {} >= {} for key '{}'",
               ret, retentionSize[key], key);
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
      return rdb::fail(rdb::Errc::IO,
                       fmt::format("dumpManager::createDumpFile: failed to remove '{}': {}", filename.string(), strerror(errno)),
                       errno != 0 ? errno : EIO);
    }
    int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, kDefaultDumpFileMode);
    if (fd >= 0) return std::make_pair(filename.string(), fd);
    if (errno != EEXIST) break;
    // Ktos utworzyl plik miedzy naszym unlink() a open(); kasujemy go jeszcze raz.
  }
  return rdb::fail(rdb::Errc::IO,
                   fmt::format("dumpManager::createDumpFile: failed to create '{}': {}", filename.string(), strerror(errno)),
                   errno != 0 ? errno : EIO);
}
