#include "dbstream.h"
#include <assert.h>            // for assert
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <string.h>            // for memcpy, NULL
#include <algorithm>           // for max
#include <iostream>            // for basic_ostream::operator<<, operator<<
#include "Buffer.h"            // for CBuffer, BF

// stacktrace -> -ldl in CMakeList.txt
#include <boost/stacktrace.hpp>

CBuffer<std::string> cbuf ;

using namespace std ;
using namespace boost ;

void saveStreamsToFile(string filename) {
    cbuf.Save(filename.c_str());
}
long streamStoredSize(string filename) {
    return cbuf.GetLen(filename) ;
}
void Dump(std::ostream &os) {
    cbuf.Dump(os, NULL);
}

number fake = boost::rational<int> (99,1);

dbStream::dbStream(std::string streamName, list < std::string > schema) :
    streamName(streamName),
    schema(schema),
    mpReadNr(-1),
    mpLenNr(-1),
    frameSize(schema.size() * sizeof(number)),
    pRawData(new char[ frameSize ]) {
    cbuf.DefBlock(streamName, frameSize, BF);
    mpShadow.resize(schema.size());
    mpRead.resize(schema.size());
}

number  &dbStream::operator[](const int &_Keyval) {
    return mpShadow[ _Keyval ] ;
}

void dbStream::store() {
    char* p(pRawData.get());
    int cnt(0);

    for (auto &str : schema) {
        number fieldData(mpShadow[ cnt ]) ;
        memcpy(p, & fieldData, sizeof(number));
        p += sizeof(number) ;
        cnt ++;
    }

    cbuf.PutBlock(streamName, pRawData);
    mpLenNr = -1 ;
    mpReadNr = -1 ;
}

number dbStream::readCache(const int &_Keyval) {
    if (mpReadNr == -1) {
        get(0);
    }

    return mpRead[ _Keyval ] ;
}

void dbStream::get(int offset, bool reverse) {
    if (offset < 0) {
        std::cerr << boost::stacktrace::stacktrace();
        assert(false);
        return;
    }

    assert(offset >= 0) ;
    int len = cbuf.GetLen(streamName) ;

    if (mpReadNr == offset && mpLenNr == len) {
        return;
    }

    if (offset >= len || len == 0) {
        for (int i = 0 ; i < schema.size() ; i++) {
            mpRead[ i ] = fake ;
        }
//        cerr << "fake data @ streamName:" << streamName << " offset:" << offset << " len:" << len << endl;
//        TODO: There is problem. Need to investigate.
//        cerr << boost::stacktrace::stacktrace() ;
//        assert(false);
        return;
    }

    assert(cbuf.GetLen(streamName) != 0);
    long ret;
    if ( reverse ) {
        ret = cbuf.GetBlock(streamName, offset, pRawData);
    } else {
        ret = cbuf.GetBlock(streamName, len - offset - 1, pRawData);
    }


    if (ret == 0) {
        cerr << len << "," << offset << endl ;
    }

    assert(ret != 0);
    mpRead.clear();
    char* p(pRawData.get());

    for (int i = 0 ; i < schema.size() ; i++) {
        number fieldData ;
        memcpy(& fieldData, p, sizeof(number));
        mpRead[ i ] = fieldData ;
        p += sizeof(number) ;
    }

    assert(mpShadow.size() != 0);
    mpReadNr = offset ; /* position */
    mpLenNr  = len ;
    return;
}
