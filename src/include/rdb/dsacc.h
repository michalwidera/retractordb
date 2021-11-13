#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <string>
#include <cstddef> // std::byte
#include <memory>  // std::unique_ptr

#include "fainterface.h"
#include "desc.h"

#include "faccfs.h"
#include "faccposix.h"
#include "faccposixprm.h"

namespace rdb
{
    /**
     * @brief This object purpose is to access data via descriptor
     */
    template <class T = std::byte, class K = rdb::posixPrmBinaryFileAccessor<T> >
    struct DataStorageAccessor
    {
        std::unique_ptr<K> pAccessor;

        Descriptor descriptor;

        bool reverse;

        size_t recordsCount;

        DataStorageAccessor() = delete;

        /**
         * @brief Construct a new Data Accessor object and create descriptor file
         *
         * @param descriptor Definition of binary schema
         * @param fileName Storage file
         * @param reverse type of Get/Set operations index - from head or from tail
         */
        DataStorageAccessor(const Descriptor descriptor, std::string fileName, bool reverse = false);

        /**
         * @brief Open existing Data Accessor object and check descriptor file
         *
         * @param fileName Storage file
         * @param reverse type of Get/Set operations index - from head or from tail
         */
        DataStorageAccessor(std::string fileName, bool reverse = false);

        /**
         * @brief Reads data package from storage
         *
         * @param inBuffer pointer to area where package will be fetched
         * @param recordIndex location from beginging of the storage [unit: Records]
         * @return success status - true eq. success
         */
        bool Get(T *inBuffer, const size_t recordIndex);

        /**
         * @brief Sends record to the storage
         *
         * @param outBuffer pointer to area when record is stored
         * @param recordIndex location from begining of the storage [unit: Records]
         * @return success status- true eq. success
         */
        bool Put(const T *outBuffer, const size_t recordIndex = std::numeric_limits<size_t>::max());
    };
}

#endif // STORAGE_RDB_INCLUDE_DACC_H_