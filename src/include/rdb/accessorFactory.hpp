#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "metaData.hpp"

namespace rdb {

/// @brief Fabryka implementacji FileInterface oraz wariantu indeksu metadanych dla klasy storage.
///
/// Funkcje fabryki powinny:
/// - odwzorowywać nazwę typu magazynu (DEFAULT/DIRECT/MEMORY/POSIX/POSIXSHD/GENERIC/DEVICE/TEXTSOURCE)
///   na konkretną implementację FileInterface (makeAccessor()); nieznany typ, pusta ścieżka danych lub
///   pusty typ kończą się przez FatalError,
/// - rozstrzygać, czy typ magazynu jest źródłem deklarowanym tylko do odczytu (isDeclaredType()),
/// - dobierać wariant indeksu metadanych null (makeMetaIndex()): wariant inertny (pusta ścieżka pliku)
///   dla źródeł deklarowanych, storageShadow gdy accessor utrzymuje plik cienia danych, bazowy metaData
///   dla pozostałych zapisywalnych — posiadanie pliku cienia danych i posiadanie metaindeksu są
///   niezależnymi własnościami magazynu (cień chroni oryginalną zarejestrowaną zawartość danych,
///   metaindeks rejestruje wartości null i przerwy w transmisji),
/// - skupiać całą wiedzę o konkretnych typach akcesorów w jednym miejscu — storage zna wyłącznie
///   abstrakcyjny FileInterface.

/// @brief Whether the storage type is a read-only declared source (DEVICE/TEXTSOURCE).
[[nodiscard]] bool isDeclaredType(std::string_view storageType);

/// @brief Create the FileInterface implementation for the given storage type; FatalError on unknown type.
/// @note Descriptor jest nie-const, bo retention()/storagePolicy() nie są metodami const.
[[nodiscard]] std::unique_ptr<FileInterface> makeAccessor(std::string_view storageType,    //
                                                          const std::string &storageFile,  //
                                                          Descriptor &descriptor,          //
                                                          bool oneShot,                    //
                                                          int percounter);

/// @brief Create the metaData variant matching the storage: inert for declared sources,
///        storageShadow when the accessor keeps a data shadow file, base metaData otherwise.
[[nodiscard]] std::unique_ptr<metaData> makeMetaIndex(bool declared,                 //
                                                      bool hasShadow,                //
                                                      const Descriptor &descriptor,  //
                                                      const std::string &metaIndexFile);

}  // namespace rdb
