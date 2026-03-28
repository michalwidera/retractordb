#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "rdb/faccposixshd.h"

typedef unsigned char BYTE;

// ctest -R '^ut-test_faccposixshd' -V

// Source under test:
// src/rdb/lib/faccposixshd.cc

// --- Helpers ---

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

std::vector<BYTE> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  return std::vector<BYTE>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// --- Test fixture ---
// Creates a clean sandbox directory before each test and removes it after

class ShadowFileTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccposixshd";
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
// posixBinaryFileWithShadow tests
// ============================================================

// Verify append writes go to main file, not shadow
TEST_F(ShadowFileTest, test_faccposixshd_append_to_main) {
  BYTE record;
  auto path = sandboxPath("shd_main");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0xAA;
  GTEST_ASSERT_EQ(shd->write(&record), EXIT_SUCCESS);
  record = 0xBB;
  GTEST_ASSERT_EQ(shd->write(&record), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(shd->count(), 2);

  // Shadow file should be empty (no updates, only appends)
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), 0);

  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xAA);

  GTEST_ASSERT_EQ(shd->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xBB);
}

// Verify update writes go to shadow file, main file unchanged
TEST_F(ShadowFileTest, test_faccposixshd_update_to_shadow) {
  BYTE record;
  auto path = sandboxPath("shd_update");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  // Append 3 records
  record = 10;
  shd->write(&record);
  record = 20;
  shd->write(&record);
  record = 30;
  shd->write(&record);

  // Update record at position 1 → goes to shadow
  record = 99;
  GTEST_ASSERT_EQ(shd->write(&record, 1), EXIT_SUCCESS);

  // Main file should still have original data (10, 20, 30)
  auto mainContents = readFile(path);
  GTEST_ASSERT_EQ(mainContents, std::vector<BYTE>({10, 20, 30}));

  // Shadow file should have one entry: (position=1, data=99)
  EXPECT_GT(filesize(path + ".shadow"), 0);

  // Read should return shadow value
  GTEST_ASSERT_EQ(shd->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 99);

  // Other records should be read from main file
  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 10);

  GTEST_ASSERT_EQ(shd->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 30);
}

// Verify multiple updates to same position - last write wins
TEST_F(ShadowFileTest, test_faccposixshd_multiple_updates_same_pos) {
  BYTE record;
  auto path = sandboxPath("shd_multi");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 10;
  shd->write(&record);
  record = 20;
  shd->write(&record);

  // Update position 0 multiple times
  record = 50;
  shd->write(&record, 0);
  record = 60;
  shd->write(&record, 0);
  record = 70;
  shd->write(&record, 0);

  // Last update should win
  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 70);

  // Main file untouched
  auto mainContents = readFile(path);
  GTEST_ASSERT_EQ(mainContents[0], 10);
}

// Verify merge applies shadow changes to main file and clears shadow
TEST_F(ShadowFileTest, test_faccposixshd_merge) {
  BYTE record;
  auto path = sandboxPath("shd_merge");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 10;
  shd->write(&record);
  record = 20;
  shd->write(&record);
  record = 30;
  shd->write(&record);

  // Update two records
  record = 99;
  shd->write(&record, 0);
  record = 88;
  shd->write(&record, 2);

  // Merge shadow into main
  GTEST_ASSERT_EQ(shd->merge(), EXIT_SUCCESS);

  // Shadow should be empty after merge
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), 0);

  // Main file should now have merged data
  auto mainContents = readFile(path);
  GTEST_ASSERT_EQ(mainContents, std::vector<BYTE>({99, 20, 88}));

  // Read should still work correctly
  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 99);

  GTEST_ASSERT_EQ(shd->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 20);

  GTEST_ASSERT_EQ(shd->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 88);
}

// Verify truncate clears both main and shadow files
TEST_F(ShadowFileTest, test_faccposixshd_truncate) {
  BYTE record;
  auto path = sandboxPath("shd_trunc");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 10;
  shd->write(&record);
  record = 99;
  shd->write(&record, 0);  // update to shadow

  // Truncate
  shd->write(nullptr, 0);

  GTEST_ASSERT_EQ(shd->count(), 0);
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), 0);
}

// Verify count returns only main file record count (shadow doesn't affect count)
TEST_F(ShadowFileTest, test_faccposixshd_count_ignores_shadow) {
  BYTE record;
  auto path = sandboxPath("shd_count");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 1;
  shd->write(&record);
  record = 2;
  shd->write(&record);

  GTEST_ASSERT_EQ(shd->count(), 2);

  // Update should not change count
  record = 99;
  shd->write(&record, 0);

  GTEST_ASSERT_EQ(shd->count(), 2);
}

// Verify reading from an empty file returns failure
TEST_F(ShadowFileTest, test_faccposixshd_read_empty_file) {
  BYTE record = 0xFF;
  auto path   = sandboxPath("shd_empty");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  GTEST_ASSERT_EQ(shd->count(), 0);
  GTEST_ASSERT_NE(shd->read(&record, 0), EXIT_SUCCESS);
}

// Verify reading beyond EOF returns failure
TEST_F(ShadowFileTest, test_faccposixshd_read_beyond_eof) {
  BYTE record;
  auto path = sandboxPath("shd_beyond");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0xAA;
  shd->write(&record);

  GTEST_ASSERT_EQ(shd->count(), 1);
  GTEST_ASSERT_NE(shd->read(&record, 5), EXIT_SUCCESS);
}

// Verify shadow entry binary format: (size_t position, data[recordSize])
TEST_F(ShadowFileTest, test_faccposixshd_shadow_entry_format) {
  BYTE record;
  auto path = sandboxPath("shd_fmt");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0x10;
  shd->write(&record);
  record = 0x20;
  shd->write(&record);

  // Update position 1
  record = 0xFF;
  shd->write(&record, 1);

  // Shadow file should contain exactly one entry: sizeof(size_t) + recsize bytes
  auto expectedSize = sizeof(size_t) + recsize;
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), expectedSize);

  // Read raw shadow file and verify content
  auto raw = readFile(path + ".shadow");
  size_t storedPos;
  std::memcpy(&storedPos, raw.data(), sizeof(size_t));
  GTEST_ASSERT_EQ(storedPos, 1);
  GTEST_ASSERT_EQ(raw[sizeof(size_t)], 0xFF);
}

// Verify multi-byte record shadow operations
TEST_F(ShadowFileTest, test_faccposixshd_multibyte_record) {
  const size_t multiRecSize = 4;
  auto path                 = sandboxPath("shd_multi_rec");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, multiRecSize);

  uint8_t rec1[4] = {0x01, 0x02, 0x03, 0x04};
  uint8_t rec2[4] = {0x0A, 0x0B, 0x0C, 0x0D};
  shd->write(rec1);
  shd->write(rec2);

  // Update position 0 (byte offset) with new data
  uint8_t upd[4] = {0xF1, 0xF2, 0xF3, 0xF4};
  shd->write(upd, 0);

  uint8_t buf[4] = {};
  GTEST_ASSERT_EQ(shd->read(buf, 0), EXIT_SUCCESS);
  EXPECT_EQ(buf[0], 0xF1);
  EXPECT_EQ(buf[1], 0xF2);
  EXPECT_EQ(buf[2], 0xF3);
  EXPECT_EQ(buf[3], 0xF4);

  // Position 1*multiRecSize (byte offset for second record) — unchanged
  GTEST_ASSERT_EQ(shd->read(buf, 1 * multiRecSize), EXIT_SUCCESS);
  EXPECT_EQ(buf[0], 0x0A);
  EXPECT_EQ(buf[1], 0x0B);
  EXPECT_EQ(buf[2], 0x0C);
  EXPECT_EQ(buf[3], 0x0D);

  // Main file still has original data
  auto mainContents = readFile(path);
  EXPECT_EQ(mainContents, std::vector<BYTE>({0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x0C, 0x0D}));
}

// Verify data persists after close and reopen
TEST_F(ShadowFileTest, test_faccposixshd_persistence) {
  BYTE record;
  auto path = sandboxPath("shd_persist");

  {
    auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);
    record   = 0x10;
    shd->write(&record);
    record = 0x20;
    shd->write(&record);
    // Update position 0
    record = 0x99;
    shd->write(&record, 0);
  }  // close files

  // Reopen and verify shadow state persists
  auto shd2 = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  GTEST_ASSERT_EQ(shd2->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x99);  // from shadow

  GTEST_ASSERT_EQ(shd2->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x20);  // from main
}

// Verify merge with multiple updates to the same position applies all (last wins in main)
TEST_F(ShadowFileTest, test_faccposixshd_merge_multiple_updates_same_pos) {
  BYTE record;
  auto path = sandboxPath("shd_merge_multi");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0xAA;
  shd->write(&record);

  // Multiple updates to position 0
  record = 0x11;
  shd->write(&record, 0);
  record = 0x22;
  shd->write(&record, 0);
  record = 0x33;
  shd->write(&record, 0);

  GTEST_ASSERT_EQ(shd->merge(), EXIT_SUCCESS);

  // After merge, main file should have last value
  auto mainContents = readFile(path);
  GTEST_ASSERT_EQ(mainContents[0], 0x33);

  // Shadow should be empty
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), 0);

  // Read still works
  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x33);
}

// Verify write/read works correctly after merge
TEST_F(ShadowFileTest, test_faccposixshd_write_after_merge) {
  BYTE record;
  auto path = sandboxPath("shd_postmerge");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0x10;
  shd->write(&record);
  record = 0x20;
  shd->write(&record);

  record = 0xFF;
  shd->write(&record, 0);

  GTEST_ASSERT_EQ(shd->merge(), EXIT_SUCCESS);

  // Continue using the accessor after merge
  record = 0x30;
  shd->write(&record);  // append
  GTEST_ASSERT_EQ(shd->count(), 3);

  record = 0xEE;
  shd->write(&record, 1);  // new shadow update

  GTEST_ASSERT_EQ(shd->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xEE);

  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xFF);  // from merged main

  GTEST_ASSERT_EQ(shd->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x30);  // appended after merge
}

// Verify name() returns the correct filename
TEST_F(ShadowFileTest, test_faccposixshd_name) {
  auto path = sandboxPath("shd_name");
  auto shd  = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  GTEST_ASSERT_EQ(shd->name(), path);
}

// Verify shadow file stays empty when only appends are performed
TEST_F(ShadowFileTest, test_faccposixshd_shadow_not_created_on_append_only) {
  BYTE record;
  auto path = sandboxPath("shd_appendonly");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  for (BYTE i = 0; i < 10; ++i) {
    record = i;
    shd->write(&record);
  }

  GTEST_ASSERT_EQ(shd->count(), 10);
  GTEST_ASSERT_EQ(filesize(path + ".shadow"), 0);
}

// Verify merge on empty shadow is a no-op and succeeds
TEST_F(ShadowFileTest, test_faccposixshd_merge_empty_shadow) {
  BYTE record;
  auto path = sandboxPath("shd_merge_empty");

  auto shd = std::make_unique<rdb::posixBinaryFileWithShadow>(path, recsize);

  record = 0xAA;
  shd->write(&record);
  record = 0xBB;
  shd->write(&record);

  // Merge with no shadow entries should succeed
  GTEST_ASSERT_EQ(shd->merge(), EXIT_SUCCESS);

  // Data unchanged
  GTEST_ASSERT_EQ(shd->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xAA);
  GTEST_ASSERT_EQ(shd->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xBB);
}
