#include "dumpManager.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>  // std::min
#include <cstdlib>    // std::abs
#include <cstring>    // strerror
#include <filesystem>

#include <spdlog/spdlog.h>

#include "dataModel.hpp"
#include "fatalError.hpp"

extern dataModel *pProc;

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
  if (pProc->qSet.find(streamName) == pProc->qSet.end()) {
    FatalError("dumpManager::registerTask: stream '{}' not found in dataModel", streamName);
  }
  if (task.range.first > task.range.second) {
    FatalError("dumpManager::registerTask: range.first {} > range.second {} for stream '{}'", task.range.first,
               task.range.second, streamName);
  }

  std::tie(task.dumpFilename, task.fd)      = createDumpFile(streamName, task.taskName);
  task.dumpedRecordsToGo                    = static_cast<int>(abs(task.range.second - task.range.first));
  retentionSize[streamName + task.taskName] = static_cast<int>(task.retentionSize);
  if (bookOfTasks[streamName].capacity() == 0) {
    bookOfTasks[streamName].set_capacity(task.retentionSize > 0 ? task.retentionSize : 1);
  }
  // This push_back will overwrite oldest task if retentionSize is exceeded
  // Task destructor will close file descriptor if still open

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

  bookOfTasks[streamName].push_back(std::move(task));
}

void dumpManager::setDumpStorage(std::string storagePathParam) { storagePath = std::move(storagePathParam); }

void dumpManager::processStreamChunk(const std::string &streamName) {
  if (pProc == nullptr) FatalError("dumpManager::processStreamChunk: dataModel pointer is null");
  if (pProc->qSet.find(streamName) == pProc->qSet.end()) {
    FatalError("dumpManager::processStreamChunk: stream '{}' not found in dataModel", streamName);
  }
  if (bookOfTasks.find(streamName) == bookOfTasks.end()) return;

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
  std::string key = std::string(streamName) + std::string(taskName);
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
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    FatalError("dumpManager::createDumpFile: failed to open '{}': {}", filename.string(), strerror(errno));
  }
  return std::make_pair(filename, fd);
}
