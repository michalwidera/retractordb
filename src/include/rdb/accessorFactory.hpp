#pragma once

#include <sys/types.h>  // mode_t

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
/// - odwzorowywać nazwę typu magazynu (DEFAULT/DIRECT/MEMORY/POSIX/POSIXSHD/GENERIC/BINFILE/DEVICE/TEXTSOURCE)
///   na konkretną implementację FileInterface (makeAccessor()); nieznany typ, pusta ścieżka danych lub
///   pusty typ kończą się przez FatalError,
/// - rozstrzygać, czy typ magazynu jest źródłem deklarowanym tylko do odczytu (isDeclaredType()),
/// - dobierać wariant indeksu metadanych null (makeMetaIndex()): wariant inertny (pusta ścieżka pliku)
///   dla źródeł deklarowanych, storageShadow gdy accessor utrzymuje plik cienia danych, bazowy metaData
///   dla pozostałych zapisywalnych - posiadanie pliku cienia danych i posiadanie metaindeksu są
///   niezależnymi własnościami magazynu (cień chroni oryginalną zarejestrowaną zawartość danych,
///   metaindeks rejestruje wartości null i przerwy w transmisji),
/// - skupiać całą wiedzę o konkretnych typach akcesorów w jednym miejscu - storage zna wyłącznie
///   abstrakcyjny FileInterface.

/// @brief Whether the storage type is a read-only declared source (BINFILE/DEVICE/TEXTSOURCE).
[[nodiscard]] bool isDeclaredType(std::string_view storageType);

/// @brief Whether the storage type is a writable SELECT output profile (DEFAULT/DIRECT/MEMORY/POSIX/POSIXSHD/GENERIC).
[[nodiscard]] bool isWritableType(std::string_view storageType);

/// @brief Odmowa rodzaju pliku zrodla deklarowanego (#346); pusta, gdy rodzaj pliku pasuje do typu.
///
/// BINFILE i TEXTSOURCE (slowo RQL TEXTFILE) przyjmuja wylacznie plik zwykly, DEVICE wylacznie
/// urzadzenie znakowe albo FIFO. Katalog, urzadzenie blokowe i gniazdo odpadaja dla wszystkich.
/// Komunikat nazywa slowo RQL i sciezke; nazwe strumienia dokleja wolajacy.
[[nodiscard]] std::string sourceKindMismatch(std::string_view storageType, const std::string &path, mode_t mode);

/// @brief To samo przez stat(), czyli BEZ otwarcia: FIFO bez pisarza niczego tu nie blokuje.
///
/// Sciezka, ktorej stat() nie widzi (brak pliku, brak uprawnien), nie jest odmowa - akcesor
/// ostrzega i daje rekordy NULL tak jak przed #346. Odmowa dotyczy istniejacej sciezki zlego rodzaju.
[[nodiscard]] std::string sourceKindMismatch(std::string_view storageType, const std::string &path);

/// @brief Create the FileInterface implementation for the given storage type; FatalError on unknown type.
/// @note Descriptor jest nie-const, bo retention()/storagePolicy() nie są metodami const.
/// @param followFinalLink zapisywalny plik danych moze byc dowiazaniem - tylko dla REF podanego przez
///        wolajacego (#374, storageFile.hpp); zrodla deklarowane czytaja sciezke jak dotad.
[[nodiscard]] std::unique_ptr<FileInterface> makeAccessor(std::string_view storageType,    //
                                                          const std::string &storageFile,  //
                                                          Descriptor &descriptor,          //
                                                          bool oneShot,                    //
                                                          int percounter,                  //
                                                          bool followFinalLink = false);

/// @brief Create the metaData variant matching the storage: inert for declared sources,
///        storageShadow when the accessor keeps a data shadow file, base metaData otherwise.
[[nodiscard]] std::unique_ptr<metaData> makeMetaIndex(bool declared,                 //
                                                      bool hasShadow,                //
                                                      const Descriptor &descriptor,  //
                                                      const std::string &metaIndexFile);

}  // namespace rdb
