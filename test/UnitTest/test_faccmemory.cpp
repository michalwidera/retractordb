#include <gtest/gtest.h>

#include <string>

#include "rdb/descriptor.hpp"
#include "rdb/faccmemory.hpp"

static rdb::Descriptor makeDesc(size_t size) { return {"f", static_cast<int>(size), 1, rdb::BYTE}; }

using BYTE = unsigned char;

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

// Verify retention ring buffer: after 4 writes with size 2, ring holds last 2 records.
// Reads at any position succeed via modular mapping; no EXIT_FAILURE for old positions.
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

  // Positions 2 and 3 map to ring slots 0 and 1 — current values 3 and 4
  GTEST_ASSERT_EQ(mfa->read(&record.data, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 4);

  // Position 0 maps to ring slot 0 — returns the record currently at that slot (value 3)
  GTEST_ASSERT_EQ(mfa->read(&record.data, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

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

// Verify retention ring boundary: ring of size 3 overwrites slot 0 on the 4th write.
// All reads succeed via modular mapping — no EXIT_FAILURE for wrapped positions.
TEST(MemoryTest, test_faccmemory_retention_boundary) {
  BYTE record;

  std::string filename = "test_file_memory_boundary";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 3);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  // Write 3 records — ring fills up, no overwrite yet
  for (BYTE i = 1; i <= 3; i++) {
    record = i;
    mfa->write(&record);
  }

  GTEST_ASSERT_EQ(mfa->count(), 3);

  // Ring slots 0,1,2 hold values 1,2,3
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 1);
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 3);

  // Write 4th record — wraps to slot 0, overwrites value 1 with value 4
  record = 4;
  mfa->write(&record);

  GTEST_ASSERT_EQ(mfa->count(), 4);

  // Position 0 maps to slot 0 — now holds value 4 (overwritten)
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 4);

  // Position 3 also maps to slot 0 — same value
  GTEST_ASSERT_EQ(mfa->read(&record, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 4);

  // Write 5th record — wraps to slot 1, overwrites value 2 with value 5
  record = 5;
  mfa->write(&record);

  GTEST_ASSERT_EQ(mfa->count(), 5);

  GTEST_ASSERT_EQ(mfa->read(&record, 4), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 5);

  // Slot 2 still holds value 3 (not yet overwritten)
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 3);

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
