#include "rdb/faccbindev.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>  // ::read, ::open ...

#include <cassert>
#include <cstring>

namespace rdb {

binaryDeviceRO::binaryDeviceRO(const std::string_view fileName,  //
                               const ssize_t recordSize,         //
                               const rdb::Descriptor &descriptor,
                               bool loopToBeginningIfEOF)  //
    : filename_(std::string(fileName)),
      recordSize_(recordSize),
      descriptor_(descriptor),
      loopToBeginningIfEOF_(loopToBeginningIfEOF),
      lastNullBitset_(descriptor.size(), false) {
  fd_ = ::open(filename_.c_str(), O_RDONLY | O_CLOEXEC, 0644);
  if (fd_ < 0) {
    SPDLOG_WARN("Unable to open binary device source: {}", filename_);
  }
}

binaryDeviceRO::~binaryDeviceRO() {
  if (fd_ >= 0) ::close(fd_);
}

auto binaryDeviceRO::name() -> std::string & { return filename_; }

ssize_t binaryDeviceRO::read(uint8_t *ptrData, const size_t position) {
  auto markAllNullAndZero = [&](ssize_t status) {
    lastNullBitset_.assign(descriptor_.size(), true);
    if (ptrData != nullptr) {
      std::memset(ptrData, 0, recordSize_);
    }
    cnt_++;
    return status;
  };

  if (fd_ < 0) return markAllNullAndZero(EXIT_FAILURE);
  if (recordSize_ == 0) return markAllNullAndZero(EXIT_FAILURE);

  assert(recordSize_ != 0);
  assert(position == 0);
  if (position != 0) {
    return markAllNullAndZero(EXIT_FAILURE);
  }

  auto read_size = ::read(fd_, ptrData, recordSize_);  // /dev/random no seek supported
  if (read_size != recordSize_) {                      // dev/random has no seek - but binary files should loop?
    if (loopToBeginningIfEOF_) {
      ::lseek(fd_, 0, SEEK_SET);
      auto read_size_sh = ::read(fd_, ptrData, recordSize_);
      if (read_size_sh != recordSize_) return markAllNullAndZero(EXIT_FAILURE);
    } else {
      return markAllNullAndZero(EXIT_SUCCESS);
    }
  }
  lastNullBitset_.assign(descriptor_.size(), false);
  cnt_++;
  return EXIT_SUCCESS;
}

size_t binaryDeviceRO::count() { return cnt_; }

const std::vector<bool> &binaryDeviceRO::lastNullBitset() const { return lastNullBitset_; }

}  // namespace rdb
