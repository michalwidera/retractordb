#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "rdb/faccposix.h"

typedef unsigned char BYTE;

// ctest -R '^ut-test_faccposix' -V

// Source under test:
// src/rdb/lib/faccposix.cc

// --- Helpers ---

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

std::vector<BYTE> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  return std::vector<BYTE>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

struct fileInfo {
  int sizeFromSystem;
  std::vector<BYTE> fileContents;
};

// --- Test fixture ---
// Creates a clean sandbox directory before each test and removes it after

class PosixFileAccessorTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccposix";
  const std::string filename                = "test_file";
  const size_t recsize                      = sizeof(BYTE);

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
    std::filesystem::permissions(              //
        sandBoxFolder,                         //
        std::filesystem::perms::others_all,    //
        std::filesystem::perm_options::remove  //
    );
    std::filesystem::current_path(sandBoxFolder);
  }

  void TearDown() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
  }

  std::string sandboxPath(const std::string &name) { return (sandBoxFolder / name).string(); }
};

// ============================================================
// posixBinaryFileAccessor tests
// ============================================================

// Verify basic write (append) and read of records
TEST_F(PosixFileAccessorTest, test_faccposix_write_and_read) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_test"), recsize);

  record = 0xAA;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  record = 0xBB;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  record = 0xCC;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(pfa->count(), 3);

  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xAA);

  GTEST_ASSERT_EQ(pfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xBB);

  GTEST_ASSERT_EQ(pfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xCC);
}

// Verify read beyond EOF returns EXIT_FAILURE (partial read handling)
TEST_F(PosixFileAccessorTest, test_faccposix_read_beyond_eof) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_eof"), recsize);

  record = 0x11;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(pfa->count(), 1);

  // Reading at position beyond file size should fail
  GTEST_ASSERT_EQ(pfa->read(&record, 99), EXIT_FAILURE);
}

// Verify read from empty file returns EXIT_FAILURE
TEST_F(PosixFileAccessorTest, test_faccposix_read_empty_file) {
  BYTE record = 0xFF;

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_empty"), recsize);

  GTEST_ASSERT_EQ(pfa->count(), 0);
  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_FAILURE);
}

// Verify truncate (write nullptr at position 0) clears the file
TEST_F(PosixFileAccessorTest, test_faccposix_truncate) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_trunc"), recsize);

  record = 0x01;
  pfa->write(&record);
  record = 0x02;
  pfa->write(&record);
  GTEST_ASSERT_EQ(pfa->count(), 2);

  // Truncate
  pfa->write(nullptr, 0);
  GTEST_ASSERT_EQ(pfa->count(), 0);

  // Read after truncate should fail
  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_FAILURE);
}

// Verify update-in-place overwrites record at given byte position
TEST_F(PosixFileAccessorTest, test_faccposix_update_in_place) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_update"), recsize);

  record = 10;
  pfa->write(&record);
  record = 20;
  pfa->write(&record);
  record = 30;
  pfa->write(&record);

  GTEST_ASSERT_EQ(pfa->count(), 3);

  // Update record at byte position 1
  record = 99;
  GTEST_ASSERT_EQ(pfa->write(&record, 1), EXIT_SUCCESS);

  // Count should remain unchanged
  GTEST_ASSERT_EQ(pfa->count(), 3);

  // Verify updated record
  GTEST_ASSERT_EQ(pfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 99);

  // Verify other records are untouched
  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 10);

  GTEST_ASSERT_EQ(pfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 30);
}

// Verify name() returns the path passed at construction
TEST_F(PosixFileAccessorTest, test_faccposix_name) {
  auto path = sandboxPath("posix_name");
  auto pfa  = std::make_unique<rdb::posixBinaryFileAccessor>(path, recsize);

  EXPECT_EQ(pfa->name(), path);
}

// Verify multi-byte record write and read
TEST_F(PosixFileAccessorTest, test_faccposix_multibyte_record) {
  int record;
  auto recsize_int = sizeof(int);

  auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(sandboxPath("posix_multi"), recsize_int);

  record = 1000;
  GTEST_ASSERT_EQ(pfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 2000;
  GTEST_ASSERT_EQ(pfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(pfa->count(), 2);

  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), 0 * recsize_int), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 1000);

  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), 1 * recsize_int), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 2000);

  // Partial read: position aligned to half a record from end
  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), recsize_int + recsize_int / 2), EXIT_FAILURE);
}

// Verify data persists on disk after destructor (durability via reopen)
TEST_F(PosixFileAccessorTest, test_faccposix_persistence) {
  BYTE record;
  auto path = sandboxPath("posix_persist");

  {
    auto pfa = std::make_unique<rdb::posixBinaryFileAccessor>(path, recsize);
    record   = 0x42;
    pfa->write(&record);
    record = 0x43;
    pfa->write(&record);
  }  // destructor closes file

  // Reopen and verify data persists
  auto pfa2 = std::make_unique<rdb::posixBinaryFileAccessor>(path, recsize);
  GTEST_ASSERT_EQ(pfa2->count(), 2);

  GTEST_ASSERT_EQ(pfa2->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x42);

  GTEST_ASSERT_EQ(pfa2->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x43);
}
