#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>
#include "compiler/compiler.hpp"
#include <string>
#include <vector>

using namespace std;

extern string parser(string sInputFile);

bool check_compile_function() {

    return ( parser( "ut_example.rql" ) == "OK" );
}


TEST( xcompiler, check_compile )
{
    ASSERT_TRUE( check_compile_function() );
}
