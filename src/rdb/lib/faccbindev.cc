#include "rdb/faccbindev.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>  // ::read, ::open ...

#include <cassert>

namespace rdb {

binaryDeviceAccessor::binaryDeviceAccessor(const std::string_view fileName,  //
                                           const size_t recSize)             //
    : filename(std::string(fileName)), recSize(recSize) {
  fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC, 0644);
  // std::cerr << "Opening file: " << filename << std::endl;
  // TODO: there is a need of support failure here
  // sometimes /dev/random is not available
  // of other file are not available
  // only debug show someting wrong.
  assert(fd >= 0);
  assert(recSize != 0);
  // checking fd on read function.
}

binaryDeviceAccessor::~binaryDeviceAccessor() { ::close(fd); }

auto binaryDeviceAccessor::name() const -> const std::string & { return filename; }

auto binaryDeviceAccessor::name() -> std::string & { return filename; }

ssize_t binaryDeviceAccessor::write(const uint8_t *ptrData, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

ssize_t binaryDeviceAccessor::read(uint8_t *ptrData, const size_t position) {
  if (fd < 0) return EXIT_FAILURE;
  if (recSize == 0) return EXIT_FAILURE;  // No read on data source supported

  assert(recSize != 0);
  assert(position == 0);
  if (position != 0) {
    return EXIT_FAILURE;
  }
  assert(fd >= 0);
  if (fd < 0) {
    return fd;  // <- Error status
  }
  size_t read_size = ::read(fd, ptrData, recSize);  // /dev/random no seek supported
  if (read_size != recSize) {                       // dev/random has no seek - but binary files should loop?
    ::lseek(fd, 0, SEEK_SET);
    size_t read_size_sh = ::read(fd, ptrData, recSize);
    if (read_size_sh != recSize) return EXIT_FAILURE;
  }
  cnt++;
  return EXIT_SUCCESS;
}

size_t binaryDeviceAccessor::count() {
  // struct stat stat_buf;
  // int rc = stat(filename.c_str(), &stat_buf);
  // return rc == 0 ? stat_buf.st_size / recSize : -1;
  return cnt;
}

}  // namespace rdb