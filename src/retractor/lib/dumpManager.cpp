#include "dumpManager.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "dataModel.h"

/** WORK IN PROGRESS - DO NOT USE **/

std::map<std::string, int> retentionCounter;  // first - streamName+taskName, second - counter
std::map<std::string, int> retentionSize;     // first - streamName+taskName, second - retention size

extern dataModel *pProc;

void dumpManager::registerTask(const std::string streamName, dumpTask task) {
  assert(pProc != nullptr && "dumpManager::registerTask dataModel is not set");
  assert(pProc->qSet.find(streamName) != pProc->qSet.end() && "dumpManager::registerTask streamName not found in dataModel");
  assert(task.range.first >= 0 && task.range.second >= 0 && "dumpManager::registerTask range values must be non-negative");
  assert(task.range.first <= task.range.second && "dumpManager::registerTask range.first must be <= range.second");

  std::tie(task.dumpFilename, task.fd)      = createDumpFile(streamName, task.taskName);
  task.dumpedRecordsToGo                    = abs(task.range.second - task.range.first);
  retentionSize[streamName + task.taskName] = task.retentionSize;
  bookOfTasks[streamName].push_back(task);
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
      // remove task form bookOfTasks
    } else {
      std::cerr << "Dump still in process: " << task.taskName << std::endl;
    }
  }

  auto new_end = std::remove_if(bookOfTasks[streamName].begin(), bookOfTasks[streamName].end(),
                                [](dumpTask &n) { return n.dumpedRecordsToGo == 0; });
  bookOfTasks[streamName].erase(new_end, bookOfTasks[streamName].end());
}

bool dumpManager::buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload) {
  // Here we would implement the actual dump logic
  if (task.dumpedRecordsToGo > 0) {
    ssize_t write_result = ::write(task.fd, payload->get(), payload->getDescriptor().getSizeInBytes());
    task.dumpedRecordsToGo--;
    if (task.dumpedRecordsToGo == 0) {
      ::close(task.fd);
      task.fd = 0;
    }
    return false;
  }
  return true;
}

std::pair<std::string, int> dumpManager::createDumpFile(std::string streamName, std::string taskName) {
  // Here we would implement the actual file creation logic
  auto filename = streamName + "_" + taskName;
  if (retentionSize[streamName + taskName] == 0) {
    filename += "_dump.tmp";
  } else {
    auto ret = (retentionCounter[streamName + taskName]++) % retentionSize[streamName + taskName];
    filename += "_dump_" + std::to_string(ret) + ".tmp";
  }
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  assert(fd >= 0);
  return std::make_pair(filename, fd);
}