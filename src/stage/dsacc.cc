#include <fstream>
#include <assert.h>

#include "dsacc.h"

namespace rdb
{
    template <class T>
    DataStorageAccessor<T>::DataStorageAccessor(const Descriptor descriptor, FileAccessorInterface<T> &accessor)
        : pAccessor(&accessor), descriptor(descriptor)
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
    };

    template <class T>
    void DataStorageAccessor<T>::Get(T *inBuffer, const size_t recordsFromHead)
    {
        auto size = descriptor.GetSize();
        auto result = pAccessor->Read(inBuffer, size, recordsFromHead * size);
        assert(result == 0);
    };

    template <class T>
    void DataStorageAccessor<T>::Put(const T *outBuffer, const size_t recordsFromHead)
    {
        auto size = descriptor.GetSize();

        if (recordsFromHead == std::numeric_limits<size_t>::max())
        {
            auto result = pAccessor->Write(outBuffer, size); // <- Call to Append Function
            assert(result == 0);
        }
        else
        {
            auto result = pAccessor->Write(outBuffer, size, recordsFromHead * size);
            assert(result == 0);
        }
    };

    template class DataStorageAccessor<std::byte>;
    template class DataStorageAccessor<char>;
    template class DataStorageAccessor<unsigned char>;
}