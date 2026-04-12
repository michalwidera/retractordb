#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "rdb/fagrp.hpp"

typedef unsigned char BYTE;

// ctest -R '^ut-test_fagrp' -V

// Source under test:
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

// --- Test fixture ---
// Creates a clean sandbox directory before each test and removes it after

class GroupFileTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_fagrp";
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
      if (entry.path().filename().string().ends_with(".shadow")) {
        continue;
      }
      mapOfFiles[entry.path().string()] = fileInfo(filesize(entry.path().string()), readFile(entry.path().string()));
    }
    return mapOfFiles;
  }

  std::string sandboxPath(const std::string &name) { return (sandBoxFolder / name).string(); }
};

// ============================================================
// groupFile tests
// ============================================================

// Verify no-retention mode writes all records into a single file
TEST_F(GroupFileTest, test_fagrp_no_retention) {
  BYTE record;

  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

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
TEST_F(GroupFileTest, test_fagrp_segmented_write_and_read) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

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
// Rotation triggers when the current segment reaches capacity. The next record
// is written to the new segment.
TEST_F(GroupFileTest, test_fagrp_segment_rotation) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 2;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  // Write 6 records with capacity=2:
  // segment_0: [1,2]
  // segment_1: [3,4]
  // segment_2: [5,6] and segment_0 is evicted (silos_count=2)
  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  auto mapOfFiles = collectFiles();

  // segment_0 should be evicted (vec_.size() exceeded silos_count)
  GTEST_ASSERT_EQ(mapOfFiles.count(sandboxPath("test_file_segment_0")), 0);

  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_1")].fileContents, std::vector<BYTE>({3, 4}));
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_2")].fileContents, std::vector<BYTE>({5, 6}));
}

// Verify rotation removes shadow file for evicted segment as well.
TEST_F(GroupFileTest, test_fagrp_segment_rotation_removes_shadow) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 2;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  // Keep writing so segment_0 gets evicted when segment_2 is created.
  for (BYTE i = 1; i <= 2; i++) {
    record = i;
    gfa->write(&record);
  }

  GTEST_ASSERT_TRUE(std::filesystem::exists(sandboxPath("test_file_segment_0.shadow")));

  for (BYTE i = 3; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  GTEST_ASSERT_FALSE(std::filesystem::exists(sandboxPath("test_file_segment_0")));
  GTEST_ASSERT_FALSE(std::filesystem::exists(sandboxPath("test_file_segment_0.shadow")));
}

// Verify purge clears all segments and resets state
TEST_F(GroupFileTest, test_fagrp_purge) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  GTEST_ASSERT_EQ(gfa->count(), 6);

  // Purge all segments using explicit API
  gfa->purge();
  GTEST_ASSERT_EQ(gfa->count(), 0);

  // Should be able to write new records after purge
  record = 42;
  gfa->write(&record);
  GTEST_ASSERT_EQ(gfa->count(), 1);

  gfa->read(&record, 0);
  GTEST_ASSERT_EQ(record, 42);
}

// Verify purge API works in no-retention mode as well.
TEST_F(GroupFileTest, test_fagrp_purge_no_retention) {
  BYTE record;

  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  record = 11;
  gfa->write(&record);
  record = 12;
  gfa->write(&record);
  GTEST_ASSERT_EQ(gfa->count(), 2);

  gfa->purge();
  GTEST_ASSERT_EQ(gfa->count(), 0);

  auto mapOfFiles = collectFiles();
  GTEST_ASSERT_EQ(mapOfFiles.size(), 1);
  GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file")].sizeFromSystem, 0);
}

// Verify name() returns base filename in no-retention mode
TEST_F(GroupFileTest, test_fagrp_name_no_retention) {
  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  EXPECT_EQ(gfa->name(), "test_file");
}

// Verify name() returns segment filename in retention mode
TEST_F(GroupFileTest, test_fagrp_name_retention) {
  auto retention = rdb::retention_t{2, 3};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  EXPECT_EQ(gfa->name(), "test_file_segment_0");
}

// Verify count is 0 for freshly created group accessor
TEST_F(GroupFileTest, test_fagrp_empty_count) {
  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  GTEST_ASSERT_EQ(gfa->count(), 0);
}

// Verify update-in-place within a segment overwrites the correct record
TEST_F(GroupFileTest, test_fagrp_update_in_place) {
  BYTE record;

  auto retention = rdb::retention_t{0, 0};
  auto gfa       = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

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
TEST_F(GroupFileTest, test_fagrp_count_after_rotation) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  // Write 8 records with capacity=3:
  // segment_0: [1,2,3]
  // segment_1: [4,5,6]
  // segment_2: [7,8] and segment_0 evicted (silos_count=2)
  for (BYTE i = 1; i <= 8; i++) {
    record = i;
    gfa->write(&record);
  }

  // count() = remaining segment counts + removedSegments * capacity
  GTEST_ASSERT_EQ(gfa->count(), 8);
}

// Verify reads and updates into already removed global positions fail safely.
TEST_F(GroupFileTest, test_fagrp_access_removed_segment_fails) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 2;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);

  for (BYTE i = 1; i <= 6; i++) {
    record = i;
    gfa->write(&record);
  }

  record = 99;
  GTEST_ASSERT_NE(gfa->read(&record, 0), 0);
  GTEST_ASSERT_NE(gfa->write(&record, 0), 0);
}

// Verify object state is restored after restart and append continues from restored state.
TEST_F(GroupFileTest, test_fagrp_restore_state_after_restart) {
  BYTE record;

  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};

  {
    auto gfa = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);
    for (BYTE i = 1; i <= 5; i++) {
      record = i;
      GTEST_ASSERT_EQ(gfa->write(&record), 0);
    }
    GTEST_ASSERT_EQ(gfa->count(), 5);
  }

  {
    auto gfa = std::make_unique<rdb::groupFile<>>(filename, recsize, retention, -1);
    GTEST_ASSERT_EQ(gfa->count(), 5);

    // Verify data persisted and is readable by global position after restart.
    for (BYTE i = 0; i < 5; i++) {
      GTEST_ASSERT_EQ(gfa->read(&record, i), 0);
      GTEST_ASSERT_EQ(record, i + 1);
    }

    // Continue appending after restart and verify rotation is based on restored writeCount_.
    record = 6;
    GTEST_ASSERT_EQ(gfa->write(&record), 0);
    record = 7;
    GTEST_ASSERT_EQ(gfa->write(&record), 0);

    auto mapOfFiles = collectFiles();
    GTEST_ASSERT_EQ(mapOfFiles.count(sandboxPath("test_file_segment_0")), 0);
    GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_1")].fileContents, std::vector<BYTE>({4, 5, 6}));
    GTEST_ASSERT_EQ(mapOfFiles[sandboxPath("test_file_segment_2")].fileContents, std::vector<BYTE>({7}));
    GTEST_ASSERT_EQ(gfa->count(), 7);
  }
}

// ============================================================
// retention_t tests
// ============================================================

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
