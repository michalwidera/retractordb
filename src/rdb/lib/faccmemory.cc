#include "rdb/faccmemory.h"

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
  std::vector<uint8_t> vec(ptrData, ptrData + size);

  if (retention_size != no_retention)
    memoryStorage[filename].erase(std::remove_if(memoryStorage[filename].begin(), memoryStorage[filename].end(),
                                                 [this](const std::vector<uint8_t> &v) {
                                                   if (v.size() > retention_size) {
                                                     removed_count++;
                                                     return true;
                                                   }
                                                   return false;
                                                 }),
                                  memoryStorage[filename].end());  // Remove records exceeding retention size

  if (position == std::numeric_limits<size_t>::max()) {
    // Append to the end of the file
    memoryStorage[filename].push_back(vec);
  } else {
    assert(position > removed_count && "Position out of bounds in memory storage");
    memoryStorage[filename][position - removed_count] = std::move(vec);
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFileAccessor::read(uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  assert(position > removed_count && "Position out of bounds in memory storage");
  auto &vec = memoryStorage[filename][position - removed_count];
  std::copy(vec.begin(), vec.end(), ptrData);
  return EXIT_SUCCESS;
}

size_t memoryFileAccessor::count() { return memoryStorage[filename].size() + removed_count; }

}  // namespace rdb