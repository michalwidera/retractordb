#include "rdb/faccfs.hpp"

#include <fcntl.h>

#include <cerrno>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <system_error>

#include <spdlog/spdlog.h>

#include "fatalError.hpp"
#include "rdb/storageFile.hpp"
namespace rdb {

genericBinaryFile::genericBinaryFile(  //
    const std::string_view fileName,   //
    const Descriptor &descriptor,      //
    int percounter,                    //
    const bool followFinalLink)        //
    : filename_(std::string(fileName)),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
      percounter_(percounter),
      followFinalLink_(followFinalLink) {}

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
  if (recordSize_ == 0) FatalError("genericBinaryFile::count: recordSize_ is zero");
  // Rozmiar pliku zamiast otwierania strumienia: ten sam kontrakt co w blizniakach
  // posixowych (ENOENT = magazyn pusty, kazdy inny blad zatrzymuje) i bez open/seek/close
  // na kazde wywolanie. Poprzednia postac nie sprawdzala, czy strumien sie otworzyl -
  // tellg() zwracalo wtedy -1, co dla recordSize_ > 1 obcinalo sie do zera przypadkiem,
  // a dla recordSize_ == 1 dawalo SIZE_MAX.
  std::error_code ec;
  const auto sizeInBytes = std::filesystem::file_size(filename_, ec);
  if (ec) {
    if (ec == std::errc::no_such_file_or_directory) return 0;
    FatalError("genericBinaryFile::count: file_size('{}') failed: {}", filename_, ec.message());
  }
  return static_cast<size_t>(sizeInBytes / static_cast<uintmax_t>(recordSize_));
}

ssize_t genericBinaryFile::write(const uint8_t *ptrData, const std::vector<bool> & /*nullBitset*/, const size_t position) {
  if (recordSize_ == 0) FatalError("genericBinaryFile::write: recordSize_ is zero");
  // Purge. Warunek wymagal dawniej takze recordSize_ == 0, czego nie da sie tu spelnic (FatalError
  // wyzej), wiec purge wpadal w zwykly zapis spod nullptr i po cichu nie robil nic.
  if (ptrData == nullptr && position == 0) {
    const StorageFd purged(filename_, O_WRONLY | O_CREAT | O_TRUNC, kStreamFileMode, followFinalLink_);
    return purged.isOpen() ? EXIT_SUCCESS : EIO;
  }
  const auto record = std::as_bytes(std::span{ptrData, static_cast<size_t>(recordSize_)});
  // Tryby jak dawniej w std::fstream: dopisanie tworzy plik ("a+"), zapis pod pozycje wymaga
  // istniejacego pliku ("r+").
  if (position == std::numeric_limits<size_t>::max()) {
    const StorageFd file(filename_, O_RDWR | O_CREAT | O_APPEND, kStreamFileMode, followFinalLink_);
    if (!file.isOpen() || !file.write(record)) return EIO;
  } else {
    const StorageFd file(filename_, O_RDWR, kStreamFileMode, followFinalLink_);
    if (!file.isOpen() || !file.writeAt(record, static_cast<off_t>(position))) return EIO;
  }
  return EXIT_SUCCESS;
}

ssize_t genericBinaryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  nullBitset.clear();
  if (recordSize_ == 0) FatalError("genericBinaryFile::read: recordSize_ is zero");
  const StorageFd file(filename_, O_RDONLY, kStreamFileMode, followFinalLink_);
  if (!file.isOpen()) return EIO;
  const ssize_t got =
      file.readAt(std::as_writable_bytes(std::span{ptrData, static_cast<size_t>(recordSize_)}), static_cast<off_t>(position));
  // Zero bajtow = pod ta pozycja nie ma rekordu; mniej niz rekord = rekord urwany w polowie.
  if (got == 0) return ERANGE;
  if (got != recordSize_) return EIO;
  return EXIT_SUCCESS;
}

}  // namespace rdb
