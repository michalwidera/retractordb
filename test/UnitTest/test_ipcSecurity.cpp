#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <string>

#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "ipcResponses.hpp"
#include "ipcSecurity.hpp"
#include "logCapture.hpp"
#include "osPlatform.hpp"
#include "qry/ipcClient.hpp"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/ipcServer.hpp"
#include "retractor/lib/lockFile.hpp"
#include "retractor/lib/lockManager.hpp"
#include "syscallWrap.hpp"

namespace {
namespace IPC = boost::interprocess;
namespace sec = ipc::security;

// Wstrzykniete metadane sprawdzaja odmowe takze bez uprawnien do zmiany UID.
// Rzeczywisty drugi UID ma osobny test integracyjny, jawnie pomijany bez root.
std::string foreignPath;
int foreignFd      = -1;
ino_t foreignLock  = 0;
int rejectedLockFd = -1;
std::string namespaceFaultPath;
int namespaceError   = 0;
mode_t namespaceMode = 0;
int objectLstats     = 0;
std::string changedObjectPath;
bool cleanupIoError = false;
uid_t otherUid() { return geteuid() == 65534 ? 65533 : 65534; }

std::string pathFor(const std::string &name) {
#if RDB_HAS_BOOST_POSIX_SHM
  return (std::filesystem::path(osplat::sharedMemoryBackingPath()) / name).string();
#else
  std::string path;
  IPC::ipcdetail::shared_filepath(name.c_str(), path);
  return path;
#endif
}

class IpcSecurity : public ::testing::Test {
 protected:
  const std::string serverName = "sec" + std::to_string(getpid());
  const ipc::ServerNames names = ipc::names(serverName);
  void TearDown() override {
    foreignPath.clear();
    foreignFd   = -1;
    foreignLock = 0;
    namespaceFaultPath.clear();
    namespaceError = 0;
    namespaceMode  = 0;
    changedObjectPath.clear();
    cleanupIoError = false;
    IPC::shared_memory_object::remove(names.shmemSegment.c_str());
    IPC::message_queue::remove(names.queryQueue.c_str());
    IPC::message_queue::remove(names.responseQueue(getpid()).c_str());
    std::filesystem::remove(pathFor(names.shmemSegment) + ".alias");
  }
};
}  // namespace

extern "C" int __real_fstat(int fd, struct stat *info);
extern "C" int __real_lstat(const char *path, struct stat *info);
extern "C" int __real_stat(const char *path, struct stat *info);
RDB_WRAP_SYSCALL(int, fstat, (int fd, struct stat *info), (fd, info));
RDB_WRAP_SYSCALL(int, lstat, (const char *path, struct stat *info), (path, info));
RDB_WRAP_SYSCALL(int, stat, (const char *path, struct stat *info), (path, info));

extern "C" int __wrap_fstat(int fd, struct stat *info) {
  const int result = __real_fstat(fd, info);
  if (result == 0 && (fd == foreignFd || (foreignLock != 0 && info->st_ino == foreignLock))) {
    info->st_uid = otherUid();
    if (foreignLock != 0 && info->st_ino == foreignLock) rejectedLockFd = fd;
  }
  return result;
}

extern "C" int __wrap_lstat(const char *path, struct stat *info) {
  if (!namespaceFaultPath.empty() && namespaceFaultPath == path && namespaceError != 0) {
    errno = namespaceError;
    return -1;
  }
  if (cleanupIoError && changedObjectPath == path) {
    errno = EIO;
    return -1;
  }
  const int result = __real_lstat(path, info);
  if (result == 0 && !foreignPath.empty() && foreignPath == path) info->st_uid = otherUid();
  // openMemory sprawdza wpis i deskryptor (trzy lstat); czwarty jest PO otwarciu kolejki.
  if (result == 0 && !changedObjectPath.empty() && changedObjectPath == path && ++objectLstats == 4) ++info->st_ino;
  return result;
}

extern "C" int __wrap_stat(const char *path, struct stat *info) {
  const int result = __real_stat(path, info);
  if (result == 0 && !namespaceFaultPath.empty() && namespaceFaultPath == path && namespaceMode != 0)
    info->st_mode = namespaceMode;
  return result;
}

TEST_F(IpcSecurity, ExclusiveCollisionPreservesQueueAndItsContents) {
  const auto queue = sec::createQueue(names.queryQueue, 3, 16);
  queue->send("sentinel", 8, 0);
  EXPECT_THROW(sec::createQueue(names.queryQueue, 9, 32), sec::Rejected);
  const auto reopened = sec::openQueue(names.queryQueue);
  EXPECT_EQ(reopened->get_max_msg(), 3U);
  EXPECT_EQ(reopened->get_max_msg_size(), 16U);
  EXPECT_EQ(reopened->get_num_msg(), 1U);
}

TEST_F(IpcSecurity, NamespaceRejectsWritableNonstickyAndNondirectoryTargets) {
  const auto memory  = sec::createMemory(names.shmemSegment);
  namespaceFaultPath = std::filesystem::path(pathFor(names.shmemSegment)).parent_path().string();
  for (const mode_t mode : {mode_t(S_IFDIR | 0777), mode_t(S_IFREG | 0600)}) {
    namespaceMode = mode;
    EXPECT_THROW(sec::checkNamespace(names.shmemSegment), sec::Rejected);
  }
  namespaceMode = S_IFDIR | S_ISVTX | 0777;
  EXPECT_NO_THROW(sec::checkNamespace(names.shmemSegment));
  namespaceMode = S_IFDIR | 0700;
  EXPECT_NO_THROW(sec::checkNamespace(names.shmemSegment));
}

TEST_F(IpcSecurity, NamespaceDistinguishesMissingDirectoryFromInspectionFailure) {
  const auto memory  = sec::createMemory(names.shmemSegment);
  namespaceFaultPath = std::filesystem::path(pathFor(names.shmemSegment)).parent_path().string();
  namespaceError     = ENOENT;
  try {
    sec::checkNamespace(names.shmemSegment);
    ADD_FAILURE() << "brak katalogu nie zostal zgloszony";
  } catch (const IPC::interprocess_exception &error) {
    EXPECT_EQ(error.get_native_error(), ENOENT);
  }
  namespaceError = EACCES;
  EXPECT_THROW(sec::checkNamespace(names.shmemSegment), sec::Rejected);
}

TEST_F(IpcSecurity, HardlinkAndSymlinkObjectsAreRejectedWithoutDamagingTheirTarget) {
  auto memory = sec::createMemory(names.shmemSegment);
  memory.truncate(16);
  const auto path  = std::filesystem::path(pathFor(names.shmemSegment));
  const auto alias = std::filesystem::path(path.string() + ".alias");
  std::filesystem::create_hard_link(path, alias);
  EXPECT_THROW(sec::openMemory(names.shmemSegment, IPC::read_only), sec::Rejected);
  EXPECT_FALSE(sec::removeOwned(names.shmemSegment));
  EXPECT_TRUE(std::filesystem::exists(path));
  std::filesystem::remove(alias);
  std::filesystem::rename(path, alias);
  std::filesystem::create_symlink(alias, path);
  EXPECT_THROW(sec::openMemory(names.shmemSegment, IPC::read_only), sec::Rejected);
  EXPECT_FALSE(sec::removeOwned(names.shmemSegment));
  EXPECT_TRUE(std::filesystem::is_symlink(path));
  EXPECT_EQ(std::filesystem::file_size(alias), 16U);
  std::filesystem::remove(path);
  std::filesystem::remove(alias);
}

TEST_F(IpcSecurity, OpenedDescriptorMustStillBeTheNamedObject) {
  const auto memory = sec::createMemory(names.shmemSegment);
  const auto path   = std::filesystem::path(pathFor(names.shmemSegment));
  const auto alias  = std::filesystem::path(path.string() + ".alias");
  std::filesystem::rename(path, alias);
  {
    const auto replacement = sec::createMemory(names.shmemSegment);
    EXPECT_THROW(sec::checkOpened(memory, names.shmemSegment), sec::Rejected);
    EXPECT_NO_THROW(sec::checkOpened(replacement, names.shmemSegment));
  }
  std::filesystem::remove(alias);
}

TEST_F(IpcSecurity, QueueOpenRejectsNameReplacementAfterOpening) {
  const auto queue  = sec::createQueue(names.queryQueue, 1, 16);
  changedObjectPath = pathFor(names.queryQueue);
  objectLstats      = 0;
  EXPECT_THROW(sec::openQueue(names.queryQueue), sec::Rejected);
  EXPECT_EQ(objectLstats, 4);
  changedObjectPath.clear();
  EXPECT_NO_THROW(sec::openQueue(names.queryQueue));
}

TEST_F(IpcSecurity, CleanupIoFailureLogsAndReturnsFalseWithoutRemovingTheObject) {
  const auto memory = sec::createMemory(names.shmemSegment);
  changedObjectPath = pathFor(names.shmemSegment);
  cleanupIoError    = true;
  const LogCapture log;
  EXPECT_FALSE(sec::removeOwned(names.shmemSegment));
  EXPECT_NE(log.text().find("cannot inspect object for cleanup"), std::string::npos);
  cleanupIoError = false;
  changedObjectPath.clear();
  EXPECT_TRUE(sec::removeOwned(names.shmemSegment));
}

TEST_F(IpcSecurity, InvalidNamesMissingObjectsAndExclusiveMemoryCreation) {
  EXPECT_THROW(sec::checkNamespace("", false), sec::Rejected);
  EXPECT_THROW(sec::checkNamespace("unsafe/name", true), sec::Rejected);
  EXPECT_THROW(sec::openMemory(names.shmemSegment, IPC::read_only), IPC::interprocess_exception);
  auto memory = sec::createMemory(names.shmemSegment);
  memory.truncate(16);
  EXPECT_THROW(sec::createMemory(names.shmemSegment), sec::Rejected);
  EXPECT_TRUE(sec::removeOwned(names.shmemSegment));
  EXPECT_TRUE(sec::removeOwned(names.shmemSegment));
}

TEST_F(IpcSecurity, NewObjectsHaveMode0600AndExpectedOwner) {
  const auto memory  = sec::createMemory(names.shmemSegment);
  const auto queue   = sec::createQueue(names.queryQueue, 1, 16);
  const auto backing = sec::openMemory(names.queryQueue, IPC::read_only);
  for (const auto *object : {&memory, &backing}) {
    struct stat info{};
    ASSERT_EQ(fstat(object->get_mapping_handle().handle, &info), 0);
    EXPECT_EQ(info.st_uid, geteuid());
    EXPECT_EQ(info.st_mode & 07777, 0600U);
  }
}

TEST_F(IpcSecurity, OpenChecksTheDescriptorOwner) {
  const LogCapture capture;
  spdlog::default_logger()->set_level(spdlog::level::err);
  const auto memory = sec::createMemory(names.shmemSegment);
  foreignFd         = memory.get_mapping_handle().handle;
  try {
    sec::checkOpened(memory, names.shmemSegment);
    ADD_FAILURE() << "obcy UID deskryptora zostal przyjety";
  } catch (const sec::Rejected &ex) {
    const std::string reason = ex.what();
    EXPECT_NE(reason.find(names.shmemSegment), std::string::npos);
    EXPECT_NE(reason.find("owner UID " + std::to_string(otherUid())), std::string::npos);
    EXPECT_NE(reason.find("expected UID " + std::to_string(geteuid())), std::string::npos);
  }
  foreignFd = -1;
  EXPECT_NE(capture.text().find(names.shmemSegment), std::string::npos);
  EXPECT_NE(capture.text().find("owner UID " + std::to_string(otherUid())), std::string::npos);
  EXPECT_NE(capture.text().find("untrusted owner"), std::string::npos);
  EXPECT_NO_THROW(sec::checkOpened(memory, names.shmemSegment));
}

TEST_F(IpcSecurity, OpenRejectsBroadPermissionsBeforeReadingQueueLayout) {
  const IPC::shared_memory_object memory(IPC::create_only, names.queryQueue.c_str(), IPC::read_write, IPC::permissions(0666));
  EXPECT_THROW(sec::openQueue(names.queryQueue), sec::Rejected);
  EXPECT_THROW(sec::openMemory(names.queryQueue, IPC::read_write), sec::Rejected);
  EXPECT_TRUE(sec::removeOwned(names.queryQueue)) << "stary tryb wlasnego obiektu mozna posprzatac";
}

TEST_F(IpcSecurity, ForeignDirectoryCannotAuthorizeAnObject) {
  const auto memory = sec::createMemory(names.shmemSegment);
  foreignPath       = std::filesystem::path(pathFor(names.shmemSegment)).parent_path().string();
  EXPECT_THROW(sec::openMemory(names.shmemSegment, IPC::read_write), sec::Rejected);
  EXPECT_FALSE(sec::removeOwned(names.shmemSegment));
  foreignPath.clear();
  EXPECT_NO_THROW(sec::openMemory(names.shmemSegment, IPC::read_write));
}

TEST_F(IpcSecurity, ClientRejectsForeignCommandQueueWithoutSending) {
  const ipc::responses::Mapping response(IPC::create_only, names.shmemSegment);
  const auto queue = sec::createQueue(names.queryQueue, 1, ipc::kQueryQueueMaxMessageSize);
  foreignPath      = pathFor(names.queryQueue);
  IpcClient client(1, 1, serverName);
  EXPECT_THROW(client.netClient("kill", ""), sec::Rejected);
  EXPECT_TRUE(client.done);
  EXPECT_EQ(queue->get_num_msg(), 0U);
}

TEST_F(IpcSecurity, ClientRejectsForeignResponseSegmentBeforeSending) {
  const ipc::responses::Mapping response(IPC::create_only, names.shmemSegment);
  const auto queue = sec::createQueue(names.queryQueue, 1, ipc::kQueryQueueMaxMessageSize);
  foreignPath      = pathFor(names.shmemSegment);
  IpcClient client(1, 1, serverName);
  EXPECT_THROW(client.netClient("kill", ""), sec::Rejected);
  EXPECT_EQ(queue->get_num_msg(), 0U);
}

TEST_F(IpcSecurity, ProducerRejectsForeignSubscriptionWithoutReceiving) {
  const std::string name = names.responseQueue(getpid());
  const auto queue       = sec::createQueue(name, 1, ipc::kResponseQueueMaxMessageSize);
  queue->send("stream sentinel", 15, 0);
  foreignPath = pathFor(name);
  IpcClient client(1, 1, serverName);
  client.producer();
  EXPECT_TRUE(client.done);
  EXPECT_TRUE(client.responseQueueMissing);
  EXPECT_EQ(queue->get_num_msg(), 1U);
}

TEST_F(IpcSecurity, SubscriptionRefusalAndCleanupPreserveForeignQueue) {
  const std::string name = names.responseQueue(getpid());
  const auto queue       = sec::createQueue(name, 1, ipc::kResponseQueueMaxMessageSize);
  foreignPath            = pathFor(name);
  IpcServer server;
  server.setServerName(serverName);
  EXPECT_THROW(server.subscribe(getpid(), "src", 8, server.subscriptionEpoch()), sec::Rejected);
  server.removeAllObjects();
  EXPECT_TRUE(std::filesystem::exists(pathFor(name)));
  EXPECT_FALSE(sec::removeOwned(name));
}

TEST_F(IpcSecurity, ResubscriptionRecreatesOwnQueueWithRequestedCapacity) {
  IpcServer server;
  server.setServerName(serverName);
  ASSERT_TRUE(server.subscribe(getpid(), "src", 2, server.subscriptionEpoch()));
  const auto oldQueue = sec::openQueue(names.responseQueue(getpid()));
  oldQueue->send("stale", 5, 0);
  ASSERT_TRUE(server.subscribe(getpid(), "dst", 7, server.subscriptionEpoch()));
  const auto newQueue = sec::openQueue(names.responseQueue(getpid()));
  EXPECT_EQ(newQueue->get_max_msg(), 7U);
  EXPECT_EQ(newQueue->get_num_msg(), 0U);
  server.removeAllObjects();
}

TEST_F(IpcSecurity, BusRejectsUntrustedSegmentWithoutDestructorDeletingIt) {
  const std::string name = names.shmemSegment;
  const IPC::shared_memory_object memory(IPC::create_only, name.c_str(), IPC::read_write, IPC::permissions(0666));
  for (const bool create : {false, true}) {
    {
      const bus::Bus refused(name, create);
      EXPECT_FALSE(refused.attached());
    }
    EXPECT_TRUE(std::filesystem::exists(pathFor(name)));
  }
}

TEST_F(IpcSecurity, BusRejectsForeignOwnerAndPreservesLiveMapper) {
  const std::string name = names.shmemSegment;
  {
    const bus::Bus owner(name);
    ASSERT_TRUE(owner.attached());
    foreignPath = pathFor(name);
    {
      const bus::Bus refused(name, false);
      EXPECT_FALSE(refused.attached());
    }
    foreignPath.clear();
    const bus::Bus reader(name, false);
    EXPECT_TRUE(reader.attached());
    EXPECT_TRUE(owner.attached());
  }
}

TEST_F(IpcSecurity, SweepDoesNotRemoveForeignLockOrInvokeItsCleanup) {
  const auto dir = std::filesystem::temp_directory_path() / serverName;
  std::filesystem::create_directories(dir);
  const auto path = dir / "foreign.lock";
  { std::ofstream touch(path); }
  struct stat info{};
  ASSERT_EQ(stat(path.c_str(), &info), 0);
  foreignLock   = info.st_ino;
  bool callback = false;
  EXPECT_EQ(
      lockfile::sweep(dir.string(), [](std::string_view) { return true; }, [&](std::string_view) { callback = true; }), 0U);
  EXPECT_FALSE(callback);
  EXPECT_TRUE(std::filesystem::exists(path));
  foreignLock = 0;
  std::filesystem::remove_all(dir);
}

TEST_F(IpcSecurity, WritableLockRejectsForeignDescriptorButReadersCanAcquireIt) {
  const auto dir = std::filesystem::temp_directory_path() / serverName;
  std::filesystem::create_directories(dir);
  const auto path = dir / "foreign.lock";
  { std::ofstream(path) << "foreign sentinel"; }
  struct stat info{};
  ASSERT_EQ(stat(path.c_str(), &info), 0);
  foreignLock    = info.st_ino;
  rejectedLockFd = -1;
  FlockServiceGuard guard("foreign");
  guard.setLockDir(dir.string());
  EXPECT_FALSE(guard.acquireLock());
  EXPECT_FALSE(guard.isLockActive());
  EXPECT_NE(guard.acquireLockError().find("untrusted owner"), std::string::npos);
  EXPECT_NE(guard.acquireLockError().find(path.string()), std::string::npos);
  ASSERT_NE(rejectedLockFd, -1);
  EXPECT_EQ(fcntl(rejectedLockFd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF) << "odrzucony deskryptor pozostal otwarty";
  for (const bool exclusive : {false, true}) {
    int fd = -1;
    ASSERT_EQ(lockfile::acquire(path.string(), exclusive, false, fd), lockfile::Result::Acquired);
    EXPECT_EQ(close(fd), 0);
  }
  foreignLock = 0;
  std::ifstream input(path);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(input), {}), "foreign sentinel");
  EXPECT_EQ(std::filesystem::file_size(path), 16U);
  std::filesystem::remove_all(dir);
}
