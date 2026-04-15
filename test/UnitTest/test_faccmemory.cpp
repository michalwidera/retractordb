#include <gtest/gtest.h>

#include <string>

#include "rdb/descriptor.hpp"
#include "rdb/faccmemory.hpp"

static rdb::Descriptor makeDesc(size_t size) { return rdb::Descriptor("f", size, 1, rdb::BYTE); }

typedef unsigned char BYTE;

// ctest -R '^ut-test_faccmemory' -V

// Source under test:
// src/rdb/lib/faccmemory.cc

// ============================================================
// memoryFile tests
// ============================================================

// Verify write and sequential read of 4 records without retention limit
TEST(MemoryTest, test_faccmemory_infinite) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

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
TEST(MemoryTest, test_faccmemory_retention) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 2);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

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
TEST(MemoryTest, test_faccmemory_multibyte_record) {
  int record;

  std::string filename = "test_file_memory_multi";

  auto recsize   = sizeof(int);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

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
TEST(MemoryTest, test_faccmemory_update_in_place) {
  BYTE record;

  std::string filename = "test_file_memory_update";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

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
TEST(MemoryTest, test_faccmemory_name) {
  std::string filename = "test_file_memory_name";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  EXPECT_EQ(mfa->name(), "test_file_memory_name");

  mfa->write(nullptr);
}

// Verify count is 0 for freshly created memory accessor
TEST(MemoryTest, test_faccmemory_empty_count) {
  std::string filename = "test_file_memory_empty";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  GTEST_ASSERT_EQ(mfa->count(), 0);

  mfa->write(nullptr);
}

// Verify retention boundary: eviction triggers when internal size exceeds retentionSize.
// The check is `size > N` (checked before push), so first eviction happens on write N+2.
TEST(MemoryTest, test_faccmemory_retention_boundary) {
  BYTE record;

  std::string filename = "test_file_memory_boundary";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 3);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

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

// Verify read beyond storage bounds returns EXIT_FAILURE
TEST(MemoryTest, test_faccmemory_read_beyond_bounds) {
  BYTE record;

  std::string filename = "test_file_memory_bounds";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record = 10;
  mfa->write(&record);
  record = 20;
  mfa->write(&record);
  record = 30;
  mfa->write(&record);

  GTEST_ASSERT_EQ(mfa->count(), 3);

  // Valid reads
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);

  // Beyond upper bound should fail
  GTEST_ASSERT_EQ(mfa->read(&record, 3), EXIT_FAILURE);
  GTEST_ASSERT_EQ(mfa->read(&record, 99), EXIT_FAILURE);

  // Empty storage should fail
  mfa->write(nullptr);
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_FAILURE);
}

// Verify data persists in static storage across instances with the same filename
TEST(MemoryTest, test_faccmemory_persistence_across_instances) {
  BYTE record;

  std::string filename = "test_file_memory_persist";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  {
    auto mfa = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
    record   = 0x42;
    mfa->write(&record);
    record = 0x43;
    mfa->write(&record);
    GTEST_ASSERT_EQ(mfa->count(), 2);
  }  // mfa destroyed

  // New instance with same filename should see persisted data
  auto mfa2 = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
  GTEST_ASSERT_EQ(mfa2->count(), 2);

  GTEST_ASSERT_EQ(mfa2->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x42);

  GTEST_ASSERT_EQ(mfa2->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x43);

  mfa2->write(nullptr);
}
