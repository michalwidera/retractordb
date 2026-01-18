#include "dumpManager.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>  // std::min
#include <cassert>
#include <cstdlib>  // std::abs
#include <filesystem>
#include <iostream>

#include "dataModel.h"

extern dataModel *pProc;

dumpTask::~dumpTask() {
  if (fd != 0 && inBook) {
    SPDLOG_INFO("dumpTask: Destroying task: {} and fd: {}", taskName, fd);
    ::close(fd);
  }
}

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
      assert(resultSeek != -1);
      ssize_t write_count_result = ::write(task.fd, payLoadPtr->get(), payLoadPtr->descriptor.getSizeInBytes());
      assert(write_count_result > 0);
    }
    assert(task.dumpedRecordsToGo >= dumpHistoryCount);
    task.dumpedRecordsToGo -= dumpHistoryCount;
  } else {
    task.delayDumpRecordsToGo = task.range.first;
  }
  SPDLOG_INFO("DumpManager: registered dump task {} for stream {}, records to go: {}", task.taskName, streamName,
              task.dumpedRecordsToGo);
}

void dumpManager::setDumpStorage(const std::string storagePathParam) { storagePath = storagePathParam; }

void dumpManager::processStreamChunk(const std::string streamName) {
  SPDLOG_INFO("dumpManager::processStreamChunk for stream: {} task in book: {}", streamName, bookOfTasks[streamName].size());

  assert(pProc != nullptr && "dumpManager::processStreamChunk dataModel is not set");
  assert(pProc->qSet.find(streamName) != pProc->qSet.end() &&
         "dumpManager::processStreamChunk streamName not found in dataModel");
  assert(bookOfTasks.find(streamName) != bookOfTasks.end() &&
         "dumpManager::processStreamChunk streamName not found in bookOfTasks");

  auto currentStreamCount = pProc->getStreamCount(streamName);
  if (currentStreamCount == 0) return;  // nothing to dump

  auto payLoadPtr = pProc->getPayload(streamName);

  assert(payLoadPtr->descriptor.getSizeInBytes() > 0 && "dumpManager::processStreamChunk payload descriptor size is zero");
  assert(payLoadPtr->get() != nullptr && "dumpManager::processStreamChunk payload data is null");

  // enumerate all tasks for this stream
  for (auto &task : bookOfTasks[streamName]) {
    if (task.dumpedRecordsToGo == 0) continue;  // already completed task - will be removed later

    if (task.fd == 0) {
      SPDLOG_ERROR("dumpManager::processStreamChunk file descriptor is not set for stream: {}", streamName);
      continue;
    }
    auto dumpTaskCompleted = buildDumpChunk(task, payLoadPtr);

    if (dumpTaskCompleted) {
      SPDLOG_INFO("DumpManager: completed dump task {} for stream {}", task.taskName, streamName);
      ::close(task.fd);
      task.fd = 0;  // mark fd in task as closed
    } else {
      SPDLOG_INFO("DumpManager: continuing dump task {} for stream {}, records to go: {}", task.taskName, streamName,
                  task.dumpedRecordsToGo);
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
}

bool dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  assert(task.dumpedRecordsToGo >= 0 && "dumpManager::buildDumpChunk dumpedRecordsToGo is negative");
  assert(task.delayDumpRecordsToGo >= 0 && "dumpManager::buildDumpChunk delayDumpRecordsToGo is negative");
  assert(task.fd != 0 && "dumpManager::buildDumpChunk file descriptor is not set");

  // tutaj trzeba będzie opóźnić zrzut danych do pliku jeśli range określa tylko zrzut w przyszłości np. range 2 to 4
  if (task.delayDumpRecordsToGo != 0) {
    SPDLOG_INFO("DumpManager: delaying dump for task {} by {} records", task.taskName, task.delayDumpRecordsToGo);
    task.delayDumpRecordsToGo--;
    return false;
  }

  SPDLOG_INFO("::lseek dump with fd: {}", task.fd);
  auto resultSeek = ::lseek(task.fd, 0, SEEK_END);
  assert(resultSeek != -1);

  SPDLOG_INFO("::write dump with fd: {}", task.fd);
  ssize_t write_count_result = ::write(task.fd, payload->get(), payload->descriptor.getSizeInBytes());
  assert(write_count_result > 0);

  if (task.dumpedRecordsToGo > 0) {
    task.dumpedRecordsToGo--;
  }

  return (task.dumpedRecordsToGo == 0);
}

std::pair<std::string, int> dumpManager::createDumpFile(std::string streamName, std::string taskName) {
  auto filename = std::filesystem::path(storagePath) / std::filesystem::path(streamName + "_" + taskName);
  if (retentionSize[streamName + taskName] == 0) {
    filename += "_dump.tmp";
  } else {
    auto ret = (retentionCounter[streamName + taskName]++) % retentionSize[streamName + taskName];
    filename += "_dump_" + std::to_string(ret) + ".tmp";
    assert(ret < retentionSize[streamName + taskName]);
  }
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  assert(fd >= 0);
  SPDLOG_INFO("Created dump file: {} with fd: {}", std::string(filename), fd);
  return std::make_pair(filename, fd);
}