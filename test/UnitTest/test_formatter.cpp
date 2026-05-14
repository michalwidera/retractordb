#include <gtest/gtest.h>

#include <boost/property_tree/ptree.hpp>

#include "qry/formatters.hpp"

using boost::property_tree::ptree;

// ---- isNullAt ----

TEST(Formatter, isNullAt_empty_nullmap) { EXPECT_FALSE(Formatter::isNullAt("", 0)); }
TEST(Formatter, isNullAt_null_at_0) { EXPECT_TRUE(Formatter::isNullAt("1", 0)); }
TEST(Formatter, isNullAt_not_null_at_0) { EXPECT_FALSE(Formatter::isNullAt("0", 0)); }
TEST(Formatter, isNullAt_null_at_index_2) { EXPECT_TRUE(Formatter::isNullAt("001", 2)); }
TEST(Formatter, isNullAt_index_out_of_range) { EXPECT_FALSE(Formatter::isNullAt("1", 1)); }
TEST(Formatter, isNullAt_negative_index) { EXPECT_FALSE(Formatter::isNullAt("1", -1)); }

// ---- isAllNull ----

TEST(Formatter, isAllNull_zero_count) { EXPECT_FALSE(Formatter::isAllNull("111", 0)); }
TEST(Formatter, isAllNull_all_null) { EXPECT_TRUE(Formatter::isAllNull("111", 3)); }
TEST(Formatter, isAllNull_partial_null) { EXPECT_FALSE(Formatter::isAllNull("110", 3)); }
TEST(Formatter, isAllNull_no_null) { EXPECT_FALSE(Formatter::isAllNull("000", 3)); }
TEST(Formatter, isAllNull_empty_nullmap) { EXPECT_FALSE(Formatter::isAllNull("", 1)); }

// ---- displayedValue ----

TEST(Formatter, displayedValue_non_null) {
  ptree row;
  row.put("0", "42");
  EXPECT_EQ(Formatter::displayedValue(row, 0, "0", formatMode::RAW), "42");
}

TEST(Formatter, displayedValue_null_raw) {
  ptree row;
  EXPECT_EQ(Formatter::displayedValue(row, 0, "1", formatMode::RAW), "null");
}

TEST(Formatter, displayedValue_null_gnuplot) {
  ptree row;
  EXPECT_EQ(Formatter::displayedValue(row, 0, "1", formatMode::GNUPLOT), "NaN");
}

TEST(Formatter, displayedValue_missing_key_returns_empty) {
  ptree row;
  EXPECT_EQ(Formatter::displayedValue(row, 0, "0", formatMode::RAW), "");
}

// ---- renderRaw ----

TEST(Formatter, renderRaw_outputs_values) {
  ptree row;
  row.put("0", "hello");
  row.put("1", "world");
  testing::internal::CaptureStdout();
  Formatter::renderRaw(row, 2, "00", false);
  auto out = testing::internal::GetCapturedStdout();
  EXPECT_NE(out.find("hello"), std::string::npos);
  EXPECT_NE(out.find("world"), std::string::npos);
}

TEST(Formatter, renderRaw_skip_all_null_produces_no_output) {
  ptree row;
  testing::internal::CaptureStdout();
  Formatter::renderRaw(row, 2, "11", true);
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

TEST(Formatter, renderRaw_no_skip_when_partial_null) {
  ptree row;
  row.put("0", "42");
  testing::internal::CaptureStdout();
  Formatter::renderRaw(row, 2, "01", true);
  EXPECT_FALSE(testing::internal::GetCapturedStdout().empty());
}

TEST(Formatter, renderRaw_skipNull_false_outputs_all_null_row) {
  ptree row;
  testing::internal::CaptureStdout();
  Formatter::renderRaw(row, 2, "11", false);
  EXPECT_FALSE(testing::internal::GetCapturedStdout().empty());
}

// ---- renderGnuplot ----

TEST(Formatter, renderGnuplot_accumulates_state) {
  ptree schema;
  schema.put("db.field.x", "x");
  ptree row1, row2;
  row1.put("0", "10");
  row2.put("0", "20");

  Formatter fmt;
  testing::internal::CaptureStdout();
  fmt.renderGnuplot(row1, 1, "0", "s", schema, {5, 0, 100});
  fmt.renderGnuplot(row2, 1, "0", "s", schema, {5, 0, 100});
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("10"), std::string::npos);
  EXPECT_NE(out.find("20"), std::string::npos);
}

TEST(Formatter, renderGnuplot_respects_window_size) {
  ptree schema;
  schema.put("db.field.x", "x");
  Formatter fmt;

  testing::internal::CaptureStdout();
  for (int i = 0; i < 5; ++i) {
    ptree row;
    row.put("0", std::to_string(i));
    fmt.renderGnuplot(row, 1, "0", "s", schema, {3, 0, 100});  // window = 3
  }
  auto out = testing::internal::GetCapturedStdout();

  // Only last 3 values should appear (0 and 1 evicted)
  EXPECT_EQ(out.find(" 0 \n"), std::string::npos);
  EXPECT_EQ(out.find(" 1 \n"), std::string::npos);
}

TEST(Formatter, renderGnuplot_null_value_becomes_NaN) {
  ptree schema;
  schema.put("db.field.x", "x");
  ptree row;

  Formatter fmt;
  testing::internal::CaptureStdout();
  fmt.renderGnuplot(row, 1, "1", "s", schema, {5, 0, 100});
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("NaN"), std::string::npos);
}

// ---- renderGraphite ----

TEST(Formatter, renderGraphite_format) {
  ptree row;
  row.put("0", "99");
  ptree schema;
  schema.put("db.field.cpu", "cpu");

  testing::internal::CaptureStdout();
  Formatter::renderGraphite(row, "0", "mystream", schema);
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("mystream.cpu"), std::string::npos);
  EXPECT_NE(out.find("99"), std::string::npos);
}

TEST(Formatter, renderGraphite_skips_null_fields) {
  ptree row;
  ptree schema;
  schema.put("db.field.x", "x");

  testing::internal::CaptureStdout();
  Formatter::renderGraphite(row, "1", "s", schema);
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

TEST(Formatter, renderGraphite_multiple_fields_partial_null) {
  ptree row;
  row.put("0", "10");
  ptree schema;
  schema.put("db.field.a", "a");
  schema.put("db.field.b", "b");

  testing::internal::CaptureStdout();
  Formatter::renderGraphite(row, "01", "s", schema);  // field 0 ok, field 1 null
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find(".a"), std::string::npos);
  EXPECT_EQ(out.find(".b"), std::string::npos);
}

// ---- renderInfluxDB ----

TEST(Formatter, renderInfluxDB_format) {
  ptree row;
  row.put("0", "3.14");
  ptree schema;
  schema.put("db.field.temp", "temp");

  testing::internal::CaptureStdout();
  Formatter::renderInfluxDB(row, "0", "sensors", schema);
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("sensors"), std::string::npos);
  EXPECT_NE(out.find("temp=3.14"), std::string::npos);
}

TEST(Formatter, renderInfluxDB_all_null_produces_no_output) {
  ptree row;
  ptree schema;
  schema.put("db.field.x", "x");

  testing::internal::CaptureStdout();
  Formatter::renderInfluxDB(row, "1", "s", schema);
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

TEST(Formatter, renderInfluxDB_multiple_fields_comma_separated) {
  ptree row;
  row.put("0", "1");
  row.put("1", "2");
  ptree schema;
  schema.put("db.field.a", "a");
  schema.put("db.field.b", "b");

  testing::internal::CaptureStdout();
  Formatter::renderInfluxDB(row, "00", "m", schema);
  auto out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("a=1,b=2"), std::string::npos);
}
