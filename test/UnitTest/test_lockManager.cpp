#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "constants.hpp"
#include "logCapture.hpp"
#include "osPlatform.hpp"
#include "platformConfig.h"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/lockFile.hpp"
#include "retractor/lib/lockManager.hpp"
#include "retractor/lib/serviceControl.hpp"
#include "syscallWrap.hpp"

namespace {
int serviceProcessFault = 0;
}
extern "C" pid_t __real_fork();
extern "C" pid_t __real_waitpid(pid_t, int *, int);
extern "C" pid_t __wrap_fork() {
  if (serviceProcessFault == 1) {
    errno = EAGAIN;
    return -1;
  }
  if (serviceProcessFault == 2) return 424242;
  return __real_fork();
}
extern "C" pid_t __wrap_waitpid(pid_t pid, int *status, int options) {
  if (serviceProcessFault == 2 && pid == 424242) {
    errno = EINTR;
    return -1;
  }
  return __real_waitpid(pid, status, options);
}
RDB_WRAP_SYSCALL(pid_t, fork, (), ());
RDB_WRAP_SYSCALL(pid_t, waitpid, (pid_t pid, int *status, int options), (pid, status, options));

using servicecontrol::deliverQueryFile;
using servicecontrol::restartCommand;
using servicecontrol::restartService;

namespace {

std::string unstablePath;
int openAttempts = 0;
std::string failingPath;
int failingErrno = 0;
std::string trackedPath;
int trackedFd = -1;
enum class LockFault { None, Stat, Flock, Relink, Truncate, Seek, Write, Sync, ZeroWrite, ShortWrite };
LockFault lockFault = LockFault::None;
int relinkChecks    = 0;
int writes          = 0;

class LockFileSafety : public ::testing::Test {
 protected:
  std::filesystem::path dir;
  const std::string service = "xretractor_service.safety";
  std::filesystem::path path;
  void SetUp() override {
    std::string pattern = (std::filesystem::temp_directory_path() / "rdb466-XXXXXX").string();
    ASSERT_NE(mkdtemp(pattern.data()), nullptr);
    dir  = pattern;
    path = dir / (service + ".lock");
  }
  void TearDown() override {
    unstablePath.clear();
    failingPath.clear();
    trackedPath.clear();
    trackedFd = -1;
    lockFault = LockFault::None;
    std::filesystem::remove_all(dir);
  }
  static std::string contents(const std::filesystem::path &file) {
    std::ifstream input(file);
    return std::string(std::istreambuf_iterator<char>(input), {});
  }
};

// Zapisuje syntetyczny plik blokady o znanej ścieżce i zwraca skonfigurowany guard,
// który tę ścieżkę odczyta (setLockDir + nazwa serwisu wyznaczają lockFilePath).
std::filesystem::path writeLockFile(const std::string &dir, const std::string &serviceName, const std::string &content) {
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = std::filesystem::path(dir) / (serviceName + ".lock");
  std::ofstream out(path, std::ios::trunc);
  out << content;
  return path;
}

}  // namespace

extern "C" int __real_open(const char *path, int flags, ...);
extern "C" int __wrap_open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  if (!unstablePath.empty() && unstablePath == path) {
    ++openAttempts;
    errno = flags & O_CREAT ? EEXIST : ENOENT;
    return -1;
  }
  if (!failingPath.empty() && failingPath == path) {
    errno = failingErrno;
    return -1;
  }
  const int fd = __real_open(path, flags, mode);
  if (!trackedPath.empty() && trackedPath == path) trackedFd = fd;
  return fd;
}

extern "C" int __real_fstat(int fd, struct stat *info);
extern "C" int __real_lstat(const char *path, struct stat *info);
extern "C" int __real_flock(int fd, int operation);
extern "C" int __real_ftruncate(int fd, off_t length);
extern "C" off_t __real_lseek(int fd, off_t offset, int whence);
extern "C" ssize_t __real_write(int fd, const void *data, size_t length);
extern "C" int __real_fsync(int fd);
RDB_WRAP_SYSCALL(int, fstat, (int fd, struct stat *info), (fd, info));
RDB_WRAP_SYSCALL(int, lstat, (const char *path, struct stat *info), (path, info));
RDB_WRAP_SYSCALL(int, flock, (int fd, int operation), (fd, operation));
RDB_WRAP_SYSCALL(int, ftruncate, (int fd, off_t length), (fd, length));
RDB_WRAP_SYSCALL(off_t, lseek, (int fd, off_t offset, int whence), (fd, offset, whence));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *data, size_t length), (fd, data, length));
RDB_WRAP_SYSCALL(int, fsync, (int fd), (fd));
extern "C" int __wrap_fstat(int fd, struct stat *info) {
  if (fd == trackedFd && lockFault == LockFault::Stat) {
    errno = EIO;
    return -1;
  }
  return __real_fstat(fd, info);
}
extern "C" int __wrap_lstat(const char *path, struct stat *info) {
  if (lockFault == LockFault::Relink && trackedPath == path) {
    ++relinkChecks;
    errno = ENOENT;
    return -1;
  }
  return __real_lstat(path, info);
}
extern "C" int __wrap_flock(int fd, int operation) {
  if (fd == trackedFd && lockFault == LockFault::Flock) {
    errno = EIO;
    return -1;
  }
  return __real_flock(fd, operation);
}
extern "C" int __wrap_ftruncate(int fd, off_t length) {
  if (fd == trackedFd && lockFault == LockFault::Truncate) {
    errno = EIO;
    return -1;
  }
  return __real_ftruncate(fd, length);
}
extern "C" off_t __wrap_lseek(int fd, off_t offset, int whence) {
  if (fd == trackedFd && lockFault == LockFault::Seek) {
    errno = EIO;
    return -1;
  }
  return __real_lseek(fd, offset, whence);
}
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t length) {
  if (fd == trackedFd && !trackedPath.empty()) {
    ++writes;
    if (lockFault == LockFault::Write) {
      errno = EIO;
      return -1;
    }
    if (lockFault == LockFault::ZeroWrite) return 0;
    if (lockFault == LockFault::ShortWrite) return __real_write(fd, data, 1);
  }
  return __real_write(fd, data, length);
}
extern "C" int __wrap_fsync(int fd) {
  if (fd == trackedFd && lockFault == LockFault::Sync) {
    errno = EIO;
    return -1;
  }
  return __real_fsync(fd);
}

#if !RDB_HAS_LD_WRAP
// Wariadyczne open przekazuje tryb tworzonego pliku takze w backendzie bez --wrap.
extern "C" int __real_open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  using Open       = int (*)(const char *, int, ...);
  static Open next = nullptr;
  if (next == nullptr) next = reinterpret_cast<Open>(::dlsym(RTLD_NEXT, "open"));
  return next(path, flags, mode);
}
extern "C" int open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  return __wrap_open(path, flags, mode);
}
#endif

TEST_F(LockFileSafety, SymlinkPreservesTargetAndReportsReason) {
  // covers: lockfile.is-held.2~1, lockfile.claim-abandoned.2~1, lockmanager.flock-service-guard.3~1
  const auto target = writeLockFile(dir.string(), "sentinel", "sentinel contents\n");
  const auto size   = std::filesystem::file_size(target);
  std::filesystem::create_symlink(target, path);
  const LogCapture capture;
  spdlog::default_logger()->set_level(spdlog::level::err);
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  EXPECT_FALSE(guard.acquireLock());
  EXPECT_FALSE(guard.isLockActive());
  EXPECT_FALSE(guard.publishLockInfo());
  guard.releaseLock();
  EXPECT_EQ(contents(target), "sentinel contents\n");
  EXPECT_EQ(std::filesystem::file_size(target), size);
  EXPECT_TRUE(std::filesystem::is_symlink(path));
  EXPECT_NE(guard.acquireLockError().find(path.string()), std::string::npos);
  EXPECT_NE(guard.acquireLockError().find("symbolic link"), std::string::npos);
  EXPECT_NE(capture.text().find("symbolic link"), std::string::npos);
  EXPECT_FALSE(lockfile::isHeld(path.string()));
  EXPECT_EQ(lockfile::claimAbandoned(path.string()), -1);
}

TEST_F(LockFileSafety, DanglingSymlinkIsRefusedWithoutCreatingTarget) {
  // covers: lockfile.acquire.1~1, lockfile.acquire.5~1
  const auto target = dir / "absent";
  std::filesystem::create_symlink(target, path);
  for (const bool writable : {false, true}) {
    int fd           = -1;
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(lockfile::acquire(path.string(), true, writable, fd), lockfile::Result::Refused);
    EXPECT_EQ(errno, ELOOP);
    EXPECT_EQ(fd, -1);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
  }
  EXPECT_FALSE(std::filesystem::exists(target));
  EXPECT_TRUE(std::filesystem::is_symlink(path));
}

TEST_F(LockFileSafety, WrongTypesDoNotBlockReadersWritersOrCleanup) {
  // covers: lockfile.acquire.5~1, lockfile.is-held.2~1, lockfile.claim-abandoned.2~1, lockmanager.flock-service-guard.3~1
  ASSERT_EQ(mkfifo(path.c_str(), 0600), 0);
  for (const bool directory : {false, true}) {
    if (directory) {
      std::filesystem::remove(path);
      std::filesystem::create_directory(path);
    }
    const auto start = std::chrono::steady_clock::now();
    for (const bool writable : {false, true}) {
      int fd = -1;
      EXPECT_EQ(lockfile::acquire(path.string(), true, writable, fd), lockfile::Result::Refused);
      EXPECT_EQ(errno, EINVAL);
      EXPECT_EQ(fd, -1);
    }
    EXPECT_FALSE(lockfile::isHeld(path.string()));
    EXPECT_EQ(lockfile::claimAbandoned(path.string()), -1);
    FlockServiceGuard guard(service);
    guard.setLockDir(dir.string());
    EXPECT_FALSE(guard.acquireLock());
    EXPECT_FALSE(guard.isAnotherInstanceRunning());
    EXPECT_NE(guard.acquireLockError().find("not a regular file"), std::string::npos);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
  }
}

// Dowiazanie dodane po zajeciu nie przekierowuje zapisu: deskryptor wskazuje i-wezel instancji.
// Odmowa publikacji niczego by nie chronila, a zostawilaby klientow bez linii PID.
TEST_F(LockFileSafety, HardlinkRefusedBeforeAcquireLaterAliasDoesNotBlockPublish) {
  // covers: lockfile.acquire.5~1, lockmanager.flock-service-guard.3~1
  const auto target = writeLockFile(dir.string(), "sentinel", "sentinel contents\n");
  std::filesystem::create_hard_link(target, path);
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  EXPECT_FALSE(guard.acquireLock());
  EXPECT_NE(guard.acquireLockError().find("single link"), std::string::npos);
  EXPECT_EQ(contents(target), "sentinel contents\n");
  std::filesystem::remove(path);
  ASSERT_TRUE(guard.acquireLock());
  ASSERT_TRUE(guard.publishLockInfo());
  const auto alias = dir / "alias";
  std::filesystem::create_hard_link(path, alias);
  EXPECT_TRUE(guard.publishLockInfo());
  EXPECT_NE(contents(path).find("PID: " + std::to_string(getpid())), std::string::npos);
  EXPECT_EQ(contents(target), "sentinel contents\n");
  guard.releaseLock();
}

TEST_F(LockFileSafety, AcquirePublishExcludeReleaseAndReacquire) {
  // covers: lockmanager.flock-service-guard.1~1, lockmanager.flock-service-guard.2~1, lockmanager.flock-service-guard.3~1
  // covers: lockmanager.flock-service-guard.5~1, lockmanager.flock-service-guard.7~1, lockmanager.flock-service-guard.8~1
  writeLockFile(dir.string(), service, "stale process info\n");
  FlockServiceGuard first(service);
  first.setLockDir(dir.string());
  ASSERT_TRUE(first.acquireLock());
  EXPECT_TRUE(contents(path).empty());
  first.setServiceQueryFile("private.rql");
  ASSERT_TRUE(first.publishLockInfo());
  EXPECT_EQ(first.readPeerInfo().pid, getpid());
  EXPECT_EQ(first.readPeerInfo().queryFile, "private.rql");
  FlockServiceGuard second(service);
  second.setLockDir(dir.string());
  EXPECT_FALSE(second.acquireLock());
  EXPECT_TRUE(second.acquireLockError().empty());
  EXPECT_TRUE(second.isAnotherInstanceRunning());
  EXPECT_EQ(second.readPeerInfo().pid, getpid());
  first.releaseLock();
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_TRUE(second.acquireLock());
  EXPECT_TRUE(second.publishLockInfo());
  second.releaseLock();
  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(LockFileSafety, ReplacementSymlinkDoesNotIdentifyHeldInode) {
  // covers: lockfile.still-linked.1~1, lockfile.remove-and-release.1~1
  int fd = -1;
  ASSERT_EQ(lockfile::acquire(path.string(), true, true, fd), lockfile::Result::Acquired);
  const auto moved = dir / "moved";
  std::filesystem::rename(path, moved);
  std::filesystem::create_symlink(moved, path);
  EXPECT_FALSE(lockfile::stillLinked(fd, path.string()));
  lockfile::removeAndRelease(path.string(), fd);
  EXPECT_TRUE(std::filesystem::is_symlink(path));
  EXPECT_TRUE(std::filesystem::exists(moved));
  std::filesystem::remove(path);
  std::ofstream(path) << "replacement";
  const int old = open(moved.c_str(), O_RDONLY);
  ASSERT_GE(old, 0);
  EXPECT_EQ(flock(old, LOCK_EX | LOCK_NB), 0);
  EXPECT_FALSE(lockfile::stillLinked(old, path.string()));
  lockfile::removeAndRelease(path.string(), old);
  EXPECT_EQ(contents(path), "replacement") << "zwolnienie starej blokady usunelo nowy zwykly plik";
  EXPECT_EQ(fcntl(old, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

// EPERM z samego open() (np. plik niezmienny) to blad systemu, nie obcy wlasciciel.
TEST_F(LockFileSafety, SystemErrorFromOpenIsNotReportedAsRefusal) {
  // covers: lockfile.acquire.6~1
  { std::ofstream(path) << "kept\n"; }
  failingPath  = path.string();
  failingErrno = EPERM;
  int fd       = -1;
  EXPECT_EQ(lockfile::acquire(path.string(), true, true, fd), lockfile::Result::Error);
  EXPECT_EQ(errno, EPERM);
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  EXPECT_FALSE(guard.acquireLock());
  EXPECT_NE(guard.acquireLockError().find(std::strerror(EPERM)), std::string::npos);
  EXPECT_EQ(guard.acquireLockError().find("untrusted owner"), std::string::npos);
  failingPath.clear();
  EXPECT_EQ(contents(path), "kept\n");
}

TEST_F(LockFileSafety, OpenCreateRaceHasAnInternalRetryLimit) {
  // covers: lockfile.acquire.5~1
  unstablePath = path.string();
  openAttempts = 0;
  int fd       = -1;
  EXPECT_EQ(lockfile::acquire(path.string(), true, true, fd), lockfile::Result::Refused);
  EXPECT_EQ(errno, EAGAIN);
  EXPECT_EQ(fd, -1);
  EXPECT_GT(openAttempts, 2);
  EXPECT_LE(openAttempts, 200);
  unstablePath.clear();
  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(LockFileSafety, NewLockIgnoresUmaskAndUsesTheRequestedAccessMode) {
  // covers: lockfile.acquire.1~1, lockfile.acquire.4~1, lockfile.still-linked.1~1, lockfile.remove-and-release.1~1
  const mode_t previous = umask(0077);
  for (const bool writable : {false, true}) {
    int fd            = -1;
    const auto result = lockfile::acquire(path.string(), true, writable, fd);
    EXPECT_EQ(result, lockfile::Result::Acquired);
    if (fd == -1) continue;
    struct stat info{};
    EXPECT_EQ(fstat(fd, &info), 0);
    EXPECT_EQ(info.st_mode & 0777, 0644U);
    EXPECT_EQ(fcntl(fd, F_GETFL) & O_ACCMODE, writable ? O_RDWR : O_RDONLY);
    EXPECT_TRUE(lockfile::stillLinked(fd, path.string()));
    lockfile::removeAndRelease(path.string(), fd);
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
  }
  umask(previous);
}

TEST_F(LockFileSafety, StatAndFlockSystemErrorsCloseTheCandidateDescriptor) {
  // covers: lockfile.acquire.6~1
  trackedPath = path.string();
  for (const auto fault : {LockFault::Stat, LockFault::Flock}) {
    lockFault = fault;
    int fd    = -1;
    EXPECT_EQ(lockfile::acquire(path.string(), true, true, fd), lockfile::Result::Error);
    EXPECT_EQ(errno, EIO);
    EXPECT_EQ(fd, -1);
    lockFault = LockFault::None;
    EXPECT_EQ(fcntl(trackedFd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
  }
}

TEST_F(LockFileSafety, RelinkingAfterSuccessfulFlockStopsAtOneHundredAttempts) {
  // covers: lockfile.acquire.3~1
  trackedPath  = path.string();
  lockFault    = LockFault::Relink;
  relinkChecks = 0;
  int fd       = -1;
  EXPECT_EQ(lockfile::acquire(path.string(), true, true, fd), lockfile::Result::Error);
  EXPECT_EQ(errno, ESTALE);
  EXPECT_EQ(relinkChecks, 100);
  EXPECT_EQ(fd, -1);
  lockFault = LockFault::None;
  EXPECT_EQ(fcntl(trackedFd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST_F(LockFileSafety, SweepCallbackRunsUnderExclusiveLockAndCountsOnlyAcceptedFiles) {
  // covers: lockfile.claim-abandoned.1~1, lockfile.claim-abandoned.2~1, lockfile.sweep.1~1, lockfile.sweep.3~1
  // covers: lockfile.sweep.4~1, lockfile.sweep.5~1
  std::ofstream(path) << "abandoned";
  std::ofstream(dir / "unrelated") << "kept";
  int callbacks = 0;
  EXPECT_EQ(lockfile::sweep(
                dir.string(), [&](std::string_view name) { return name == path.filename().string(); },
                [&](std::string_view name) {
                  ++callbacks;
                  EXPECT_EQ(name, path.filename().string());
                  EXPECT_TRUE(lockfile::isHeld(path.string()));
                  EXPECT_EQ(lockfile::claimAbandoned(path.string()), -1);
                }),
            1U);
  EXPECT_EQ(callbacks, 1);
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_TRUE(std::filesystem::exists(dir / "unrelated"));
  EXPECT_EQ(
      lockfile::sweep((dir / "absent").string(), [](std::string_view) { return true; }, [](std::string_view) { ADD_FAILURE(); }),
      0U);
  EXPECT_EQ(lockfile::claimAbandoned(path.string()), -1);
  EXPECT_FALSE(std::filesystem::exists(path));
  std::ofstream(path) << "unheld";
  trackedPath = path.string();
  lockFault   = LockFault::Relink;
  EXPECT_EQ(lockfile::claimAbandoned(path.string()), -1) << "sprzatacz przejal podmieniony i-wezel";
  lockFault = LockFault::None;
  EXPECT_EQ(fcntl(trackedFd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST_F(LockFileSafety, TruncateFailureDuringAcquireKeepsTheLockAndReportsWarning) {
  // covers: lockmanager.flock-service-guard.2~1
  std::ofstream(path) << "previous";
  trackedPath = path.string();
  lockFault   = LockFault::Truncate;
  const LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::debug);
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  EXPECT_TRUE(guard.acquireLock());
  EXPECT_TRUE(guard.isLockActive());
  EXPECT_TRUE(lockfile::isHeld(path.string()));
  EXPECT_EQ(contents(path), "previous");
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
  EXPECT_NE(log.text().find("Cannot truncate lock file"), std::string::npos);
#else
  EXPECT_EQ(log.text().find("Cannot truncate lock file"), std::string::npos);
#endif
  lockFault = LockFault::None;
}

TEST_F(LockFileSafety, PublishingChecksEveryIoBoundaryAndUsesOneWrite) {
  // covers: lockmanager.flock-service-guard.5~1
  trackedPath = path.string();
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  EXPECT_FALSE(guard.publishLockInfo());
  ASSERT_TRUE(guard.acquireLock());
  for (const auto fault : {LockFault::Truncate, LockFault::Seek, LockFault::Write, LockFault::Sync}) {
    lockFault = fault;
    EXPECT_FALSE(guard.publishLockInfo());
    EXPECT_TRUE(guard.isLockActive());
  }
  lockFault = LockFault::None;
  writes    = 0;
  ASSERT_TRUE(guard.publishLockInfo());
  EXPECT_EQ(writes, 1);
  const auto text = contents(path);
  EXPECT_NE(text.find("PID: " + std::to_string(getpid()) + "\n"), std::string::npos);
  EXPECT_NE(text.find("PPID: " + std::to_string(getppid()) + "\n"), std::string::npos);
  const auto identity = detectSystemdIdentity();
  EXPECT_NE(text.find(identity.unit ? "MODE: service\n" : "MODE: process\n"), std::string::npos);
  if (identity.unit) {
    EXPECT_NE(text.find("UNIT: " + *identity.unit + "\n"), std::string::npos);
    EXPECT_NE(text.find(identity.userScope ? "SCOPE: user\n" : "SCOPE: system\n"), std::string::npos);
  }
  EXPECT_EQ(text.find("QUERYFILE:"), std::string::npos);
}

TEST_F(LockFileSafety, DocumentedShortWritesAreAcceptedWithoutRetry) {
  // covers: lockmanager.flock-service-guard.5~1
  trackedPath = path.string();
  FlockServiceGuard guard(service);
  guard.setLockDir(dir.string());
  ASSERT_TRUE(guard.acquireLock());
  for (const auto fault : {LockFault::ZeroWrite, LockFault::ShortWrite}) {
    lockFault = fault;
    writes    = 0;
    EXPECT_TRUE(guard.publishLockInfo());
    EXPECT_EQ(writes, 1);
    EXPECT_EQ(contents(path), fault == LockFault::ZeroWrite ? "" : "P");
  }
  lockFault = LockFault::None;
}

TEST_F(LockFileSafety, IpcLockRequiresInstanceLockAndCannotBeAcquiredTwice) {
  // covers: lockmanager.flock-service-guard.1~1, lockmanager.flock-service-guard.4~1, lockmanager.flock-service-guard.7~1
  // covers: lockmanager.flock-service-guard.8~1
  FlockServiceGuard guard(service);
  EXPECT_TRUE(std::filesystem::equivalent(guard.lockDirectory(), std::filesystem::temp_directory_path()));
  guard.setLockDir(dir.string());
  EXPECT_EQ(guard.lockDirectory(), dir.string());
  EXPECT_FALSE(guard.acquireIpcLock("contract", dir.string()));
  ASSERT_TRUE(guard.acquireLock());
  EXPECT_TRUE(guard.acquireIpcLock("contract", dir.string()));
  EXPECT_FALSE(guard.acquireIpcLock("other", dir.string()));
  guard.releaseLock();
  guard.releaseLock();
  EXPECT_FALSE(guard.isLockActive());
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_FALSE(std::filesystem::exists(ipc::identityLockPath("contract", dir.string())));
}

TEST(LockManagerIdentity, CopiesThePlatformServiceIdentity) {
  // covers: lockmanager.detect-systemd-identity.1~1
  const auto platform = osplat::detectServiceIdentity();
  const auto guard    = detectSystemdIdentity();
  EXPECT_EQ(guard.unit, platform.unit);
  EXPECT_EQ(guard.userScope, platform.userScope);
}

// Straznik nie zmienia ani wlasciciela deskryptorow, ani adresu (serviceGuardPtr) - R-01, #274.
// covers: lockmanager.flock-service-guard.9~1
static_assert(!std::is_copy_constructible_v<FlockServiceGuard>);
static_assert(!std::is_copy_assignable_v<FlockServiceGuard>);
static_assert(!std::is_move_constructible_v<FlockServiceGuard>);
static_assert(!std::is_move_assignable_v<FlockServiceGuard>);

// --- readPeerInfo: parsowanie MODE/UNIT/SCOPE/QUERYFILE ---

TEST(LockManagerPeerInfo, parses_service_with_unit_scope_and_queryfile) {
  // covers: lockmanager.flock-service-guard.6~1
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_svc").string();
  const std::string svc = "ut_svc";
  writeLockFile(dir, svc,
                "PID: 1234\nPPID: 1\nMODE: service\nUNIT: xretractor.service\nSCOPE: system\n"
                "QUERYFILE: /etc/retractor/startup.rql\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.pid, 1234);
  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Service);
  EXPECT_EQ(info.scope, FlockServiceGuard::PeerInfo::Scope::System);
  EXPECT_EQ(info.unit, "xretractor.service");
  EXPECT_EQ(info.queryFile, "/etc/retractor/startup.rql");

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, parses_user_scope) {
  // covers: lockmanager.flock-service-guard.6~1
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_user").string();
  const std::string svc = "ut_user";
  writeLockFile(dir, svc, "PID: 5\nMODE: service\nUNIT: xretractor.service\nSCOPE: user\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Service);
  EXPECT_EQ(info.scope, FlockServiceGuard::PeerInfo::Scope::User);

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, plain_process_lock_yields_process_kind) {
  // covers: lockmanager.flock-service-guard.6~1
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_proc").string();
  const std::string svc = "ut_proc";
  writeLockFile(dir, svc, "PID: 9\nPPID: 1\nMODE: process\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Process);
  EXPECT_TRUE(info.unit.empty());
  EXPECT_TRUE(info.queryFile.empty());

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, missing_lock_file_yields_unknown) {
  // covers: lockmanager.flock-service-guard.6~1
  const std::string dir = (std::filesystem::temp_directory_path() / ("ut_lockmgr_absent_" + std::to_string(getpid()))).string();
  FlockServiceGuard guard("ut_absent");
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Unknown);
  EXPECT_EQ(info.pid, 0);
  EXPECT_EQ(info.scope, FlockServiceGuard::PeerInfo::Scope::Unknown);
  EXPECT_TRUE(info.unit.empty());
  EXPECT_TRUE(info.queryFile.empty());
  writeLockFile(dir, "ut_absent", "UNRELATED: ignored\n");
  const auto partial = guard.readPeerInfo();
  EXPECT_EQ(partial.kind, FlockServiceGuard::PeerInfo::Kind::Unknown);
  EXPECT_EQ(partial.pid, 0);
  EXPECT_EQ(partial.scope, FlockServiceGuard::PeerInfo::Scope::Unknown);
  EXPECT_TRUE(partial.unit.empty());
  EXPECT_TRUE(partial.queryFile.empty());
  std::filesystem::remove_all(dir);
}

TEST(LockManagerFlock, releaseRemovesFileAndOrphanedOpenerCannotWin) {
  // covers: lockfile.acquire.3~1, lockfile.remove-and-release.1~1
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / ("ut_lockmgr_inode_" + std::to_string(getpid()));
  const std::string serviceName   = "xretractor_service.alfa";
  const std::filesystem::path lockPath = dir / (serviceName + ".lock");
  std::filesystem::create_directories(dir);
  std::filesystem::remove(lockPath);

  FlockServiceGuard first(serviceName);
  first.setLockDir(dir.string());
  ASSERT_TRUE(first.acquireLock());

  // Drugi uczestnik otwiera TEN SAM i-wezel jeszcze przed zwolnieniem pierwszej blokady. Po
  // skasowaniu pliku zajmie flock na i-wezle bez nazwy. Dawne kasowanie przy zwolnieniu dawalo
  // wtedy dwoch wlascicieli, bo trzeci tworzyl pod ta sciezka nowy plik; wyscig zamyka
  // sprawdzenie i-wezla po zajeciu blokady (lockFile.hpp).
  const int secondFd = open(lockPath.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_NE(secondFd, -1);

  first.releaseLock();
  EXPECT_FALSE(std::filesystem::exists(lockPath)) << "wlasciciel nie skasowal pliku przy zwolnieniu";
  ASSERT_EQ(flock(secondFd, LOCK_EX | LOCK_NB), 0);
  EXPECT_FALSE(lockfile::stillLinked(secondFd, lockPath.string())) << "blokada na osieroconym i-wezle wyglada na wazna";

  // Uczestnicy protokolu widza jednego wlasciciela: trzeci zajmuje nowy plik, czwarty odpada.
  FlockServiceGuard third(serviceName);
  third.setLockDir(dir.string());
  EXPECT_TRUE(third.acquireLock());
  FlockServiceGuard fourth(serviceName);
  fourth.setLockDir(dir.string());
  EXPECT_FALSE(fourth.acquireLock());

  EXPECT_EQ(close(secondFd), 0);
  third.releaseLock();
  std::filesystem::remove_all(dir);
}

// --- restartService: składanie argv przez wstrzykiwalny runner ---

TEST(ServiceControlRestart, builds_system_scope_argv) {
  // covers: svc.restart-command.systemd-argv~1, svc.restart-command.launchd-argv~1, svc.restart-service.runner~1
  std::vector<std::string> captured;
  const int rc = restartService(/*userScope=*/false, "xretractor.service", [&](const std::vector<std::string> &argv) {
    captured = argv;
    return 0;
  });

  EXPECT_EQ(rc, 0);
  // Program i skladnia zaleza od menedzera uslug systemu, wiec oczekiwanie bierzemy
  // z restartCommand - jedynego miejsca, w ktorym ta roznica jest zapisana. Test
  // pilnuje tego, co go naprawde dotyczy: ze restartService NIE ZMIENIA polecenia
  // po drodze i podaje je runnerowi w calosci.
  EXPECT_EQ(captured, restartCommand(/*userScope=*/false, "xretractor.service"));
#if RDB_HAS_SYSTEMD
  const std::vector<std::string> expected{"systemctl", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
#elif RDB_HAS_LAUNCHD
  ASSERT_GE(captured.size(), 4U);
  EXPECT_EQ(captured[0], "launchctl");
  EXPECT_EQ(captured[1], "kickstart");
  EXPECT_EQ(captured[2], "-k");
  EXPECT_EQ(captured[3], "system/xretractor.service");
#endif
}

TEST(ServiceControlRestart, builds_user_scope_argv) {
  // covers: svc.restart-command.systemd-argv~1, svc.restart-command.launchd-argv~1, svc.restart-service.runner~1
  std::vector<std::string> captured;
  restartService(/*userScope=*/true, "xretractor.service", [&](const std::vector<std::string> &argv) {
    captured = argv;
    return 0;
  });

  EXPECT_EQ(captured, restartCommand(/*userScope=*/true, "xretractor.service"));
#if RDB_HAS_SYSTEMD
  const std::vector<std::string> expected{"systemctl", "--user", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
#elif RDB_HAS_LAUNCHD
  // Zakres uzytkownika to u launchd INNA DOMENA, a nie dodatkowy przelacznik.
  ASSERT_GE(captured.size(), 4U);
  EXPECT_EQ(captured[0], "launchctl");
  EXPECT_TRUE(captured[3].starts_with("gui/")) << "polecenie nie trafia w domene uzytkownika: " << captured[3];
  EXPECT_TRUE(captured[3].ends_with("/xretractor.service"));
  EXPECT_EQ(captured[3], "gui/" + std::to_string(getuid()) + "/xretractor.service");
#endif
}

TEST(ServiceControlRestart, propagates_runner_exit_code) {
  // covers: svc.restart-service.runner~1
  const int rc = restartService(false, "x.service", [](const std::vector<std::string> &) { return 5; });
  EXPECT_EQ(rc, 5);
}

// --- deliverQueryFile: atomowe nadpisanie pliku docelowego ---

TEST(ServiceControlDeliver, overwrites_target_atomically) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_deliver";
  std::filesystem::create_directories(dir);
  const std::filesystem::path src = dir / "src.rql";
  const std::filesystem::path dst = dir / "startup.rql";

  {
    std::ofstream s(src);
    s << "SELECT a FROM b;\n";
  }
  {
    std::ofstream d(dst);
    d << "OLD CONTENT\n";
  }

  EXPECT_TRUE(deliverQueryFile(src.string(), dst.string()));

  std::ifstream check(dst);
  std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "SELECT a FROM b;\n");

  std::filesystem::remove_all(dir);
}

// --- writeQueryFile: ta sama atomowa droga, ale dla TRESCI, nie pliku zrodlowego ---
//
// Uzywa jej przeladowanie planu w locie (`xqry --reset`): plan przychodzi kanalem IPC, wiec
// pliku zrodlowego po stronie serwera nie ma. Tresc pusta jest zadaniem poprawnym - tak
// sprowadza sie plik zapytan uslugi do stanu zerowego po bledzie krytycznym.

TEST(ServiceControlWrite, writes_content_over_existing_target) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery";
  std::filesystem::create_directories(dir);
  const std::filesystem::path dst = dir / "startup.rql";
  {
    std::ofstream d(dst);
    d << "OLD CONTENT\n";
  }

  EXPECT_TRUE(servicecontrol::writeQueryFile("SELECT a STREAM d FROM b\n", dst.string()));

  std::ifstream check(dst);
  const std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "SELECT a STREAM d FROM b\n");

  std::filesystem::remove_all(dir);
}

TEST(ServiceControlWrite, empty_content_truncates_the_target) {
  // covers: svc.write.empty-content~1
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery_empty";
  std::filesystem::create_directories(dir);
  const std::filesystem::path dst = dir / "startup.rql";
  {
    std::ofstream d(dst);
    d << "SELECT a STREAM d FROM b\n";
  }

  EXPECT_TRUE(servicecontrol::writeQueryFile("", dst.string()));

  EXPECT_TRUE(std::filesystem::exists(dst));
  EXPECT_EQ(std::filesystem::file_size(dst), 0U);

  std::filesystem::remove_all(dir);
}

TEST(ServiceControlWrite, fails_when_the_target_directory_does_not_exist) {
  // covers: svc.write.result-and-cleanup~1
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery_nodir";
  std::filesystem::remove_all(dir);
  LogCapture log;
  EXPECT_FALSE(servicecontrol::writeQueryFile("x\n", (dir / "startup.rql").string()));
  EXPECT_TRUE(log.text().contains("cannot open temp"));
}

// Odmowa planu przed startem czysci plik zapytan WYLACZNIE w jednostce uslugowej: zwykly proces
// z terminala dostaje plik .rql operatora, a tego ruszac nie wolno.
TEST(ServiceControlDropRefused, clears_the_query_file_only_inside_a_service_unit) {
  // covers: svc.drop-refused.empty-args~1
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_drop_refused";
  std::filesystem::create_directories(dir);
  const std::filesystem::path plan = dir / "startup.rql";
  std::ofstream(plan) << "SELECT a STREAM d FROM b\n";

  EXPECT_FALSE(servicecontrol::dropRefusedPlan("", plan.string()));
  EXPECT_EQ(std::filesystem::file_size(plan), 25U) << "plik operatora poza jednostka";

  EXPECT_TRUE(servicecontrol::dropRefusedPlan("xretractor.service", plan.string()));
  EXPECT_EQ(std::filesystem::file_size(plan), 0U);

  std::filesystem::remove_all(dir);
}

TEST(ServiceControlDeliver, fails_on_missing_source) {
  // covers: svc.deliver.missing-source~1
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_deliver_missing";
  std::filesystem::create_directories(dir);
  LogCapture log;
  EXPECT_FALSE(deliverQueryFile((dir / "nope.rql").string(), (dir / "out.rql").string()));
  EXPECT_TRUE(log.text().contains("cannot open source"));
  EXPECT_TRUE(log.text().contains((dir / "nope.rql").string()));
  std::filesystem::remove_all(dir);
}

namespace {
class ServiceControlContract : public ::testing::Test {
 protected:
  std::filesystem::path dir;
  std::optional<std::string> previousPath;
  void SetUp() override {
    std::string pattern = (std::filesystem::temp_directory_path() / "rdb521-service-XXXXXX").string();
    ASSERT_NE(mkdtemp(pattern.data()), nullptr);
    dir = pattern;
    if (const auto *path = std::getenv("PATH")) previousPath = path;
  }
  void TearDown() override {
    serviceProcessFault = 0;
    if (previousPath)
      setenv("PATH", previousPath->c_str(), 1);
    else
      unsetenv("PATH");
    std::filesystem::remove_all(dir);
  }
  static std::string read(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  }
};
}  // namespace

TEST_F(ServiceControlContract, real_exec_passes_literal_arguments_and_propagates_exit_and_signal) {
  // covers: svc.restart-command.literal-unit~1, svc.restart-service.fork-exec~1, svc.restart-service.exit-codes~1
#if RDB_HAS_LAUNCHD
  const auto manager = dir / "launchctl";
#else
  const auto manager = dir / "systemctl";
#endif
  ASSERT_EQ(setenv("PATH", dir.c_str(), 1), 0);
  for (const bool signaled : {false, true}) {
    {
      std::ofstream executable(manager);
      executable << "#!/bin/sh\nprintf '%s\\n' \"$@\"\n";
      executable << (signaled ? "kill -TERM \"$$\"\n" : "exit 7\n");
    }
    ASSERT_EQ(chmod(manager.c_str(), 0700), 0);
    testing::internal::CaptureStdout();
    const int status     = restartService(false, "literal; exit 99");
    const auto arguments = testing::internal::GetCapturedStdout();
    EXPECT_EQ(status, signaled ? -1 : 7);
#if RDB_HAS_LAUNCHD
    EXPECT_EQ(arguments, "kickstart\n-k\nsystem/literal; exit 99\n");
#else
    EXPECT_EQ(arguments, "restart\nliteral; exit 99\n");
#endif
  }
  std::filesystem::remove(manager);
  EXPECT_EQ(restartService(false, "unused"), 127);
}

TEST_F(ServiceControlContract, fork_and_wait_failures_return_minus_one) {
  // covers: svc.restart-service.exit-codes~1
  // Podejrzane bez --wrap (Darwin): falszywy fork przez RDB_WRAP_SYSCALL nie byl tam wykonany.
  if (!RDB_HAS_LD_WRAP) GTEST_SKIP() << "podmiana fork/waitpid sprawdzona tylko z -Wl,--wrap";
  for (int fault : {1, 2}) {
    serviceProcessFault = fault;
    EXPECT_EQ(restartService(false, "unused"), -1);
  }
}

TEST_F(ServiceControlContract, delivery_preserves_binary_bytes_and_rename_keeps_the_old_reader) {
  // covers: svc.deliver.binary-copy~1, svc.write.tmp-rename~1
  const auto source = dir / "source.rql";
  const auto target = dir / "target.rql";
  const std::string content("a\0b\r\n", 5);
  std::ofstream(source, std::ios::binary).write(content.data(), static_cast<std::streamsize>(content.size()));
  std::ofstream(target) << "old contents";
  std::ifstream oldReader(target);
  ASSERT_TRUE(deliverQueryFile(source.string(), target.string()));
  EXPECT_EQ(read(target), content);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(oldReader), std::istreambuf_iterator<char>()), "old contents");
  EXPECT_FALSE(std::filesystem::exists(target.string() + ".tmp." + std::to_string(getpid())));
}

TEST_F(ServiceControlContract, rename_failure_preserves_the_target_and_removes_the_temporary_file) {
  // covers: svc.write.result-and-cleanup~1
  const auto target = dir / "directory";
  std::filesystem::create_directory(target);
  std::ofstream(target / "kept") << "sentinel";
  LogCapture log;
  EXPECT_FALSE(servicecontrol::writeQueryFile("replacement", target.string()));
  EXPECT_EQ(read(target / "kept"), "sentinel");
  EXPECT_FALSE(std::filesystem::exists(target.string() + ".tmp." + std::to_string(getpid())));
  EXPECT_TRUE(log.text().contains("rename"));
  EXPECT_TRUE(log.text().contains(target.string()));
}

TEST_F(ServiceControlContract, write_failure_preserves_the_target_and_reports_the_temporary_file) {
  // covers: svc.write.result-and-cleanup~1
  if (!std::filesystem::exists("/dev/full")) GTEST_SKIP() << "test wymaga urzadzenia /dev/full";
  const auto target    = dir / "plan.rql";
  const auto temporary = target.string() + ".tmp." + std::to_string(getpid());
  std::ofstream(target) << "old contents";
  std::filesystem::create_symlink("/dev/full", temporary);
  LogCapture log;
  EXPECT_FALSE(servicecontrol::writeQueryFile("replacement", target.string()));
  EXPECT_EQ(read(target), "old contents");
  EXPECT_TRUE(log.text().contains("write failed to temp"));
  EXPECT_TRUE(log.text().contains(temporary));
  EXPECT_TRUE(std::filesystem::is_symlink(temporary));
}

TEST_F(ServiceControlContract, refused_plan_handles_empty_paths_and_logs_both_outcomes) {
  // covers: svc.drop-refused.empty-args~1, svc.drop-refused.clear-file~1, svc.drop-refused.log-critical~1
  EXPECT_FALSE(servicecontrol::dropRefusedPlan("unit.service", ""));
  const auto target = dir / "plan.rql";
  std::ofstream(target) << "kept";
  LogCapture log;
  EXPECT_TRUE(servicecontrol::dropRefusedPlan("unit.service", target.string()));
  EXPECT_EQ(read(target), "");
  EXPECT_TRUE(log.text().contains("cleared"));
  EXPECT_FALSE(servicecontrol::dropRefusedPlan("unit.service", (dir / "missing" / "plan.rql").string()));
  EXPECT_TRUE(log.text().contains("could NOT clear"));
  EXPECT_TRUE(log.text().contains("critical"));
}

TEST(LockManagerFlock, HashCollisionCannotAcquireTheSameIpcIdentity) {
  // covers: lockmanager.flock-service-guard.4~1
  // Dwie rozne poprawne nazwy, ten sam skrot FNV. Kolizja wymuszona danymi,
  // nie podmiana funkcji haszujacej; test dziala tez na Linuksie.
  const std::string firstName  = "review_collision_65273";
  const std::string secondName = "review_collision_108288";
  ASSERT_EQ(ipc::shortServerTag(firstName), ipc::shortServerTag(secondName));
  const std::string object = "ut_ipc_" + std::to_string(getpid()) + "." + ipc::shortServerTag(firstName);
  const auto dir           = std::filesystem::temp_directory_path() / ("ut_ipc_guard_" + std::to_string(getpid()));
  std::filesystem::create_directories(dir / "a");
  std::filesystem::create_directories(dir / "b");
  {
    FlockServiceGuard first(firstName);
    FlockServiceGuard second(secondName);
    first.setLockDir((dir / "a").string());
    second.setLockDir((dir / "b").string());
    ASSERT_TRUE(first.acquireLock());
    ASSERT_TRUE(second.acquireLock());
    ASSERT_TRUE(first.acquireIpcLock(object));
    EXPECT_FALSE(second.acquireIpcLock(object));
    second.releaseLock();
    ASSERT_TRUE(second.acquireLock());
    EXPECT_FALSE(second.acquireIpcLock(object)) << "odrzucony uczestnik zwolnil cudza blokade";
    first.releaseLock();
    EXPECT_TRUE(second.acquireIpcLock(object));
  }
  std::filesystem::remove_all(dir);
  std::filesystem::remove("/tmp/xretractor_ipc." + object + ".lock");
}

TEST(IpcIdentity, HashedNamesCannotAliasLiteralNames) {
  EXPECT_EQ(ipc::shortServerTag("review_long_name_10"), "0cb235fc4");
  EXPECT_NE(ipc::serverNameToken("review_long_name_10"), ipc::serverNameToken("cb235fc4"));
  const auto names = ipc::names("review_long_name_10");
  EXPECT_LE(names.shmemSegment.size() + 1, ipc::kMaxObjectNameLength);
  EXPECT_LE(names.queryQueue.size() + 1, ipc::kMaxObjectNameLength);
  EXPECT_LE(names.responseQueue(2147483647).size() + 1, ipc::kMaxObjectNameLength);
}

// --- Protokol kasowalnych blokad (lockFile.hpp) i sprzatanie pozostalosci ---

namespace IPC = boost::interprocess;

namespace {
bool shmExists(const std::string &name) {
  try {
    IPC::shared_memory_object probe(IPC::open_only, name.c_str(), IPC::read_only);
    return true;
  } catch (const IPC::interprocess_exception &) {
    return false;
  }
}
}  // namespace

TEST(LockFile, ExclusiveAcquireWaitsOutTransientHolder) {
  // covers: lockfile.acquire.2~1
  // Sprzatacz i proces kasujacy plik trzymaja blokade przez chwile. Startujaca instancja nie moze
  // wtedy odpasc z "juz dziala" - czeka krotko i dostaje nowy plik.
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_transient_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  std::thread releaser([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lockfile::removeAndRelease(path, holder);
  });
  int fd = -1;
  EXPECT_EQ(lockfile::acquire(path, true, false, fd), lockfile::Result::Acquired);
  releaser.join();
  ASSERT_NE(fd, -1);
  EXPECT_TRUE(lockfile::stillLinked(fd, path));
  lockfile::removeAndRelease(path, fd);
  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(LockFile, LiveHolderMeansBusyAndIsVisible) {
  // covers: lockfile.acquire.2~1, lockfile.acquire.4~1, lockfile.is-held.1~1, lockfile.is-held.2~1, lockfile.claim-abandoned.1~1
  // covers: lockfile.claim-abandoned.2~1
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_live_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  int fd = -1;
  EXPECT_EQ(lockfile::acquire(path, true, false, fd), lockfile::Result::Busy);
  EXPECT_TRUE(lockfile::isHeld(path));
  EXPECT_EQ(lockfile::claimAbandoned(path), -1) << "sprzatacz zajal blokade zywego wlasciciela";
  lockfile::removeAndRelease(path, holder);
  EXPECT_FALSE(lockfile::isHeld(path));
}

TEST(LockFile, SharedAcquireWaitsOutTransientExclusiveHolder) {
  // covers: lockfile.acquire.2~1
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_shared_transient_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  std::thread releaser([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lockfile::removeAndRelease(path, holder);
  });
  int fd            = -1;
  const auto result = lockfile::acquire(path, false, false, fd);
  releaser.join();
  ASSERT_EQ(result, lockfile::Result::Acquired);
  ASSERT_NE(fd, -1);
  EXPECT_TRUE(lockfile::stillLinked(fd, path));
  lockfile::removeAndRelease(path, fd);
}

TEST(LockFile, SharedAcquireHasDeadlineWithExclusiveHolder) {
  // covers: lockfile.acquire.2~1
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_shared_busy_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  // Zabezpieczenie testu: dawny blokujacy LOCK_SH tez w koncu wroci, ale przekroczy termin.
  std::thread releaser([&] {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    lockfile::removeAndRelease(path, holder);
  });
  int fd             = -1;
  const auto start   = std::chrono::steady_clock::now();
  const auto result  = lockfile::acquire(path, false, false, fd);
  const auto elapsed = std::chrono::steady_clock::now() - start;
  releaser.join();
  EXPECT_EQ(result, lockfile::Result::Busy);
  EXPECT_EQ(fd, -1);
  EXPECT_GE(elapsed, std::chrono::milliseconds(500));
  EXPECT_LT(elapsed, std::chrono::milliseconds(1500));
  if (fd != -1) lockfile::removeAndRelease(path, fd);
}

TEST(LockManagerFlock, IpcIdentityLockNeedsNoWriteAccess) {
  // covers: lockmanager.flock-service-guard.4~1, lockmanager.flock-service-guard.8~1
  // Blokada tozsamosci lezy w katalogu wspolnym dla maszyny i moze nalezec do innego uzytkownika:
  // 0644, a przy fs.protected_regular jadro odrzuca nawet O_CREAT na takim pliku. Plik bez prawa
  // zapisu odtwarza to bez drugiego konta (pod rootem test przechodzi trywialnie). Katalog wlasny,
  // nie /tmp: niezablokowany plik zabieral tam sprzatacz konczacego sie sasiedniego serwera.
  const std::string object = "ut_ipc_ro_" + std::to_string(getpid());
  const auto dir           = std::filesystem::temp_directory_path() / ("ut_ipc_ro_" + std::to_string(getpid()));
  std::filesystem::create_directories(dir);
  const std::string path = ipc::identityLockPath(object, dir.string());
  { std::ofstream touch(path); }
  ASSERT_EQ(chmod(path.c_str(), S_IRUSR | S_IRGRP | S_IROTH), 0);
  {
    FlockServiceGuard guard("ut_ipc_ro");
    guard.setLockDir(dir.string());
    ASSERT_TRUE(guard.acquireLock());
    EXPECT_TRUE(guard.acquireIpcLock(object, dir.string()));
  }
  EXPECT_FALSE(std::filesystem::exists(path)) << "wlasciciel nie skasowal pliku przy zwolnieniu";
  std::filesystem::remove(path);
  std::filesystem::remove_all(dir);
}

TEST(LockManagerSweep, RemovesOnlyAbandonedLocksAndTheirIpcObjects) {
  // covers: bus.sweep-abandoned-segments.1~2, lockfile.sweep.1~1, lockfile.sweep.4~1, lockmanager.sweep-abandoned-resources.1~1
  // covers: lockmanager.sweep-abandoned-resources.2~2, lockmanager.sweep-abandoned-resources.3~1
  // covers: lockmanager.sweep-abandoned-resources.4~1
  // Wlasne katalogi zamiast /tmp: tam porzucone blokady zabieral sprzatacz konczacego sie
  // sasiedniego serwera i licznik wychodzil 0. Osobny katalog na kazdy rodzaj, zeby zamiana
  // argumentow nie przeszla niezauwazona.
  const std::string pid            = std::to_string(getpid());
  const auto dir                   = std::filesystem::temp_directory_path() / ("ut_sweep_" + pid);
  const auto machineDir            = std::filesystem::temp_directory_path() / ("ut_sweep_machine_" + pid);
  const std::string dead           = "utdead" + pid;
  const std::string live           = "utlive" + pid;
  const ipc::ServerNames deadNames = ipc::names(dead);
  const ipc::ServerNames liveNames = ipc::names(live);
  const std::string deadSegment    = std::string(bus::kSegmentName) + "_utdead" + pid;
  const auto identityLock          = [&](const std::string &queue) { return ipc::identityLockPath(queue, machineDir.string()); };
  const std::string deadPresence   = (machineDir / (deadSegment + ".lock")).string();
  std::filesystem::create_directories(dir);
  std::filesystem::create_directories(machineDir);

  // Instancja zabita SIGKILL-em: pliki blokad bez wlasciciela, komplet globalnych obiektow IPC
  // i segment magistrali, ktorego nikt nie mapuje.
  { std::ofstream touch(dir / ("xretractor_service." + dead + ".lock")); }
  { std::ofstream touch(identityLock(deadNames.queryQueue)); }
  IPC::shared_memory_object(IPC::create_only, deadNames.shmemSegment.c_str(), IPC::read_write);
  IPC::message_queue(IPC::create_only, deadNames.queryQueue.c_str(), 1, 16);
  IPC::shared_memory_object(IPC::create_only, deadSegment.c_str(), IPC::read_write);
  { std::ofstream touch(deadPresence); }

  // Instancja zywa: te same rodzaje zasobow, ale blokady trzyma jej straznik.
  FlockServiceGuard liveGuard("xretractor_service." + live);
  liveGuard.setLockDir(dir.string());
  ASSERT_TRUE(liveGuard.acquireLock());
  ASSERT_TRUE(liveGuard.acquireIpcLock(liveNames.queryQueue, machineDir.string()));
  IPC::shared_memory_object(IPC::create_only, liveNames.shmemSegment.c_str(), IPC::read_write);

  // Magistrala testu, nie przestrzen srodowiska: sprzatacz czyta jej sloty.
  const std::string busSegment = std::string(bus::kSegmentName) + "_utsweepbus" + pid;
  const SweepReport swept      = sweepAbandonedResources(dir.string(), machineDir.string(), busSegment);
  EXPECT_EQ(swept.serviceLocks, 1U);
  EXPECT_EQ(swept.ipcIdentities, 1U);
  EXPECT_EQ(swept.busSegments, 1U) << "katalog nie dotarl do sprzatacza magistrali";

  EXPECT_FALSE(std::filesystem::exists(dir / ("xretractor_service." + dead + ".lock")));
  EXPECT_FALSE(std::filesystem::exists(identityLock(deadNames.queryQueue)));
  EXPECT_FALSE(shmExists(deadNames.shmemSegment));
  EXPECT_FALSE(IPC::message_queue::remove(deadNames.queryQueue.c_str())) << "kolejka komend przetrwala sprzatanie";
  EXPECT_FALSE(shmExists(deadSegment));
  EXPECT_FALSE(std::filesystem::exists(deadPresence));

  EXPECT_TRUE(std::filesystem::exists(dir / ("xretractor_service." + live + ".lock")));
  EXPECT_TRUE(std::filesystem::exists(identityLock(liveNames.queryQueue)));
  EXPECT_TRUE(shmExists(liveNames.shmemSegment)) << "sprzatacz skasowal obiekty zywej instancji";

  liveGuard.releaseLock();
  IPC::shared_memory_object::remove(liveNames.shmemSegment.c_str());
  std::filesystem::remove_all(dir);
  std::filesystem::remove_all(machineDir);
}

// A2 M3 (#565): plik blokady tozsamosci zywego serwera zniknal z katalogu (czyszczenie wedlug wieku),
// a pod ta sama sciezka zostal nietrzymany plik instancji zabitej w trakcie startu. Sama blokada
// pozwalala wtedy sprzataczowi skasowac kolejke komend i segment zywego serwera; jego slot na
// magistrali dowodzi, ze zyje.
TEST(LockManagerSweep, KeepsIpcObjectsOfInstanceLiveOnBus) {
  // covers: lockmanager.sweep-abandoned-resources.2~2
  const std::string pid            = std::to_string(getpid());
  const auto dir                   = std::filesystem::temp_directory_path() / ("ut_sweep_live_bus_" + pid);
  const auto machineDir            = std::filesystem::temp_directory_path() / ("ut_sweep_live_bus_machine_" + pid);
  const std::string live           = "utbuslive" + pid;
  const ipc::ServerNames liveNames = ipc::names(live);
  const std::string busSegment     = std::string(bus::kSegmentName) + "_utsweeplive" + pid;
  const std::string identityLock   = ipc::identityLockPath(liveNames.queryQueue, machineDir.string());
  std::filesystem::create_directories(dir);
  std::filesystem::create_directories(machineDir);
  {
    bus::Bus server(busSegment, true, machineDir.string());
    ASSERT_TRUE(server.attached());
    ASSERT_EQ(server.claim({.name = live, .streams = {"core0"}}).status, bus::ClaimStatus::Claimed);
    IPC::shared_memory_object(IPC::create_only, liveNames.shmemSegment.c_str(), IPC::read_write);
    IPC::message_queue(IPC::create_only, liveNames.queryQueue.c_str(), 1, 16);
    { std::ofstream touch(identityLock); }

    const SweepReport swept = sweepAbandonedResources(dir.string(), machineDir.string(), busSegment);
    EXPECT_EQ(swept.ipcIdentities, 1U);
    EXPECT_FALSE(std::filesystem::exists(identityLock));
    EXPECT_TRUE(shmExists(liveNames.shmemSegment)) << "sprzatacz skasowal segment instancji zywej na magistrali";
    EXPECT_TRUE(IPC::message_queue::remove(liveNames.queryQueue.c_str())) << "sprzatacz skasowal kolejke komend";
  }
  IPC::shared_memory_object::remove(liveNames.shmemSegment.c_str());
  IPC::shared_memory_object::remove(busSegment.c_str());
  std::filesystem::remove_all(dir);
  std::filesystem::remove_all(machineDir);
}

// To, co sklada ipc::serviceName, musi rozpoznac ipc::serviceLockInstance: pierwsza strone wola
// silnik, druga sprzatacz i straznik xtrdb. Oczekiwane nazwy plikow sa wpisane recznie, zeby test
// nie porownywal funkcji z nia sama.
TEST(ServiceLockFamily, NamesRoundTripAndForeignFilesAreRejected) {
  // covers: lockmanager.sweep-abandoned-resources.1~1
  EXPECT_EQ(ipc::serviceLockFile(ipc::serviceName("")), "xretractor_service.lock");
  EXPECT_EQ(ipc::serviceLockFile(ipc::serviceName("alfa")), "xretractor_service.alfa.lock");

  EXPECT_EQ(ipc::serviceLockInstance("xretractor_service.lock"), std::optional<std::string_view>(""));
  EXPECT_EQ(ipc::serviceLockInstance("xretractor_service.alfa.lock"), std::optional<std::string_view>("alfa"));

  EXPECT_FALSE(ipc::serviceLockInstance("xretractor_service..lock").has_value());  // pusty czlon po kropce
  EXPECT_FALSE(ipc::serviceLockInstance("xretractor_servicealfa.lock").has_value());
  EXPECT_FALSE(ipc::serviceLockInstance("xretractor_service.alfa.lock.bak").has_value());
  EXPECT_FALSE(ipc::serviceLockInstance("xretractor_ipc.RetractorQueryQueue.alfa.lock").has_value());
}
