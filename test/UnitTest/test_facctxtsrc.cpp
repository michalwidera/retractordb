#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "rdb/facctxtsrc.h"

// ctest -R '^ut-test_facctxtsrc' -V

// Source under test:
// src/rdb/lib/facctxtsrc.cc

namespace fs = std::filesystem;

class TextSourceROTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {
    // Clean up any created test files
    for (auto &f : createdFiles_) {
      fs::remove(f);
    }
  }

  std::string createTestFile(const std::string &name, const std::string &content) {
    std::ofstream ofs(name);
    ofs << content;
    ofs.close();
    createdFiles_.push_back(name);
    return name;
  }

  std::vector<std::string> createdFiles_;
};

// ============================================================
// textSourceRO - integer reading tests
// ============================================================

// Verify reading a single integer field from a text source
TEST_F(TextSourceROTest, test_read_single_integer) {
  auto filename = createTestFile("test_int.txt", "42\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int value;
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 42);
}

// Verify reading multiple integer records sequentially
TEST_F(TextSourceROTest, test_read_multiple_integers) {
  auto filename = createTestFile("test_multi_int.txt", "10\n20\n30\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  int value;

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 10);

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 20);

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 30);
}

// ============================================================
// textSourceRO - float/double reading tests
// ============================================================

// Verify reading a float value
TEST_F(TextSourceROTest, test_read_float) {
  auto filename = createTestFile("test_float.txt", "3.14\n");

  rdb::Descriptor desc{{"x", static_cast<int>(sizeof(float)), 1, rdb::FLOAT}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  float value;
  std::memcpy(&value, buffer.get(), sizeof(float));
  EXPECT_NEAR(value, 3.14f, 0.001f);
}

// Verify reading a double value
TEST_F(TextSourceROTest, test_read_double) {
  auto filename = createTestFile("test_double.txt", "2.718281828\n");

  rdb::Descriptor desc{{"x", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  double value;
  std::memcpy(&value, buffer.get(), sizeof(double));
  EXPECT_NEAR(value, 2.718281828, 0.0000001);
}

// ============================================================
// textSourceRO - unsigned integer and byte tests
// ============================================================

// Verify reading unsigned integer
TEST_F(TextSourceROTest, test_read_uint) {
  auto filename = createTestFile("test_uint.txt", "100\n");

  rdb::Descriptor desc{{"u", static_cast<int>(sizeof(unsigned)), 1, rdb::UINT}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  unsigned value;
  std::memcpy(&value, buffer.get(), sizeof(unsigned));
  GTEST_ASSERT_EQ(value, 100u);
}

// Verify reading BYTE type (read as unsigned, stored as uint8_t)
TEST_F(TextSourceROTest, test_read_byte) {
  auto filename = createTestFile("test_byte.txt", "255\n");

  rdb::Descriptor desc{{"b", 1, 1, rdb::BYTE}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(buffer[0], 255);
}

// Verify NULL token maps to null/fallback for scalar numeric fields
TEST_F(TextSourceROTest, test_read_integer_null_token) {
  auto filename = createTestFile("test_int_null.txt", "NULL\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int value = -1;
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 0);

  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

// Verify failed file open is handled by read() as an all-null zero row
TEST_F(TextSourceROTest, test_read_missing_file_returns_null_row) {
  const std::string filename = "does_not_exist.txt";

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_FAILURE);

  int value = -1;
  std::memcpy(&value, buffer.get(), sizeof(int));
  EXPECT_EQ(value, 0);

  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
  EXPECT_EQ(src->count(), 1u);
}

// ============================================================
// textSourceRO - string reading tests
// ============================================================

// Verify reading a quoted string field
TEST_F(TextSourceROTest, test_read_string) {
  auto filename = createTestFile("test_str.txt", "\"hello\"\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  std::string result(reinterpret_cast<char *>(buffer.get()), 5);
  EXPECT_EQ(result, "hello");
}

// Verify reading string shorter than field length is padded
TEST_F(TextSourceROTest, test_read_string_padded) {
  auto filename = createTestFile("test_str_pad.txt", "\"hi\"\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  // First two chars should be "hi", rest padded with zeros
  EXPECT_EQ(buffer[0], 'h');
  EXPECT_EQ(buffer[1], 'i');
}

// Verify NULL token clears string field bytes in text input
TEST_F(TextSourceROTest, test_read_string_null_token) {
  auto filename = createTestFile("test_str_null.txt", "NULL\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  for (int i = 0; i < desc.getSizeInBytes(); ++i) EXPECT_EQ(buffer[i], 0);
}

// ============================================================
// textSourceRO - multi-field descriptor tests
// ============================================================

// Verify reading a record with multiple fields (int + float)
TEST_F(TextSourceROTest, test_read_multi_field) {
  auto filename = createTestFile("test_multi.txt", "7 2.5\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(float)), 1, rdb::FLOAT}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int intVal;
  std::memcpy(&intVal, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(intVal, 7);

  float floatVal;
  std::memcpy(&floatVal, buffer.get() + sizeof(int), sizeof(float));
  EXPECT_NEAR(floatVal, 2.5f, 0.001f);
}

// Verify null bitset exported by source for mixed token record
TEST_F(TextSourceROTest, test_last_null_bitset_for_mixed_record) {
  auto filename = createTestFile("test_mixed_nulls.txt", "NULL 5\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 2U);
  EXPECT_TRUE(nulls[0]);
  EXPECT_FALSE(nulls[1]);
}

// ============================================================
// textSourceRO - array field tests
// ============================================================

// Verify reading an array of integers (rarray > 1)
TEST_F(TextSourceROTest, test_read_integer_array) {
  auto filename = createTestFile("test_arr.txt", "1 2 3\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 3, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int values[3];
  std::memcpy(values, buffer.get(), 3 * sizeof(int));
  GTEST_ASSERT_EQ(values[0], 1);
  GTEST_ASSERT_EQ(values[1], 2);
  GTEST_ASSERT_EQ(values[2], 3);
}

// ============================================================
// textSourceRO - loop to beginning (EOF wrapping) tests
// ============================================================

// Verify loopToBeginningIfEOF wraps around to re-read data
TEST_F(TextSourceROTest, test_loop_to_beginning) {
  auto filename = createTestFile("test_loop.txt", "100\n200\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, true);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  int value;

  // Read first two values
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 100);

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 200);

  // Third read should loop back and return first value again
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 100);
}

// Verify no-loop mode returns zeroed data at EOF
TEST_F(TextSourceROTest, test_no_loop_eof_zeros) {
  auto filename = createTestFile("test_noloop.txt", "42\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  int value;

  // First read succeeds normally
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 42);

  // Second read at EOF should return zeroed data
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 0);

  // and expose null metadata for this fallback row
  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

// ============================================================
// textSourceRO - count() tests
// ============================================================

// Verify count increments with each read
TEST_F(TextSourceROTest, test_count_increments) {
  auto filename = createTestFile("test_count.txt", "1\n2\n3\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  GTEST_ASSERT_EQ(src->count(), 0u);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(src->count(), 1u);

  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(src->count(), 2u);

  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(src->count(), 3u);
}

// ============================================================
// textSourceRO - name() test
// ============================================================

// Verify name() returns the filename passed at construction
TEST_F(TextSourceROTest, test_name) {
  auto filename = createTestFile("test_name.txt", "1\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  EXPECT_EQ(src->name(), "test_name.txt");
}

// ============================================================
// textSourceRO - write is read-only
// ============================================================

// Verify write always returns EXIT_FAILURE (read-only source)
TEST_F(TextSourceROTest, test_write_returns_failure) {
  auto filename = createTestFile("test_ro.txt", "1\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  uint8_t data[] = {0};
  GTEST_ASSERT_EQ(src->write(data, 0), EXIT_FAILURE);
  GTEST_ASSERT_EQ(src->write(data), EXIT_FAILURE);
}

// ============================================================
// textSourceRO - string loop test
// ============================================================

// Verify string reading with loop to beginning
TEST_F(TextSourceROTest, test_read_string_loop) {
  auto filename = createTestFile("test_str_loop.txt", "\"abc\"\n");

  rdb::Descriptor desc{{"s", 3, 1, rdb::STRING}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, true);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());

  // First read
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::string result1(reinterpret_cast<char *>(buffer.get()), 3);
  EXPECT_EQ(result1, "abc");

  // Second read should loop to beginning
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::string result2(reinterpret_cast<char *>(buffer.get()), 3);
  EXPECT_EQ(result2, "abc");
}

// Verify string read at EOF without loop returns zero-padded field
TEST_F(TextSourceROTest, test_read_string_no_loop_eof_zero_padded) {
  auto filename = createTestFile("test_str_noloop_eof.txt", "\"abc\"\n");

  rdb::Descriptor desc{{"s", 3, 1, rdb::STRING}};
  auto recsize = static_cast<ssize_t>(desc.getSizeInBytes());

  auto src = std::make_unique<rdb::textSourceRO>(filename, recsize, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  std::string first(reinterpret_cast<char *>(buffer.get()), 3);
  EXPECT_EQ(first, "abc");

  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  EXPECT_EQ(buffer[0], 0);
  EXPECT_EQ(buffer[1], 0);
  EXPECT_EQ(buffer[2], 0);
}
