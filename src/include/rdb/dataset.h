#ifndef STORAGE_RDB_INCLUDE_DATASET_H_
#define STORAGE_RDB_INCLUDE_DATASET_H_

#include <map>
#include <string>
#include <memory>  // unique_ptr

#include "rdb/dsacc.h"

namespace rdb
{
    struct dataSet
    {
        std::map < std::string , std::unique_ptr<rdb::DataStorageAccessor<std::byte>>> data;

        int recordSize = 0;

        dataSet(){}

        long streamStoredSize(std::string filename)
        {
            return data[filename]->recordsCount ;
        }

        long GetLen(std::string filename)
        {
            return data[filename]->recordsCount * recordSize;
        }

        void DefBlock(std::string filename, int frameSize)
        {

        }

        void PutBlock(std::string filename, char *pRawData)
        {

        }

        int GetBlock(std::string filename, int offset, char *pRawData)
        {
            return 0;
        }
    };
}

#endif // STORAGE_RDB_INCLUDE_DATASET_H_