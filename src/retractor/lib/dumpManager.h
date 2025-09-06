#pragma once

/** WORK IN PROGRESS - DO NOT USE **/

#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <utility>  // pair
#include <vector>

#include "rdb/payload.h"

class dumpManager {
 public:
  dumpManager()  = default;
  ~dumpManager() = default;

  struct dumpTask {
    // from rule definition
    std::string taskName;
    std::pair<long int, long int> range;
    size_t retentionSize{0};  // How many dumps to retain

    // configuration
    int dumpedRecordsToGo{0};      // How many records left to dump - 0 close task
    std::string dumpFilename{""};  // name of dump file
    int fd{0};                     // file descriptor Linux posix file handle
    int delayDumpRecordsToGo{0};   // How many records to delay the dump ( for range starting in future )
  };

  // Register a dump function
  void registerTask(const std::string streamName, dumpTask task);

  // Call all registered dump functions
  void processStreamChunk(const std::string streamName);

 private:
  std::map<std::string, std::vector<dumpTask>> bookOfTasks;

  bool buildDumpChunk(dumpTask &task, std::unique_ptr<rdb::payload>::pointer payload);
  std::pair<std::string, int> createDumpFile(std::string streamName, std::string taskName);
};