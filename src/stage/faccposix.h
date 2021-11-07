#ifndef STORAGE_RDB_INCLUDE_FACCPOSIX_H_
#define STORAGE_RDB_INCLUDE_FACCPOSIX_H_

#include "fainterface.h"

namespace rdb
{
    /**
     * @brief Object that implements storage interface via posix calls
     */
    template<typename T>
    struct posixBinaryFileAccessor : public FileAccessorInterface<T>
    {
        std::string fileNameStr;

        posixBinaryFileAccessor(std::string fileName);

        void Read(T *ptrData, const size_t size, const size_t position) override;
        void Write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
        std::string FileName() override;

        posixBinaryFileAccessor() = delete;
        posixBinaryFileAccessor(const posixBinaryFileAccessor&) = delete;
        posixBinaryFileAccessor& operator=(const posixBinaryFileAccessor&) = delete;
    };
} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACCPOSIX_H_