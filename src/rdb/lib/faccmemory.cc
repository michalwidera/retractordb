#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <cassert>
#include <limits>
#include <map>
#include <vector>

static std::map<std::string, std::vector<std::vector<uint8_t>>> memoryStorage;
static std::map<std::string, std::vector<std::vector<bool>>> memoryNullStorage;

namespace rdb {

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::writeRaw(const uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  auto location = position / recordSize_;
  // If ptrData is null, clear the storage and reset removed_count
  if (ptrData == nullptr) {
    memoryStorage[filename_].clear();  // Clear the storage if ptrData is null
    removed_count_ = 0;                // Reset removed count
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + recordSize_);

  if (retentionSize_ != no_retention)                        // If retention size is set, manage the retention
    if (memoryStorage[filename_].size() > retentionSize_) {  // NOLINT(bugprone-implicit-widening-of-multiplication-result)
      // Remove the oldest record if retention size is reached
      memoryStorage[filename_].erase(memoryStorage[filename_].begin());
      removed_count_++;
    }

  if (position == std::numeric_limits<size_t>::max()) {
    // Append to the end of the file
    memoryStorage[filename_].push_back(vec);
  } else {
    if (location < removed_count_) {
      SPDLOG_ERROR("Write failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                   removed_count_);
      return EXIT_FAILURE;  // Return an error code if position is out of bounds
    }
    assert(location >= removed_count_ && "write failed: Position out of bounds in memory storage");
    memoryStorage[filename_][location - removed_count_] = std::move(vec);
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::readRaw(uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  auto location = position / recordSize_;
  if (location < removed_count_) {
    SPDLOG_ERROR("Read failed: Position out of bounds in memory storage: location {}, removed_count {}", location,
                 removed_count_);
    return EXIT_FAILURE;  // Return an error code if position is out of bounds
  }
  assert(location >= removed_count_ && "read failed: Position out of bounds in memory storage");

  auto adjustedLocation = location - removed_count_;
  if (adjustedLocation >= memoryStorage[filename_].size()) {
    SPDLOG_ERROR("Read failed: Position beyond end of memory storage: location {}, size {}", location,
                 memoryStorage[filename_].size() + removed_count_);
    return EXIT_FAILURE;
  }

  auto &vec = memoryStorage[filename_][adjustedLocation];
  std::copy(vec.begin(), vec.end(), ptrData);
  return EXIT_SUCCESS;
}

size_t memoryFile::count() { return memoryStorage[filename_].size() + removed_count_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const size_t position, const std::vector<bool> &nullBitset) {
  ssize_t result = writeRaw(ptrData, position);
  if (result == EXIT_SUCCESS && ptrData != nullptr) {
    if (position == std::numeric_limits<size_t>::max()) {
      memoryNullStorage[filename_].push_back(nullBitset);
    } else {
      const size_t location         = position / recordSize_;
      const size_t adjustedLocation = location - removed_count_;
      if (adjustedLocation < memoryNullStorage[filename_].size()) {
        memoryNullStorage[filename_][adjustedLocation] = nullBitset;
      }
    }
  }
  return result;
}

ssize_t memoryFile::read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) {
  ssize_t result            = readRaw(ptrData, position);
  const size_t location     = position / recordSize_;
  const size_t adjustedLoc  = location - removed_count_;
  auto &nullVec             = memoryNullStorage[filename_];
  if (result == EXIT_SUCCESS && adjustedLoc < nullVec.size()) {
    nullBitset = nullVec[adjustedLoc];
  } else {
    nullBitset.clear();
  }
  return result;
}

}  // namespace rdb
