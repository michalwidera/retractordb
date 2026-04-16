#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/faccfs.hpp"

// ctest -R '^ut-test_faccfs' -V

typedef unsigned char BYTE;

static rdb::Descriptor makeDesc(size_t size) {
  return rdb::Descriptor("f", static_cast<int>(size), 1, rdb::BYTE);
}

// --- Test fixture ---

class FaccfsTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccfs";
  const std::string filename                = "test_file";
  const size_t AREA_SIZE                    = 10;

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
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
// Basic write and read
// ============================================================

TEST_F(FaccfsTest, write_and_read_single_record) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t wData[10];
  std::memcpy(wData, "test data!", AREA_SIZE);
  ASSERT_EQ(gf.write(wData), EXIT_SUCCESS);

  uint8_t rData[10] = {};
  ASSERT_EQ(gf.read(rData, 0), EXIT_SUCCESS);
  EXPECT_EQ(std::memcmp(rData, "test data!", AREA_SIZE), 0);
}

// ============================================================
// count() — previously 0% coverage
// ============================================================

TEST_F(FaccfsTest, count_returns_zero_for_new_file) {
  auto desc = makeDesc(AREA_SIZE);
  // Create an empty file so count() can open it
  std::ofstream(sandboxPath(filename), std::ios::binary).close();

  rdb::genericBinaryFile gf(sandboxPath(filename), desc);
  EXPECT_EQ(gf.count(), 0u);
}

TEST_F(FaccfsTest, count_returns_correct_number_of_records) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "record aaa", AREA_SIZE);
  ASSERT_EQ(gf.write(data), EXIT_SUCCESS);

  std::memcpy(data, "record bbb", AREA_SIZE);
  ASSERT_EQ(gf.write(data), EXIT_SUCCESS);

  std::memcpy(data, "record ccc", AREA_SIZE);
  ASSERT_EQ(gf.write(data), EXIT_SUCCESS);

  EXPECT_EQ(gf.count(), 3u);
}

TEST_F(FaccfsTest, count_after_update_does_not_change) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "record aaa", AREA_SIZE);
  gf.write(data);
  std::memcpy(data, "record bbb", AREA_SIZE);
  gf.write(data);

  EXPECT_EQ(gf.count(), 2u);

  // Update record 0 (in-place) should not change count
  std::memcpy(data, "record xxx", AREA_SIZE);
  gf.write(data, 0);

  EXPECT_EQ(gf.count(), 2u);
}

// ============================================================
// name()
// ============================================================

TEST_F(FaccfsTest, name_returns_filename) {
  auto desc = makeDesc(AREA_SIZE);
  auto path = sandboxPath(filename);
  rdb::genericBinaryFile gf(path, desc);

  EXPECT_EQ(gf.name(), path);
}

// ============================================================
// Append multiple records and read them back
// ============================================================

TEST_F(FaccfsTest, append_multiple_and_read_back) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  const char *records[] = {"aaaaaaaaaa", "bbbbbbbbbb", "cccccccccc", "dddddddddd"};
  uint8_t data[10];

  for (auto &rec : records) {
    std::memcpy(data, rec, AREA_SIZE);
    ASSERT_EQ(gf.write(data), EXIT_SUCCESS);
  }

  EXPECT_EQ(gf.count(), 4u);

  for (size_t i = 0; i < 4; ++i) {
    uint8_t rData[10] = {};
    ASSERT_EQ(gf.read(rData, i * AREA_SIZE), EXIT_SUCCESS);
    EXPECT_EQ(std::memcmp(rData, records[i], AREA_SIZE), 0);
  }
}

// ============================================================
// Update in-place
// ============================================================

TEST_F(FaccfsTest, update_in_place_modifies_correct_record) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "record aaa", AREA_SIZE);
  gf.write(data);
  std::memcpy(data, "record bbb", AREA_SIZE);
  gf.write(data);
  std::memcpy(data, "record ccc", AREA_SIZE);
  gf.write(data);

  // Update middle record (position = AREA_SIZE = record 1)
  std::memcpy(data, "record XXX", AREA_SIZE);
  ASSERT_EQ(gf.write(data, AREA_SIZE), EXIT_SUCCESS);

  // Verify record 0 unchanged
  uint8_t rData[10] = {};
  gf.read(rData, 0);
  EXPECT_EQ(std::memcmp(rData, "record aaa", AREA_SIZE), 0);

  // Verify record 1 updated
  gf.read(rData, AREA_SIZE);
  EXPECT_EQ(std::memcmp(rData, "record XXX", AREA_SIZE), 0);

  // Verify record 2 unchanged
  gf.read(rData, 2 * AREA_SIZE);
  EXPECT_EQ(std::memcmp(rData, "record ccc", AREA_SIZE), 0);
}

// ============================================================
// Destructor with percounter >= 0 — file rotation
// ============================================================

TEST_F(FaccfsTest, destructor_rotates_file_when_percounter_set) {
  auto desc = makeDesc(AREA_SIZE);
  auto path = sandboxPath("rotate_file");

  {
    rdb::genericBinaryFile gf(path, desc, 3);

    uint8_t data[10];
    std::memcpy(data, "rotate dat", AREA_SIZE);
    gf.write(data);

    // File should exist at original path before destruction
    EXPECT_TRUE(std::filesystem::exists(path));
  }
  // After destruction, file should be renamed to .old3
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_TRUE(std::filesystem::exists(path + ".old3"));

  // Verify rotated file contents are intact
  std::ifstream in(path + ".old3", std::ios::binary);
  uint8_t rData[10] = {};
  in.read(reinterpret_cast<char *>(rData), AREA_SIZE);
  EXPECT_EQ(std::memcmp(rData, "rotate dat", AREA_SIZE), 0);
}

TEST_F(FaccfsTest, destructor_rotates_file_percounter_zero) {
  auto desc = makeDesc(AREA_SIZE);
  auto path = sandboxPath("rotate_zero");

  {
    rdb::genericBinaryFile gf(path, desc, 0);

    uint8_t data[10];
    std::memcpy(data, "rotatezero", AREA_SIZE);
    gf.write(data);
  }
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_TRUE(std::filesystem::exists(path + ".old0"));
}

TEST_F(FaccfsTest, destructor_no_rotation_when_percounter_negative) {
  auto desc = makeDesc(AREA_SIZE);
  auto path = sandboxPath("no_rotate");

  {
    rdb::genericBinaryFile gf(path, desc, -1);

    uint8_t data[10];
    std::memcpy(data, "no rotate!", AREA_SIZE);
    gf.write(data);
  }
  // File should still exist at original path — no rotation
  EXPECT_TRUE(std::filesystem::exists(path));
  EXPECT_FALSE(std::filesystem::exists(path + ".old-1"));
}

// ============================================================
// Null-aware interface: nullBitset is cleared on read
// ============================================================

TEST_F(FaccfsTest, read_clears_null_bitset) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "nullbitset", AREA_SIZE);
  std::vector<bool> nullBitset = {true, false, true};
  gf.write(data, nullBitset, std::numeric_limits<size_t>::max());

  // Pre-fill nullBitset with values
  std::vector<bool> readNulls = {true, true, true};
  uint8_t rData[10] = {};
  ASSERT_EQ(gf.read(rData, readNulls, 0), EXIT_SUCCESS);

  // genericBinaryFile clears nullBitset on read (no null tracking)
  EXPECT_TRUE(readNulls.empty());
  EXPECT_EQ(std::memcmp(rData, "nullbitset", AREA_SIZE), 0);
}

// ============================================================
// Write with null-aware interface (explicit nullBitset)
// ============================================================

TEST_F(FaccfsTest, write_with_null_bitset_append) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "null write", AREA_SIZE);
  std::vector<bool> nullBitset = {false, true};

  ASSERT_EQ(gf.write(data, nullBitset, std::numeric_limits<size_t>::max()), EXIT_SUCCESS);

  uint8_t rData[10] = {};
  gf.read(rData, 0);
  EXPECT_EQ(std::memcmp(rData, "null write", AREA_SIZE), 0);
}

TEST_F(FaccfsTest, write_with_null_bitset_update) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "first rec!", AREA_SIZE);
  gf.write(data);

  std::memcpy(data, "updated!ab", AREA_SIZE);
  std::vector<bool> nullBitset = {false};
  ASSERT_EQ(gf.write(data, nullBitset, 0), EXIT_SUCCESS);

  uint8_t rData[10] = {};
  gf.read(rData, 0);
  EXPECT_EQ(std::memcmp(rData, "updated!ab", AREA_SIZE), 0);
}

// ============================================================
// Single byte record size
// ============================================================

TEST_F(FaccfsTest, single_byte_record_size) {
  auto desc = makeDesc(1);
  rdb::genericBinaryFile gf(sandboxPath("single_byte"), desc);

  uint8_t w = 0xAB;
  ASSERT_EQ(gf.write(&w), EXIT_SUCCESS);
  w = 0xCD;
  ASSERT_EQ(gf.write(&w), EXIT_SUCCESS);

  EXPECT_EQ(gf.count(), 2u);

  uint8_t r = 0;
  gf.read(&r, 0);
  EXPECT_EQ(r, 0xAB);
  gf.read(&r, 1);
  EXPECT_EQ(r, 0xCD);
}

// ============================================================
// Large record size
// ============================================================

TEST_F(FaccfsTest, large_record_size) {
  const size_t LARGE = 4096;
  auto desc = makeDesc(LARGE);
  rdb::genericBinaryFile gf(sandboxPath("large_record"), desc);

  std::vector<uint8_t> wData(LARGE, 0);
  std::memset(wData.data(), 0xAA, LARGE);
  ASSERT_EQ(gf.write(wData.data()), EXIT_SUCCESS);

  std::memset(wData.data(), 0xBB, LARGE);
  ASSERT_EQ(gf.write(wData.data()), EXIT_SUCCESS);

  EXPECT_EQ(gf.count(), 2u);

  std::vector<uint8_t> rData(LARGE, 0);
  gf.read(rData.data(), 0);
  EXPECT_EQ(rData[0], 0xAA);
  EXPECT_EQ(rData[LARGE - 1], 0xAA);

  gf.read(rData.data(), LARGE);
  EXPECT_EQ(rData[0], 0xBB);
  EXPECT_EQ(rData[LARGE - 1], 0xBB);
}

// ============================================================
// Destructor rotation does not crash if file does not exist
// ============================================================

TEST_F(FaccfsTest, destructor_rotation_nonexistent_file_no_crash) {
  auto desc = makeDesc(AREA_SIZE);
  auto path = sandboxPath("nonexistent");

  {
    // percounter=5 but no file written — destructor should not crash
    rdb::genericBinaryFile gf(path, desc, 5);
  }
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_FALSE(std::filesystem::exists(path + ".old5"));
}

// ============================================================
// Write append then update first record, verify with count
// ============================================================

TEST_F(FaccfsTest, append_and_update_first_record) {
  auto desc = makeDesc(AREA_SIZE);
  rdb::genericBinaryFile gf(sandboxPath(filename), desc);

  uint8_t data[10];
  std::memcpy(data, "original!a", AREA_SIZE);
  gf.write(data);
  std::memcpy(data, "second rec", AREA_SIZE);
  gf.write(data);

  EXPECT_EQ(gf.count(), 2u);

  // Update first record at position 0
  std::memcpy(data, "MODIFIED!a", AREA_SIZE);
  gf.write(data, 0);

  // Count should remain the same
  EXPECT_EQ(gf.count(), 2u);

  uint8_t rData[10] = {};
  gf.read(rData, 0);
  EXPECT_EQ(std::memcmp(rData, "MODIFIED!a", AREA_SIZE), 0);

  gf.read(rData, AREA_SIZE);
  EXPECT_EQ(std::memcmp(rData, "second rec", AREA_SIZE), 0);
}
