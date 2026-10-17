#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <regex>

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
  for (const auto mode : {formatMode::RAW, formatMode::GRAPHITE, formatMode::INFLUXDB})
    EXPECT_EQ(Formatter::displayedValue(row, 0, "1", mode), "null");
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
  EXPECT_EQ(out, "hello world \r\n");
}

TEST(Formatter, null_marker_and_gnuplot_axis_contracts) {
  EXPECT_FALSE(Formatter::isNullAt("x", 0));
  EXPECT_FALSE(Formatter::isAllNull("1", -1));
  for (const bool reversed : {false, true}) {
    testing::internal::CaptureStdout();
    Formatter::initGnuplot({12, -3, 7}, reversed);
    const auto text = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(text.contains("set term qt noraise"));
    EXPECT_TRUE(text.contains(reversed ? "set xrange [12:0]" : "set xrange [0:12]"));
    EXPECT_TRUE(text.contains("set yrange [-3:7]"));
  }
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

// Podaje kolejne wartosci do formattera i zwraca to, co poszlo do gnuplota.
namespace {
std::string feedGnuplot(Formatter &fmt, int firstValue, int howMany, int window) {
  ptree schema;
  schema.put("db.field.x", "x");
  testing::internal::CaptureStdout();
  for (int i = 0; i < howMany; ++i) {
    ptree row;
    row.put("0", std::to_string(firstValue + i));
    fmt.renderGnuplot(row, 1, "0", "s", schema, {window, 0, 100});
  }
  return testing::internal::GetCapturedStdout();
}
}  // namespace

TEST(Formatter, renderGnuplot_accumulates_state) {
  Formatter fmt;
  auto out = feedGnuplot(fmt, 10, 2, 5);

  EXPECT_NE(out.find("10"), std::string::npos);
  EXPECT_NE(out.find("11"), std::string::npos);
}

TEST(Formatter, renderGnuplot_respects_window_size) {
  Formatter fmt;
  auto out = feedGnuplot(fmt, 0, 5, 3);  // window = 3

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

TEST(Formatter, renderGnuplot_expands_arrays_and_fallbacks_with_newest_first) {
  Formatter formatter;
  ptree schema;
  schema.put("db.field.vec_values", "vec_values");
  schema.put("db.field_count.vec_values", 2);
  testing::internal::CaptureStdout();
  for (const int value : {10, 20, 30}) {
    ptree row;
    row.put("0", value);
    row.put("1", value + 1);
    row.put("2", value + 2);
    formatter.renderGnuplot(row, 3, "000", "s", schema, {2, 0, 100});
  }
  const auto output = testing::internal::GetCapturedStdout();
  const auto last   = output.rfind("plot");
  ASSERT_NE(last, std::string::npos);
  EXPECT_EQ(output.substr(last),
            "plot '-' u 1:2 t '[vec-values[0]]' w lines lc rgb 'red',"
            " '-' u 1:2 t '[vec-values[1]]' w lines lc rgb 'blue',"
            " '-' u 1:2 t '[col2]' w lines lc rgb 'green'\r\n"
            "0 30\r\n1 20\r\ne\r\n0 31\r\n1 21\r\ne\r\n0 32\r\n1 22\r\ne\r\n");
}

// ---- renderGnuplotOhlc ----

namespace {
std::string feedOhlc(Formatter &fmt, const std::vector<std::vector<std::string>> &rows, const std::string &nullmap, int window) {
  testing::internal::CaptureStdout();
  for (const auto &values : rows) {
    ptree row;
    for (size_t i = 0; i < values.size(); ++i)
      row.put(std::to_string(i), values[i]);
    fmt.renderGnuplotOhlc(row, static_cast<int>(values.size()), nullmap, "chart", {window, 0, 100});
  }
  return testing::internal::GetCapturedStdout();
}
}  // namespace

// Wiersz: open, high, low, close, probki w kolejnosci naplywu. Najnowsza probka stoi w x = 0,
// a swieca nad srodkiem swoich probek - dane swiecy ida w porzadku gnuplota: x open low high close.
TEST(Formatter, renderGnuplotOhlc_places_candle_over_its_samples) {
  Formatter fmt;
  auto out = feedOhlc(fmt, {{"10", "40", "5", "30", "10", "40", "5", "30"}}, "", 20);

  EXPECT_NE(out.find("1.5 10 5 40 30 3.2\r\ne\r\n"), std::string::npos) << out;
  EXPECT_NE(out.find("e\r\n0 30\r\n1 5\r\n2 40\r\n3 10\r\ne\r\n"), std::string::npos) << out;
}

// Okno -p liczy probki: 8 probek to dwie swiece po 4, wiec pierwszy wiersz wypada.
TEST(Formatter, renderGnuplotOhlc_window_counts_samples) {
  Formatter fmt;
  auto out             = feedOhlc(fmt,
                                  {{"1", "1", "1", "1", "1", "1", "1", "1"},
                                   {"2", "2", "2", "2", "2", "2", "2", "2"},
                                   {"3", "3", "3", "3", "3", "3", "3", "3"}},
                                  "", 8);
  const auto lastFrame = out.substr(out.rfind("plot"));

  EXPECT_NE(lastFrame.find("\r\n1.5 3 3 3 3 3.2\r\n5.5 2 2 2 2 3.2\r\n"
                           "e\r\n0 3\r\n1 3\r\n2 3\r\n3 3\r\n4 2\r\n5 2\r\n6 2\r\n7 2\r\ne\r\n"),
            std::string::npos)
      << lastFrame;
}

// Swieca z NULL-em znika w calosci, jej probki zostaja.
TEST(Formatter, renderGnuplotOhlc_null_candle_is_skipped) {
  Formatter fmt;
  auto out = feedOhlc(fmt, {{"", "40", "5", "30", "7"}}, "10000", 5);

  EXPECT_NE(out.find("plot '-' u 1:2 t '[chart]' w lines lc rgb 'blue'\r\n0 7\r\ne\r\n"), std::string::npos) << out;
  EXPECT_EQ(out.find("candlesticks"), std::string::npos) << out;
}

TEST(Formatter, renderGnuplotOhlc_all_null_waits_for_valid_points) {
  Formatter fmt;
  auto empty = feedOhlc(fmt, {{"", "", "", "", ""}}, "11111", 5);
  EXPECT_TRUE(empty.empty()) << empty;

  auto out = feedOhlc(fmt, {{"10", "40", "5", "30", "7"}}, "00000", 5);
  EXPECT_NE(out.find("candlesticks"), std::string::npos) << out;
  EXPECT_NE(out.find("0 7\r\n"), std::string::npos) << out;
}

TEST(Formatter, renderGnuplotOhlc_null_samples_draws_only_candle) {
  Formatter fmt;
  auto out = feedOhlc(fmt, {{"10", "40", "5", "30", ""}}, "00001", 5);

  EXPECT_NE(out.find("plot '-' u 1:2:3:4:5:6:"), std::string::npos) << out;
  EXPECT_EQ(out.find("w lines"), std::string::npos) << out;
}

TEST(Formatter, renderGnuplotOhlc_row_without_samples_plots_nothing) {
  Formatter fmt;
  testing::internal::CaptureStderr();
  auto out = feedOhlc(fmt, {{"10", "40", "5", "30"}}, "", 5);
  out += feedOhlc(fmt, {{"10", "40", "5", "30"}}, "", 5);
  const auto error = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(out.empty()) << out;
  EXPECT_NE(error.find("--gnuplot-ohlc"), std::string::npos) << error;
  EXPECT_EQ(error.find("--gnuplot-ohlc"), error.rfind("--gnuplot-ohlc"));
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

TEST(Formatter, monitoring_formats_expand_arrays_keep_empty_values_and_ignore_surplus) {
  ptree schema, row;
  schema.put("db.field.vec", "vec");
  schema.put("db.field_count.vec", 2);
  schema.put("db.field.scalar", "scalar");
  row.put("0", "10");
  row.put("1", "");
  row.put("2", "99");
  row.put("3", "777");
  const auto secondsBefore = std::time(nullptr);
  testing::internal::CaptureStdout();
  Formatter::renderGraphite(row, "0010", "s", schema);
  const auto graphite     = testing::internal::GetCapturedStdout();
  const auto secondsAfter = std::time(nullptr);
  std::smatch match;
  ASSERT_TRUE(std::regex_match(graphite, match, std::regex("s\\.vec\\.0 10 ([0-9]+)\ns\\.vec\\.1  ([0-9]+)\n"))) << graphite;
  for (int index : {1, 2}) {
    EXPECT_GE(std::stoll(match[index].str()), secondsBefore);
    EXPECT_LE(std::stoll(match[index].str()), secondsAfter);
  }

  const auto nanoseconds = [] {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  };
  const auto before = nanoseconds();
  testing::internal::CaptureStdout();
  Formatter::renderInfluxDB(row, "0010", "s", schema);
  const auto influx = testing::internal::GetCapturedStdout();
  const auto after  = nanoseconds();
  ASSERT_TRUE(std::regex_match(influx, match, std::regex("s vec_0=10,vec_1= ([0-9]+)\n"))) << influx;
  EXPECT_GE(std::stoll(match[1].str()), before);
  EXPECT_LE(std::stoll(match[1].str()), after);
}
