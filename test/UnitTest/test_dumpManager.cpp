#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>

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
