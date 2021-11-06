#ifndef STORAGE_RDB_INCLUDE_FACC_H_
#define STORAGE_RDB_INCLUDE_FACC_H_

#include <string>
#include <cstddef> // std::byte

namespace rdb
{

    /**
     * @brief This is interface for accessor interface.
     * 
     * There is only 3 opeations over file/object/blob aka storage
     * Append data at the end of storage
     * Read data from the storage
     * Update data in the middle of storage
     */
    struct FileAccessorInterface
    {
        /**
         * @brief Function adds bytes from memory of given size into the storage
         * 
         * @param ptrData pointer to table of bytes in memory that will be stored in storage
         * @param size size of data to be appened
         */
        virtual void Append(const std::byte *ptrData, const size_t size) = 0;

        /**
         * @brief Reads from storage amount of bytes into memory pointed by ptrData from position in storage
         * 
         * @param ptrData pointer to data in memory where data will be feteched from storage
         * @param size size of data that will be transfered
         * @param position position as size * position in bytes
         */
        virtual void Read(std::byte *ptrData, const size_t size, const size_t position) = 0;

        /**
         * @brief Updates data in the storage
         * 
         * @param ptrData pointer to table of bytes in memory that will be updated in storage
         * @param size  size of data to be updated
         * @param position position as size * position in bytes
         */
        virtual void Update(const std::byte *ptrData, const size_t size, const size_t position) = 0;

        /**
         * @brief gets name of storage (file name)
         * 
         * @return std::string filename
         */
        virtual std::string FileName() = 0;

        virtual ~FileAccessorInterface(){};
    };

    /**
     * @brief Object that implements accessor interface
     * 
     * This object reads files from disk via fstream
     */
    struct genericBinaryFileAccessor : public FileAccessorInterface
    {
        std::string fileNameStr;

        genericBinaryFileAccessor(std::string fileName);

        void Append(const std::byte *ptrData, const size_t size) override;
        void Read(std::byte *ptrData, const size_t size, const size_t position) override;
        void Update(const std::byte *ptrData, const size_t size, const size_t position) override;
        std::string FileName() override;

        genericBinaryFileAccessor() = delete;
        genericBinaryFileAccessor(const genericBinaryFileAccessor&) = delete;
        genericBinaryFileAccessor& operator=(const genericBinaryFileAccessor&) = delete;
    };

} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACC_H_