#include "rdb/embed/storageGuard.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include "constants.hpp"
#include "rdb/embed/engine.hpp"
#include "rdb/storageFile.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
namespace {

constexpr std::string_view kOwnerFile = ".retractordb-embedded";
constexpr std::string_view kOwnerText = "retractordb embedded storage v1\n";

[[noreturn]] void ioError(const std::string &path, const int err) {
  throw Refused(Refused::Stage::Check,
                {ErrorClass::Environment, std::format("storage: '{}': {}", path, std::strerror(err)), err});
}

[[noreturn]] void planError(const std::string &message) { throw Refused(Refused::Stage::Check, {ErrorClass::Plan, message}); }

[[noreturn]] void busy(const std::string &path) {
  throw Refused(Refused::Stage::Request, {ErrorClass::Request, "storage is busy: " + path, EBUSY});
}

std::filesystem::path resolve(const std::string &path) {
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(path, ec);
  if (ec) ioError(path, ec.value());
  const auto resolved = std::filesystem::weakly_canonical(absolute, ec);
  if (ec) ioError(path, ec.value());
  return resolved;
}

/// Porownanie po rozwiazaniu dowiazan; sciezka bez katalogu porownywana leksykalnie.
bool sameDirectory(const std::filesystem::path &left, const std::filesystem::path &right) {
  std::error_code ec;
  const auto a = std::filesystem::weakly_canonical(left, ec);
  if (ec) return left.lexically_normal() == right.lexically_normal();
  const auto b = std::filesystem::weakly_canonical(right, ec);
  return !ec && a == b;
}

}  // namespace

StorageGuard::StorageGuard(const std::string_view directory, const bool claim)
    : directory_(resolve(directory.empty() ? "." : std::string(directory)).string()) {
  fd_ = ::open(directory_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd_ < 0) ioError(directory_, errno);
  try {
    // Kazde open ma osobny opis pliku, wiec flock wykrywa takze drugi obiekt w tym procesie.
    if (::flock(fd_, LOCK_EX | LOCK_NB) != 0) {
      const int err = errno;
      if (err == EWOULDBLOCK || err == EAGAIN) busy(directory_);
      ioError(directory_, err);
    }
    struct stat held{};
    struct stat named{};
    if (::fstat(fd_, &held) != 0 || ::stat(directory_.c_str(), &named) != 0) ioError(directory_, errno);
    if (held.st_dev != named.st_dev || held.st_ino != named.st_ino) planError("storage directory changed: " + directory_);

    const std::string marker = (std::filesystem::path(directory_) / kOwnerFile).string();
    struct stat info{};
    if (::lstat(marker.c_str(), &info) != 0) {
      const int err = errno;
      if (err != ENOENT) ioError(marker, err);
      if (!claim) {
        ::close(fd_);
        fd_ = -1;
        return;
      }
      std::error_code ec;
      const bool empty = std::filesystem::is_empty(directory_, ec);
      if (ec) ioError(directory_, ec.value());
      if (!empty) planError("Engine requires an empty directory or an owned embedded storage: " + directory_);
      const StorageFd owner(marker, O_WRONLY | O_CREAT | O_EXCL, 0600);
      if (!owner.isOpen()) ioError(marker, errno);
      if (!owner.write(std::as_bytes(std::span(kOwnerText)))) ioError(marker, errno != 0 ? errno : EIO);
    } else {
      if (!S_ISREG(info.st_mode) || info.st_uid != ::geteuid() || info.st_nlink != 1)
        planError("invalid embedded storage owner marker: " + marker);
      const StorageFd owner(marker, O_RDONLY | O_NONBLOCK);
      if (!owner.isOpen()) ioError(marker, errno);
      std::string contents(kOwnerText.size() + 1, '\0');
      const auto count = owner.readAt(std::as_writable_bytes(std::span(contents)), 0);
      if (count < 0) ioError(marker, errno);
      if (count != static_cast<ssize_t>(kOwnerText.size()) || contents.substr(0, kOwnerText.size()) != kOwnerText)
        planError("invalid embedded storage owner marker: " + marker);
    }
  } catch (...) {
    ::close(fd_);
    fd_ = -1;
    throw;
  }
}

StorageGuard::~StorageGuard() {
  if (fd_ >= 0) ::close(fd_);
}

void StorageGuard::checkPath(const std::string &path) const {
  const std::filesystem::path base(directory_);
  const auto resolved    = resolve(path);
  const auto [end, rest] = std::ranges::mismatch(base, resolved);
  if (end != base.end() || rest == resolved.end() || resolved == base / kOwnerFile)
    planError("embedded storage artifact leaves its owned directory or names its owner marker: " + path);
}

OfflinePolicy offlinePolicy(const std::string_view lockDir, const std::string_view configPath) {
  if (!lockDir.empty() && !std::filesystem::path(lockDir).is_absolute())
    throw Refused(Refused::Stage::Request, {ErrorClass::Request, "lock_dir must be absolute"});
  OfflinePolicy policy{.lockDir = std::string(lockDir)};
  // Tylko wskazany plik, bez warstw /etc i XDG: biblioteka nie szuka konfiguracji sama.
  AppConfig cfg;
  if (!configPath.empty()) {
    try {
      cfg = loadAppConfig(std::string(configPath));
    } catch (const std::exception &error) {
      throw Refused(Refused::Stage::Request, {ErrorClass::Request, error.what()});
    }
    if (!cfg.embeddedLenientLockDirsError.empty())
      throw Refused(Refused::Stage::Request, {ErrorClass::Request, std::format("Configuration error in '{}': {}", configPath,
                                                                               cfg.embeddedLenientLockDirsError)});
    if (policy.lockDir.empty()) policy.lockDir = cfg.lockDir;
  }

  // Katalog tymczasowy systemu tak, jak go bierze serwis przy pustym paths.lock_dir.
  std::error_code ec;
  const std::filesystem::path systemTemp = std::filesystem::temp_directory_path(ec);
  // Bez klucza lista lagodnych katalogow to z definicji katalog tymczasowy systemu.
  if (!cfg.embeddedLenientLockDirs) {
    policy.lenient = policy.lockDir.empty() || (!ec && sameDirectory(policy.lockDir, systemTemp));
    return policy;
  }
  const std::filesystem::path checked = policy.lockDir.empty() ? systemTemp : std::filesystem::path(policy.lockDir);
  policy.lenient                      = !checked.empty() && std::ranges::any_of(*cfg.embeddedLenientLockDirs,
                                                                                [&](const std::string &dir) { return sameDirectory(checked, dir); });
  return policy;
}

void checkOffline(const OfflinePolicy &policy) {
  // Tryb lagodny: niepewnosc nie jest odmowa, bo domyslny katalog to tylko zgadywanie,
  // a we wspoldzielonym /tmp obcy wpis blokowalby wszystkich.
  const auto fail = [&](const std::string &path, const int err) {
    if (!policy.lenient) ioError(path, err);
  };
  std::error_code ec;
  const auto directory =
      policy.lockDir.empty() ? std::filesystem::temp_directory_path(ec) : std::filesystem::path(policy.lockDir);
  if (ec) return fail(policy.lockDir.empty() ? "TMPDIR" : policy.lockDir, ec.value());
  std::filesystem::directory_iterator entry(directory, ec);
  if (ec) return fail(directory.string(), ec.value());
  // increment(ec) przy bledzie konczy iteracje; blad oceniamy po petli.
  for (; entry != std::filesystem::directory_iterator(); entry.increment(ec)) {
    const auto &path = entry->path();
    if (!ipc::serviceLockInstance(path.filename().string())) continue;
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
      const int err = errno;
      if (err != ENOENT) fail(path.string(), err);
      continue;
    }
    struct stat info{};
    const int inspected  = ::fstat(fd, &info);
    const int inspectErr = errno;
    const int locked     = inspected == 0 && S_ISREG(info.st_mode) ? ::flock(fd, LOCK_SH | LOCK_NB) : 0;
    const int lockErr    = errno;
    ::close(fd);
    if (inspected != 0) {
      fail(path.string(), inspectErr);
      continue;
    }
    if (!S_ISREG(info.st_mode)) {
      if (!policy.lenient) planError("invalid xretractor lock file: " + path.string());
      continue;
    }
    if (locked != 0) {
      if (lockErr == EWOULDBLOCK || lockErr == EAGAIN) busy(path.string());
      fail(path.string(), lockErr);
    }
  }
  if (ec) fail(directory.string(), ec.value());
}

std::string temporaryStorageDirectory() {
  std::error_code ec;
  const auto parent = std::filesystem::temp_directory_path(ec);
  if (ec) ioError("TMPDIR", ec.value());
  const std::string pattern = (parent / "retractordb-engine-XXXXXX").string();
  std::vector<char> name(pattern.begin(), pattern.end());
  name.push_back('\0');
  if (::mkdtemp(name.data()) == nullptr) ioError(pattern, errno);
  return name.data();
}

void removeTemporaryStorage(const std::string &directory) noexcept {
  try {
    const StorageGuard guard(directory, true);
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
  } catch (...) {
    // Zajety lub niedostepny magazyn zostaje: sprzatanie nie moze zniszczyc zywego odczytu.
  }
}

}  // namespace rdb::embed
