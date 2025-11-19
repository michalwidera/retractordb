#include "rdb/faccposix.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <filesystem>
namespace rdb {

posixBinaryFileAccessor::posixBinaryFileAccessor(const std::string_view fileName,  //
                                                 const size_t size,                //
                                                 int percounter)                   //
    : filename(std::string(fileName)), size(size), percounter_(percounter) {
  fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0)
    SPDLOG_ERROR("::open {} -> {}", filename, fd);
  else
    SPDLOG_INFO("::open {} -> {}", filename, fd);
  assert(fd >= 0);
}

posixBinaryFileAccessor::~posixBinaryFileAccessor() {
  ::close(fd);
  if (percounter_ >= 0) {
    SPDLOG_INFO("Percounter mode - rotating file on close: {}", filename);
    std::string rotated_filename = filename + ".old" + std::to_string(percounter_);
    std::error_code ec;
    std::filesystem::rename(filename, rotated_filename, ec);
    if (ec) {
      SPDLOG_ERROR("Failed to rotate file {} to {}: {}", filename, rotated_filename, ec.message());
    } else {
      SPDLOG_INFO("Rotated file {} to {}", filename, rotated_filename);
    }
  }
}

auto posixBinaryFileAccessor::name() const -> const std::string & { return filename; }

auto posixBinaryFileAccessor::name() -> std::string & { return filename; }

ssize_t posixBinaryFileAccessor::write(const uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  assert(fd >= 0);
  if (fd < 0) return errno;  // Error status

  if (ptrData == nullptr && position == 0) {
    // nullptr, position 0,0 - truncate file.
    auto result = ::ftruncate(fd, 0);
    assert(result != -1);
    return errno;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    auto result = ::lseek(fd, 0, SEEK_END);
    assert(result != -1);
  } else {
    auto result = ::lseek(fd, position, SEEK_SET);
    assert(result != -1);
    if (result == -1) return errno;  // Error status
  }
  size_t sizesh(size);
  while (sizesh > 0) {
    ssize_t write_result = ::write(fd, ptrData, sizesh);
    if (write_result < 0) {
      if (errno == EINTR) continue;  // Retry

      assert(errno);
      return errno;  // Error status
    }
    ptrData += write_result;
    sizesh -= write_result;
  }
  return EXIT_SUCCESS;
}

ssize_t posixBinaryFileAccessor::read(uint8_t *ptrData, const size_t position) {
  assert(size != 0);
  assert(fd >= 0);
  if (fd < 0) return fd;  // <- Error status

  ssize_t read_size = ::pread(fd, ptrData, size, static_cast<off_t>(position));
  return EXIT_SUCCESS;
}

size_t posixBinaryFileAccessor::count() {
  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);
  return rc == 0 ? stat_buf.st_size / size : -1;
}

}  // namespace rdb