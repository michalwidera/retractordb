#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>
#include "compiler/compiler.hpp"
#include <string>
#include <vector>

using namespace std;

extern string parser(vector<string> vsInputFile);

bool check_compile_function() {

    vector<string> cs;

    cs.push_back("declare a i16, b i8 stream core0, 1 file /dev/urandom");
    cs.push_back("declare c integer,d i8 stream core1, 0.5 file /dev/urandom");
    cs.push_back("select core0[0],core0[1] as test stream str1 from core0#core1");

    return ( parser( cs ) == "OK" );
}


TEST( xcompiler, check_compile )
{
    ASSERT_TRUE( check_compile_function() );
}
