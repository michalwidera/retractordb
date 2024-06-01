#include "rdb/faccposix.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
namespace rdb {

template <class T>
posixBinaryFileAccessor<T>::posixBinaryFileAccessor(const std::string &fileName) : fileNameStr(fileName) {
  fd = ::open(fileNameStr.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0)
    SPDLOG_ERROR("::open {} -> {}", fileNameStr, fd);
  else
    SPDLOG_INFO("::open {} -> {}", fileNameStr, fd);
  assert(fd >= 0);
}

template <class T>
posixBinaryFileAccessor<T>::~posixBinaryFileAccessor() {
  ::close(fd);
}

template <class T>
std::string posixBinaryFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
ssize_t posixBinaryFileAccessor<T>::write(const T *ptrData, const size_t size, const size_t position) {
  assert(fd >= 0);
  if (fd < 0) {
    return errno;  // Error status
  }
  if (ptrData == nullptr && size == 0 && position == 0) {
    // nullptr, size, position 0,0,0 - truncate file.
    auto result = ::ftruncate(fd, 0);
    assert(result != static_cast<off_t>(-1));
    return errno;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    auto result = ::lseek(fd, 0, SEEK_END);
    assert(result != static_cast<off_t>(-1));
  } else {
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
  return EXIT_SUCCESS;
}

template <class T>
ssize_t posixBinaryFileAccessor<T>::read(T *ptrData, const size_t size, const size_t position) {
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  ssize_t read_size = ::pread(fd, ptrData, size, static_cast<off_t>(position));
  return EXIT_SUCCESS;
}

template class posixBinaryFileAccessor<uint8_t>;
template class posixBinaryFileAccessor<char>;

}  // namespace rdb