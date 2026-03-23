#include <gtest/gtest.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "qry/qry.hpp"

// ctest -R '^ut-xqry' -V

// Single-stream fake - returns the same canned response regardless of command.
// Used by the original hello/dir tests where command differentiation is not needed.

class qry_fake : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &);
} obj;

boost::property_tree::ptree qry_fake::netClient(const std::string &arg1, const std::string &arg2) {
  boost::property_tree::ptree retval;

  retval.put("db.message", arg1);
  if (arg2 != "") retval.put("db.argument", arg2);
  retval.put("db.id", "core0");

  retval.put("db.stream.core0", "core0");
  retval.put("db.stream.core0.duration", "1");
  retval.put("db.stream.core0.size", "123");
  retval.put("db.stream.core0.count", "345");
  retval.put("db.stream.core0.location", "/dev/location");
  retval.put("db.stream.core0.cap", "789");

  retval.put("db.field.rname1", "rname1");
  retval.put("db.field.rname2", "rname2");
  retval.put("db", "world");
  return retval;
}

// Verify hello handshake succeeds when server responds with "world"
TEST(xqry, test_hello) { EXPECT_TRUE(obj.hello() == boost::system::errc::success); }

// Verify dir() formats single-stream table with pipe-delimited columns
TEST(xqry, test_dir) { EXPECT_TRUE(obj.dir() == "|core0|1|123|345|/dev/location|789|\n"); }

// Command-aware fake - dispatches different responses based on netClient command.
// Supports: hello, get, detail, adhoc, show. Used for detailShow, adhoc, and dirYaml tests.

class qry_fake_detail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &) override;
};

boost::property_tree::ptree qry_fake_detail::netClient(const std::string &arg1, const std::string &arg2) {
  boost::property_tree::ptree retval;

  if (arg1 == "hello") {
    retval.put("db", "world");
  } else if (arg1 == "get") {
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "123");
    retval.put("db.stream.core0.count", "345");
    retval.put("db.stream.core0.location", "/dev/location");
    retval.put("db.stream.core0.cap", "789");
  } else if (arg1 == "detail") {
    retval.put("db.stream", arg2);
    retval.put("db.duration", "1");
    retval.put("db.processed_line", "SELECT * STREAM " + arg2 + " FROM source");
    retval.put("db.field.rname1", "rname1");
    retval.put("db.field.rname2", "rname2");
    retval.put("db.field_type.rname1", "INTEGER");
    retval.put("db.field_type.rname2", "FLOAT");
  } else if (arg1 == "adhoc") {
    retval.put("db", "OK");
  } else if (arg1 == "show") {
    retval.put("db.stream", arg2);
  }

  return retval;
}

// Verify detailShow() produces correct YAML for an existing stream:
// checks stream name, delta, query line, field names, field types, and apiVersion header
TEST(xqry, test_detailShow_found) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.detailShow("core0");

  EXPECT_TRUE(result.find("name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
  EXPECT_TRUE(result.find("query: SELECT * STREAM core0 FROM source") != std::string::npos);
  EXPECT_TRUE(result.find("core0.rname1:") != std::string::npos);
  EXPECT_TRUE(result.find("core0.rname2:") != std::string::npos);
  EXPECT_TRUE(result.find("type: INTEGER") != std::string::npos);
  EXPECT_TRUE(result.find("type: FLOAT") != std::string::npos);
  EXPECT_TRUE(result.find("apiVersion: xqry/v1") != std::string::npos);
}

// Verify detailShow() returns empty string when queried stream does not exist
TEST(xqry, test_detailShow_not_found) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.detailShow("nonexistent");

  EXPECT_TRUE(result.empty());
}

// Verify adhoc() returns false (success) when server acknowledges with "OK"
TEST(xqry, test_adhoc_success) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.adhoc("SELECT * STREAM test FROM core0");

  EXPECT_FALSE(result);
}

// Fake that always returns "FAIL" - simulates server rejecting an adhoc query

class qry_fake_adhoc_fail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &) override;
};

boost::property_tree::ptree qry_fake_adhoc_fail::netClient(const std::string &arg1, const std::string &arg2) {
  boost::property_tree::ptree retval;
  retval.put("db", "FAIL");
  return retval;
}

// Verify adhoc() returns true (error) when server responds with non-"OK" value
TEST(xqry, test_adhoc_failure) {
  qry_fake_adhoc_fail obj_fail;
  auto result = obj_fail.adhoc("INVALID QUERY");

  EXPECT_TRUE(result);
}

// Fake that responds with unexpected value instead of "world" - simulates protocol mismatch

class qry_fake_hello_fail : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &) override;
};

boost::property_tree::ptree qry_fake_hello_fail::netClient(const std::string &, const std::string &) {
  boost::property_tree::ptree retval;
  retval.put("db", "unexpected_response");
  return retval;
}

// Verify hello() returns protocol_error when server responds with unexpected value
TEST(xqry, test_hello_failure) {
  qry_fake_hello_fail obj_fail;
  EXPECT_TRUE(obj_fail.hello() == boost::system::errc::protocol_error);
}

// Verify dirYaml() produces correct YAML with apiVersion header, stream list,
// and all stream properties (name, delta, size, count, location)
TEST(xqry, test_dirYaml) {
  qry_fake_detail obj_detail;
  auto result = obj_detail.dirYaml();

  EXPECT_TRUE(result.find("apiVersion: xqry/v1") != std::string::npos);
  EXPECT_TRUE(result.find("streams:") != std::string::npos);
  EXPECT_TRUE(result.find("- name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
  EXPECT_TRUE(result.find("size: 123") != std::string::npos);
  EXPECT_TRUE(result.find("count: 345") != std::string::npos);
  EXPECT_TRUE(result.find("location: /dev/location") != std::string::npos);
}

// Multi-stream fake - returns two streams (core0, core1) with different deltas and sizes.
// Used to verify dir/dirYaml/detailShow work correctly with multiple streams.

class qry_fake_multi : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &) override;
};

boost::property_tree::ptree qry_fake_multi::netClient(const std::string &arg1, const std::string &arg2) {
  boost::property_tree::ptree retval;

  if (arg1 == "hello") {
    retval.put("db", "world");
  } else if (arg1 == "get") {
    retval.put("db.stream.core0", "core0");
    retval.put("db.stream.core0.duration", "1");
    retval.put("db.stream.core0.size", "100");
    retval.put("db.stream.core0.count", "200");
    retval.put("db.stream.core0.location", "/dev/loc0");
    retval.put("db.stream.core0.cap", "300");

    retval.put("db.stream.core1", "core1");
    retval.put("db.stream.core1.duration", "0.5");
    retval.put("db.stream.core1.size", "400");
    retval.put("db.stream.core1.count", "500");
    retval.put("db.stream.core1.location", "/dev/loc1");
    retval.put("db.stream.core1.cap", "600");
  } else if (arg1 == "detail") {
    retval.put("db.stream", arg2);
    retval.put("db.duration", arg2 == "core0" ? "1" : "0.5");
    retval.put("db.processed_line", "DECLARE a INTEGER STREAM " + arg2);
    retval.put("db.field.a", "a");
    retval.put("db.field_type.a", "INTEGER");
  }

  return retval;
}

// Verify dir() output contains both streams and their respective sizes
TEST(xqry, test_dir_multi_stream) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.dir();

  EXPECT_TRUE(result.find("core0") != std::string::npos);
  EXPECT_TRUE(result.find("core1") != std::string::npos);
  EXPECT_TRUE(result.find("100") != std::string::npos);
  EXPECT_TRUE(result.find("400") != std::string::npos);
}

// Verify dirYaml() lists both streams with correct names and deltas
TEST(xqry, test_dirYaml_multi_stream) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.dirYaml();

  EXPECT_TRUE(result.find("- name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("- name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Verify detailShow() resolves correct stream (core0) from a multi-stream setup
TEST(xqry, test_detailShow_multi_stream_core0) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShow("core0");

  EXPECT_TRUE(result.find("name: core0") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 1") != std::string::npos);
}

// Verify detailShow() resolves correct stream (core1) with its own delta from a multi-stream setup
TEST(xqry, test_detailShow_multi_stream_core1) {
  qry_fake_multi obj_multi;
  auto result = obj_multi.detailShow("core1");

  EXPECT_TRUE(result.find("name: core1") != std::string::npos);
  EXPECT_TRUE(result.find("delta: 0.5") != std::string::npos);
}

// Fake with size="-1" and empty location - tests edge-case omission logic in dirYaml()

class qry_fake_nosize : public qry {
 public:
  boost::property_tree::ptree netClient(const std::string &, const std::string &) override;
};

boost::property_tree::ptree qry_fake_nosize::netClient(const std::string &arg1, const std::string &arg2) {
  boost::property_tree::ptree retval;

  if (arg1 == "get") {
    retval.put("db.stream.str0", "str0");
    retval.put("db.stream.str0.duration", "2");
    retval.put("db.stream.str0.size", "-1");
    retval.put("db.stream.str0.count", "10");
    retval.put("db.stream.str0.location", "");
    retval.put("db.stream.str0.cap", "0");
  }

  return retval;
}

// Verify dirYaml() omits "size:" line when stream size is "-1" (unbounded)
TEST(xqry, test_dirYaml_size_minus_one_omitted) {
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml();

  EXPECT_TRUE(result.find("- name: str0") != std::string::npos);
  EXPECT_TRUE(result.find("size:") == std::string::npos);
}

// Verify dirYaml() omits "location:" line when stream location is empty
TEST(xqry, test_dirYaml_empty_location_omitted) {
  qry_fake_nosize obj_nosize;
  auto result = obj_nosize.dirYaml();

  EXPECT_TRUE(result.find("location:") == std::string::npos);
}

// Verify default outputFormatMode is RAW
TEST(xqry, test_default_format_mode) {
  qry_fake_detail obj_detail;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::RAW);
}

// Verify all formatMode enum values can be assigned and read back
TEST(xqry, test_format_mode_set) {
  qry_fake_detail obj_detail;
  obj_detail.outputFormatMode = formatMode::GRAPHITE;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::GRAPHITE);

  obj_detail.outputFormatMode = formatMode::INFLUXDB;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::INFLUXDB);

  obj_detail.outputFormatMode = formatMode::GNUPLOT;
  EXPECT_TRUE(obj_detail.outputFormatMode == formatMode::GNUPLOT);
}
