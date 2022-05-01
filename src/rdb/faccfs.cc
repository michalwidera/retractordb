#include <fstream>
#include <cassert>
#include <limits>
#include <cstddef> // std::byte

#include "rdb/faccfs.h"

namespace rdb
{
    // https://courses.cs.vt.edu/~cs2604/fall02/binio.html
    // https://stackoverflow.com/questions/1658476/c-fopen-vs-open

    // Turn off buffering (this must appear before open)
    // http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

    // https://en.cppreference.com/w/cpp/io/ios_base/openmode
    // https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

    template <class T> genericBinaryFileAccessor<T>::genericBinaryFileAccessor(std::string fileName) : fileNameStr(fileName) {};

    template <class T>
    std::string genericBinaryFileAccessor<T>::FileName()
    {
        return fileNameStr;
    }

    template <class T>
    int genericBinaryFileAccessor<T>::Write(const T* ptrData, const size_t size, const size_t position)
    {
        std::fstream myFile;
        myFile.rdbuf()->pubsetbuf(0, 0);
        if (position == std::numeric_limits<size_t>::max()) {
            myFile.open(FileName(), std::ios::in | std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
            assert((myFile.rdstate() & std::ofstream::failbit) == 0);
            if ((myFile.rdstate() & std::ofstream::failbit) != 0)
                return EXIT_FAILURE;
            // Note: no seekp here!
        } else {
            myFile.open(FileName(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
            assert((myFile.rdstate() & std::ofstream::failbit) == 0);
            if ((myFile.rdstate() & std::ofstream::failbit) != 0)
                return EXIT_FAILURE;
            myFile.seekp(position);
            assert((myFile.rdstate() & std::ofstream::failbit) == 0);
            if ((myFile.rdstate() & std::ofstream::failbit) != 0)
                return EXIT_FAILURE;
        }
        myFile.write(reinterpret_cast<const char*>(ptrData), size);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        if ((myFile.rdstate() & std::ofstream::failbit) != 0)
            return EXIT_FAILURE;
        myFile.close();
        return EXIT_SUCCESS;
    };

    template <class T>
    int genericBinaryFileAccessor<T>::Read(T* ptrData, const size_t size, const size_t position)
    {
        std::fstream myFile;
        myFile.rdbuf()->pubsetbuf(0, 0);
        myFile.open(FileName(), std::ios::in | std::ios::binary);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        if ((myFile.rdstate() & std::ofstream::failbit) != 0)
            return EXIT_FAILURE;
        myFile.seekg(position);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        if ((myFile.rdstate() & std::ofstream::failbit) != 0)
            return EXIT_FAILURE;
        // This (+1) look's like error (need investigate)
        // During test posix interface have properly read size data
        // but fstream get read instead 12 bytes only 11
        // Therefore +1 appears.
        // Last byte was omitted.
        // Look's like some inconsistency is here.
        myFile.get(reinterpret_cast<char*>(ptrData), size + 1);
        assert((myFile.rdstate() & std::ifstream::failbit) == 0);
        if ((myFile.rdstate() & std::ofstream::failbit) != 0)
            return EXIT_FAILURE;
        myFile.close();
        return EXIT_SUCCESS;
    };

    template class genericBinaryFileAccessor<std::byte>;
    template class genericBinaryFileAccessor<char>;
    template class genericBinaryFileAccessor<unsigned char>;

} // namespace rdb