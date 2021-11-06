#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <string>
#include <cstddef> // std::byte

#include "facc.h"
#include "desc.h"

namespace rdb
{
    /**
     * @brief This object purpose is to access data via descriptor
     */
    struct DataAccessor
    {
        FileAccessorInterface *pAccessor;
        Descriptor descriptor;

        DataAccessor() = delete;

        /**
         * @brief Construct a new Data Accessor object
         * 
         * @param descriptor Definition of binary schema
         * @param accessor storage information
         */
        DataAccessor(const Descriptor descriptor, FileAccessorInterface &accessor);

        void Get(std::byte *inBuffer, const size_t offsetFromHead);
        void Put(const std::byte *outBuffer, const size_t offsetFromHead = std::numeric_limits<size_t>::max());
    };
}

#endif // STORAGE_RDB_INCLUDE_DACC_H_