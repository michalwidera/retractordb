#include "dumpManager.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <algorithm>  // std::min
#include <cstdlib>   // std::abs
#include <cstring>   // strerror
#include <filesystem>

#include "fatalError.hpp"

#include "dataModel.hpp"

extern dataModel *pProc;

dumpTask::~dumpTask() {
  if (fd != 0 && inBook) {
    ::close(fd);
  }
}

void dumpManager::registerTask(const std::string &streamName, dumpTask task) {
  if (pProc == nullptr) FATAL_ERROR("dumpManager::registerTask: dataModel pointer is null");
  if (pProc->qSet.find(streamName) == pProc->qSet.end()) {
    FATAL_ERROR("dumpManager::registerTask: stream '{}' not found in dataModel", streamName);
  }
  if (task.range.first > task.range.second) {
    FATAL_ERROR("dumpManager::registerTask: range.first {} > range.second {} for stream '{}'", task.range.first, task.range.second, streamName);
  }

  std::tie(task.dumpFilename, task.fd)      = createDumpFile(streamName, task.taskName);
  task.dumpedRecordsToGo                    = abs(task.range.second - task.range.first);
  retentionSize[streamName + task.taskName] = task.retentionSize;
  if (bookOfTasks[streamName].capacity() == 0) {
    bookOfTasks[streamName].set_capacity(task.retentionSize > 0 ? task.retentionSize : 1);
  }
  bookOfTasks[streamName].push_back(task);
  bookOfTasks[streamName].back().inBook = true;
  // This push_back will overwrite oldest task if retentionSize is exceeded
  // Task destructor will close file descriptor if still open

  if (task.range.first < 0) {
    // Filling dump with data already in stream history
    // we need to dump abs(range.first) records from history
    // if range.first is 0 or positive - no history dump needed
    // CHECK IF WE HAVE ENOUGH HISTORY
    // CHECK SEQUENCE IF THIS IS IN REVERSE ORDER
    size_t dumpHistoryCount   = abs(task.range.first);
    size_t currentStreamCount = pProc->getStreamCount(streamName);
    for (auto i = 0; i < dumpHistoryCount; ++i) {
      auto payLoadPtr = pProc->getPayload(streamName, dumpHistoryCount - i);
      auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
      if (resultSeek == -1) FATAL_ERROR("dumpManager::registerTask: lseek failed during history dump");
      ssize_t write_count_result = ::write(task.fd, payLoadPtr->span().data(), payLoadPtr->descriptor.getSizeInBytes());
      if (write_count_result <= 0) FATAL_ERROR("dumpManager::registerTask: write failed during history dump");
    }
    if (task.dumpedRecordsToGo < dumpHistoryCount) {
      FATAL_ERROR("dumpManager::registerTask: dumpedRecordsToGo {} < dumpHistoryCount {}", task.dumpedRecordsToGo, dumpHistoryCount);
    }
    task.dumpedRecordsToGo -= dumpHistoryCount;
  } else {
    task.delayDumpRecordsToGo = task.range.first;
  }
}

void dumpManager::setDumpStorage(std::string storagePathParam) { storagePath = std::move(storagePathParam); }

void dumpManager::processStreamChunk(const std::string &streamName) {
  if (pProc == nullptr) FATAL_ERROR("dumpManager::processStreamChunk: dataModel pointer is null");
  if (pProc->qSet.find(streamName) == pProc->qSet.end()) {
    FATAL_ERROR("dumpManager::processStreamChunk: stream '{}' not found in dataModel", streamName);
  }
  if (bookOfTasks.find(streamName) == bookOfTasks.end()) return;

  auto currentStreamCount = pProc->getStreamCount(streamName);
  if (currentStreamCount == 0) return;  // nothing to dump

  auto payLoadPtr = pProc->getPayload(streamName);

  if (payLoadPtr->descriptor.getSizeInBytes() == 0) FATAL_ERROR("dumpManager::processStreamChunk: payload descriptor size is zero");
  if (payLoadPtr->span().empty()) FATAL_ERROR("dumpManager::processStreamChunk: payload data span is empty");

  // enumerate all tasks for this stream
  for (auto &task : bookOfTasks[streamName]) {
    if (task.dumpedRecordsToGo == 0) continue;  // already completed task - will be removed later

    if (task.fd == 0) {
      SPDLOG_ERROR("dumpManager::processStreamChunk file descriptor is not set for stream: {}", streamName);
      continue;
    }
    auto dumpTaskCompleted = buildDumpChunk(task, payLoadPtr);

    if (dumpTaskCompleted) {
      ::close(task.fd);
      task.fd = 0;  // mark fd in task as closed
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
}

bool dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  if (task.dumpedRecordsToGo < 0) FATAL_ERROR("dumpManager::buildDumpChunk: dumpedRecordsToGo is negative");
  if (task.delayDumpRecordsToGo < 0) FATAL_ERROR("dumpManager::buildDumpChunk: delayDumpRecordsToGo is negative");
  if (task.fd == 0) FATAL_ERROR("dumpManager::buildDumpChunk: file descriptor is not set");

  // tutaj trzeba będzie opóźnić zrzut danych do pliku jeśli range określa tylko zrzut w przyszłości np. range 2 to 4
  if (task.delayDumpRecordsToGo != 0) {
    task.delayDumpRecordsToGo--;
    return false;
  }

  auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
  if (resultSeek == -1) FATAL_ERROR("dumpManager::buildDumpChunk: lseek to end failed");

  ssize_t write_count_result = ::write(task.fd, payload->span().data(), payload->descriptor.getSizeInBytes());
  if (write_count_result <= 0) FATAL_ERROR("dumpManager::buildDumpChunk: write failed");

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
      FATAL_ERROR("dumpManager::createDumpFile: retention counter out of bounds: {} >= {} for key '{}'", ret, retentionSize[key], key);
    }
  }
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    FATAL_ERROR("dumpManager::createDumpFile: failed to open '{}': {}", filename.string(), strerror(errno));
  }
  return std::make_pair(filename, fd);
}
