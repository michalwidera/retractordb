#include <fstream>
#include <assert.h>

#include "facc.h"

namespace rdb
{
    // https://courses.cs.vt.edu/~cs2604/fall02/binio.html
    // https://stackoverflow.com/questions/1658476/c-fopen-vs-open

    // Turn off buffering (this must apear before open)
    // http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

    // https://en.cppreference.com/w/cpp/io/ios_base/openmode
    // https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

    genericBinaryFileAccessor::genericBinaryFileAccessor(std::string fileName) : fileNameStr(fileName), BinaryFileAccessorInterface(fileName){};

    void genericBinaryFileAccessor::Append(const std::byte *ptrData, const uint size)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(FileName(), std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char *>(ptrData), size);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };

    void genericBinaryFileAccessor::Read(std::byte *ptrData, const uint size, const uint position)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(FileName(), std::ios::in | std::ios::binary);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.seekg(position);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.get(reinterpret_cast<char *>(ptrData), size);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        myFile.close();
    };

    void genericBinaryFileAccessor::Update(const std::byte *ptrData, const uint size, const uint position)
    {
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(FileName(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.seekp(position);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char *>(ptrData), size);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };

    std::string genericBinaryFileAccessor::FileName()
    {
        return fileNameStr;
    }

} // namespace rdb