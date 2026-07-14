#include "rdb/accessorFactory.hpp"

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

bool isDeclaredType(const std::string_view storageType) { return (storageType == "DEVICE") || (storageType == "TEXTSOURCE"); }

std::unique_ptr<FileInterface> makeAccessor(const std::string_view storageType,  //
                                            const std::string &storageFile,      //
                                            Descriptor &descriptor,              //
                                            const bool oneShot,                  //
                                            const int percounter) {
  if (storageFile.empty()) FatalError("storage: storage file path is empty — storage not properly configured");
  if (storageType.empty()) FatalError("storage: storage type is empty — storage type not set");

  if (storageType == "DEFAULT") {
    return std::make_unique<rdb::groupFile<posixBinaryFileWithShadow>>(storageFile, descriptor, descriptor.retention(),
                                                                       percounter);
  }
  if (storageType == "DIRECT") {
    return std::make_unique<rdb::groupFile<posixBinaryFile>>(storageFile, descriptor, descriptor.retention(), percounter);
  }
  if (storageType == "MEMORY") {
    return std::make_unique<rdb::memoryFile>(storageFile, descriptor, descriptor.storagePolicy());
  }
  if (storageType == "POSIX") {
    return std::make_unique<rdb::posixBinaryFile>(storageFile, descriptor, percounter);
  }
  if (storageType == "POSIXSHD") {
    return std::make_unique<rdb::posixBinaryFileWithShadow>(storageFile, descriptor, percounter);
  }
  if (storageType == "GENERIC") {
    return std::make_unique<rdb::genericBinaryFile>(storageFile, descriptor, percounter);
  }
  if (storageType == "DEVICE") {
    return std::make_unique<rdb::binaryDeviceRO>(storageFile, descriptor, !oneShot);
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
    // Źródła deklarowane są tylko do odczytu — wstrzykiwany jest wariant inertny (pusta ścieżka = bez persystencji).
    return std::make_unique<rdb::metaData>(descriptor, "");
  }

  // Posiadanie pliku cienia danych (hasShadow) jest niezależne od posiadania metaindeksu: cień chroni
  // oryginalną zarejestrowaną zawartość danych, metaindeks rejestruje wartości null i przerwy w transmisji.
  // Magazyny z cieniem danych dostają storageShadow (aktualizacje → cień indeksu .meta.shadow); pozostałe —
  // bazowy metaData.
  if (hasShadow) {
    return std::make_unique<rdb::storageShadow>(descriptor, metaIndexFile);
  }
  return std::make_unique<rdb::metaData>(descriptor, metaIndexFile);
}

}  // namespace rdb
