#include "rdb/faccposixshd.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <filesystem>
#include <memory>

namespace rdb {

std::string posixBinaryFileWithShadow::shadowName() const { return filename_ + ".shadow"; }

/// @brief Wyszukuje rekord w pliku cienia na podstawie pozycji.
/// Shadow file przechowuje pary (size_t position, uint8_t data[recordSize_]).
/// Przeszukuje wpisy od końca — ostatni wpis z daną pozycją jest aktualny.
ssize_t posixBinaryFileWithShadow::shadowFind(uint8_t *ptrData, size_t position) {
  struct stat stat_buf;
  if (fstat(fd_shadow, &stat_buf) != 0) return EXIT_FAILURE;

  const ssize_t entrySize  = sizeof(size_t) + recordSize_;
  const ssize_t numEntries = stat_buf.st_size / entrySize;
  if (numEntries == 0) return EXIT_FAILURE;

  // Szukaj od końca — ostatni wpis z daną pozycją ma najnowsze dane
  for (ssize_t i = numEntries - 1; i >= 0; --i) {
    size_t storedPos;
    ssize_t rd = ::pread(fd_shadow, &storedPos, sizeof(size_t), i * entrySize);
    if (rd != sizeof(size_t)) continue;

    if (storedPos == position) {
      rd = ::pread(fd_shadow, ptrData, recordSize_, i * entrySize + sizeof(size_t));
      if (rd != recordSize_) return EXIT_FAILURE;
      return EXIT_SUCCESS;
    }
  }
  return EXIT_FAILURE;
}

posixBinaryFileWithShadow::posixBinaryFileWithShadow(const std::string_view fileName, const Descriptor &descriptor,
                                                     int percounter)
    : filename_(std::string(fileName)),
      recordSize_(descriptor.getSizeInBytes()),
      percounter_(percounter) {
  assert(recordSize_ > 0);

  std::error_code fs_ec;
  const bool mainFileExisted = std::filesystem::exists(filename_, fs_ec);
  if (fs_ec) {
    SPDLOG_WARN("Failed to check if {} exists: {}", filename_, fs_ec.message());
  }

  fs_ec.clear();
  const bool shadowFileExisted = std::filesystem::exists(shadowName(), fs_ec);
  if (fs_ec) {
    SPDLOG_WARN("Failed to check if {} exists: {}", shadowName(), fs_ec.message());
  }

  fd = ::open(filename_.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0)
    SPDLOG_ERROR("::open {} -> {}", filename_, fd);
  else
    SPDLOG_INFO("::open {} -> {}", filename_, fd);
  assert(fd >= 0);

  fd_shadow = ::open(shadowName().c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd_shadow < 0)
    SPDLOG_ERROR("::open shadow {} -> {}", shadowName(), fd_shadow);
  else
    SPDLOG_INFO("::open shadow {} -> {}", shadowName(), fd_shadow);
  assert(fd_shadow >= 0);

  if (fd >= 0 && mainFileExisted) {
    const off_t mainSize = ::lseek(fd, 0, SEEK_END);
    if (mainSize < 0) {
      SPDLOG_ERROR("::lseek {} failed during state restore: {}", filename_, strerror(errno));
    } else {
      const off_t alignedMainSize = (mainSize / recordSize_) * recordSize_;
      if (alignedMainSize != mainSize) {
        SPDLOG_WARN("{} has {} trailing bytes; truncating to {} to restore consistent records", filename_,
                    mainSize - alignedMainSize, alignedMainSize);
        if (::ftruncate(fd, alignedMainSize) != 0) {
          SPDLOG_ERROR("::ftruncate {} to {} failed during state restore: {}", filename_, alignedMainSize, strerror(errno));
        }
      }
      SPDLOG_INFO("Restored {} with {} persisted records", filename_, alignedMainSize / recordSize_);
    }
  }

  if (fd_shadow >= 0 && shadowFileExisted) {
    const ssize_t entrySize = sizeof(size_t) + recordSize_;
    const off_t shadowSize  = ::lseek(fd_shadow, 0, SEEK_END);
    if (shadowSize < 0) {
      SPDLOG_ERROR("::lseek shadow {} failed during state restore: {}", shadowName(), strerror(errno));
    } else {
      const off_t alignedShadowSize = (shadowSize / entrySize) * entrySize;
      if (alignedShadowSize != shadowSize) {
        SPDLOG_WARN("{} has {} trailing bytes; truncating to {} to restore consistent shadow entries", shadowName(),
                    shadowSize - alignedShadowSize, alignedShadowSize);
        if (::ftruncate(fd_shadow, alignedShadowSize) != 0) {
          SPDLOG_ERROR("::ftruncate shadow {} to {} failed during state restore: {}", shadowName(), alignedShadowSize,
                       strerror(errno));
        }
      }
      SPDLOG_INFO("Restored {} with {} pending shadow entries", shadowName(), alignedShadowSize / entrySize);
    }
  }
}

posixBinaryFileWithShadow::~posixBinaryFileWithShadow() {
  if (fd_shadow >= 0) {
    if (::fsync(fd_shadow) != 0) {
      SPDLOG_ERROR("::fsync shadow {} failed before close: {}", shadowName(), strerror(errno));
    }
    if (::close(fd_shadow) != 0) {
      SPDLOG_ERROR("::close shadow {} failed: {}", shadowName(), strerror(errno));
    }
    fd_shadow = -1;
  }

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

auto posixBinaryFileWithShadow::name() -> std::string & { return filename_; }

ssize_t posixBinaryFileWithShadow::write(const uint8_t *ptrData, const std::vector<bool> & /*nullBitset*/,
                                         const size_t position) {
  assert(recordSize_ != 0);
  assert(fd >= 0);
  if (fd < 0) return errno;

  if (ptrData == nullptr && position == 0) {
    // Truncate — czyści oba pliki
    std::filesystem::remove(name());
    std::filesystem::remove(name() + ".shadow");
    return EXIT_SUCCESS;
  }

  if (position == std::numeric_limits<size_t>::max()) {
    // Append → zapis do głównego pliku
    auto result = ::lseek(fd, 0, SEEK_END);
    assert(result != -1);
    if (result == -1) return errno;

    ssize_t sizesh(recordSize_);
    const uint8_t *ptr       = ptrData;
    constexpr int maxRetries = 5;
    int retries              = 0;
    while (sizesh > 0) {
      ssize_t write_result = ::write(fd, ptr, sizesh);
      if (write_result < 0) {
        if (errno == EINTR && ++retries <= maxRetries) continue;
        if (errno == EINTR) {
          SPDLOG_ERROR("::write {} failed after {} EINTR retries", filename_, maxRetries);
          return errno;
        }
        SPDLOG_ERROR("::write {} failed: {}", filename_, strerror(errno));
        return EXIT_FAILURE;
      }
      retries = 0;
      ptr += write_result;
      sizesh -= write_result;
    }
    return EXIT_SUCCESS;
  }

  // Update → zapis do pliku cienia jako para (position, data)
  auto result = ::lseek(fd_shadow, 0, SEEK_END);
  assert(result != -1);
  if (result == -1) return errno;

  // Zapisz pozycję
  ssize_t wr = ::write(fd_shadow, &position, sizeof(size_t));
  if (wr != sizeof(size_t)) {
    SPDLOG_ERROR("::write shadow position {} failed: {}", shadowName(), strerror(errno));
    return EXIT_FAILURE;
  }

  // Zapisz dane
  ssize_t sizesh(recordSize_);
  const uint8_t *ptr       = ptrData;
  constexpr int maxRetries = 5;
  int retries              = 0;
  while (sizesh > 0) {
    ssize_t write_result = ::write(fd_shadow, ptr, sizesh);
    if (write_result < 0) {
      if (errno == EINTR && ++retries <= maxRetries) continue;
      if (errno == EINTR) {
        SPDLOG_ERROR("::write shadow {} failed after {} EINTR retries", shadowName(), maxRetries);
        return errno;
      }
      SPDLOG_ERROR("::write shadow {} failed: {}", shadowName(), strerror(errno));
      return EXIT_FAILURE;
    }
    retries = 0;
    ptr += write_result;
    sizesh -= write_result;
  }
  return EXIT_SUCCESS;
}

ssize_t posixBinaryFileWithShadow::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  nullBitset.clear();
  assert(recordSize_ != 0);
  assert(fd >= 0);
  if (fd < 0) return fd;

  // Najpierw szukaj w pliku cienia
  if (shadowFind(ptrData, position) == EXIT_SUCCESS) return EXIT_SUCCESS;

  // Fallback — odczyt z głównego pliku
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

size_t posixBinaryFileWithShadow::count() {
  if (!std::filesystem::exists(filename_)) {
    return 0;
  }
  struct stat stat_buf;
  int rc = stat(filename_.c_str(), &stat_buf);
  if (rc != 0) {
    SPDLOG_ERROR("::stat {} failed: {}", filename_, strerror(errno));
    return 0;
  }
  return stat_buf.st_size / recordSize_;
}

ssize_t posixBinaryFileWithShadow::merge() {
  struct stat shadow_stat;
  if (fstat(fd_shadow, &shadow_stat) != 0) {
    SPDLOG_ERROR("::fstat shadow {} failed: {}", shadowName(), strerror(errno));
    return EXIT_FAILURE;
  }

  const ssize_t entrySize  = sizeof(size_t) + recordSize_;
  const ssize_t numEntries = shadow_stat.st_size / entrySize;
  if (numEntries == 0) return EXIT_SUCCESS;

  auto buffer = std::make_unique<uint8_t[]>(recordSize_);

  for (ssize_t i = 0; i < numEntries; ++i) {
    size_t storedPos;
    ssize_t rd = ::pread(fd_shadow, &storedPos, sizeof(size_t), i * entrySize);
    if (rd != sizeof(size_t)) continue;

    rd = ::pread(fd_shadow, buffer.get(), recordSize_, i * entrySize + sizeof(size_t));
    if (rd != recordSize_) continue;

    // Nadpisz rekord w głównym pliku
    ssize_t wr = ::pwrite(fd, buffer.get(), recordSize_, static_cast<off_t>(storedPos));
    if (wr != recordSize_) {
      SPDLOG_ERROR("::pwrite {} failed at pos {}: {}", filename_, storedPos, strerror(errno));
      return EXIT_FAILURE;
    }
  }

  // Wyczyść plik cienia po udanym scaleniu
  auto result = ::ftruncate(fd_shadow, 0);
  if (result != 0) {
    SPDLOG_ERROR("::ftruncate shadow {} failed: {}", shadowName(), strerror(errno));
    return EXIT_FAILURE;
  }
  SPDLOG_INFO("Merged {} shadow entries into {}", numEntries, filename_);
  return EXIT_SUCCESS;
}

}  // namespace rdb
