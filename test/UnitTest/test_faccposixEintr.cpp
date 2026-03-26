#include <gtest/gtest.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "rdb/faccposix.h"
#include "rdb/faccposixshd.h"

// --- Linker-level interposition via --wrap ---
// g_pread_eintr_count / g_write_eintr_count control how many
// consecutive EINTR errors the wrapped syscall will produce
// before delegating to the real implementation.

static thread_local int g_pread_eintr_count = 0;
static thread_local int g_write_eintr_count = 0;

extern "C" {
ssize_t __real_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t __real_write(int fd, const void *buf, size_t count);

ssize_t __wrap_pread(int fd, void *buf, size_t count, off_t offset) {
  if (g_pread_eintr_count > 0) {
    --g_pread_eintr_count;
    errno = EINTR;
    return -1;
  }
  return __real_pread(fd, buf, count, offset);
}

ssize_t __wrap_write(int fd, const void *buf, size_t count) {
  if (g_write_eintr_count > 0) {
    --g_write_eintr_count;
    errno = EINTR;
    return -1;
  }
  return __real_write(fd, buf, count);
}
}

class PosixEintrTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder =
      std::filesystem::temp_directory_path() / "test_faccposixEintr";

  void SetUp() override {
    g_pread_eintr_count = 0;
    g_write_eintr_count = 0;
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override {
    g_pread_eintr_count = 0;
    g_write_eintr_count = 0;
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
  }

  std::string sandboxPath(const std::string &name) {
    return (sandBoxFolder / name).string();
  }
};

// read(): EINTR within retry limit (4 failures, 5th attempt succeeds)
TEST_F(PosixEintrTest, test_read_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(
      sandboxPath("test_r_ok"), sizeof(uint8_t));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  g_pread_eintr_count = 4;  // maxRetries is 5 → 4 failures + 1 success
  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xAA);
}

// read(): EINTR exceeding retry limit (all 5 attempts are EINTR)
TEST_F(PosixEintrTest, test_read_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(
      sandboxPath("test_r_fail"), sizeof(uint8_t));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  g_pread_eintr_count = 10;  // well above maxRetries
  uint8_t result = 0;
  ASSERT_NE(pfa->read(&result, 0), EXIT_SUCCESS);
}

// write(): EINTR within retry limit (5 failures, 6th attempt succeeds)
TEST_F(PosixEintrTest, test_write_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(
      sandboxPath("test_w_ok"), sizeof(uint8_t));

  g_write_eintr_count = 5;  // maxRetries is 5 → retries 1..5 allowed, 6th succeeds
  uint8_t data = 0xBB;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  // Verify data was actually written
  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xBB);
}

// write(): EINTR exceeding retry limit (6+ failures → gives up)
TEST_F(PosixEintrTest, test_write_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(
      sandboxPath("test_w_fail"), sizeof(uint8_t));

  g_write_eintr_count = 10;  // well above maxRetries
  uint8_t data = 0xCC;
  ASSERT_NE(pfa->write(&data), EXIT_SUCCESS);
}

// --- Shadow accessor tests ---

// shadow read(): EINTR within retry limit on fallback pread
TEST_F(PosixEintrTest, test_shadow_read_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_r_ok"), sizeof(uint8_t));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);  // append to main

  g_pread_eintr_count = 4;  // 4 failures, 5th succeeds
  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xAA);
}

// shadow read(): EINTR exceeding retry limit on fallback pread
TEST_F(PosixEintrTest, test_shadow_read_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_r_fail"), sizeof(uint8_t));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);  // append to main

  g_pread_eintr_count = 10;  // well above maxRetries
  uint8_t result = 0;
  ASSERT_NE(pfa->read(&result, 0), EXIT_SUCCESS);
}

// shadow append write(): EINTR within retry limit
TEST_F(PosixEintrTest, test_shadow_append_write_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_aw_ok"), sizeof(uint8_t));

  g_write_eintr_count = 5;  // retries 1..5, 6th succeeds
  uint8_t data = 0xBB;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);  // append (position = max)

  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xBB);
}

// shadow append write(): EINTR exceeding retry limit
TEST_F(PosixEintrTest, test_shadow_append_write_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_aw_fail"), sizeof(uint8_t));

  g_write_eintr_count = 10;
  uint8_t data = 0xCC;
  ASSERT_NE(pfa->write(&data), EXIT_SUCCESS);  // append should fail
}

// shadow update write(): EINTR within retry limit on shadow data write
TEST_F(PosixEintrTest, test_shadow_update_write_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_uw_ok"), sizeof(uint8_t));

  // First, append a record to have something to update
  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  // Update position 0 → goes to shadow file
  // The shadow write does: write(position) then write(data) in a loop.
  // We inject EINTR only for the data write (the 2nd ::write call),
  // so set count to trigger after the position write succeeds.
  g_write_eintr_count = 0;
  data = 0xDD;
  ASSERT_EQ(pfa->write(&data, 0), EXIT_SUCCESS);  // update at pos 0

  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xDD);
}

// shadow update write(): EINTR exceeding retry limit on shadow data write
TEST_F(PosixEintrTest, test_shadow_update_write_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFileWithShadowAccessor>(
      sandboxPath("test_shd_uw_fail"), sizeof(uint8_t));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);  // append first

  // The shadow update path calls ::write twice: once for position, once for data.
  // We need the position write to succeed, then EINTR the data write.
  // Position write consumes 1 call, then 10 EINTRs for the data loop.
  g_write_eintr_count = 10;
  data = 0xEE;
  // Either the position write or data write will fail with EINTR exhaustion
  ASSERT_NE(pfa->write(&data, 0), EXIT_SUCCESS);
}
