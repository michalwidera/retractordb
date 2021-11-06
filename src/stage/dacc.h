#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <string>
#include <cstddef> // std::byte

#include "fainterface.h"
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

        /**
         * @brief Reads data package from storage
         * 
         * @param inBuffer pointer to area where package will be fetched
         * @param recordsFromHead location from beginging of the storage [unit: Records]
         */
        void Get(std::byte *inBuffer, const size_t recordsFromHead);

        /**
         * @brief Sends record to the storage
         * 
         * @param outBuffer pointer to area when record is stored
         * @param recordsFromHead location from begining of the storage [unit: Records]
         */
        void Put(const std::byte *outBuffer, const size_t recordsFromHead = std::numeric_limits<size_t>::max());
    };
}

#endif // STORAGE_RDB_INCLUDE_DACC_H_