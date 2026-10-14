#include "rdb/embed/storageGuard.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "configBridge.hpp"
#include "constants.hpp"
#include "rdb/storageFile.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
namespace {

constexpr std::string_view kOwnerFile = ".retractordb-embedded";
constexpr std::string_view kOwnerText = "retractordb embedded storage v1\n";

std::unexpected<Error> ioError(const std::string &path, const int err) {
  return fail(Errc::IO, std::format("storage: '{}': {}", path, std::strerror(err)), err);
}

std::unexpected<Error> planError(const std::string &message) { return fail(Errc::Compile, message); }

std::unexpected<Error> busy(const std::string &path) { return fail(Errc::Config, "storage is busy: " + path, EBUSY); }

Result<std::filesystem::path> resolve(const std::string &path) {
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(path, ec);
  if (ec) return ioError(path, ec.value());
  auto resolved = std::filesystem::weakly_canonical(absolute, ec);
  if (ec) return ioError(path, ec.value());
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

Result<std::unique_ptr<StorageGuard>> StorageGuard::create(const std::string_view directory, const bool claim) {
  std::unique_ptr<StorageGuard> guard(new StorageGuard);
  RDB_TRY(guard->initialize(directory, claim));
  return guard;
}

Result<> StorageGuard::initialize(const std::string_view directory, const bool claim) {
  RDB_TRY_ASSIGN(const auto path, resolve(directory.empty() ? "." : std::string(directory)));
  directory_ = path.string();
  fd_        = ::open(directory_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd_ < 0) return ioError(directory_, errno);
  // Kazde open ma osobny opis pliku, wiec flock wykrywa takze drugi obiekt w tym procesie.
  if (::flock(fd_, LOCK_EX | LOCK_NB) != 0) {
    const int err = errno;
    if (err == EWOULDBLOCK || err == EAGAIN) return busy(directory_);
    return ioError(directory_, err);
  }
  struct stat held{};
  struct stat named{};
  if (::fstat(fd_, &held) != 0 || ::stat(directory_.c_str(), &named) != 0) return ioError(directory_, errno);
  if (held.st_dev != named.st_dev || held.st_ino != named.st_ino) return planError("storage directory changed: " + directory_);

  const std::string marker = (std::filesystem::path(directory_) / kOwnerFile).string();
  struct stat info{};
  if (::lstat(marker.c_str(), &info) != 0) {
    const int err = errno;
    if (err != ENOENT) return ioError(marker, err);
    if (!claim) {
      ::close(fd_);
      fd_ = -1;
      return {};
    }
    std::error_code ec;
    const bool empty = std::filesystem::is_empty(directory_, ec);
    if (ec) return ioError(directory_, ec.value());
    if (!empty) return planError("Engine requires an empty directory or an owned embedded storage: " + directory_);
    const StorageFd owner(marker, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (!owner.isOpen()) return ioError(marker, errno);
    if (!owner.write(std::as_bytes(std::span(kOwnerText)))) return ioError(marker, errno != 0 ? errno : EIO);
  } else {
    if (!S_ISREG(info.st_mode) || info.st_uid != ::geteuid() || info.st_nlink != 1)
      return planError("invalid embedded storage owner marker: " + marker);
    const StorageFd owner(marker, O_RDONLY | O_NONBLOCK);
    if (!owner.isOpen()) return ioError(marker, errno);
    std::string contents(kOwnerText.size() + 1, '\0');
    const auto count = owner.readAt(std::as_writable_bytes(std::span(contents)), 0);
    if (count < 0) return ioError(marker, errno);
    if (std::cmp_not_equal(count, kOwnerText.size()) || !contents.starts_with(kOwnerText))
      return planError("invalid embedded storage owner marker: " + marker);
  }
  return {};
}

StorageGuard::~StorageGuard() {
  if (fd_ >= 0) ::close(fd_);
}

Result<> StorageGuard::checkPath(const std::string &path) const {
  const std::filesystem::path base(directory_);
  RDB_TRY_ASSIGN(const auto resolved, resolve(path));
  const auto [end, rest] = std::ranges::mismatch(base, resolved);
  if (end != base.end() || rest == resolved.end() || resolved == base / kOwnerFile)
    return planError("embedded storage artifact leaves its owned directory or names its owner marker: " + path);
  return {};
}

Result<OfflinePolicy> offlinePolicy(const std::string_view lockDir, const std::string_view configPath) {
  if (!lockDir.empty() && !std::filesystem::path(lockDir).is_absolute()) return fail(Errc::Config, "lock_dir must be absolute");
  OfflinePolicy policy{.lockDir = std::string(lockDir)};
  // Tylko wskazany plik, bez warstw /etc i XDG: biblioteka nie szuka konfiguracji sama.
  AppConfig cfg;
  if (!configPath.empty()) {
    RDB_TRY_ASSIGN(cfg, embeddedConfig(std::string(configPath)));
    if (!cfg.embeddedLenientLockDirsError.empty())
      return fail(Errc::Config, std::format("Configuration error in '{}': {}", configPath, cfg.embeddedLenientLockDirsError));
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

Result<> checkOffline(const OfflinePolicy &policy) {
  // Tryb lagodny: niepewnosc nie jest odmowa, bo domyslny katalog to tylko zgadywanie,
  // a we wspoldzielonym /tmp obcy wpis blokowalby wszystkich.
  const auto uncertainty = [&](const std::string &path, const int err) -> Result<> {
    if (!policy.lenient) return ioError(path, err);
    return {};
  };
  std::error_code ec;
  const auto directory =
      policy.lockDir.empty() ? std::filesystem::temp_directory_path(ec) : std::filesystem::path(policy.lockDir);
  if (ec) return uncertainty(policy.lockDir.empty() ? "TMPDIR" : policy.lockDir, ec.value());
  std::filesystem::directory_iterator entry(directory, ec);
  if (ec) return uncertainty(directory.string(), ec.value());
  // increment(ec) przy bledzie konczy iteracje; blad oceniamy po petli.
  for (; entry != std::filesystem::directory_iterator(); entry.increment(ec)) {
    const auto &path = entry->path();
    if (!ipc::serviceLockInstance(path.filename().string())) continue;
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
      const int err = errno;
      if (err != ENOENT) RDB_TRY(uncertainty(path.string(), err));
      continue;
    }
    struct stat info{};
    const int inspected  = ::fstat(fd, &info);
    const int inspectErr = errno;
    const int locked     = inspected == 0 && S_ISREG(info.st_mode) ? ::flock(fd, LOCK_SH | LOCK_NB) : 0;
    const int lockErr    = errno;
    ::close(fd);
    if (inspected != 0) {
      RDB_TRY(uncertainty(path.string(), inspectErr));
      continue;
    }
    if (!S_ISREG(info.st_mode)) {
      if (!policy.lenient) return planError("invalid xretractor lock file: " + path.string());
      continue;
    }
    if (locked != 0) {
      if (lockErr == EWOULDBLOCK || lockErr == EAGAIN) return busy(path.string());
      RDB_TRY(uncertainty(path.string(), lockErr));
    }
  }
  if (ec) return uncertainty(directory.string(), ec.value());
  return {};
}

Result<std::string> temporaryStorageDirectory() {
  std::error_code ec;
  const auto parent = std::filesystem::temp_directory_path(ec);
  if (ec) return ioError("TMPDIR", ec.value());
  const std::string pattern = (parent / "retractordb-engine-XXXXXX").string();
  std::vector<char> name(pattern.begin(), pattern.end());
  name.push_back('\0');
  if (::mkdtemp(name.data()) == nullptr) return ioError(pattern, errno);
  return std::string(name.data());
}

void removeTemporaryStorage(const std::string &directory) noexcept {
  const auto guard = StorageGuard::create(directory, true);
  if (!guard) return;
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

}  // namespace rdb::embed
