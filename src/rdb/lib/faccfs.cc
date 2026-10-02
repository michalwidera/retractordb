#include "rdb/faccfs.hpp"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "rdb/exceptions.hpp"

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
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
      percounter_(percounter) {}

genericBinaryFile::~genericBinaryFile() {
  if (percounter_ >= 0) {
    std::string rotated_filename = filename_ + ".old" + std::to_string(percounter_);
    std::error_code ec;
    // Nadpisanie istniejacego archiwum zostawia slad w logu - uzasadnienie w faccposix.cc.
    const bool overwrites = std::filesystem::exists(rotated_filename, ec);
    std::filesystem::rename(filename_, rotated_filename, ec);
    if (!ec && overwrites)
      SPDLOG_ERROR("Rotation of {} overwrote existing archive {}; its previous content is lost", filename_, rotated_filename);
  }
}

auto genericBinaryFile::name() -> std::string & { return filename_; }

size_t genericBinaryFile::count() {
  if (recordSize_ == 0) throw LogicError("genericBinaryFile::count: recordSize_ is zero");
  // Rozmiar pliku zamiast otwierania strumienia: ten sam kontrakt co w blizniakach
  // posixowych (ENOENT = magazyn pusty, kazdy inny blad rzuca IOError) i bez open/seek/close
  // na kazde wywolanie. Poprzednia postac nie sprawdzala, czy strumien sie otworzyl -
  // tellg() zwracalo wtedy -1, co dla recordSize_ > 1 obcinalo sie do zera przypadkiem,
  // a dla recordSize_ == 1 dawalo SIZE_MAX.
  std::error_code ec;
  const auto sizeInBytes = std::filesystem::file_size(filename_, ec);
  if (ec) {
    if (ec == std::errc::no_such_file_or_directory) return 0;
    throw IOError(fmt::format("genericBinaryFile::count: file_size('{}') failed: {}", filename_, ec.message()));
  }
  return static_cast<size_t>(sizeInBytes / static_cast<uintmax_t>(recordSize_));
}

ssize_t genericBinaryFile::write(const uint8_t *ptrData, const std::vector<bool> & /*nullBitset*/, const size_t position) {
  if (recordSize_ == 0)
    throw LogicError("genericBinaryFile::write: recordSize_ is zero - accessor built on a zero-width descriptor");
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  // Purge. Warunek wymagal dawniej takze recordSize_ == 0, czego nie da sie tu spelnic (LogicError
  // wyzej), wiec purge wpadal w zwykly zapis spod nullptr i po cichu nie robil nic.
  if (ptrData == nullptr && position == 0) {
    myFile.open(filename_, std::ofstream::out | std::ofstream::trunc);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
    myFile.close();
    return EXIT_SUCCESS;
  }
  if (position == std::numeric_limits<size_t>::max()) {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
    // Note: no seekp here!
  } else {
    myFile.open(filename_, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
    myFile.seekp(static_cast<std::streamoff>(position));
    if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
  }
  myFile.write(reinterpret_cast<const char *>(ptrData), recordSize_);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
  myFile.close();
  return EXIT_SUCCESS;
}

ssize_t genericBinaryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  nullBitset.clear();
  if (recordSize_ == 0)
    throw LogicError("genericBinaryFile::read: recordSize_ is zero - accessor built on a zero-width descriptor");
  std::ifstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(filename_, std::ios::in | std::ios::binary);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
  myFile.seekg(static_cast<std::streamoff>(position));
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EIO;
  myFile.read(reinterpret_cast<char *>(ptrData), recordSize_);
  // Zero bajtow = pod ta pozycja nie ma rekordu; mniej niz rekord = rekord urwany w polowie.
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return myFile.gcount() == 0 ? ERANGE : EIO;
  myFile.close();
  return EXIT_SUCCESS;
}

}  // namespace rdb
