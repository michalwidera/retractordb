#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/faccbindev.hpp"

// Tests intentionally use raw byte buffers for binary device I/O coverage.
// NOLINTBEGIN(modernize-avoid-c-arrays)

namespace {

class BinaryDeviceROTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccbindev";

  void SetUp() override {
    std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override { std::filesystem::remove_all(sandBoxFolder); }

  [[nodiscard]] std::string sandboxPath(const std::string &name) const { return (sandBoxFolder / name).string(); }

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

static rdb::Descriptor fixedIntDescriptor() { return rdb::Descriptor{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}}; }

TEST_F(BinaryDeviceROTest, read_exact_record_and_count) {
  auto path = sandboxPath("device.bin");
  writeBinaryFile(path, {0x11, 0x22, 0x33, 0x44});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x11);
  EXPECT_EQ(out[1], 0x22);
  EXPECT_EQ(out[2], 0x33);
  EXPECT_EQ(out[3], 0x44);
  EXPECT_EQ(dev.count(), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_FALSE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, count_starts_at_zero_before_any_read) {
  auto path = sandboxPath("position.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  EXPECT_EQ(dev.count(), 0U);

  uint8_t out[4] = {0, 0, 0, 0};
  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(dev.count(), 1U);
}

TEST_F(BinaryDeviceROTest, read_loops_to_beginning_on_eof_when_enabled) {
  auto path = sandboxPath("loop.bin");
  writeBinaryFile(path, {0x01, 0x02, 0x03, 0x04});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
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

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, false, "BINFILE");
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

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, read_fails_on_empty_file_when_loop_enabled) {
  auto path = sandboxPath("empty.bin");
  writeBinaryFile(path, {});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};

  EXPECT_EQ(dev.read(out, 0), EIO);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(dev.count(), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, read_fails_on_missing_file_with_null_metadata) {
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(sandboxPath("missing.bin"), desc, true, "BINFILE");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};

  EXPECT_EQ(dev.read(out, 0), EBADF);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(dev.count(), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, name_and_write_contract) {
  auto path = sandboxPath("name.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0x10, 0x20, 0x30, 0x40};

  EXPECT_EQ(dev.name(), path);
  EXPECT_EQ(dev.write(out, 0), ENOTSUP);
}

// Regression: ::read on a FIFO may return fewer bytes than requested. The old code treated any
// short read as end of stream, so a record arriving in two writes was lost and the reader
// desynchronised.
TEST_F(BinaryDeviceROTest, combines_short_fifo_reads_into_one_record) {
  auto path = sandboxPath("short-read.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_GE(writer, 0);

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, false, "DEVICE");
  uint8_t out[4] = {0, 0, 0, 0};
  std::promise<void> started;
  auto readResult = std::async(std::launch::async, [&] {
    started.set_value();
    return dev.read(out, 0);
  });

  started.get_future().wait();
  const uint8_t firstHalf[2] = {0x11, 0x22};
  EXPECT_EQ(::write(writer, firstHalf, sizeof(firstHalf)), static_cast<ssize_t>(sizeof(firstHalf)));
  EXPECT_EQ(readResult.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);

  const uint8_t secondHalf[2] = {0x33, 0x44};
  EXPECT_EQ(::write(writer, secondHalf, sizeof(secondHalf)), static_cast<ssize_t>(sizeof(secondHalf)));
  ::close(writer);
  EXPECT_EQ(readResult.get(), EXIT_SUCCESS);
  EXPECT_EQ(std::vector<uint8_t>(std::begin(out), std::end(out)), std::vector<uint8_t>({0x11, 0x22, 0x33, 0x44}));
}

// BINFILE czyta wylacznie plik zwykly i sprawdza to PRZED otwarciem (#346): open(O_RDONLY) na FIFO
// bez pisarza wisi w samym wywolaniu. Konstrukcja idzie w osobnym watku z limitem czasu - gdyby
// akcesor otworzyl FIFO, test zakonczylby proces zamiast wisiec do limitu ctest.
TEST_F(BinaryDeviceROTest, binfile_refuses_a_fifo_without_blocking) {
  auto path = sandboxPath("feed.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();

  auto pending = std::async(std::launch::async, [&] {
    rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
    uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    return std::make_pair(dev.initializationError(), dev.read(out, 0));
  });
  if (pending.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    std::cerr << "binaryDeviceRO(BINFILE) blocked on a FIFO without a writer\n";
    std::_Exit(EXIT_FAILURE);
  }
  const auto [error, readResult] = pending.get();
  EXPECT_EQ(error, "BINFILE '" + path + "' is a FIFO, not a regular file");
  EXPECT_EQ(readResult, EBADF);
}

TEST_F(BinaryDeviceROTest, binfile_refuses_a_character_device_and_device_a_regular_file) {
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO null(std::string("/dev/null"), desc, true, "BINFILE");
  EXPECT_EQ(null.initializationError(), "BINFILE '/dev/null' is a character device, not a regular file");

  auto path = sandboxPath("plain.bin");
  writeBinaryFile(path, {0x01, 0x02, 0x03, 0x04});
  rdb::binaryDeviceRO plain(path, desc, true, "DEVICE");
  EXPECT_EQ(plain.initializationError(), "DEVICE '" + path + "' is a regular file, not a character device or FIFO");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  EXPECT_EQ(plain.read(out, 0), EBADF) << "odrzucona sciezka nie moze byc czytana";

  rdb::binaryDeviceRO device(std::string("/dev/zero"), desc, true, "DEVICE");
  EXPECT_EQ(device.initializationError(), "");
  EXPECT_EQ(device.read(out, 0), EXIT_SUCCESS);
}

// NOLINTEND(modernize-avoid-c-arrays)
