#ifndef STORAGE_RDB_INCLUDE_FACC_H_
#define STORAGE_RDB_INCLUDE_FACC_H_

#include <string>
#include <cstddef> // std::byte

namespace rdb
{

    struct BinaryFileAccessorInterface
    {
        virtual void append(const std::byte *ptrData, const uint size) = 0;
        virtual void read(std::byte *ptrData, const uint size, const uint position) = 0;
        virtual void update(const std::byte *ptrData, const uint size, const uint position) = 0;
        virtual std::string fileName() = 0;

        virtual ~BinaryFileAccessorInterface(){};
        BinaryFileAccessorInterface(std::string fileName){};
    };

    struct genericBinaryFileAccessor : public BinaryFileAccessorInterface
    {
        std::string fileNameStr;

        genericBinaryFileAccessor(std::string fileName);

        void append(const std::byte *ptrData, const uint size) override;
        void read(std::byte *ptrData, const uint size, const uint position) override;
        void update(const std::byte *ptrData, const uint size, const uint position) override;
        std::string fileName() override;
    };

} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_FACC_H_