#include <gtest/gtest.h>

#include <sys/stat.h>  // mkfifo

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "logCapture.hpp"
#include "rdb/facctxtsrc.hpp"
#include "rdbResult.hpp"

// Tests intentionally use raw byte buffers for parser and file-interface validation.
// NOLINTBEGIN(modernize-avoid-c-arrays)

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
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_int.txt", "42\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int value;
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 42);
}

// Verify reading multiple integer records sequentially
TEST_F(TextSourceROTest, test_read_multiple_integers) {
  // covers: facctxtsrc.text-source-ro.1~1
  auto filename = createTestFile("test_multi_int.txt", "10\n20\n30\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_float.txt", "3.14\n");

  rdb::Descriptor desc{{"x", static_cast<int>(sizeof(float)), 1, rdb::FLOAT}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  float value;
  std::memcpy(&value, buffer.get(), sizeof(float));
  EXPECT_NEAR(value, 3.14F, 0.001F);
}

// Verify reading a double value
TEST_F(TextSourceROTest, test_read_double) {
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_double.txt", "2.718281828\n");

  rdb::Descriptor desc{{"x", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_uint.txt", "100\n");

  rdb::Descriptor desc{{"u", static_cast<int>(sizeof(unsigned)), 1, rdb::UINT}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  unsigned value;
  std::memcpy(&value, buffer.get(), sizeof(unsigned));
  GTEST_ASSERT_EQ(value, 100U);
}

// Verify reading BYTE type (read as unsigned, stored as uint8_t)
TEST_F(TextSourceROTest, test_read_byte) {
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_byte.txt", "255\n");

  rdb::Descriptor desc{{"b", 1, 1, rdb::BYTE}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(buffer[0], 255);
}

// Verify NULL token maps to null/fallback for scalar numeric fields
TEST_F(TextSourceROTest, test_read_integer_null_token) {
  auto filename = createTestFile("test_int_null.txt", "NULL\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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
  // covers: facctxtsrc.text-source-ro.11~1
  const std::string filename = "does_not_exist.txt";

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EBADF);

  int value = -1;
  std::memcpy(&value, buffer.get(), sizeof(int));
  EXPECT_EQ(value, 0);

  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
  EXPECT_EQ(rdbtest::ok(src->count()), 1U);
}

// ============================================================
// textSourceRO - string reading tests
// ============================================================

// Verify reading a quoted string field
TEST_F(TextSourceROTest, test_read_string) {
  auto filename = createTestFile("test_str.txt", "\"hello\"\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  std::string result(reinterpret_cast<char *>(buffer.get()), 5);
  EXPECT_EQ(result, "hello");
}

// Verify reading string shorter than field length is padded
TEST_F(TextSourceROTest, test_read_string_padded) {
  auto filename = createTestFile("test_str_pad.txt", "\"hi\"\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  std::memset(buffer.get(), 0xFF, desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  for (int i = 0; i < desc.getSizeInBytes(); ++i)
    EXPECT_EQ(buffer[i], 0);
}

// Napis BEZ cudzyslowu jest zwyklym tokenem i jego wartosc ma trafic do pola. Do 2026-08-30
// przeczytany token byl wyrzucany, a skan cudzyslowu ponizej konsumowal reszte pliku
// (pozycja 12 w paper-arXiv/debs/done/requested.md).
TEST_F(TextSourceROTest, test_read_string_unquoted) {
  auto filename = createTestFile("test_str_plain.txt", "hello\n");

  rdb::Descriptor desc{{"s", 5, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get()), 5), "hello");
}

// Sedno objawu z pozycji 12: dla `DECLARE txt STRING[8], k INTEGER` nad wierszem `42 7`
// pole txt wychodzilo puste, a k dostawalo 42 - czyli PIERWSZY token wiersza.
TEST_F(TextSourceROTest, test_read_unquoted_string_and_int_stay_aligned) {
  // covers: facctxtsrc.text-source-ro.3~1
  auto filename = createTestFile("test_str_plain_int.txt", "42 7\n43 8\n");

  rdb::Descriptor desc{{"s", 1, 8, rdb::STRING}, {"k", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

  auto buffer                                             = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  const std::vector<std::pair<std::string, int>> expected = {{"42", 7}, {"43", 8}, {"42", 7}};

  for (size_t i = 0; i < expected.size(); ++i) {
    GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS) << "read #" << i;
    EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get())), expected[i].first) << "read #" << i;
    int value = 0;
    std::memcpy(&value, buffer.get() + 8, sizeof(int));
    EXPECT_EQ(value, expected[i].second) << "read #" << i;
  }
}

// Token dluzszy od pola jest przycinany dokladnie tak, jak napis w cudzyslowie.
TEST_F(TextSourceROTest, test_read_string_unquoted_truncated_to_field) {
  // covers: facctxtsrc.text-source-ro.3~1
  auto filename = createTestFile("test_str_plain_long.txt", "abcdefgh\n");

  rdb::Descriptor desc{{"s", 3, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get()), 3), "abc");
}

// ============================================================
// textSourceRO - multi-field descriptor tests
// ============================================================

// Verify reading a record with multiple fields (int + float)
TEST_F(TextSourceROTest, test_read_multi_field) {
  // covers: facctxtsrc.text-source-ro.1~1
  auto filename = createTestFile("test_multi.txt", "7 2.5\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(float)), 1, rdb::FLOAT}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int intVal;
  std::memcpy(&intVal, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(intVal, 7);

  float floatVal;
  std::memcpy(&floatVal, buffer.get() + sizeof(int), sizeof(float));
  EXPECT_NEAR(floatVal, 2.5F, 0.001F);
}

// Verify null bitset exported by source for mixed token record
TEST_F(TextSourceROTest, test_last_null_bitset_for_mixed_record) {
  // covers: facctxtsrc.text-source-ro.5~1
  auto filename = createTestFile("test_mixed_nulls.txt", "NULL 5\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int values[3];
  std::memcpy(values, buffer.get(), 3 * sizeof(int));
  GTEST_ASSERT_EQ(values[0], 1);
  GTEST_ASSERT_EQ(values[1], 2);
  GTEST_ASSERT_EQ(values[2], 3);
}

// Verify NULL on any element of a numeric array makes the whole field NULL, wherever the element stands
TEST_F(TextSourceROTest, test_read_integer_array_null_element_nulls_whole_field) {
  // covers: facctxtsrc.text-source-ro.5~1
  auto filename = createTestFile("test_arr_null.txt", "NULL 2 3\n2 NULL 3\n2 3 NULL\n1 2 3\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 3, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  for (int row = 0; row < 3; ++row) {
    GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS) << "row " << row;
    ASSERT_EQ(src->lastNullBitset().size(), 1U);
    EXPECT_TRUE(src->lastNullBitset()[0]) << "row " << row;
  }

  // A row without NULL after rows with NULL - the field is defined again
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);
  EXPECT_FALSE(src->lastNullBitset()[0]);
  int values[3];
  std::memcpy(values, buffer.get(), 3 * sizeof(int));
  GTEST_ASSERT_EQ(values[0], 1);
  GTEST_ASSERT_EQ(values[1], 2);
  GTEST_ASSERT_EQ(values[2], 3);
}

// Verify NULLTYPE[N] consumes N NULL tokens, so the next field reads its own token
TEST_F(TextSourceROTest, test_read_nulltype_array_keeps_next_field_aligned) {
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_nulltype_arr.txt", "NULL NULL NULL 7\n");

  rdb::Descriptor desc{{"n", 0, 3, rdb::NULLTYPE}, {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int value = 0;
  std::memcpy(&value, buffer.get(), sizeof(int));
  GTEST_ASSERT_EQ(value, 7);

  auto nulls = src->lastNullBitset();
  ASSERT_EQ(nulls.size(), 2U);
  EXPECT_TRUE(nulls[0]);
  EXPECT_FALSE(nulls[1]);
}

// ============================================================
// textSourceRO - loop to beginning (EOF wrapping) tests
// ============================================================

// Verify loopToBeginningIfEOF wraps around to re-read data
TEST_F(TextSourceROTest, test_loop_to_beginning) {
  // covers: accessorfactory.make-accessor.3~1, facctxtsrc.text-source-ro.8~1
  auto filename = createTestFile("test_loop.txt", "100\n200\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

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
  // covers: facctxtsrc.text-source-ro.9~1
  auto filename = createTestFile("test_noloop.txt", "42\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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
  // covers: facctxtsrc.text-source-ro.13~1
  auto filename = createTestFile("test_count.txt", "1\n2\n3\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  GTEST_ASSERT_EQ(rdbtest::ok(src->count()), 0U);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(rdbtest::ok(src->count()), 1U);

  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(rdbtest::ok(src->count()), 2U);

  src->read(buffer.get(), 0);
  GTEST_ASSERT_EQ(rdbtest::ok(src->count()), 3U);
}

// ============================================================
// textSourceRO - name() test
// ============================================================

// Verify name() returns the filename passed at construction
TEST_F(TextSourceROTest, test_name) {
  auto filename = createTestFile("test_name.txt", "1\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  EXPECT_EQ(src->name(), "test_name.txt");
}

// ============================================================
// textSourceRO - write is read-only
// ============================================================

// Verify write always returns ENOTSUP (read-only source)
TEST_F(TextSourceROTest, test_write_returns_failure) {
  // covers: facctxtsrc.text-source-ro.4~1
  auto filename = createTestFile("test_ro.txt", "1\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  uint8_t data[] = {0};
  GTEST_ASSERT_EQ(src->write(data, 0), ENOTSUP);
  GTEST_ASSERT_EQ(src->write(data), ENOTSUP);
}

// ============================================================
// textSourceRO - string loop test
// ============================================================

// Verify string reading with loop to beginning
TEST_F(TextSourceROTest, test_read_string_loop) {
  auto filename = createTestFile("test_str_loop.txt", "\"abc\"\n");

  rdb::Descriptor desc{{"s", 3, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

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
  // covers: facctxtsrc.text-source-ro.9~1
  auto filename = createTestFile("test_str_noloop_eof.txt", "\"abc\"\n");

  rdb::Descriptor desc{{"s", 3, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

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

// Regression: the end-of-file branch used to consume the first string of the file before
// rescanning for a quote, so after each wrap the sequence skipped record #0 (aa,bb,cc,bb,cc,...).
TEST_F(TextSourceROTest, test_read_string_loop_keeps_first_record) {
  // covers: facctxtsrc.text-source-ro.8~1
  auto filename = createTestFile("test_str_loop_multi.txt", "\"aa\"\n\"bb\"\n\"cc\"\n");

  rdb::Descriptor desc{{"s", 2, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

  auto buffer                             = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  const std::vector<std::string> expected = {"aa", "bb", "cc", "aa", "bb", "cc", "aa"};

  for (size_t i = 0; i < expected.size(); ++i) {
    GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS) << "read #" << i;
    EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get()), 2), expected[i]) << "read #" << i;
  }
}

// The same wrap, but with a string followed by a numeric field: both fields of a record
// must come from the same line after looping back.
TEST_F(TextSourceROTest, test_read_string_and_int_loop_stays_aligned) {
  // covers: facctxtsrc.text-source-ro.8~1
  auto filename = createTestFile("test_str_int_loop.txt", "\"aa\" 1\n\"bb\" 2\n");

  rdb::Descriptor desc{{"s", 2, 1, rdb::STRING}, {"v", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

  auto buffer                                             = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  const std::vector<std::pair<std::string, int>> expected = {{"aa", 1}, {"bb", 2}, {"aa", 1}, {"bb", 2}, {"aa", 1}};

  for (size_t i = 0; i < expected.size(); ++i) {
    GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS) << "read #" << i;
    EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get()), 2), expected[i].first) << "read #" << i;
    int value;
    std::memcpy(&value, buffer.get() + 2, sizeof(int));
    EXPECT_EQ(value, expected[i].second) << "read #" << i;
  }
}

// A single-record file must keep returning that record, not run out of data.
TEST_F(TextSourceROTest, test_read_string_loop_single_record) {
  auto filename = createTestFile("test_str_loop1.txt", "\"zz\"\n");

  rdb::Descriptor desc{{"s", 2, 1, rdb::STRING}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, true);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  for (int i = 0; i < 4; ++i) {
    GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS) << "read #" << i;
    EXPECT_EQ(std::string(reinterpret_cast<char *>(buffer.get()), 2), "zz") << "read #" << i;
  }
}

// Regression: a field following an array used the flat position of the array's second element,
// so with DECLARE a INTEGER[3], b INTEGER the value of b landed in a[1] and b stayed unwritten.
TEST_F(TextSourceROTest, test_read_integer_array_followed_by_scalar) {
  auto filename = createTestFile("test_arr_scalar.txt", "1 2 3 4\n5 6 7 NULL\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 3, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto src = std::make_unique<rdb::textSourceRO>(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  int values[4];
  std::memcpy(values, buffer.get(), 4 * sizeof(int));
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 2);
  EXPECT_EQ(values[2], 3);
  EXPECT_EQ(values[3], 4);

  GTEST_ASSERT_EQ(src->read(buffer.get(), 0), EXIT_SUCCESS);

  std::memcpy(values, buffer.get(), 4 * sizeof(int));
  EXPECT_EQ(values[0], 5);
  EXPECT_EQ(values[1], 6);
  EXPECT_EQ(values[2], 7);
}

// TEXTFILE czyta wylacznie plik zwykly (#346), sprawdzany przez stat() PRZED otwarciem - std::ifstream
// na FIFO bez pisarza wisi w samym open(). Do #346 tekst przez FIFO czytano bez bufora.
TEST_F(TextSourceROTest, textfile_refuses_a_fifo_without_blocking) {
  // covers: facctxtsrc.text-source-ro.14~1
  const std::string path = "text_source_kind.fifo";
  fs::remove(path);
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  createdFiles_.push_back(path);
  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};

  auto pending = std::async(std::launch::async, [&] {
    rdb::textSourceRO src(path, desc, true);
    int value = -1;
    return std::make_pair(src.initializationError(), src.read(reinterpret_cast<uint8_t *>(&value), 0));
  });
  if (pending.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    std::cerr << "textSourceRO blocked on a FIFO without a writer\n";
    std::_Exit(EXIT_FAILURE);
  }
  const auto [error, readResult] = pending.get();
  EXPECT_EQ(error, "TEXTFILE '" + path + "' is a FIFO, not a regular file");
  EXPECT_EQ(readResult, EBADF);
}

TEST_F(TextSourceROTest, textfile_refuses_a_character_device) {
  // covers: facctxtsrc.text-source-ro.14~1
  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  rdb::textSourceRO src(std::string("/dev/null"), desc, true);
  EXPECT_EQ(src.initializationError(), "TEXTFILE '/dev/null' is a character device, not a regular file");
}

// Nieudane open zachowuje errno do odmowy K4, takze po udanym preflight (#488).
TEST_F(TextSourceROTest, missing_file_reports_initialization_error_and_errno) {
  // covers: facctxtsrc.text-source-ro.16~1
  const auto regular = createTestFile("source_parent.txt", "1");
  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  for (const auto &[path, expected] :
       std::vector<std::pair<std::string, int>>{{"does_not_exist_kind.txt", ENOENT}, {regular + "/input", ENOTDIR}}) {
    rdb::textSourceRO src(path, desc, true);
    EXPECT_EQ(src.initializationErrno(), expected);
    EXPECT_TRUE(src.initializationError().contains(path));
    EXPECT_TRUE(src.initializationError().contains(std::strerror(expected)));
    EXPECT_FALSE(src.exhausted());
  }
}

// ============================================================
// textSourceRO - niepoprawny token liczbowy (#471)
// ============================================================

namespace {
size_t countOccurrences(const std::string &text, const std::string &needle) {
  size_t count = 0;
  for (auto pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos + needle.size()))
    ++count;
  return count;
}

// W Release SPDLOG_WARN jest wycinany w kompilacji (SPDLOG_ACTIVE_LEVEL=ERROR) - ostrzezenia nie ma wcale.
constexpr size_t kExpectedWarnings = SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN ? 1 : 0;
}  // namespace

// Token nieodczytany w calosci albo poza zakresem typu pola daje NULL, a nie 0, wartosc graniczna albo
// obcieta liczbe. Sasiednie pole tego rekordu i nastepny rekord czytaja sie bez przesuniecia.
TEST_F(TextSourceROTest, invalid_numeric_token_reads_as_null) {
  // covers: facctxtsrc.text-source-ro.2~1, facctxtsrc.text-source-ro.15~1
  struct Case {
    std::string token;
    rdb::descFld type;
    int size;
  };
  const std::vector<Case> cases{
      {"abc", rdb::INTEGER, sizeof(int)},         {"abc", rdb::UINT, sizeof(unsigned)}, {"abc", rdb::FLOAT, sizeof(float)},
      {"abc", rdb::DOUBLE, sizeof(double)},       {"12x", rdb::INTEGER, sizeof(int)},   {"12x", rdb::UINT, sizeof(unsigned)},
      {"99999999999", rdb::INTEGER, sizeof(int)}, {"-5", rdb::UINT, sizeof(unsigned)},  {"300", rdb::BYTE, 1},
      {"1e999", rdb::DOUBLE, sizeof(double)},     {"1e39", rdb::FLOAT, sizeof(float)},  {"inf", rdb::DOUBLE, sizeof(double)},
      {"nan", rdb::DOUBLE, sizeof(double)},       {"+-5", rdb::INTEGER, sizeof(int)},   {"-5", rdb::BYTE, 1},
      {"1.5x", rdb::DOUBLE, sizeof(double)},
  };

  for (const auto &c : cases) {
    SCOPED_TRACE(c.token + " as " + std::string(rdb::GetStringdescFld(c.type)));
    auto filename = createTestFile("test_bad_token.txt", c.token + " 9\n7 8\n");

    rdb::Descriptor desc{{"a", c.size, 1, c.type}, {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
    rdb::textSourceRO src(filename, desc, false);

    auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
    int b       = 0;

    GTEST_ASSERT_EQ(src.read(buffer.get(), 0), EXIT_SUCCESS);
    ASSERT_EQ(src.lastNullBitset().size(), 2U);
    EXPECT_TRUE(src.lastNullBitset()[0]);
    EXPECT_FALSE(src.lastNullBitset()[1]);
    std::memcpy(&b, buffer.get() + c.size, sizeof(int));
    EXPECT_EQ(b, 9);

    GTEST_ASSERT_EQ(src.read(buffer.get(), 0), EXIT_SUCCESS);
    EXPECT_FALSE(src.lastNullBitset()[0]);
    EXPECT_FALSE(src.lastNullBitset()[1]);
    std::memcpy(&b, buffer.get() + c.size, sizeof(int));
    EXPECT_EQ(b, 8);
  }
}

// Wiodacy '+' i liczba ujemna w typie ze znakiem pozostaja poprawne.
TEST_F(TextSourceROTest, signed_numeric_tokens_still_parse) {
  // covers: facctxtsrc.text-source-ro.2~1
  auto filename = createTestFile("test_signed_tokens.txt", "+7 -5 +7 +255 +2.5\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER},
                       {"c", static_cast<int>(sizeof(unsigned)), 1, rdb::UINT},
                       {"d", 1, 1, rdb::BYTE},
                       {"e", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE}};
  rdb::textSourceRO src(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src.read(buffer.get(), 0), EXIT_SUCCESS);
  for (size_t k = 0; k < src.lastNullBitset().size(); ++k)
    EXPECT_FALSE(src.lastNullBitset()[k]) << "field " << k;

  int a          = 0;
  int b          = 0;
  unsigned c     = 0;
  uint8_t d      = 0;
  double e       = 0;
  const auto *at = buffer.get();
  std::memcpy(&a, at, sizeof(int));
  std::memcpy(&b, at + 4, sizeof(int));
  std::memcpy(&c, at + 8, sizeof(unsigned));
  std::memcpy(&d, at + 12, 1);
  std::memcpy(&e, at + 13, sizeof(double));
  EXPECT_EQ(a, 7);
  EXPECT_EQ(b, -5);
  EXPECT_EQ(c, 7U);
  EXPECT_EQ(d, 255);
  EXPECT_DOUBLE_EQ(e, 2.5);
}

// Niepoprawny element tablicy unewaznia cale pole - ta sama regula co token NULL na elemencie.
TEST_F(TextSourceROTest, invalid_array_element_nulls_whole_field) {
  // covers: facctxtsrc.text-source-ro.15~1
  auto filename = createTestFile("test_bad_arr.txt", "1 x 3 4\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 3, rdb::INTEGER},
                       {"b", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  rdb::textSourceRO src(filename, desc, false);

  auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
  GTEST_ASSERT_EQ(src.read(buffer.get(), 0), EXIT_SUCCESS);
  ASSERT_EQ(src.lastNullBitset().size(), 2U);
  EXPECT_TRUE(src.lastNullBitset()[0]);
  EXPECT_FALSE(src.lastNullBitset()[1]);
  int b = 0;
  std::memcpy(&b, buffer.get() + (3 * sizeof(int)), sizeof(int));
  EXPECT_EQ(b, 4);
}

// Jedno ostrzezenie na zrodlo: drugi zly token i kolejne przejscia zawijanego pliku nie dopisuja nic.
TEST_F(TextSourceROTest, invalid_token_warned_once_across_wrap) {
  // covers: facctxtsrc.text-source-ro.15~1
  auto filename = createTestFile("test_bad_wrap.txt", "abc\n12x\n");

  rdb::Descriptor desc{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}};
  LogCapture log;
  {
    rdb::textSourceRO src(filename, desc, true);
    auto buffer = std::make_unique<uint8_t[]>(desc.getSizeInBytes());
    for (int row = 0; row < 5; ++row) {
      GTEST_ASSERT_EQ(src.read(buffer.get(), 0), EXIT_SUCCESS) << "row " << row;
      EXPECT_TRUE(src.lastNullBitset()[0]) << "row " << row;
    }
  }
  EXPECT_EQ(countOccurrences(log.text(), "invalid numeric token"), kExpectedWarnings) << log.text();
  if (kExpectedWarnings == 1)
    EXPECT_NE(log.text().find("Text source test_bad_wrap.txt: record 0, field a: invalid numeric token 'abc'"),
              std::string::npos)
        << log.text();
}

TEST_F(TextSourceROTest, invalid_position_does_not_consume_input_and_counts_the_null_row) {
  // covers: facctxtsrc.text-source-ro.7~1, facctxtsrc.text-source-ro.13~1, fainterface.file-interface.5~1
  const auto file = createTestFile("test_position.txt", "42");
  const rdb::Descriptor descriptor{{"a", 4, 1, rdb::INTEGER}};
  rdb::textSourceRO source(file, descriptor, false);
  int value = -1;
  std::vector<bool> nulls{false};
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(&value), nulls, 1), EINVAL);
  EXPECT_EQ(value, 0);
  EXPECT_EQ(nulls, std::vector<bool>{true});
  EXPECT_FALSE(source.exhausted());
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(&value), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(value, 42);
  EXPECT_EQ(nulls, std::vector<bool>{false});
  EXPECT_EQ(rdbtest::ok(source.count()), 2U);
}

TEST_F(TextSourceROTest, null_spellings_and_missing_tokens_preserve_the_final_record) {
  // covers: facctxtsrc.text-source-ro.6~1, facctxtsrc.text-source-ro.9~1, facctxtsrc.text-source-ro.10~1
  const auto file = createTestFile("test_null_spellings.txt", "null NULL Null 42");
  const rdb::Descriptor descriptor{{"a", 4, 1, rdb::INTEGER}, {"b", 4, 1, rdb::INTEGER}};
  rdb::textSourceRO source(file, descriptor, false);
  std::array<int, 2> values{-1, -1};
  std::vector<bool> nulls;
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(values.data()), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(values, (std::array<int, 2>{0, 0}));
  EXPECT_EQ(nulls, (std::vector<bool>{true, true}));
  EXPECT_FALSE(source.exhausted());
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(values.data()), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(values, (std::array<int, 2>{0, 42}));
  EXPECT_EQ(nulls, (std::vector<bool>{true, false}));
  EXPECT_FALSE(source.exhausted());
  for (int i = 0; i < 2; ++i) {
    EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(values.data()), nulls, 0), EXIT_SUCCESS);
    EXPECT_EQ(values, (std::array<int, 2>{0, 0}));
    EXPECT_EQ(nulls, (std::vector<bool>{true, true}));
    EXPECT_TRUE(source.exhausted());
  }
  EXPECT_EQ(rdbtest::ok(source.count()), 4U);

  const auto partial = createTestFile("test_missing_token.txt", "7");
  rdb::textSourceRO shortSource(partial, descriptor, false);
  EXPECT_EQ(shortSource.read(reinterpret_cast<uint8_t *>(values.data()), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(values, (std::array<int, 2>{7, 0}));
  EXPECT_EQ(nulls, (std::vector<bool>{false, true}));
}

TEST_F(TextSourceROTest, nulltype_content_error_preserves_output_and_does_not_count) {
  // covers: facctxtsrc.text-source-ro.12~1, facctxtsrc.text-source-ro.13~1
  const auto file = createTestFile("test_nulltype_error.txt", "12 bad 9 Null");
  const rdb::Descriptor descriptor{{"a", 4, 1, rdb::INTEGER}, {"missing", 0, 1, rdb::NULLTYPE}};
  rdb::textSourceRO source(file, descriptor, false);
  int value = 81;
  std::vector<bool> nulls{false, true};
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(&value), nulls, 0), EILSEQ);
  EXPECT_EQ(value, 81);
  EXPECT_EQ(nulls, (std::vector<bool>{false, true}));
  EXPECT_EQ(rdbtest::ok(source.count()), 0U);
  EXPECT_NE(source.inputError().find("expected a NULL token"), std::string::npos);
  EXPECT_NE(source.inputError().find("bad"), std::string::npos);
  EXPECT_NE(source.inputError().find(file), std::string::npos);
  EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(&value), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(value, 9);
  EXPECT_EQ(nulls, (std::vector<bool>{false, true}));
  EXPECT_EQ(rdbtest::ok(source.count()), 1U);
}

TEST_F(TextSourceROTest, quoted_strings_keep_backslashes_and_zero_pad_the_field) {
  // covers: facctxtsrc.text-source-ro.3~1
  const auto file = createTestFile("test_literal_string.txt", "\"a\\nb\"\n");
  const rdb::Descriptor descriptor{{"s", 1, 6, rdb::STRING}};
  rdb::textSourceRO source(file, descriptor, false);
  std::array<uint8_t, 6> bytes;
  bytes.fill(0xFF);
  std::vector<bool> nulls;
  EXPECT_EQ(source.read(bytes.data(), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(bytes, (std::array<uint8_t, 6>{'a', '\\', 'n', 'b', 0, 0}));
  EXPECT_EQ(nulls, std::vector<bool>{false});
}

// Pusty plik w petli nie ma rekordu takze po zawinieciu - jak BINFILE (read_fails_on_empty_file_when_loop_enabled)
// zwraca EIO juz w pierwszym odczycie. Do 2026-10-16 pierwszy odczyt zwracal EXIT_SUCCESS z rekordem all-null,
// a EIO dopiero kolejne.
TEST_F(TextSourceROTest, empty_looped_file_reports_eio_on_every_read) {
  // covers: facctxtsrc.text-source-ro.11~1
  const auto file = createTestFile("test_empty_loop.txt", "");
  const rdb::Descriptor descriptor{{"a", 4, 1, rdb::INTEGER}};
  rdb::textSourceRO source(file, descriptor, true);
  for (size_t attempt = 1; attempt <= 2; ++attempt) {
    SCOPED_TRACE(attempt);
    int value = -1;
    std::vector<bool> nulls{false};
    EXPECT_EQ(source.read(reinterpret_cast<uint8_t *>(&value), nulls, 0), EIO);
    EXPECT_EQ(value, 0);
    EXPECT_EQ(nulls, std::vector<bool>{true});
    EXPECT_EQ(rdbtest::ok(source.count()), attempt);
    EXPECT_FALSE(source.exhausted());
  }
}

TEST_F(TextSourceROTest, floating_point_underflow_is_zero_with_clear_null_bits) {
  // covers: facctxtsrc.text-source-ro.2~1
  const auto file = createTestFile("test_underflow.txt", "1e-999 1e-999");
  const rdb::Descriptor descriptor{{"f", 4, 1, rdb::FLOAT}, {"d", 8, 1, rdb::DOUBLE}};
  rdb::textSourceRO source(file, descriptor, false);
  std::array<uint8_t, 12> bytes;
  bytes.fill(0xFF);
  std::vector<bool> nulls;
  EXPECT_EQ(source.read(bytes.data(), nulls, 0), EXIT_SUCCESS);
  float f  = 1;
  double d = 1;
  std::memcpy(&f, bytes.data(), sizeof(f));
  std::memcpy(&d, bytes.data() + sizeof(f), sizeof(d));
  EXPECT_EQ(f, 0.0F);
  EXPECT_EQ(d, 0.0);
  EXPECT_EQ(nulls, (std::vector<bool>{false, false}));
}

// NOLINTEND(modernize-avoid-c-arrays)
