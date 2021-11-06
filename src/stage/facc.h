#ifndef STORAGE_RDB_INCLUDE_FACC_H_
#define STORAGE_RDB_INCLUDE_FACC_H_

#include <string>
#include <cstddef> // std::byte

namespace rdb
{

    struct BinaryFileAccessorInterface
    {
        virtual void Append(const std::byte *ptrData, const size_t size) = 0;
        virtual void Read(std::byte *ptrData, const size_t size, const size_t position) = 0;
        virtual void Update(const std::byte *ptrData, const size_t size, const size_t position) = 0;
        virtual std::string FileName() = 0;

        virtual ~BinaryFileAccessorInterface(){};
        BinaryFileAccessorInterface(std::string fileName){};
    };

    struct genericBinaryFileAccessor : public BinaryFileAccessorInterface
    {
        std::string fileNameStr;

        genericBinaryFileAccessor(std::string fileName);

        void Append(const std::byte *ptrData, const size_t size) override;
        void Read(std::byte *ptrData, const size_t size, const size_t position) override;
        void Update(const std::byte *ptrData, const size_t size, const size_t position) override;
        std::string FileName() override;
    };

} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACC_H_