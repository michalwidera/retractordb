#include "rdb/storageRotation.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

#include <spdlog/spdlog.h>

namespace rdb {
namespace {

bool syncDirectory(const std::filesystem::path &directory, const std::string &source, const std::string &archive) {
  const int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) {
    const int error = errno;
    SPDLOG_ERROR("Rotation of '{}' to '{}': open directory '{}' failed: {}", source, archive, directory.string(),
                 std::strerror(error));
    return false;
  }
  bool ok = true;
  if (::fsync(fd) != 0) {
    const int error = errno;
    SPDLOG_ERROR("Rotation of '{}' to '{}': fsync directory '{}' failed: {}", source, archive, directory.string(),
                 std::strerror(error));
    ok = false;
  }
  // close nie ponawiamy: po bledzie deskryptor mogl juz zostac zwolniony i uzyty ponownie.
  if (::close(fd) != 0) {
    const int error = errno;
    SPDLOG_ERROR("Rotation of '{}' to '{}': close directory '{}' failed: {}", source, archive, directory.string(),
                 std::strerror(error));
    ok = false;
  }
  return ok;
}

}  // namespace

bool rotateStorageFile(const std::string &source, const std::string &archive) {
  std::error_code ec;
  const bool overwrites = std::filesystem::exists(archive, ec);
  if (ec) {
    SPDLOG_ERROR("Rotation of '{}' to '{}': checking archive failed: {}", source, archive, ec.message());
    return false;
  }
  if (::rename(source.c_str(), archive.c_str()) != 0) {
    const int error = errno;
    SPDLOG_ERROR("Failed to rotate file {} to {}: {}", source, archive, std::strerror(error));
    return false;
  }
  // Nadpisanie istniejacego archiwum musi byc widoczne takze w Release (#281).
  if (overwrites) SPDLOG_ERROR("Rotation of {} overwrote existing archive {}; its previous content is lost", source, archive);

  const std::filesystem::path sourcePath(source);
  const std::filesystem::path archivePath(archive);
  const auto sourceDirectory  = sourcePath.has_parent_path() ? sourcePath.parent_path() : std::filesystem::path(".");
  const auto archiveDirectory = archivePath.has_parent_path() ? archivePath.parent_path() : std::filesystem::path(".");
  bool ok                     = syncDirectory(sourceDirectory, source, archive);
  // Nawet po bledzie pierwszego fsync probujemy utrwalic drugi katalog.
  if (archiveDirectory != sourceDirectory) {
    const bool archiveOk = syncDirectory(archiveDirectory, source, archive);
    ok                   = ok && archiveOk;
  }
  return ok;
}

}  // namespace rdb
