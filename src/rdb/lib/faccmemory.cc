#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <limits>
#include <ranges>
#include <utility>
#include <vector>
#include "rdb/exceptions.hpp"

namespace rdb {

// Faza 2 refaktoru: trzy mapy `static` tego pliku - rekordy, mapy NULL i licznik zapisów -
// przeniosły się do MemoryStore. Były jedynym stanem magazynu MEMORY i były stanem CAŁEGO
// PROCESU, więc dwa silniki zbudowane w jednym procesie dzieliły dane strumienia o tej samej
// nazwie, nic o sobie nie wiedząc. Skróty poniżej nazywają ten sam stan, tylko u właściciela.

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) throw LogicError("memoryFile::write: recordSize_ is zero - accessor built on a zero-width descriptor");
  auto location = position / recordSize_;
  if (ptrData == nullptr) {
    store_.clear(filename_);
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + recordSize_);

  if (position == std::numeric_limits<size_t>::max()) {
    if (retentionSize_ != no_retention) {
      // Kołowy bufor: zapisz do slotu writeCount % retentionSize_
      const size_t wc   = store_.writeCount(filename_);
      const size_t slot = wc % retentionSize_;
      if (slot < store_.records(filename_).size()) {
        store_.records(filename_)[slot] = std::move(vec);
        store_.nulls(filename_)[slot]   = nullBitset;
      } else {
        store_.records(filename_).push_back(std::move(vec));
        store_.nulls(filename_).push_back(nullBitset);
      }
    } else {
      store_.records(filename_).push_back(std::move(vec));
      store_.nulls(filename_).push_back(nullBitset);
    }
    store_.writeCount(filename_)++;
  } else {
    // Nadpisanie rekordu pod wskazaną pozycją
    const size_t slot = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;
    if (slot >= store_.records(filename_).size()) {
      SPDLOG_ERROR("Write failed: slot {} out of range, storage size {}", slot, store_.records(filename_).size());
      return EXIT_FAILURE;
    }
    store_.records(filename_)[slot] = std::move(vec);
    if (slot < store_.nulls(filename_).size()) store_.nulls(filename_)[slot] = nullBitset;
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) throw LogicError("memoryFile::read: recordSize_ is zero - accessor built on a zero-width descriptor");
  const auto location = position / recordSize_;
  const size_t slot   = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;

  if (slot >= store_.records(filename_).size()) {
    SPDLOG_ERROR("Read failed: slot {} out of range, storage size {}", slot, store_.records(filename_).size());
    return EXIT_FAILURE;
  }

  auto &vec = store_.records(filename_)[slot];
  std::ranges::copy(vec, ptrData);

  auto &nullVec = store_.nulls(filename_);
  if (slot < nullVec.size()) {
    nullBitset = nullVec[slot];
  } else {
    nullBitset.clear();
  }
  return EXIT_SUCCESS;
}

size_t memoryFile::count() { return store_.writeCount(filename_); }

}  // namespace rdb
