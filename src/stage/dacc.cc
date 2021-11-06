#include <fstream>
#include <assert.h>

#include "dacc.h"

namespace rdb {
    
    DataAccessor::DataAccessor(const Descriptor descriptor, BinaryFileAccessorInterface &accessor)
        : pAccessor(&accessor) , descriptor(descriptor)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        std::string fileDesc(pAccessor->fileName());
        fileDesc.append(".desc");

        myFile.open(fileDesc, std::ios::out);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile << descriptor;
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };

    void DataAccessor::Update(const std::byte *outBuffer, const uint offsetFromHead)
    {
        auto size = descriptor.getSize();
        pAccessor->update(outBuffer, size, offsetFromHead * size);
    };

    void DataAccessor::Get(std::byte *inBuffer, const uint offsetFromHead)
    {
        auto size = descriptor.getSize();
        pAccessor->read(inBuffer, size, offsetFromHead * size);
    };

    void DataAccessor::Put(const std::byte *outBuffer)
    {
        auto size = descriptor.getSize();
        pAccessor->append(outBuffer, size);
    };
}