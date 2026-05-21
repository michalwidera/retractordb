#include "rdb/faccfs.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include "fatalError.hpp"
namespace rdb {
// https://courses.cs.vt.edu/~cs2604/fall02/binio.html
// https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must appear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

// https://en.cppreference.com/w/cpp/io/ios_base/openmode
// https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

genericBinaryFile::genericBinaryFile(  //
    const std::string_view fileName,   //
    const Descriptor &descriptor,      //
    int percounter)                    //
    : filename_(std::string(fileName)),
      recordSize_(descriptor.getSizeInBytes()),
      percounter_(percounter) {}

genericBinaryFile::~genericBinaryFile() {
  if (percounter_ >= 0) {
    std::string rotated_filename = filename_ + ".old" + std::to_string(percounter_);
    std::error_code ec;
    std::filesystem::rename(filename_, rotated_filename, ec);
  }
}

auto genericBinaryFile::name() -> std::string & { return filename_; }

size_t genericBinaryFile::count() {
  std::ifstream in(filename_, std::ifstream::ate | std::ifstream::binary);
  return in.tellg() / recordSize_;
}

ssize_t genericBinaryFile::write(const uint8_t *ptrData, const std::vector<bool> & /*nullBitset*/, const size_t position) {
  if (recordSize_ == 0) FatalError("genericBinaryFile::write: recordSize_ is zero");
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  if (ptrData == nullptr && recordSize_ == 0 && position == 0) {
    myFile.open(filename_, std::ofstream::out | std::ofstream::trunc);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    myFile.close();
    return EXIT_SUCCESS;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    // Note: no seekp here!
  } else {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    myFile.seekp(position);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  }
  myFile.write(static_cast<const char *>(static_cast<const void *>(ptrData)), recordSize_);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.close();
  return EXIT_SUCCESS;
}

ssize_t genericBinaryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  nullBitset.clear();
  if (recordSize_ == 0) FatalError("genericBinaryFile::read: recordSize_ is zero");
  std::ifstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(filename_, std::ios::in | std::ios::binary);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.seekg(position);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.read(static_cast<char *>(static_cast<void *>(ptrData)), recordSize_);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.close();
  return EXIT_SUCCESS;
}

}  // namespace rdb
