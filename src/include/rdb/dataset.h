#ifndef STORAGE_RDB_INCLUDE_DATASET_H_
#define STORAGE_RDB_INCLUDE_DATASET_H_

#include <map>
#include <string>
#include <memory>  // unique_ptr

#include "rdb/dsacc.h"

namespace rdb
{
    /**
     * @brief This is temporary class required to seamless kick off CBuffer crap.
     *
     */
    struct dataSet
    {
        std::map < std::string , std::unique_ptr<rdb::DataStorageAccessor<std::byte>>> data;

        int recordSize = 0;
        int count = 0;

        dataSet(){}

        long streamStoredSize(std::string filename);
        long GetLen(std::string filename);
        void DefBlock(std::string filename, int frameSize);
        void PutBlock(std::string filename, char *pRawData);
        int GetBlock(std::string filename, int offset, char *pRawData);
        void reverse(std::string filename, bool val);
    };
}

#endif // STORAGE_RDB_INCLUDE_DATASET_H_