#include "ipcSecurity.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>

#include <spdlog/spdlog.h>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include "constants.hpp"
#include "osPlatform.hpp"
#include "platformConfig.h"

#if RDB_HAS_BOOST_POSIX_SHM != defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)
#error "Backend IPC nie zgadza sie z proba Boosta; rekonfiguruj build."
#endif

namespace ipc::security {
namespace {

namespace IPC = boost::interprocess;

constexpr mode_t kPermissionBits            = S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO;
constexpr mode_t kDirectoryPermissions      = S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO;
constexpr mode_t kUntrustedWritePermissions = S_IWGRP | S_IWOTH;

std::filesystem::path objectPath(const std::string &name) {
#if RDB_HAS_BOOST_POSIX_SHM
  return std::filesystem::path(osplat::sharedMemoryBackingPath()) / name;
#else
  std::string path;
  IPC::ipcdetail::shared_filepath(name.c_str(), path);
  return path;
#endif
}

[[noreturn]] void reject(const std::string &name, const std::string &reason, std::optional<uid_t> owner = {}) {
  const std::string detail =
      owner ? std::format("IPC object '{}': {} (owner UID {}, expected UID {})", name, reason, *owner, geteuid())
            : std::format("IPC object '{}': {} (owner UID unavailable, expected UID {})", name, reason, geteuid());
  SPDLOG_ERROR("{}", detail);
  throw Rejected(detail);
}

struct stat metadata(int fd, const std::string &name) {
  struct stat info{};
  if (fstat(fd, &info) != 0) reject(name, std::format("fstat failed: {}", std::strerror(errno)));
  return info;
}

void checkIdentity(const struct stat &held, const std::string &name) {
  struct stat named{};
  if (lstat(objectPath(name).c_str(), &named) != 0)
    reject(name, std::format("name changed or lstat failed: {}", std::strerror(errno)), held.st_uid);
  if (held.st_dev != named.st_dev || held.st_ino != named.st_ino || !S_ISREG(named.st_mode))
    reject(name, "name no longer identifies the checked object", named.st_uid);
}

void checkMetadata(const struct stat &info, const std::string &name, bool checkMode) {
  if (info.st_uid != geteuid()) reject(name, "untrusted owner", info.st_uid);
  if (!S_ISREG(info.st_mode) || info.st_nlink != 1) reject(name, "not a regular object with a single link", info.st_uid);
  if (checkMode && (info.st_mode & kPermissionBits) != kObjectPermissions)
    reject(name, std::format("mode {:04o}, required 0600", info.st_mode & kPermissionBits), info.st_uid);
  checkIdentity(info, name);
}

[[noreturn]] void creationFailed(const std::string &name, const IPC::interprocess_exception &error) {
  struct stat info{};
  const bool found = lstat(objectPath(name).c_str(), &info) == 0;
  reject(name, std::format("exclusive creation failed: {}", error.what()), found ? std::optional(info.st_uid) : std::nullopt);
}

}  // namespace

void checkNamespace(const std::string &name, bool createDirectories) {
  // Nazwy sa identyfikatorami IPC, nigdy sciezkami dostarczonymi przez klienta.
  if (name.empty() || name.find('/') != std::string::npos) reject(name, "invalid IPC name");
  const auto parent = objectPath(name).parent_path();
  std::filesystem::path current;
  for (const auto &part : parent) {
    current /= part;
    struct stat entry{};
    if (lstat(current.c_str(), &entry) != 0) {
      if (!createDirectories && errno == ENOENT) throw IPC::interprocess_exception(IPC::error_info(ENOENT));
      if (!createDirectories || errno != ENOENT)
        reject(name, std::format("cannot inspect directory '{}': {}", current.string(), std::strerror(errno)));
      if (mkdir(current.c_str(), kDirectoryPermissions) != 0 && errno != EEXIST)
        reject(name, std::format("cannot create directory '{}': {}", current.string(), std::strerror(errno)));
      if (lstat(current.c_str(), &entry) != 0) reject(name, "cannot inspect newly created IPC directory");
    }
    // /tmp bywa systemowym dowiazaniem na Darwinie. Wlasciciel dowiazania i celu
    // musi byc zaufany; katalog obcego konta daje mu prawo podmiany wpisow mimo sticky.
    if (entry.st_uid != 0 && entry.st_uid != geteuid())
      reject(name, std::format("untrusted directory '{}'", current.string()), entry.st_uid);
    if (stat(current.c_str(), &entry) != 0) reject(name, "cannot inspect IPC directory target");
    if (!S_ISDIR(entry.st_mode) || (entry.st_uid != 0 && entry.st_uid != geteuid()) ||
        ((entry.st_mode & kUntrustedWritePermissions) && !(entry.st_mode & S_ISVTX)))
      reject(name, std::format("directory '{}' permits name substitution", current.string()), entry.st_uid);
  }
}

void checkOpened(const IPC::shared_memory_object &object, const std::string &name) {
  checkNamespace(name);
  checkMetadata(metadata(object.get_mapping_handle().handle, name), name, true);
}

IPC::shared_memory_object createMemory(const std::string &name) {
  checkNamespace(name, true);
  try {
    IPC::shared_memory_object object(IPC::create_only, name.c_str(), IPC::read_write, IPC::permissions(kObjectPermissions));
    checkOpened(object, name);
    return object;
  } catch (const IPC::interprocess_exception &ex) {
    creationFailed(name, ex);
  }
}

IPC::shared_memory_object openMemory(const std::string &name, IPC::mode_t mode) {
  checkNamespace(name);
  // Sprawdzamy takze sam wpis: Boost w backendzie plikowym otwiera dowiazania.
  struct stat named{};
  if (lstat(objectPath(name).c_str(), &named) == 0) checkMetadata(named, name, true);
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), mode);
  checkOpened(object, name);
  return object;
}

std::unique_ptr<IPC::message_queue> createQueue(const std::string &name, std::size_t elements, std::size_t messageSize) {
  checkNamespace(name, true);
  try {
    auto queue         = std::make_unique<IPC::message_queue>(IPC::create_only, name.c_str(), elements, messageSize,
                                                              IPC::permissions(kObjectPermissions));
    const auto checked = openMemory(name, IPC::read_only);
    return queue;
  } catch (const IPC::interprocess_exception &ex) {
    creationFailed(name, ex);
  }
}

std::unique_ptr<IPC::message_queue> openQueue(const std::string &name) {
  // Kolejka Boosta nie udostepnia deskryptora. Najpierw otwieramy jej obiekt
  // bez mapowania i sprawdzamy UID/0600. W chronionym katalogu inne konto nie
  // moze odlinkowac zweryfikowanego wpisu. Po konstrukcji sprawdzamy jego tozsamosc.
  const auto object = openMemory(name, IPC::read_only);
  const auto held   = metadata(object.get_mapping_handle().handle, name);
  auto queue        = std::make_unique<IPC::message_queue>(IPC::open_only, name.c_str());
  checkIdentity(held, name);
  return queue;
}

bool removeOwned(const std::string &name) noexcept {
  try {
    struct stat named{};
    if (lstat(objectPath(name).c_str(), &named) != 0) {
      if (errno == ENOENT) return true;
      reject(name, std::format("cannot inspect object for cleanup: {}", std::strerror(errno)));
    }
    checkNamespace(name);
    // Nie mapujemy ani nie czytamy starej tresci. Blokada wolajacego wyklucza
    // zywy serwer, UID i chroniony katalog wykluczaja podmiane przez inne konto.
    checkMetadata(named, name, false);
    if (!IPC::shared_memory_object::remove(name.c_str())) reject(name, "unlink failed", named.st_uid);
    return true;
  } catch (const Rejected &) {
    return false;
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("IPC object '{}': cleanup failed: {}", name, ex.what());
    return false;
  }
}

}  // namespace ipc::security
