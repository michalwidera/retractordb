#include "dbstream.h"
#include <assert.h>            // for assert
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <string.h>            // for memcpy, NULL
#include <algorithm>           // for max
#include <iostream>            // for basic_ostream::operator<<, operator<<
#include "Buffer.h"            // for CBuffer, BF

#include "rdb/dataset.h"

// stacktrace -> -ldl in CMakeList.txt
#include <boost/stacktrace.hpp>

//#define OLD

#ifdef OLD
CBuffer<std::string> cbuf ;
#else
rdb::dataSet database;
#endif

using namespace std ;
using namespace boost ;

void saveStreamsToFile(string filename)
{
#ifdef OLD
cbuf.Save(filename.c_str());
#endif
    // no support here for data
}
long streamStoredSize(string filename)
{
#ifdef OLD
    return cbuf.GetLen(filename) ;
#else
    return database.streamStoredSize(filename);
#endif
}
void Dump(std::ostream &os)
{
    //cbuf.Dump(os, NULL);
}

dbStream::dbStream(std::string streamName, list < std::string > schema) :
    streamName(streamName),
    schema(schema),
    mpReadNr(-1),
    mpLenNr(-1),
    frameSize(schema.size() * sizeof(number)),
    pRawData(new char[ frameSize ])
{
#ifdef OLD
    cbuf.DefBlock(streamName, frameSize, BF);
#else
    database.DefBlock(streamName, frameSize);
#endif
    mpShadow.resize(schema.size());
    mpRead.resize(schema.size());
}

number  &dbStream::operator[](const int &_Keyval)
{
    return mpShadow[ _Keyval ] ;
}

void dbStream::store()
{
    char* p(pRawData.get());
    int cnt(0);

    for (auto &str : schema)
    {
        number fieldData(mpShadow[ cnt ]) ;
        memcpy(p, & fieldData, sizeof(number));
        p += sizeof(number) ;
        cnt ++;
    }

#ifdef OLD
cbuf.PutBlock(streamName, pRawData);
#else
    database.PutBlock(streamName,pRawData.get());
#endif
    mpLenNr = -1 ;
    mpReadNr = -1 ;
}

number dbStream::readCache(const int &_Keyval)
{
    if (mpReadNr == -1)
    {
        get(0);
    }

    return mpRead[ _Keyval ] ;
}

void dbStream::get(int offset, bool reverse)
{
    if (offset < 0)
    {
        std::cerr << boost::stacktrace::stacktrace();
        assert(false);
        return;
    }

    assert(offset >= 0) ;
#ifdef OLD
    int len = cbuf.GetLen(streamName) ;
#else
    int len = database.GetLen(streamName);
#endif
    if (mpReadNr == offset && mpLenNr == len)
    {
        return;
    }

    if (offset > len || len == 0)
    {
        const number fake = boost::rational<int> (999, 1);

        for (auto i = 0 ; i < schema.size() ; i++)
        {
            mpRead[i] = fake ;
        }

//      cerr << "fake data @ streamName:" << streamName << " offset:" << offset << " len:" << len << endl;
//        TODO: There is problem. Need to investigate.
//        cerr << boost::stacktrace::stacktrace() ;
//        assert(false);
        return;
    }

    assert(len != 0);
    long ret;

    if (reverse)
    {
#ifdef OLD
        ret = cbuf.GetBlock(streamName, offset, pRawData);
#else
        ret = database.GetBlock(streamName, offset, pRawData.get()) ;
#endif
    }
    else
    {
#ifdef OLD
        ret = cbuf.GetBlock(streamName, len - offset - 1, pRawData);
#else
        database.reverse(streamName, true);
        database.GetBlock(streamName, offset, pRawData.get());
        database.reverse(streamName, false);
#endif
    }

    assert(ret != 0);
    mpRead.clear();
    char* p(pRawData.get());

    for (int i = 0 ; i < schema.size() ; i++)
    {
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