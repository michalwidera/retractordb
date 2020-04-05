#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Hello
#include <boost/test/unit_test.hpp>

#include <boost/system/error_code.hpp>
#include "compiler/compiler.hpp"
#include <string>
#include <vector>

using namespace std;

extern string parser(vector<string> vsInputFile);

bool check_compile_function() {
    
    vector<string> cs;

    cs.push_back("declare a i16 ,b i8 stream core0, 1 file /dev/urandom");
    cs.push_back("declare c integer,d stream core1, 1 file /dev/urandom");
    cs.push_back("select core0[0],core0[1] as test stream str1 from core0#core1");

    return ( parser( cs ) == "OK" );
}


BOOST_AUTO_TEST_CASE( check_compile )
{
    BOOST_CHECK( check_compile_function() == true );
}
