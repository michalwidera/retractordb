#include "rdb/faccposixprm.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
namespace rdb {
constexpr const int kOpenBaseFlags = O_CLOEXEC;

template <class T>
posixPrmBinaryFileAccessor<T>::posixPrmBinaryFileAccessor(std::string fileName) : fileNameStr(fileName) {
  fd = ::open(fileNameStr.c_str(), O_RDWR | O_CREAT | kOpenBaseFlags, 0644);
  if (fd < 0)
    SPDLOG_ERROR("::open {} -> {}", fileNameStr, fd);
  else
    SPDLOG_INFO("::open {} -> {}", fileNameStr, fd);
  assert(fd >= 0);
}

template <class T>
posixPrmBinaryFileAccessor<T>::~posixPrmBinaryFileAccessor() {
  ::close(fd);
}

template <class T>
std::string posixPrmBinaryFileAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
int posixPrmBinaryFileAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  assert(fd >= 0);
  if (fd < 0) {
    return errno;  // Error status
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
int posixPrmBinaryFileAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  ssize_t read_size = ::pread(fd, ptrData, size, static_cast<off_t>(position));
  return EXIT_SUCCESS;
}

template class posixPrmBinaryFileAccessor<std::byte>;
template class posixPrmBinaryFileAccessor<char>;
template class posixPrmBinaryFileAccessor<unsigned char>;

}  // namespace rdb