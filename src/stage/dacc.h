#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <string>
#include <cstddef> // std::byte

#include "facc.h"
#include "desc.h"

namespace rdb
{
    struct DataAccessor
    {
        BinaryFileAccessorInterface *pAccessor;
        Descriptor descriptor;

        DataAccessor(const Descriptor descriptor, BinaryFileAccessorInterface &accessor);
        void Update(const std::byte *outBuffer, const uint offsetFromHead);
        void Get(std::byte *inBuffer, const uint offsetFromHead);
        void Put(const std::byte *outBuffer);
    };
}

#endif // STORAGE_RDB_INCLUDE_DACC_H_