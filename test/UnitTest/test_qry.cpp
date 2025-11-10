#include <gtest/gtest.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "qry/qry.hpp"

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

TEST(xqry, test_hello) { ASSERT_TRUE(obj.hello() == boost::system::errc::success); }

TEST(xqry, test_dir) { ASSERT_TRUE(obj.dir() == "|core0|1|123|345|/dev/location|789|\n"); }

// TEST(xqry, test_detail) { ASSERT_TRUE(obj.detailShow("core0") == "core0.rname1\ncore0.rname2\n"); }
// now it works as yaml
