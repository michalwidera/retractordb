#include <fstream>
#include <assert.h>

#include <iostream>

#include "rdb/dsacc.h"

namespace rdb
{
    template <class T, class K>
    DataStorageAccessor<T, K>::DataStorageAccessor(std::string fileName, bool reverse)
        : pAccessor(new K(fileName)), reverse(reverse)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        std::string fileDesc(pAccessor->FileName());
        fileDesc.append(".desc");

        myFile.open(fileDesc, std::ios::in);
        if ( myFile.good() == true )
        {
            myFile >> descriptor;
        }
        myFile.close();

        recordsCount = 0;
        if ( descriptor.GetSize() > 0 )
        {
            std::ifstream in(fileName.c_str(), std::ifstream::ate | std::ifstream::binary);
            if ( in.good() )
            {
                recordsCount = int(in.tellg() / descriptor.GetSize());
            }
        }
    };

    template <class T, class K>
    DataStorageAccessor<T, K>::DataStorageAccessor(const Descriptor descriptor, std::string fileName, bool reverse)
        : pAccessor(new K(fileName)), descriptor(descriptor), reverse(reverse)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        std::string fileDesc(pAccessor->FileName());
        fileDesc.append(".desc");

        myFile.open(fileDesc, std::ios::out);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile << descriptor;
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();

        recordsCount = 0;
        std::ifstream in(fileName.c_str(), std::ifstream::ate | std::ifstream::binary);
        if ( ! in.fail() )
        {
            recordsCount = int(in.tellg() / descriptor.GetSize());
        }
    };

    template <class T, class K>
    void DataStorageAccessor<T, K>::Get(T *inBuffer, const size_t recordIndex)
    {
        auto size = descriptor.GetSize();
        auto result = pAccessor->Read(inBuffer, size, recordIndex * size);
        assert(result == 0);
    };

    template <class T, class K>
    void DataStorageAccessor<T, K>::Put(const T *outBuffer, const size_t recordIndex)
    {
        auto size = descriptor.GetSize();

        if (recordIndex == std::numeric_limits<size_t>::max())
        {
            auto result = pAccessor->Write(outBuffer, size); // <- Call to Append Function
            assert(result == 0);
        }
        else
        {
            auto result = pAccessor->Write(outBuffer, size, recordIndex * size);
            assert(result == 0);
        }
    };

    template class DataStorageAccessor<std::byte, rdb::genericBinaryFileAccessor<std::byte>>;
    template class DataStorageAccessor<char, rdb::genericBinaryFileAccessor<char>>;
    template class DataStorageAccessor<unsigned char, rdb::genericBinaryFileAccessor<unsigned char>>;
}