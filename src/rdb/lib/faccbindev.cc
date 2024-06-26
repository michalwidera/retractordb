#include "rdb/faccbindev.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>  // ::read, ::open ...

#include <cassert>

namespace rdb {

template <class T>
binaryDeviceAccessor<T>::binaryDeviceAccessor(const std::string fileName, const size_t size)  //
    : filename(fileName), size(size) {
  fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC, 0644);
}

template <class T>
binaryDeviceAccessor<T>::~binaryDeviceAccessor() {
  ::close(fd);
}

template <class T>
std::string binaryDeviceAccessor<T>::fileName() {
  return filename;
}

template <class T>
ssize_t binaryDeviceAccessor<T>::write(const T *ptrData, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <class T>
ssize_t binaryDeviceAccessor<T>::read(T *ptrData, const size_t position) {
  assert(size != 0);
  assert(position == 0);
  if (position != 0) {
    return EXIT_FAILURE;
  }
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  size_t read_size = ::read(fd, ptrData, size);  // /dev/random no seek supported
  if (read_size != size) {                       // dev/random has no seek - but binary files should loop?
    ::lseek(fd, 0, SEEK_SET);
    size_t read_size_sh = ::read(fd, ptrData, size);
    if (read_size_sh != size) return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

template <class T>
size_t binaryDeviceAccessor<T>::count() {
  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);
  return rc == 0 ? stat_buf.st_size / size : -1;
}

template class binaryDeviceAccessor<uint8_t>;
template class binaryDeviceAccessor<char>;

}  // namespace rdb