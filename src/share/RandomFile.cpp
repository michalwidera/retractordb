#include "RandomFile.h"
#include <cassert>

//risk: Storing the pointer to the string returned by getenv() can result in overwritten environmental data.
//https://www.securecoding.cert.org/confluence/display/cplusplus/ENV00-CPP.+Do+not+store+the+pointer+to+the+string+returned+by+getenv()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

CRandomFile::CRandomFile( string murExt, const bool destroyOnDestructor ) :
    destroyOnDestructor( destroyOnDestructor ) {
    std::string TMP_PATH;

    if ( getenv("TMP") != NULL ) {
        TMP_PATH = getenv("TMP") ;
    } else if ( getenv("TEMP") != NULL ) {
        TMP_PATH = getenv("TEMP") ;
    } else if ( getenv("HOME") != NULL ) {
        TMP_PATH = getenv("HOME") ;
    } else {
        throw std::out_of_range("No TEMP/TMP var - set enviroment");
    }

    assert( TMP_PATH != "" );
    assert( murExt != "" );
#ifdef WIN32
    char randomFilename [MAX_PATH];

    if ( ! GetTempFileNameA( TMP_PATH.c_str(), murExt.c_str(), 0, randomFilename ) ) {
        throw std::out_of_range("Thrown/RandomFile.cpp/CRandomFile/WINAPI32 GetTempFileNameA");
    }

#else
    char randomFilename [MAX_PATH] = "/tmp/stream.XXXXXXXX";

    if ( mkstemp(randomFilename) == -1 ) {
        throw std::out_of_range("Thrown/RandomFile.cpp/CRandomFile/mkstemp(template)");
    }

#endif
    filename = randomFilename ;
    open( filename.c_str(), ios::in | ios::out | ios::binary ) ;

    if ( !good() ) {
        throw std::out_of_range("Thrown/RandomFile.cpp/CRandomFile/Randomfile does not created file");
    }
}

CRandomFile::~CRandomFile() {
    if ( rdbuf()->is_open() ) {
        close();
    }

    if( destroyOnDestructor ) {
        if( remove( filename.c_str() ) ) {
            assert(false);
        }
    }
}

const CRandomFile &CRandomFile::operator << ( string &text ) {
    write( text.c_str(), static_cast<long>( text.length() ) );
    flush();
    return *this ;
}

#ifdef __REGTEST
//g++ RandomFile.cpp -D__REGTEST
int main(int argc, char* argv[]) {
    try {
        std::cerr << "start regression\n";
        CRandomFile* test = new CRandomFile();
        *test << "test";
        delete test;
        std::cerr << "stop regression\n";
        return 1;
    } catch(std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
#endif
