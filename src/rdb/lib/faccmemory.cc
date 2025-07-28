#include "rdb/faccmemory.h"

#include <cassert>
#include <fstream>
#include <limits>
#include <map>
#include <vector>
#include <span>

static std::map<std::string, std::vector<std::span<uint8_t>>> memoryStorage;

namespace rdb {
template <class T>
memoryFileAccessor<T>::memoryFileAccessor(  //
    const std::string &fileName,                          //
    const size_t size)                                    //
    : filename(fileName), size(size) {}

template <class T>
auto memoryFileAccessor<T>::name() const -> const std::string & {
  return filename;
}

template <class T>
auto memoryFileAccessor<T>::name() -> std::string & {
  return filename;
}

template <class T>
ssize_t memoryFileAccessor<T>::write(const T *ptrData, const size_t position) {
  assert(size != 0);
  // TODO
  return EXIT_SUCCESS;
}

template <class T>
ssize_t memoryFileAccessor<T>::read(T *ptrData, const size_t position) {
  assert(size != 0);
  // TODO
  return EXIT_SUCCESS;
}

template <class T>
size_t memoryFileAccessor<T>::count() {
  // TODO
  return 0;
}

template class memoryFileAccessor<uint8_t>;
template class memoryFileAccessor<char>;

}  // namespace rdb