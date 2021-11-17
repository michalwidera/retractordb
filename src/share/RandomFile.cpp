#include "RandomFile.h"
#include <stdio.h>    // for remove
#include <stdlib.h>   // for getenv, NULL, mkstemp
#include <stdexcept>  // for out_of_range

//risk: Storing the pointer to the string returned by getenv() can result in overwritten environmental data.
//https://www.securecoding.cert.org/confluence/display/cplusplus/ENV00-CPP.+Do+not+store+the+pointer+to+the+string+returned+by+getenv()

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

CRandomFile::CRandomFile(string murExt, const bool destroyOnDestructor) :
    destroyOnDestructor(destroyOnDestructor)
{
    std::string TMP_PATH;

    if (getenv("TMP") != nullptr)
    {
        TMP_PATH = getenv("TMP") ;
    }
    else if (getenv("TEMP") != nullptr)
    {
        TMP_PATH = getenv("TEMP") ;
    }
    else if (getenv("HOME") != nullptr)
    {
        TMP_PATH = getenv("HOME") ;
    }
    else
    {
        TMP_PATH = "." ;
    }

    char randomFilename [MAX_PATH] = "/tmp/stream.XXXXXXXX";

    mkstemp(randomFilename) ;

    filename = randomFilename ;
    open(filename.c_str(), ios::in | ios::out | ios::binary) ;
}

CRandomFile::~CRandomFile()
{
    if (rdbuf()->is_open())
    {
        close();
    }

    if (destroyOnDestructor)
    {
        remove(filename.c_str());
    }
}
/*
const CRandomFile &CRandomFile::operator << (string &text)
{
    write(text.c_str(), static_cast<long>(text.length()));
    flush();
    return *this ;
}

*/