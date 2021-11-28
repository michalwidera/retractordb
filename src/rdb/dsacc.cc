#include <fstream>
#include <cassert>

#include <iostream>

#include "rdb/dsacc.h"

namespace rdb
{
    template <class T, class K>
    DataStorageAccessor<T, K>::DataStorageAccessor(std::string fileName, bool reverse)
        : pAccessor(new K(fileName)), reverse(reverse), filename(fileName), removeOnExit(true)
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
        : pAccessor(new K(fileName)), descriptor(descriptor), reverse(reverse), filename(fileName), removeOnExit(true)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        std::string fileDesc(pAccessor->FileName());
        fileDesc.append(".desc");

        // Creating .desc file

        myFile.open(fileDesc, std::ios::out);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile << descriptor;
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
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
    DataStorageAccessor<T, K>::~DataStorageAccessor()
    {
        if ( removeOnExit )
        {
            auto statusRemove1 = remove(filename.c_str());
            auto descFilename(filename + ".desc");
            auto statusRemove2 = remove(descFilename.c_str());
        }
    };

    template <class T, class K>
    bool DataStorageAccessor<T, K>::Get(T *inBuffer, const size_t recordIndex)
    {
        auto size = descriptor.GetSize();
        auto recordIndexRv = reverse ? ( recordsCount - 1 ) - recordIndex : recordIndex;
        int result = 0;

        if ( recordsCount > 0 && recordIndex < recordsCount )
        {
            result = pAccessor->Read(inBuffer, size, recordIndexRv * size);
            assert(result == 0);
        }
        return result == 0;
    };

    template <class T, class K>
    Descriptor &DataStorageAccessor<T, K>::getDescriptor()
    {
        return descriptor;
    }

    template <class T, class K>
    void DataStorageAccessor<T, K>::setReverse( bool value )
    {
        reverse = value;
    }

    template <class T, class K>
    void DataStorageAccessor<T, K>::setRemoveOnExit( bool value )
    {
        removeOnExit = value;
    }

    template <class T, class K>
    const size_t DataStorageAccessor<T, K>::getRecordsCount()
    {
        return recordsCount;
    }


    template <class T, class K>
    bool DataStorageAccessor<T, K>::Put(const T *outBuffer, const size_t recordIndex)
    {
        auto size = descriptor.GetSize();
        int result = 0;
        if (recordIndex == std::numeric_limits<size_t>::max())
        {
            result = pAccessor->Write(outBuffer, size); // <- Call to Append Function
            assert(result == 0);
            if ( result == 0 )
            {
                recordsCount ++;
            }
        }
        else
        {
            if ( recordsCount > 0 && recordIndex < recordsCount )
            {
                auto recordIndexRv = reverse ? ( recordsCount - 1 ) - recordIndex : recordIndex;
                result = pAccessor->Write(outBuffer, size, recordIndexRv * size);
                assert(result == 0);
            }
        }
        return result == 0;
    };

    template class DataStorageAccessor<std::byte, rdb::genericBinaryFileAccessor<std::byte>>;
    template class DataStorageAccessor<char, rdb::genericBinaryFileAccessor<char>>;
    template class DataStorageAccessor<unsigned char, rdb::genericBinaryFileAccessor<unsigned char>>;

    template class DataStorageAccessor<std::byte, rdb::posixBinaryFileAccessor<std::byte>>;
    template class DataStorageAccessor<char, rdb::posixBinaryFileAccessor<char>>;
    template class DataStorageAccessor<unsigned char, rdb::posixBinaryFileAccessor<unsigned char>>;

    template class DataStorageAccessor<std::byte, rdb::posixPrmBinaryFileAccessor<std::byte>>;
    template class DataStorageAccessor<char, rdb::posixPrmBinaryFileAccessor<char>>;
    template class DataStorageAccessor<unsigned char, rdb::posixPrmBinaryFileAccessor<unsigned char>>;
}