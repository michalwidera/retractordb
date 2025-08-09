#include "rdb/faccmemory.h"

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy, std::min
#include <array>
#include <cassert>
#include <fstream>
#include <limits>
#include <map>
#include <vector>

static std::map<std::string, std::vector<std::vector<uint8_t>>> memoryStorage;

namespace rdb {

auto memoryFileAccessor::name() const -> const std::string & { return filename; }

auto memoryFileAccessor::name() -> std::string & { return filename; }

ssize_t memoryFileAccessor::write(const uint8_t *ptrData, const size_t position) {
  assert(size != 0);

  // If ptrData is null, clear the storage and reset removed_count
  if (ptrData == nullptr) {
    memoryStorage[filename].clear();  // Clear the storage if ptrData is null
    removed_count = 0;                // Reset removed count
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + size);

  if (retention_size != no_retention)  // If retention size is set, manage the retention
    if (memoryStorage[filename].size() > retention_size) {
      // Remove the oldest record if retention size is reached
      memoryStorage[filename].erase(memoryStorage[filename].begin());
      removed_count++;
    }

  if (position == std::numeric_limits<size_t>::max()) {
    // Append to the end of the file
    memoryStorage[filename].push_back(vec);
  } else {
    if (position < removed_count) {
      SPDLOG_ERROR("Write failed: Position out of bounds in memory storage: position {}, removed_count {}", position,
                   removed_count);
      return EXIT_FAILURE;  // Return an error code if position is out of bounds
    }
    assert(position >= removed_count && "write failed: Position out of bounds in memory storage");
    memoryStorage[filename][position - removed_count] = std::move(vec);
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFileAccessor::read(uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  if (position < removed_count) {
    SPDLOG_ERROR("Read failed: Position out of bounds in memory storage: position {}, removed_count {}", position,
                 removed_count);
    return EXIT_FAILURE;  // Return an error code if position is out of bounds
  }
  assert(position >= removed_count && "read failed: Position out of bounds in memory storage");

  auto &vec = memoryStorage[filename][position - removed_count];
  std::copy(vec.begin(), vec.end(), ptrData);
  return EXIT_SUCCESS;
}

size_t memoryFileAccessor::count() { return memoryStorage[filename].size() + removed_count; }

}  // namespace rdb