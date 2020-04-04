#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Hello
#include <boost/test/unit_test.hpp>

#include <boost/system/error_code.hpp>
#include "compiler/compiler.hpp"

bool check_hello_function() {
    return true;
}

BOOST_AUTO_TEST_CASE( test_hello )
{
    BOOST_CHECK( check_hello_function() == true );
}
