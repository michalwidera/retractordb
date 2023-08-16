#include "rdb/faccposix.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

namespace rdb {
template <class T>
posixBinaryFileAccessor<T>::posixBinaryFileAccessor(const std::string fileName) : fileNameStr(fileName) {}

template <class T>
const std::string posixBinaryFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
ssize_t posixBinaryFileAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  int fd;
  if (position == std::numeric_limits<size_t>::max()) {
    fd = ::open(fileNameStr.c_str(), O_APPEND | O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    assert(fd >= 0);
    if (fd < 0) {
      return errno;  // Error status
    }
    auto result = ::lseek(fd, 0, SEEK_END);
    assert(result != static_cast<off_t>(-1));
  } else {
    fd = ::open(fileNameStr.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    assert(fd >= 0);
    if (fd < 0) {
      return errno;  // Error status
    }
    auto result = ::lseek(fd, position, SEEK_SET);
    assert(result != static_cast<off_t>(-1));
    if (result == static_cast<off_t>(-1)) {
      return errno;  // Error status
    }
  }
  size_t sizesh(size);
  while (sizesh > 0) {
    ssize_t write_result = ::write(fd, ptrData, sizesh);
    if (write_result < 0) {
      if (errno == EINTR) {
        continue;  // Retry
      }
      assert(errno);
      return errno;  // Error status
    }
    ptrData += write_result;
    sizesh -= write_result;
  }
  ::close(fd);
  return EXIT_SUCCESS;
}

template <class T>
ssize_t posixBinaryFileAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  int fd = -1;
  fd     = ::open(fileNameStr.c_str(), O_RDONLY | O_CLOEXEC);
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  ::pread(fd, ptrData, size, static_cast<off_t>(position));
  ::close(fd);
  return EXIT_SUCCESS;
}

template class posixBinaryFileAccessor<uint8_t>;
template class posixBinaryFileAccessor<char>;

}  // namespace rdb