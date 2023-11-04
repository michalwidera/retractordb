#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>

#include "qry/qry.hpp"

qry obj;

TEST(xqry, test_hello) { ASSERT_TRUE(obj.hello() == boost::system::errc::protocol_error); }