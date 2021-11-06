#ifndef STORAGE_RDB_INCLUDE_FACCPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPOSIX_H_

#include "fainterface.h"

namespace rdb
{
    /**
     * @brief Object that implements storage interface via fstream
     */
    struct posixBinaryFileAccessor : public FileAccessorInterface
    {
        std::string fileNameStr;

        posixBinaryFileAccessor(std::string fileName);

        void Read(std::byte *ptrData, const size_t size, const size_t position) override;
        void Write(const std::byte *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
        std::string FileName() override;

        posixBinaryFileAccessor() = delete;
        posixBinaryFileAccessor(const posixBinaryFileAccessor&) = delete;
        posixBinaryFileAccessor& operator=(const posixBinaryFileAccessor&) = delete;
    };
} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACCPOSIX_H_