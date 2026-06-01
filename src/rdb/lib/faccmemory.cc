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
// Globalny licznik zapisów na strumień — utrzymuje logiczną pozycję niezależnie od instancji.
static std::map<std::string, size_t> memoryWriteCount;

namespace rdb {

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::write: recordSize_ is zero");
  auto location = position / recordSize_;
  if (ptrData == nullptr) {
    memoryStorage[filename_].clear();
    memoryNullStorage[filename_].clear();
    memoryWriteCount[filename_] = 0;
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + recordSize_);

  if (position == std::numeric_limits<size_t>::max()) {
    if (retentionSize_ != no_retention) {
      // Kołowy bufor: zapisz do slotu writeCount % retentionSize_
      const size_t wc   = memoryWriteCount[filename_];
      const size_t slot = wc % retentionSize_;
      if (slot < memoryStorage[filename_].size()) {
        memoryStorage[filename_][slot]     = std::move(vec);
        memoryNullStorage[filename_][slot] = nullBitset;
      } else {
        memoryStorage[filename_].push_back(std::move(vec));
        memoryNullStorage[filename_].push_back(nullBitset);
      }
    } else {
      memoryStorage[filename_].push_back(std::move(vec));
      memoryNullStorage[filename_].push_back(nullBitset);
    }
    memoryWriteCount[filename_]++;
  } else {
    // Nadpisanie rekordu pod wskazaną pozycją
    const size_t slot = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;
    if (slot >= memoryStorage[filename_].size()) {
      SPDLOG_ERROR("Write failed: slot {} out of range, storage size {}", slot, memoryStorage[filename_].size());
      return EXIT_FAILURE;
    }
    memoryStorage[filename_][slot] = std::move(vec);
    if (slot < memoryNullStorage[filename_].size()) memoryNullStorage[filename_][slot] = nullBitset;
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::read: recordSize_ is zero");
  const auto location = position / recordSize_;
  const size_t slot   = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;

  if (slot >= memoryStorage[filename_].size()) {
    SPDLOG_ERROR("Read failed: slot {} out of range, storage size {}", slot, memoryStorage[filename_].size());
    return EXIT_FAILURE;
  }

  auto &vec = memoryStorage[filename_][slot];
  std::ranges::copy(vec, ptrData);

  auto &nullVec = memoryNullStorage[filename_];
  if (slot < nullVec.size()) {
    nullBitset = nullVec[slot];
  } else {
    nullBitset.clear();
  }
  return EXIT_SUCCESS;
}

size_t memoryFile::count() { return memoryWriteCount[filename_]; }

}  // namespace rdb
