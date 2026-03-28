#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "rdb/faccbindev.h"

namespace {

class BinaryDeviceROTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccbindev";

  void SetUp() override {
    std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override { std::filesystem::remove_all(sandBoxFolder); }

  std::string sandboxPath(const std::string &name) const { return (sandBoxFolder / name).string(); }

  void writeBinaryFile(const std::string &path, const std::vector<uint8_t> &bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    if (!bytes.empty()) {
      out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      ASSERT_TRUE(out.good());
    }
  }
};

}  // namespace

TEST_F(BinaryDeviceROTest, read_exact_record_and_count) {
  auto path = sandboxPath("device.bin");
  writeBinaryFile(path, {0x11, 0x22, 0x33, 0x44});

  rdb::binaryDeviceRO dev(path, 4, true);
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x11);
  EXPECT_EQ(out[1], 0x22);
  EXPECT_EQ(out[2], 0x33);
  EXPECT_EQ(out[3], 0x44);
  EXPECT_EQ(dev.count(), 1U);
}

TEST_F(BinaryDeviceROTest, count_starts_at_zero_before_any_read) {
  auto path = sandboxPath("position.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  rdb::binaryDeviceRO dev(path, 4, true);
  EXPECT_EQ(dev.count(), 0U);

  uint8_t out[4] = {0, 0, 0, 0};
  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(dev.count(), 1U);
}

TEST_F(BinaryDeviceROTest, read_loops_to_beginning_on_eof_when_enabled) {
  auto path = sandboxPath("loop.bin");
  writeBinaryFile(path, {0x01, 0x02, 0x03, 0x04});

  rdb::binaryDeviceRO dev(path, 4, true);
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x01);
  EXPECT_EQ(out[1], 0x02);
  EXPECT_EQ(out[2], 0x03);
  EXPECT_EQ(out[3], 0x04);
  EXPECT_EQ(dev.count(), 2U);
}

TEST_F(BinaryDeviceROTest, read_zero_fills_on_eof_when_loop_disabled) {
  auto path = sandboxPath("noloop.bin");
  writeBinaryFile(path, {0x10, 0x20, 0x30, 0x40});

  rdb::binaryDeviceRO dev(path, 4, false);
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  out[0] = 0xFF;
  out[1] = 0xFF;
  out[2] = 0xFF;
  out[3] = 0xFF;

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(dev.count(), 2U);
}

TEST_F(BinaryDeviceROTest, read_fails_on_empty_file_when_loop_enabled) {
  auto path = sandboxPath("empty.bin");
  writeBinaryFile(path, {});

  rdb::binaryDeviceRO dev(path, 4, true);
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_FAILURE);
  EXPECT_EQ(dev.count(), 0U);
}

TEST_F(BinaryDeviceROTest, name_and_write_contract) {
  auto path = sandboxPath("name.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  rdb::binaryDeviceRO dev(path, 4, true);
  uint8_t out[4] = {0x10, 0x20, 0x30, 0x40};

  EXPECT_EQ(dev.name(), path);
  EXPECT_EQ(dev.write(out, 0), EXIT_FAILURE);
}
