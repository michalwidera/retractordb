#include <fstream>
#include <assert.h>

#include "facc.h"

namespace rdb
{

    // https://en.cppreference.com/w/cpp/io/ios_base/openmode
    // https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

    genericBinaryFileAccessor::genericBinaryFileAccessor(std::string fileName) : fileNameStr(fileName), BinaryFileAccessorInterface(fileName){};

    void genericBinaryFileAccessor::append(const std::byte *ptrData, const uint size)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char *>(ptrData), size);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };

    void genericBinaryFileAccessor::read(std::byte *ptrData, const uint size, const uint position)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), std::ios::in | std::ios::binary);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.seekg(position);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.get(reinterpret_cast<char *>(ptrData), size);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.close();
    };

    void genericBinaryFileAccessor::update(const std::byte *ptrData, const uint size, const uint position)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.seekp(position);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char *>(ptrData), size);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };

    std::string genericBinaryFileAccessor::fileName()
    {
        return fileNameStr;
    }

} // namespace rdb