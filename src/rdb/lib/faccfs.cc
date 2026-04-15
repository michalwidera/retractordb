#include "rdb/faccfs.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
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

ssize_t genericBinaryFile::writeRaw(const uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  if (ptrData == nullptr && recordSize_ == 0 && position == 0) {
    myFile.open(filename_, std::ofstream::out | std::ofstream::trunc);
    assert((myFile.rdstate() & std::ofstream::failbit) == 0);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    myFile.close();
    return EXIT_SUCCESS;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
    assert((myFile.rdstate() & std::ofstream::failbit) == 0);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    // Note: no seekp here!
  } else {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    assert((myFile.rdstate() & std::ofstream::failbit) == 0);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
    myFile.seekp(position);
    assert((myFile.rdstate() & std::ofstream::failbit) == 0);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  }
  myFile.write(static_cast<const char *>(static_cast<const void *>(ptrData)), recordSize_);
  assert((myFile.rdstate() & std::ofstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.close();
  return EXIT_SUCCESS;
}

ssize_t genericBinaryFile::readRaw(uint8_t *ptrData, const size_t position) {
  assert(recordSize_ != 0);
  std::ifstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(filename_, std::ios::in | std::ios::binary);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.seekg(position);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  // This (+1) look's like error (need investigate)
  // During test posix interface have properly read size data
  // but fstream get read instead 12 bytes only 11
  // Therefore +1 appears.
  // Last byte was omitted.
  // Look's like some inconsistency is here.
  myFile.read(static_cast<char *>(static_cast<void *>(ptrData)), recordSize_);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.close();
  return EXIT_SUCCESS;
}

size_t genericBinaryFile::count() {
  std::ifstream in(filename_, std::ifstream::ate | std::ifstream::binary);
  return in.tellg() / recordSize_;
}

ssize_t genericBinaryFile::write(const uint8_t *ptrData, const size_t position, const std::vector<bool> &nullBitset) {
  return writeRaw(ptrData, position);
}

ssize_t genericBinaryFile::read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) {
  nullBitset.clear();
  return readRaw(ptrData, position);
}

}  // namespace rdb
