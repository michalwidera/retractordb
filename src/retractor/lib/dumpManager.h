#pragma once

#include <unistd.h>  // for ::lseek, ::write, ::close

#include <map>
#include <memory>  // unique_ptr
#include <string>

#include <boost/circular_buffer.hpp>

#include "rdb/payload.h"

struct dumpTask {
  // from rule definition
  std::string taskName;                                        // name of task
  std::pair<long int, long int> range = std::make_pair(0, 0);  // first - from, second - to
  size_t retentionSize{0};                                     // How many dumps to retain

  // configuration
  int dumpedRecordsToGo{0};      // How many records left to dump - 0 close task
  std::string dumpFilename{""};  // name of dump file
  int fd{0};                     // file descriptor Linux posix file handle
  int delayDumpRecordsToGo{0};   // How many records to delay the dump ( for range starting in future )
  bool inBook{false};            // is task in bookOfTasks

  dumpTask(const std::string name, const std::pair<long int, long int> rangeParam, const size_t retention)
      : taskName(name), range(rangeParam), retentionSize(retention), inBook(false) {}
  ~dumpTask();
};

class dumpManager {
 public:
  dumpManager()  = default;
  ~dumpManager() = default;

  void registerTask(const std::string streamName, dumpTask task);  // Register a dump function
  void processStreamChunk(const std::string streamName);           // Call all registered dump functions
  void setDumpStorage(const std::string storagePathParam);         // Set storage path for dump files

 private:
  std::map<std::string, int> retentionCounter;  // first - streamName+taskName, second - counter
  std::map<std::string, int> retentionSize;     // first - streamName+taskName, second - retention size
  std::string storagePath{""};
  std::map<std::string, boost::circular_buffer<dumpTask>> bookOfTasks;  // streamName -> list of tasks
  // circular buffer to track retention - will set by .set_capacity(retentionSize)

  bool buildDumpChunk(dumpTask &task,
                      std::unique_ptr<rdb::payload>::pointer payload);  // Execute dump task - return true if task is completed
  std::pair<std::string, int> createDumpFile(std::string streamName, std::string taskName);
};