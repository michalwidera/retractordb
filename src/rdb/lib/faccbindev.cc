#include "rdb/faccbindev.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>  // ::read, ::open ...

#include <cassert>

namespace rdb {

binaryDeviceAccessorRO::binaryDeviceAccessorRO(const std::string_view fileName,  //
                                               const size_t recSize,             //
                                               bool loopToBeginningIfEOF)        //
    : filename_(std::string(fileName)), recSize_(recSize), loopToBeginningIfEOF_(loopToBeginningIfEOF) {
  fd_ = ::open(filename_.c_str(), O_RDONLY | O_CLOEXEC, 0644);
  // TODO: there is a need of support failure here
  // sometimes /dev/random is not available
  // of other file are not available
  // only debug show someting wrong.
  assert(fd_ >= 0);
  assert(recSize_ != 0);
  // checking fd on read function.
}

binaryDeviceAccessorRO::~binaryDeviceAccessorRO() { ::close(fd_); }

auto binaryDeviceAccessorRO::name() -> std::string & { return filename_; }

ssize_t binaryDeviceAccessorRO::read(uint8_t *ptrData, const size_t position) {
  if (fd_ < 0) return EXIT_FAILURE;
  if (recSize_ == 0) return EXIT_FAILURE;  // No read on data source supported

  assert(recSize_ != 0);
  assert(position == 0);
  if (position != 0) {
    return EXIT_FAILURE;
  }
  assert(fd_ >= 0);
  if (fd_ < 0) {
    return fd_;  // <- Error status
  }
  size_t read_size = ::read(fd_, ptrData, recSize_);  // /dev/random no seek supported
  if (read_size != recSize_) {                        // dev/random has no seek - but binary files should loop?
    if (loopToBeginningIfEOF_) {
      ::lseek(fd_, 0, SEEK_SET);
      size_t read_size_sh = ::read(fd_, ptrData, recSize_);
      if (read_size_sh != recSize_) return EXIT_FAILURE;
    } else {
      std::memset(ptrData, 0, recSize_);  // zero the rest of data
    }
  }
  cnt_++;
  return EXIT_SUCCESS;
}

size_t binaryDeviceAccessorRO::count() { return cnt_; }

}  // namespace rdb