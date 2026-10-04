#include "rdb/accessorFactory.hpp"

#include <sys/stat.h>

#include <format>

#include "fatalError.hpp"
#include "rdb/faccbindev.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccmemory.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/faccposixshd.hpp"
#include "rdb/facctxtsrc.hpp"
#include "rdb/fagrp.hpp"
#include "rdb/storageShadow.hpp"

namespace rdb {

bool isDeclaredType(const std::string_view storageType) {
  return (storageType == "BINFILE") || (storageType == "DEVICE") || (storageType == "TEXTSOURCE");
}

bool isWritableType(const std::string_view storageType) {
  return (storageType == "DEFAULT") || (storageType == "DIRECT") || (storageType == "MEMORY") || (storageType == "POSIX") ||
         (storageType == "POSIXSHD") || (storageType == "GENERIC");
}

namespace {
std::string_view fileKindName(const mode_t mode) {
  if (S_ISREG(mode)) return "a regular file";
  if (S_ISDIR(mode)) return "a directory";
  if (S_ISCHR(mode)) return "a character device";
  if (S_ISBLK(mode)) return "a block device";
  if (S_ISFIFO(mode)) return "a FIFO";
  if (S_ISSOCK(mode)) return "a socket";
  return "an unknown file type";
}
}  // namespace

std::string sourceKindMismatch(const std::string_view storageType, const std::string &path, const mode_t mode) {
  if (storageType == "DEVICE") {
    if (S_ISCHR(mode) || S_ISFIFO(mode)) return {};
    return std::format("DEVICE '{}' is {}, not a character device or FIFO", path, fileKindName(mode));
  }
  if (S_ISREG(mode)) return {};
  const std::string_view keyword = (storageType == "TEXTSOURCE") ? "TEXTFILE" : storageType;
  return std::format("{} '{}' is {}, not a regular file", keyword, path, fileKindName(mode));
}

std::string sourceKindMismatch(const std::string_view storageType, const std::string &path) {
  struct stat sourceStat{};
  if (::stat(path.c_str(), &sourceStat) != 0) return {};
  return sourceKindMismatch(storageType, path, sourceStat.st_mode);
}

std::unique_ptr<FileInterface> makeAccessor(const std::string_view storageType,  //
                                            const std::string &storageFile,      //
                                            Descriptor &descriptor,              //
                                            const bool oneShot,                  //
                                            const int percounter,                //
                                            const bool followFinalLink) {
  if (storageFile.empty()) FatalError("storage: storage file path is empty - storage not properly configured");
  if (storageType.empty()) FatalError("storage: storage type is empty - storage type not set");

  if (storageType == "DEFAULT") {
    return std::make_unique<rdb::groupFile<posixBinaryFileWithShadow>>(storageFile, descriptor, descriptor.retention(),
                                                                       percounter, followFinalLink);
  }
  if (storageType == "DIRECT") {
    return std::make_unique<rdb::groupFile<posixBinaryFile>>(storageFile, descriptor, descriptor.retention(), percounter,
                                                             followFinalLink);
  }
  if (storageType == "MEMORY") {
    return std::make_unique<rdb::memoryFile>(storageFile, descriptor, descriptor.storagePolicy());
  }
  if (storageType == "POSIX") {
    return std::make_unique<rdb::posixBinaryFile>(storageFile, descriptor, percounter, followFinalLink);
  }
  if (storageType == "POSIXSHD") {
    return std::make_unique<rdb::posixBinaryFileWithShadow>(storageFile, descriptor, percounter, followFinalLink);
  }
  if (storageType == "GENERIC") {
    return std::make_unique<rdb::genericBinaryFile>(storageFile, descriptor, percounter, followFinalLink);
  }
  if (storageType == "BINFILE" || storageType == "DEVICE") {
    return std::make_unique<rdb::binaryDeviceRO>(storageFile, descriptor, !oneShot, storageType);
  }
  if (storageType == "TEXTSOURCE") {
    return std::make_unique<rdb::textSourceRO>(storageFile, descriptor, !oneShot);
  }
  FatalError("storage: unsupported storage type '{}'", storageType);
}

std::unique_ptr<metaData> makeMetaIndex(const bool declared,           //
                                        const bool hasShadow,          //
                                        const Descriptor &descriptor,  //
                                        const std::string &metaIndexFile) {
  if (declared) {
    // Źródła deklarowane są tylko do odczytu - wstrzykiwany jest wariant inertny (pusta ścieżka = bez persystencji).
    return std::make_unique<rdb::metaData>(descriptor, "");
  }

  // Posiadanie pliku cienia danych (hasShadow) jest niezależne od posiadania metaindeksu: cień chroni
  // oryginalną zarejestrowaną zawartość danych, metaindeks rejestruje wartości null i przerwy w transmisji.
  // Magazyny z cieniem danych dostają storageShadow (aktualizacje → cień indeksu .meta.shadow); pozostałe -
  // bazowy metaData.
  if (hasShadow) {
    return std::make_unique<rdb::storageShadow>(descriptor, metaIndexFile);
  }
  return std::make_unique<rdb::metaData>(descriptor, metaIndexFile);
}

}  // namespace rdb
