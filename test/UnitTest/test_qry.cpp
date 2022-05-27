#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>

#include "qry/qry.hpp"

bool check_hello_function() {
  return (hello() == boost::system::errc::protocol_error);
}

TEST(xqry, test_hello) { ASSERT_TRUE(check_hello_function()); }