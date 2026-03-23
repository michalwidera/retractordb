#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "rdb/faccmemory.h"
#include "rdb/fagrp.h"

typedef unsigned char BYTE;

// ctest -R '^ut-test_fileAccessor' -V

// Source under test:
// src/rdb/lib/faccmemory.cc
// src/rdb/lib/fagrp.cc

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

// --- Test fixture for groupFileAccessor tests ---
// Creates a clean sandbox directory before each test and removes it after

class GroupFileAccessorTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_fileAccessor";
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

  // Collect all files in sandbox with their sizes and contents
  std::map<std::string, fileInfo> collectFiles() {
    std::map<std::string, fileInfo> mapOfFiles;
    for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder)) {
      mapOfFiles[entry.path().string()] = fileInfo(filesize(entry.path().string()), readFile(entry.path().string()));
    }
    return mapOfFiles;
  }

  std::string sandboxPath(const std::string &name) { return (sandBoxFolder / name).string(); }
};

// ============================================================
// memoryFileAccessor tests
// ============================================================

// Verify write and sequential read of 4 records without retention limit
TEST(MemoryAccessorTest, test_faccmemory_infinite) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFileAccessor::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(mfa->count(), 4);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 1);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 2);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 4);

  mfa->write(nullptr);               // Clear the storage
  GTEST_ASSERT_EQ(mfa->count(), 0);  // After clearing, count should be 0
}

// Verify retention evicts oldest records and reading evicted positions returns failure
TEST(MemoryAccessorTest, test_faccmemory_retention) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 2);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(mfa->count(), 4);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 4);

  // Evicted record should fail to read
  GTEST_ASSERT_EQ(mfa->read(&record.data, 0), EXIT_FAILURE);

  mfa->write(nullptr);               // Clear the storage
  GTEST_ASSERT_EQ(mfa->count(), 0);  // After clearing, count should be 0
}

// Verify write and read of multi-byte records (4-byte integers) without retention
TEST(MemoryAccessorTest, test_faccmemory_multibyte_record) {
  int record;

  std::string filename = "test_file_memory_multi";

  auto recsize   = sizeof(int);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFileAccessor::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  record = 1000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 2000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 3000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(mfa->count(), 3);

  // Read back using byte positions (position = index * recordSize)
  mfa->read(reinterpret_cast<uint8_t *>(&record), 0 * sizeof(int));
  GTEST_ASSERT_EQ(record, 1000);

  mfa->read(reinterpret_cast<uint8_t *>(&record), 1 * sizeof(int));
  GTEST_ASSERT_EQ(record, 2000);

  mfa->read(reinterpret_cast<uint8_t *>(&record), 2 * sizeof(int));
  GTEST_ASSERT_EQ(record, 3000);

  mfa->write(nullptr);
  GTEST_ASSERT_EQ(mfa->count(), 0);
}

// Verify update-in-place overwrites existing record at given position
TEST(MemoryAccessorTest, test_faccmemory_update_in_place) {
  BYTE record;

  std::string filename = "test_file_memory_update";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFileAccessor::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  record = 10;
  mfa->write(&record);
  record = 20;
  mfa->write(&record);
  record = 30;
  mfa->write(&record);

  GTEST_ASSERT_EQ(mfa->count(), 3);

  // Update record at position 1 (byte offset = 1 * recsize = 1)
  record = 99;
  GTEST_ASSERT_EQ(mfa->write(&record, 1), EXIT_SUCCESS);

  // Count should remain the same after update
  GTEST_ASSERT_EQ(mfa->count(), 3);

  // Verify updated record
  mfa->read(&record, 1);
  GTEST_ASSERT_EQ(record, 99);

  // Verify other records are untouched
  mfa->read(&record, 0);
  GTEST_ASSERT_EQ(record, 10);

  mfa->read(&record, 2);
  GTEST_ASSERT_EQ(record, 30);

  mfa->write(nullptr);
}

// Verify name() returns the filename passed at construction
TEST(MemoryAccessorTest, test_faccmemory_name) {
  std::string filename = "test_file_memory_name";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFileAccessor::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  EXPECT_EQ(mfa->name(), "test_file_memory_name");

  mfa->write(nullptr);
}

// Verify count is 0 for freshly created memory accessor
TEST(MemoryAccessorTest, test_faccmemory_empty_count) {
  std::string filename = "test_file_memory_empty";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFileAccessor::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  GTEST_ASSERT_EQ(mfa->count(), 0);

  mfa->write(nullptr);
}

// Verify retention boundary: eviction triggers when internal size exceeds retentionSize.
// The check is `size > N` (checked before push), so first eviction happens on write N+2.
TEST(MemoryAccessorTest, test_faccmemory_retention_boundary) {
  BYTE record;

  std::string filename = "test_file_memory_boundary";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 3);
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  // Write 4 records - no eviction (size checked before push: 0,1,2,3 are all <= 3)
  for (BYTE i = 1; i <= 4; i++) {
    record = i;
    mfa->write(&record);
  }

  GTEST_ASSERT_EQ(mfa->count(), 4);

  // All 4 records should still be readable (no eviction)
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 1);

  // Write 5th record - size is 4 > 3, triggers first eviction
  record = 5;
  mfa->write(&record);

  GTEST_ASSERT_EQ(mfa->count(), 5);

  // First record evicted (removed_count_ = 1)
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_FAILURE);

  // Second record now readable (location 1 >= removed_count 1)
  GTEST_ASSERT_EQ(mfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 2);

  GTEST_ASSERT_EQ(mfa->read(&record, 4), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 5);

  mfa->write(nullptr);
}

// ============================================================
// groupFileAccessor tests
// ============================================================

// Verify no-retention mode writes all records into a single file
TEST_F(GroupFileAccessorTest, test_fagrp_no_retention) {
  BYTE record;

  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  record = 11;
  gfa->write(&record);
  record = 12;
  gfa->write(&record);

  auto mapOfFiles = collectFiles();

  GTEST_ASSERT_EQ(mapOfFiles.size(), 1);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file")].sizeFromSystem, 2);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file")].fileContents, std::vector<BYTE>({11, 12}));
  GTEST_ASSERT_EQ(gfa->count(), 2);
}

// Verify retention mode splits records across segment files with correct contents
TEST_F(GroupFileAccessorTest, test_fagrp_segmented_write_and_read) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  // Write 6 records across 2 segments of capacity 3
  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  auto mapOfFiles = collectFiles();

  // Verify segment file structure
  GTEST_ASSERT_EQ(mapOfFiles.size(), 2);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_0")].sizeFromSystem, 3);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_1")].sizeFromSystem, 3);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_0")].fileContents, std::vector<BYTE>({1, 2, 3}));
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_1")].fileContents, std::vector<BYTE>({4, 5, 6}));
  GTEST_ASSERT_EQ(gfa->count(), 6);

  // Verify all records can be read back by global index
  for (BYTE i = 0; i < 6; i++) {
    gfa->read(&record, i);
    GTEST_ASSERT_EQ(record, i + 1);
  }
}

// Verify segment rotation evicts oldest segment when segment count exceeds limit.
// Rotation triggers when writeCount_ > capacity. The record that triggers rotation
// is written to the new segment (not the old one).
TEST_F(GroupFileAccessorTest, test_fagrp_segment_rotation) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 2;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  // Write 6 records with capacity=2:
  // segment_0: [1,2]   (rotation triggered by record 3, which goes to segment_1)
  // segment_1: [3,4,5] (rotation triggered by record 6, which goes to segment_2; segment_0 evicted)
  // segment_2: [6]
  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  auto mapOfFiles = collectFiles();

  // segment_0 should be evicted (vec_.size() exceeded silos_count)
  GTEST_ASSERT_EQ(mapOfFiles.count(sandboxPath("test_file_segment_0")), 0);

  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_1")].fileContents, std::vector<BYTE>({3, 4, 5}));
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_2")].fileContents, std::vector<BYTE>({6}));
}

// Verify purge clears all segments and resets state
TEST_F(GroupFileAccessorTest, test_fagrp_purge) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  GTEST_ASSERT_EQ(gfa->count(), 6);

  // Purge all segments (write nullptr at position 0)
  gfa->write(nullptr, 0);
  GTEST_ASSERT_EQ(gfa->count(), 0);

  // Should be able to write new records after purge
  record = 42;
  gfa->write(&record);
  GTEST_ASSERT_EQ(gfa->count(), 1);

  gfa->read(&record, 0);
  GTEST_ASSERT_EQ(record, 42);
}

// Verify name() returns base filename in no-retention mode
TEST_F(GroupFileAccessorTest, test_fagrp_name_no_retention) {
  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  EXPECT_EQ(gfa->name(), "test_file");
}

// Verify name() returns segment filename in retention mode
TEST_F(GroupFileAccessorTest, test_fagrp_name_retention) {
  auto retention = rdb::retention_t{2, 3};
  auto gfa       = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  EXPECT_EQ(gfa->name(), "test_file_segment_0");
}

// Verify count is 0 for freshly created group accessor
TEST_F(GroupFileAccessorTest, test_fagrp_empty_count) {
  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  GTEST_ASSERT_EQ(gfa->count(), 0);
}

// Verify update-in-place within a segment overwrites the correct record
TEST_F(GroupFileAccessorTest, test_fagrp_update_in_place) {
  BYTE record;

  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  record = 10;
  gfa->write(&record);
  record = 20;
  gfa->write(&record);
  record = 30;
  gfa->write(&record);

  GTEST_ASSERT_EQ(gfa->count(), 3);

  // Update record at byte position 1
  record = 99;
  gfa->write(&record, 1);

  // Count should remain unchanged
  GTEST_ASSERT_EQ(gfa->count(), 3);

  // Verify updated record
  gfa->read(&record, 1);
  GTEST_ASSERT_EQ(record, 99);

  // Verify other records are untouched
  gfa->read(&record, 0);
  GTEST_ASSERT_EQ(record, 10);

  gfa->read(&record, 2);
  GTEST_ASSERT_EQ(record, 30);
}

// Verify count compensates for removed segments after rotation.
// With capacity=2, rotation triggers at writeCount > 2, so each segment holds 3 records.
TEST_F(GroupFileAccessorTest, test_fagrp_count_after_rotation) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention, -1);

  // Write 8 records with capacity=3 (rotation at writeCount > 3):
  // segment_0: [1,2,3,4] (4 records)
  // segment_1: [5,6,7,8] (4 records) -> segment_0 evicted (silos_count=2)
  for (BYTE i = 1; i <= 8; i++) {
    record = i;
    gfa->write(&record);
  }

  // count() = remaining segment counts + removedSegments * capacity
  GTEST_ASSERT_EQ(gfa->count(), 8);
}

// Verify retention_t::noRetention() returns true when both segments and capacity are 0
TEST(RetentionTest, test_no_retention) {
  rdb::retention_t ret{0, 0};
  EXPECT_TRUE(ret.noRetention());
}

// Verify retention_t::noRetention() returns false when segments or capacity are non-zero
TEST(RetentionTest, test_has_retention) {
  rdb::retention_t ret{2, 3};
  EXPECT_FALSE(ret.noRetention());
}

// Verify retention_t assignment from std::pair
TEST(RetentionTest, test_pair_assignment) {
  rdb::retention_t ret{0, 0};
  ret = std::pair<rdb::segments_t, rdb::capacity_t>(5, 10);
  EXPECT_EQ(ret.segments, 5);
  EXPECT_EQ(ret.capacity, 10);
  EXPECT_FALSE(ret.noRetention());
}
