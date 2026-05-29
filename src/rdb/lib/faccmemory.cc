#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <limits>
#include <map>
#include <ranges>
#include <utility>
#include <vector>
#include "fatalError.hpp"

static std::map<std::string, std::vector<std::vector<uint8_t>>> memoryStorage;
static std::map<std::string, std::vector<std::vector<bool>>> memoryNullStorage;

namespace rdb {

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::write: recordSize_ is zero");
  auto location = position / recordSize_;
  // If ptrData is null, clear the storage and reset removed_count
  if (ptrData == nullptr) {
    memoryStorage[filename_].clear();  // Clear the storage if ptrData is null
    removed_count_ = 0;                // Reset removed count
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + recordSize_);

  if (retentionSize_ != no_retention)  // If retention size is set, manage the retention
    if (memoryStorage[filename_].size() > retentionSize_) {
      // Remove the oldest record if retention size is reached
      memoryStorage[filename_].erase(memoryStorage[filename_].begin());
      removed_count_++;
    }

  if (position == std::numeric_limits<size_t>::max()) {
    // Append to the end of the file
    memoryStorage[filename_].push_back(vec);
    memoryNullStorage[filename_].push_back(nullBitset);
  } else {
    if (std::cmp_less(location, removed_count_)) {
      SPDLOG_ERROR("Write failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                   removed_count_);
      return EXIT_FAILURE;  // Return an error code if position is out of bounds
    }
    const size_t adjustedLocation              = location - static_cast<size_t>(removed_count_);
    memoryStorage[filename_][adjustedLocation] = std::move(vec);
    if (adjustedLocation < memoryNullStorage[filename_].size()) {
      memoryNullStorage[filename_][adjustedLocation] = nullBitset;
    }
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::read: recordSize_ is zero");
  auto location = position / recordSize_;
  if (std::cmp_less(location, removed_count_)) {
    SPDLOG_ERROR("Read failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                 removed_count_);
    return EXIT_FAILURE;  // Return an error code if position is out of bounds
  }
  auto adjustedLocation = location - static_cast<size_t>(removed_count_);
  if (adjustedLocation >= memoryStorage[filename_].size()) {
    SPDLOG_ERROR("Read failed: Position beyond end of memory storage: location {}, size {}", location,
                 memoryStorage[filename_].size() + removed_count_);
    return EXIT_FAILURE;
  }

  auto &vec = memoryStorage[filename_][adjustedLocation];
  std::ranges::copy(vec, ptrData);

  auto &nullVec = memoryNullStorage[filename_];
  if (adjustedLocation < nullVec.size()) {
    nullBitset = nullVec[adjustedLocation];
  } else {
    nullBitset.clear();
  }
  return EXIT_SUCCESS;
}

size_t memoryFile::count() { return memoryStorage[filename_].size() + removed_count_; }

}  // namespace rdb
