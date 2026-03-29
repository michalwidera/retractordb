#include "rdb/faccposix.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <filesystem>
namespace rdb {

posixBinaryFile::posixBinaryFile(const std::string_view fileName,  //
                                 const ssize_t recordSize,         //
                                 int percounter)                   //
    : filename_(std::string(fileName)),
      recordSize_(recordSize),
      percounter_(percounter) {
  assert(recordSize_ > 0);

  std::error_code fs_ec;
  const bool fileExisted = std::filesystem::exists(filename_, fs_ec);
  if (fs_ec) {
    SPDLOG_WARN("Failed to check if {} exists: {}", filename_, fs_ec.message());
  }

  fd = ::open(filename_.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0)
    SPDLOG_ERROR("::open {} -> {}", filename_, fd);
  else
    SPDLOG_INFO("::open {} -> {}", filename_, fd);
  assert(fd >= 0);

  if (fd >= 0 && fileExisted) {
    const off_t fileSize = ::lseek(fd, 0, SEEK_END);
    if (fileSize < 0) {
      SPDLOG_ERROR("::lseek {} failed during state restore: {}", filename_, strerror(errno));
    } else {
      const off_t alignedSize = (fileSize / recordSize_) * recordSize_;
      if (alignedSize != fileSize) {
        SPDLOG_WARN("{} has {} trailing bytes; truncating to {} to restore consistent records", filename_,
                    fileSize - alignedSize, alignedSize);
        if (::ftruncate(fd, alignedSize) != 0) {
          SPDLOG_ERROR("::ftruncate {} to {} failed during state restore: {}", filename_, alignedSize, strerror(errno));
        }
      }
      SPDLOG_INFO("Restored {} with {} persisted records", filename_, alignedSize / recordSize_);
    }
  }
}

posixBinaryFile::~posixBinaryFile() {
  if (fd >= 0) {
    if (::fsync(fd) != 0) {
      SPDLOG_ERROR("::fsync {} failed before close: {}", filename_, strerror(errno));
    }
    if (::close(fd) != 0) {
      SPDLOG_ERROR("::close {} failed: {}", filename_, strerror(errno));
    }
    fd = -1;
  }

  if (percounter_ >= 0) {
    SPDLOG_INFO("Percounter mode - rotating file on close: {}", filename_);
    std::string rotated_filename = filename_ + ".old" + std::to_string(percounter_);
    std::error_code ec;
    std::filesystem::rename(filename_, rotated_filename, ec);
    if (ec) {
      SPDLOG_ERROR("Failed to rotate file {} to {}: {}", filename_, rotated_filename, ec.message());
    } else {
      SPDLOG_INFO("Rotated file {} to {}", filename_, rotated_filename);
    }
  }
}

auto posixBinaryFile::name() -> std::string & { return filename_; }

ssize_t posixBinaryFile::write(const uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  assert(fd >= 0);
  if (fd < 0) return errno;  // Error status

  if (ptrData == nullptr && position == 0) {
    // nullptr, position 0,0 - truncate file.
    std::filesystem::remove(name());

    return EXIT_SUCCESS;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    auto result = ::lseek(fd, 0, SEEK_END);
    assert(result != -1);
    if (result == -1) return errno;  // Error status
  } else {
    auto result = ::lseek(fd, position, SEEK_SET);
    assert(result != -1);
    if (result == -1) return errno;  // Error status
  }
  ssize_t sizesh(recordSize_);
  constexpr int maxRetries = 5;
  int retries              = 0;
  while (sizesh > 0) {
    ssize_t write_result = ::write(fd, ptrData, sizesh);
    if (write_result < 0) {
      if (errno == EINTR && ++retries <= maxRetries) continue;  // Retry
      if (errno == EINTR) {
        SPDLOG_ERROR("::write {} failed after {} EINTR retries", filename_, maxRetries);
        return errno;
      }
      assert(errno);
      return errno;  // Error status
    }
    retries = 0;  // Reset on successful write
    ptrData += write_result;
    sizesh -= write_result;
  }
  return EXIT_SUCCESS;
}

ssize_t posixBinaryFile::read(uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  assert(fd >= 0);
  if (fd < 0) return fd;

  constexpr int maxRetries = 5;
  for (int attempt = 0; attempt < maxRetries; ++attempt) {
    ssize_t read_size = ::pread(fd, ptrData, recordSize_, static_cast<off_t>(position));
    if (read_size == recordSize_) return EXIT_SUCCESS;
    if (read_size < 0) {
      if (errno == EINTR) continue;  // Retry
      SPDLOG_ERROR("::pread {} failed: {}", filename_, strerror(errno));
      return EXIT_FAILURE;
    }
    SPDLOG_WARN("::pread {} partial read: {} of {} bytes at pos {}", filename_, read_size, recordSize_, position);
    return EXIT_FAILURE;
  }
  SPDLOG_ERROR("::pread {} failed after {} EINTR retries", filename_, maxRetries);
  return EXIT_FAILURE;
}

size_t posixBinaryFile::count() {
  if (!std::filesystem::exists(filename_)) {
    return 0;
  }
  struct stat stat_buf;
  int rc = stat(filename_.c_str(), &stat_buf);
  if (rc != 0) {
    SPDLOG_ERROR("::stat {} failed: {}", filename_, strerror(errno));
    return -1;
  }
  return stat_buf.st_size / recordSize_;
}

}  // namespace rdb
