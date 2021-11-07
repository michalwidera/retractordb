#ifndef STORAGE_RDB_INCLUDE_FACCFS_H_
#define STORAGE_RDB_INCLUDE_FACCFS_H_

#include "fainterface.h"

namespace rdb
{
    /**
     * @brief Object that implements storage interface via fstream
     */
    struct genericBinaryFileAccessor : public FileAccessorInterface<std::byte>
    {
        std::string fileNameStr;

        genericBinaryFileAccessor(std::string fileName);

        void Read(std::byte *ptrData, const size_t size, const size_t position) override;
        void Write(const std::byte *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
        std::string FileName() override;

        genericBinaryFileAccessor() = delete;
        genericBinaryFileAccessor(const genericBinaryFileAccessor&) = delete;
        genericBinaryFileAccessor& operator=(const genericBinaryFileAccessor&) = delete;
    };

} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACCFS_H_