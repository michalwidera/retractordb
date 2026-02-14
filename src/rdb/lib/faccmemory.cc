#include "rdb/faccmemory.h"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <cassert>
#include <limits>
#include <map>
#include <vector>

static std::map<std::string, std::vector<std::vector<uint8_t>>> memoryStorage;

namespace rdb {

auto memoryFileAccessor::name() -> std::string & { return filename; }

ssize_t memoryFileAccessor::write(const uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  auto location = position / size;
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
    if (location < removed_count) {
      SPDLOG_ERROR("Write failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                   removed_count);
      return EXIT_FAILURE;  // Return an error code if position is out of bounds
    }
    assert(location >= removed_count && "write failed: Position out of bounds in memory storage");
    memoryStorage[filename][location - removed_count] = std::move(vec);
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFileAccessor::read(uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  auto location = position / size;
  if (location < removed_count) {
    SPDLOG_ERROR("Read failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                 removed_count);
    return EXIT_FAILURE;  // Return an error code if position is out of bounds
  }
  assert(location >= removed_count && "read failed: Position out of bounds in memory storage");

  auto &vec = memoryStorage[filename][location - removed_count];
  std::copy(vec.begin(), vec.end(), ptrData);
  return EXIT_SUCCESS;
}

size_t memoryFileAccessor::count() { return memoryStorage[filename].size() + removed_count; }

}  // namespace rdb
