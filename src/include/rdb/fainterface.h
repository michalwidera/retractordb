#ifndef STORAGE_RDB_INCLUDE_FAINTERFACE_H_
#define STORAGE_RDB_INCLUDE_FAINTERFACE_H_

#include <string>

namespace rdb
{
    /**
     * @brief This is interface for accessor interface.
     *
     * There is only 3 opeations over file/object/blob aka storage
     * Append data at the end of storage
     * Read data from the storage
     * Update data in the middle of storage
     * This three has been covered by following interface
     * Read ::= Read(data, position)
     * Append :== Write(data, position == max_possible_value )
     * Update :== Write(data, position)
     */
    template<typename T>
    struct FileAccessorInterface
    {
        /**
         * @brief Reads from storage amount of bytes into memory pointed by ptrData from position in storage
         *
         * @param ptrData pointer to data in memory where data will be feteched from storage
         * @param size size of data that will be transfered
         * @param position position from the begining of file [unit: Bytes]
         * @return status of operation - 0/EXIT_SUCCESS success
         */
        virtual int Read(T *ptrData, const size_t size, const size_t position) = 0;

        /**
         * @brief Updates or appends data in the storage
         *
         * @param ptrData pointer to table of bytes in memory that will be updated in storage
         * @param size  size of data to be updated
         * @param position position from the begining of file [unit: Bytes]. If max possible value - works as append.
         * @return status of operation - 0/EXIT_SUCCESS success
         */
        virtual int Write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) = 0;

        /**
         * @brief gets name of storage (file name)
         *
         * @return std::string filename
         */
        virtual std::string FileName() = 0;

        virtual ~FileAccessorInterface(){};
    };
} // rdb

#endif // STORAGE_RDB_INCLUDE_FAINTERFACE_H_