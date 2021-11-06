#include <fstream>
#include <assert.h>

#include "dacc.h"

namespace rdb
{

    DataAccessor::DataAccessor(const Descriptor descriptor, BinaryFileAccessorInterface &accessor)
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

    void DataAccessor::Update(const std::byte *outBuffer, const size_t offsetFromHead)
    {
        auto size = descriptor.GetSize();
        pAccessor->Update(outBuffer, size, offsetFromHead * size);
    };

    void DataAccessor::Get(std::byte *inBuffer, const size_t offsetFromHead)
    {
        auto size = descriptor.GetSize();
        pAccessor->Read(inBuffer, size, offsetFromHead * size);
    };

    void DataAccessor::Put(const std::byte *outBuffer)
    {
        auto size = descriptor.GetSize();
        pAccessor->Append(outBuffer, size);
    };
}