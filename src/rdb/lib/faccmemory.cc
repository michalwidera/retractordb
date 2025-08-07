#include "rdb/faccmemory.h"

#include <array>
#include <cassert>
#include <fstream>
#include <limits>
#include <map>
#include <vector>

static std::map<std::string, std::vector<std::vector<uint8_t>>> memoryStorage;

namespace rdb {

memoryFileAccessor::memoryFileAccessor(  //
    const std::string_view fileName,     //
    const size_t size)                   //
    : filename(std::string(fileName)), size(size) {}

auto memoryFileAccessor::name() const -> const std::string & { return filename; }

auto memoryFileAccessor::name() -> std::string & { return filename; }

ssize_t memoryFileAccessor::write(const uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  std::vector<uint8_t> vec(ptrData, ptrData + size);
  if (position == std::numeric_limits<size_t>::max()) {
    // Append to the end of the file
    memoryStorage[filename].push_back(vec);
  } else {
    memoryStorage[filename][position] = std::move(vec);
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFileAccessor::read(uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  auto &vec = memoryStorage[filename][position];
  std::copy(vec.begin(), vec.end(), ptrData);
  return EXIT_SUCCESS;
}

size_t memoryFileAccessor::count() { return memoryStorage[filename].size(); }

}  // namespace rdb