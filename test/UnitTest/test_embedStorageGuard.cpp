#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include "rdb/embed/storageGuard.hpp"
#include "rdbResult.hpp"
#include "syscallWrap.hpp"

// covers: storageguard.remove-temporary-storage.2~1
static_assert(noexcept(rdb::embed::removeTemporaryStorage(std::declval<const std::string &>())));

namespace {
namespace fs = std::filesystem;
enum class GuardFault { none, device, inode, stat, flock, owner };
GuardFault guardFault    = GuardFault::none;
bool observeDirectory    = false;
int observedFd           = -1;
int observedFlags        = 0;
int observedOperation    = 0;
bool failTemporaryCreate = false;

class embedStorageGuardBoundary : public ::testing::Test {
 protected:
  const fs::path directory = fs::absolute("guard_boundary");
  void SetUp() override {
    fs::remove_all(directory);
    fs::create_directory(directory);
  }
  void TearDown() override {
    guardFault          = GuardFault::none;
    observeDirectory    = false;
    failTemporaryCreate = false;
    fs::remove_all(directory);
  }
};

std::string readText(const fs::path &path) {
  std::ifstream input(path);
  return std::string(std::istreambuf_iterator<char>(input), {});
}
}  // namespace

extern "C" int __real_fstat(int fd, struct stat *info);
extern "C" int __real_flock(int fd, int operation);
extern "C" int __real_lstat(const char *path, struct stat *info);
extern "C" char *__real_mkdtemp(char *pattern);
RDB_WRAP_SYSCALL(int, fstat, (int fd, struct stat *info), (fd, info));
RDB_WRAP_SYSCALL(int, flock, (int fd, int operation), (fd, operation));
RDB_WRAP_SYSCALL(int, lstat, (const char *path, struct stat *info), (path, info));
RDB_WRAP_SYSCALL(char *, mkdtemp, (char *pattern), (pattern));

extern "C" char *__wrap_mkdtemp(char *pattern) {
  if (std::exchange(failTemporaryCreate, false)) {
    errno = ENOSPC;
    return nullptr;
  }
  return __real_mkdtemp(pattern);
}

extern "C" int __wrap_fstat(int fd, struct stat *info) {
  const int result = __real_fstat(fd, info);
  if (result == 0 && observeDirectory && S_ISDIR(info->st_mode)) {
    if (guardFault == GuardFault::device) {
      guardFault = GuardFault::none;
      ++info->st_dev;
    } else if (guardFault == GuardFault::inode) {
      guardFault = GuardFault::none;
      ++info->st_ino;
    } else if (guardFault == GuardFault::stat) {
      guardFault = GuardFault::none;
      errno      = EIO;
      return -1;
    }
  }
  return result;
}

extern "C" int __wrap_flock(int fd, int operation) {
  struct stat info{};
  if (observeDirectory && __real_fstat(fd, &info) == 0 && S_ISDIR(info.st_mode)) {
    observedFd        = fd;
    observedFlags     = fcntl(fd, F_GETFD);
    observedOperation = operation;
    if (guardFault == GuardFault::flock) {
      guardFault = GuardFault::none;
      errno      = EACCES;
      return -1;
    }
  }
  return __real_flock(fd, operation);
}

extern "C" int __wrap_lstat(const char *path, struct stat *info) {
  const int result = __real_lstat(path, info);
  if (result == 0 && guardFault == GuardFault::owner && std::string_view(path).ends_with("/.retractordb-embedded")) {
    guardFault   = GuardFault::none;
    info->st_uid = geteuid() == 65534 ? 65533 : 65534;
  }
  return result;
}

TEST_F(embedStorageGuardBoundary, cloexec_alias_busy_and_destruction_release) {
  // covers: storageguard.storage-guard.2~1, storageguard.storage-guard.3~1, storageguard.storage-guard.8~1
  // covers: storageguard.storage-guard.11~1
  observeDirectory = true;
  auto owner       = rdbtest::ok(rdb::embed::StorageGuard::create(directory.string(), true));
  EXPECT_NE(observedFlags & FD_CLOEXEC, 0);
  EXPECT_EQ(observedOperation, LOCK_EX | LOCK_NB);
  const int heldFd = observedFd;
  fs::create_directory_symlink(directory, directory / "alias");
  const auto marker   = directory / ".retractordb-embedded";
  const auto contents = readText(marker);
  std::ofstream(marker) << "invalid marker while held";
  for (const auto &path : {directory, directory / "alias"}) {
    const auto refused = rdb::embed::StorageGuard::create(path.string(), true);
    ASSERT_RDB_ERROR(refused, rdb::Errc::Config, "busy");
    EXPECT_EQ(refused.error().err(), EBUSY);
  }
  std::ofstream(marker) << contents;
  owner.reset();
  errno = 0;
  EXPECT_EQ(fcntl(heldFd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
  EXPECT_TRUE(fs::exists(directory / ".retractordb-embedded"));
  EXPECT_RDB_OK(rdb::embed::StorageGuard::create((directory / "alias").string(), true));
}

TEST_F(embedStorageGuardBoundary, metadata_and_lock_failures_close_the_candidate_fd) {
  // covers: storageguard.storage-guard.4~1, storageguard.storage-guard.10~1, storageguard.storage-guard.11~1
  observeDirectory = true;
  for (const auto fault : {GuardFault::device, GuardFault::inode, GuardFault::stat, GuardFault::flock}) {
    guardFault         = fault;
    observedFd         = -1;
    const auto refused = rdb::embed::StorageGuard::create(directory.string(), true);
    ASSERT_FALSE(refused);
    EXPECT_EQ(guardFault, GuardFault::none) << "podmiana nie zostala wywolana";
    EXPECT_EQ(refused.error().code(),
              fault == GuardFault::inode || fault == GuardFault::device ? rdb::Errc::Compile : rdb::Errc::IO);
    EXPECT_EQ(refused.error().err(), fault == GuardFault::stat ? EIO : fault == GuardFault::flock ? EACCES : 0);
    EXPECT_TRUE(refused.error().message().contains(directory.string()));
    ASSERT_GE(observedFd, 0);
    errno = 0;
    EXPECT_EQ(fcntl(observedFd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_TRUE(fs::is_empty(directory));
  }
  EXPECT_RDB_OK(rdb::embed::StorageGuard::create(directory.string(), true));
}

TEST_F(embedStorageGuardBoundary, foreign_owner_nonregular_and_trailing_marker_are_refused) {
  // covers: storageguard.storage-guard.7~1
  const auto marker          = directory / ".retractordb-embedded";
  const std::string contents = "retractordb embedded storage v1\n";
  std::ofstream(marker) << contents;
  guardFault = GuardFault::owner;
  EXPECT_RDB_ERROR(rdb::embed::StorageGuard::create(directory.string(), true), rdb::Errc::Compile);
  EXPECT_EQ(guardFault, GuardFault::none);
  EXPECT_EQ(readText(marker), contents);
  const std::string corrupted(contents.size(), 'x');
  std::ofstream(marker) << corrupted;
  EXPECT_RDB_ERROR(rdb::embed::StorageGuard::create(directory.string(), true), rdb::Errc::Compile);
  EXPECT_EQ(readText(marker), corrupted);
  std::ofstream(marker) << contents;
  std::ofstream(marker, std::ios::app) << "x";
  EXPECT_RDB_ERROR(rdb::embed::StorageGuard::create(directory.string(), true), rdb::Errc::Compile);
  EXPECT_EQ(readText(marker), contents + "x");
  fs::remove(marker);
  fs::create_directory(marker);
  EXPECT_RDB_ERROR(rdb::embed::StorageGuard::create(directory.string(), true), rdb::Errc::Compile);
  EXPECT_TRUE(fs::is_directory(marker));
}

TEST_F(embedStorageGuardBoundary, filesystem_failures_keep_path_and_errno) {
  // covers: storageguard.storage-guard.9~1, storageguard.storage-guard.10~1
  const auto missing = directory / "missing";
  const auto refused = rdb::embed::StorageGuard::create(missing.string(), true);
  ASSERT_RDB_ERROR(refused, rdb::Errc::IO, missing.string());
  EXPECT_EQ(refused.error().err(), ENOENT);
  std::ofstream(directory / "file") << "keep";
  const auto notDirectory = rdb::embed::StorageGuard::create((directory / "file").string(), true);
  ASSERT_RDB_ERROR(notDirectory, rdb::Errc::IO, "file");
  EXPECT_EQ(notDirectory.error().err(), ENOTDIR);
  fs::remove(directory / "file");
  auto guard = rdbtest::ok(rdb::embed::StorageGuard::create(directory.string(), true));
  EXPECT_RDB_OK(guard->checkPath((directory.filename() / "data").string()));
  EXPECT_RDB_ERROR(guard->checkPath("data"), rdb::Errc::Compile);
  fs::create_symlink("cycle", directory / "cycle");
  const auto cycle = guard->checkPath((directory / "cycle/data").string());
  ASSERT_RDB_ERROR(cycle, rdb::Errc::IO, "cycle");
  EXPECT_EQ(cycle.error().err(), ELOOP);
}

TEST_F(embedStorageGuardBoundary, offline_busy_and_uncertainty_checks_do_not_modify_locks) {
  // covers: storageguard.check-offline.1~1, storageguard.check-offline.2~1, storageguard.check-offline.3~1
  // covers: storageguard.check-offline.4~1
  const auto lock = directory / "xretractor_service.boundary.lock";
  std::ofstream(lock) << "PID: sentinel\n";
  const int fd = open(lock.c_str(), O_RDONLY | O_CLOEXEC);
  ASSERT_GE(fd, 0);
  ASSERT_EQ(flock(fd, LOCK_EX | LOCK_NB), 0);
  for (const bool lenient : {false, true}) {
    const auto refused = rdb::embed::checkOffline({directory.string(), lenient});
    ASSERT_RDB_ERROR(refused, rdb::Errc::Config, lock.string());
    EXPECT_EQ(refused.error().err(), EBUSY);
  }
  EXPECT_EQ(close(fd), 0);
  EXPECT_RDB_OK(rdb::embed::checkOffline({directory.string(), false}));
  // Proba wspoldzielona kontroli schodzi razem z jej deskryptorem.
  const int probe = open(lock.c_str(), O_RDONLY | O_CLOEXEC);
  ASSERT_GE(probe, 0);
  EXPECT_EQ(flock(probe, LOCK_EX | LOCK_NB), 0);
  EXPECT_EQ(close(probe), 0);
  EXPECT_EQ(readText(lock), "PID: sentinel\n");
  EXPECT_EQ(std::distance(fs::directory_iterator(directory), fs::directory_iterator()), 1);
  fs::remove(lock);
  fs::create_directory(lock);
  EXPECT_RDB_ERROR(rdb::embed::checkOffline({directory.string(), false}), rdb::Errc::Compile);
  EXPECT_RDB_OK(rdb::embed::checkOffline({directory.string(), true}));
  fs::remove(lock);
  fs::create_symlink("missing", lock);
  const auto symlink = rdb::embed::checkOffline({directory.string(), false});
  ASSERT_RDB_ERROR(symlink, rdb::Errc::IO);
  EXPECT_EQ(symlink.error().err(), ELOOP);
  EXPECT_RDB_OK(rdb::embed::checkOffline({directory.string(), true}));
  const auto missing = rdb::embed::checkOffline({(directory / "missing").string(), false});
  ASSERT_RDB_ERROR(missing, rdb::Errc::IO);
  EXPECT_EQ(missing.error().err(), ENOENT);
  EXPECT_RDB_OK(rdb::embed::checkOffline({(directory / "missing").string(), true}));
}

TEST_F(embedStorageGuardBoundary, current_directory_and_temporary_cleanup) {
  // covers: engine.engine.4~1, storageguard.storage-guard.1~1, storageguard.storage-guard.6~1
  // covers: storageguard.temporary-storage-directory.1~1, storageguard.remove-temporary-storage.1~1
  // covers: storageguard.remove-temporary-storage.2~1
  const auto previous = fs::current_path();
  fs::current_path(directory);
  const auto unclaimed = rdb::embed::StorageGuard::create("", false);
  fs::current_path(previous);
  ASSERT_TRUE(unclaimed);
  EXPECT_EQ((*unclaimed)->directory(), directory.string());
  EXPECT_FALSE((*unclaimed)->ownsDirectory());
  EXPECT_TRUE(fs::is_empty(directory));
  const auto first  = rdbtest::ok(rdb::embed::temporaryStorageDirectory());
  const auto second = rdbtest::ok(rdb::embed::temporaryStorageDirectory());
  EXPECT_NE(first, second);
  EXPECT_TRUE(fs::path(first).filename().string().starts_with("retractordb-engine-"));
  struct stat info{};
  ASSERT_EQ(stat(first.c_str(), &info), 0);
  EXPECT_EQ(info.st_mode & 0777, 0700);
  std::ofstream(fs::path(first) / "foreign") << "keep";
  rdb::embed::removeTemporaryStorage(first);
  EXPECT_EQ(readText(fs::path(first) / "foreign"), "keep");
  fs::remove(fs::path(first) / "foreign");
  rdb::embed::removeTemporaryStorage(first);
  rdb::embed::removeTemporaryStorage(second);
  EXPECT_FALSE(fs::exists(first));
  EXPECT_FALSE(fs::exists(second));
  rdb::embed::removeTemporaryStorage(second);
}

TEST_F(embedStorageGuardBoundary, invalid_tmpdir_preserves_errno) {
  // covers: storageguard.temporary-storage-directory.2~1
  const std::optional<std::string> previous = getenv("TMPDIR") ? std::optional<std::string>{getenv("TMPDIR")} : std::nullopt;
  const auto missing                        = (directory / "missing").string();
  ASSERT_EQ(setenv("TMPDIR", missing.c_str(), 1), 0);
  std::error_code expected;
  (void)fs::temp_directory_path(expected);
  const auto refused = rdb::embed::temporaryStorageDirectory();
  if (previous)
    setenv("TMPDIR", previous->c_str(), 1);
  else
    unsetenv("TMPDIR");
  ASSERT_TRUE(expected);
  ASSERT_RDB_ERROR(refused, rdb::Errc::IO, "TMPDIR");
  EXPECT_EQ(refused.error().err(), expected.value());
}

TEST_F(embedStorageGuardBoundary, mkdtemp_failure_preserves_errno_without_creating_a_directory) {
  // covers: storageguard.temporary-storage-directory.2~1
  failTemporaryCreate = true;
  const auto refused  = rdb::embed::temporaryStorageDirectory();
  ASSERT_RDB_ERROR(refused, rdb::Errc::IO, "retractordb-engine-XXXXXX");
  EXPECT_FALSE(failTemporaryCreate);
  EXPECT_EQ(refused.error().err(), ENOSPC);
}

TEST_F(embedStorageGuardBoundary, offline_policy_resolves_configuration_and_rejects_relative_arguments) {
  // covers: storageguard.offline-policy.1~1, storageguard.offline-policy.2~1, storageguard.offline-policy.3~1
  // covers: storageguard.offline-policy.4~1
  EXPECT_RDB_ERROR(rdb::embed::offlinePolicy("relative", ""), rdb::Errc::Config, "absolute");
  const auto defaults = rdbtest::ok(rdb::embed::offlinePolicy("", ""));
  EXPECT_TRUE(defaults.lockDir.empty());
  EXPECT_TRUE(defaults.lenient);
  const auto config = directory / "explicit.toml";
  const auto alias  = directory / "alias";
  fs::create_directory_symlink(directory, alias);
  std::ofstream(config) << "[paths]\nlock_dir = '" << directory.string()
                        << "'\n"
                           "[embedded]\nlenient_lock_dirs = ['"
                        << alias.string() << "']\n";
  const auto configured = rdbtest::ok(rdb::embed::offlinePolicy("", config.string()));
  EXPECT_EQ(configured.lockDir, directory.string());
  EXPECT_TRUE(configured.lenient);
  const auto other      = (directory / "other").string();
  const auto overridden = rdbtest::ok(rdb::embed::offlinePolicy(other, config.string()));
  EXPECT_EQ(overridden.lockDir, other);
  EXPECT_FALSE(overridden.lenient);
  std::ofstream(config) << "[embedded]\nlenient_lock_dirs = ['relative']\n";
  EXPECT_RDB_ERROR(rdb::embed::offlinePolicy("", config.string()), rdb::Errc::Config, "lenient_lock_dirs");
}
