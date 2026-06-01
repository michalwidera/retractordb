#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#define private public
#include "retractor/lib/dumpManager.hpp"
#undef private

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"

extern "C" off_t __real_lseek(int fd, off_t offset, int whence);
extern "C" ssize_t __real_write(int fd, const void *buf, size_t count);

namespace {
bool wrapEnabled      = false;
int wrappedLseekCalls = 0;
int wrappedWriteCalls = 0;
int wrappedLastFd     = -1;
}  // namespace

extern "C" off_t __wrap_lseek(int fd, off_t offset, int whence) {
  if (!wrapEnabled) return __real_lseek(fd, offset, whence);
  (void)offset;
  (void)whence;
  wrappedLseekCalls++;
  wrappedLastFd = fd;
  return 0;
}

extern "C" ssize_t __wrap_write(int fd, const void *buf, size_t count) {
  if (!wrapEnabled) return __real_write(fd, buf, count);
  (void)buf;
  wrappedWriteCalls++;
  wrappedLastFd = fd;
  return static_cast<ssize_t>(count);
}

TEST(dumpManager, buildDumpChunk_accepts_fd_zero_as_valid) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 42);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;
  wrappedLastFd     = -1;

  const bool completed = manager.buildDumpChunk(task, payload.get());

  wrapEnabled = false;

  EXPECT_TRUE(completed);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);
  EXPECT_EQ(wrappedLseekCalls, 1);
  EXPECT_EQ(wrappedWriteCalls, 1);
  EXPECT_EQ(wrappedLastFd, 0);
}

TEST(dumpManager, buildDumpChunk_rejects_negative_fd) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 7);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = -1;

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "file descriptor is not set");
}

// ============================================================
// buildDumpChunk — opóźnienie (delayDumpRecordsToGo > 0)
// ============================================================

TEST(dumpManager, buildDumpChunk_delay_decrements_returns_false) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {2, 4}, 0);
  task.dumpedRecordsToGo    = 2;
  task.delayDumpRecordsToGo = 2;
  task.fd                   = 0;  // wrapping nie jest potrzebne — zapis nie nastąpi

  const bool result = manager.buildDumpChunk(task, payload.get());

  EXPECT_FALSE(result);
  EXPECT_EQ(task.delayDumpRecordsToGo, 1);
  EXPECT_EQ(task.dumpedRecordsToGo, 2);  // bez zmian
}

TEST(dumpManager, buildDumpChunk_delay_exhausted_then_writes) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 99);

  dumpTask task("task", {1, 2}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 1;
  task.fd                   = 0;

  // Pierwsze wywołanie: opóźnienie, brak zapisu
  bool r1 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.delayDumpRecordsToGo, 0);

  // Drugie wywołanie: opóźnienie wyczerpane, zapis
  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;

  bool r2 = manager.buildDumpChunk(task, payload.get());

  wrapEnabled = false;

  EXPECT_TRUE(r2);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);
  EXPECT_EQ(wrappedLseekCalls, 1);
  EXPECT_EQ(wrappedWriteCalls, 1);
}

// ============================================================
// buildDumpChunk — wiele rekordów
// ============================================================

TEST(dumpManager, buildDumpChunk_multiple_records_not_done_until_last) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 3}, 0);
  task.dumpedRecordsToGo    = 3;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  wrapEnabled       = true;
  wrappedWriteCalls = 0;

  bool r1 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.dumpedRecordsToGo, 2);

  bool r2 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r2);
  EXPECT_EQ(task.dumpedRecordsToGo, 1);

  bool r3 = manager.buildDumpChunk(task, payload.get());
  EXPECT_TRUE(r3);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);

  wrapEnabled = false;

  EXPECT_EQ(wrappedWriteCalls, 3);
}

// ============================================================
// buildDumpChunk — FatalError dla ujemnych wartości
// ============================================================

TEST(dumpManager, buildDumpChunk_negative_dumpedRecordsToGo_fatals) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = -1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "dumpedRecordsToGo is negative");
}

TEST(dumpManager, buildDumpChunk_negative_delayDumpRecordsToGo_fatals) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 1}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = -1;
  task.fd                   = 0;

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "delayDumpRecordsToGo is negative");
}

// ============================================================
// dumpTask — semantyka move i destruktor
// ============================================================

TEST(dumpTask, move_constructor_transfers_fd) {
  int fd = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd, 0);

  dumpTask src("src", {0, 1}, 0);
  src.fd = fd;

  dumpTask dst(std::move(src));

  EXPECT_EQ(dst.fd, fd);
  EXPECT_EQ(src.fd, -1);  // NOLINT(bugprone-use-after-move)
}

TEST(dumpTask, move_assignment_closes_old_fd_and_transfers) {
  int fd1 = ::open("/dev/null", O_RDWR);
  int fd2 = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd1, 0);
  ASSERT_GE(fd2, 0);

  dumpTask src("src", {0, 1}, 0);
  src.fd = fd1;

  dumpTask dst("dst", {0, 1}, 0);
  dst.fd = fd2;

  dst = std::move(src);  // zamyka fd2, przejmuje fd1

  EXPECT_EQ(dst.fd, fd1);
  EXPECT_EQ(src.fd, -1);  // NOLINT(bugprone-use-after-move)
  // fd2 powinien być zamknięty
  EXPECT_EQ(fcntl(fd2, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST(dumpTask, destructor_closes_fd) {
  int fd = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd, 0);

  {
    dumpTask task("t", {0, 1}, 0);
    task.fd = fd;
  }  // destruktor zamyka fd

  EXPECT_EQ(fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST(dumpTask, destructor_with_negative_fd_does_not_crash) {
  dumpTask task("t", {0, 1}, 0);
  // task.fd = -1 domyślnie — destruktor nie wywołuje close
}

// ============================================================
// setDumpStorage + createDumpFile
// ============================================================

class DumpManagerFileTest : public ::testing::Test {
 protected:
  std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_dumpManager_files";

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override {
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
  }
};

TEST_F(DumpManagerFileTest, setDumpStorage_sets_path) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  EXPECT_EQ(manager.storagePath, sandBoxFolder.string());
}

TEST_F(DumpManagerFileTest, createDumpFile_without_retention_creates_tmp_suffix) {
  dumpManager manager;
  manager.storagePath = sandBoxFolder.string();
  // retentionSize["streamt"] = 0 (domyślna wartość mapy)

  auto [filename, fd] = manager.createDumpFile("stream", "t");
  ASSERT_GE(fd, 0);
  ::close(fd);

  EXPECT_TRUE(std::filesystem::exists(filename));
  EXPECT_TRUE(std::string_view(filename).ends_with("_dump.tmp"));
}

TEST_F(DumpManagerFileTest, createDumpFile_with_retention_creates_numbered_files) {
  dumpManager manager;
  manager.storagePath                  = sandBoxFolder.string();
  manager.retentionSize["streamtask"]  = 3;

  auto [f0, fd0] = manager.createDumpFile("stream", "task");
  ASSERT_GE(fd0, 0);
  ::close(fd0);

  auto [f1, fd1] = manager.createDumpFile("stream", "task");
  ASSERT_GE(fd1, 0);
  ::close(fd1);

  EXPECT_TRUE(std::string_view(f0).ends_with("_dump_0.tmp"));
  EXPECT_TRUE(std::string_view(f1).ends_with("_dump_1.tmp"));
  EXPECT_TRUE(std::filesystem::exists(f0));
  EXPECT_TRUE(std::filesystem::exists(f1));
}
