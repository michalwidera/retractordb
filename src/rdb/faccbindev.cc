#include "rdb/faccbindev.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

namespace rdb {
constexpr const int kOpenBaseFlags = O_CLOEXEC;

template <class T>
binaryDeviceAccessor<T>::binaryDeviceAccessor(std::string fileName) : fileNameStr(fileName) {
  fd = ::open(fileNameStr.c_str(), O_RDONLY | kOpenBaseFlags, 0644);
}

template <class T>
binaryDeviceAccessor<T>::~binaryDeviceAccessor() {
  ::close(fd);
}

template <class T>
std::string binaryDeviceAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
int binaryDeviceAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <class T>
int binaryDeviceAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  assert(position == 0);
  if (position != 0) {
    return EXIT_FAILURE;
  }
  ssize_t read_size = ::read(fd, ptrData, size);  // /dev/random no seek supported
  return EXIT_SUCCESS;
}

template class binaryDeviceAccessor<std::byte>;
template class binaryDeviceAccessor<char>;
template class binaryDeviceAccessor<unsigned char>;

}  // namespace rdb