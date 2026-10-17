#pragma once

#include <sys/types.h>  // mode_t

#include <memory>
#include <string>
#include <string_view>

#include "descriptor.hpp"
#include "error.hpp"
#include "fainterface.hpp"
#include "memoryStore.hpp"
#include "metaData.hpp"

namespace rdb {

/// @brief Fabryka implementacji FileInterface oraz wariantu indeksu metadanych dla klasy storage.
///
/// Funkcje fabryki powinny:
/// - [accessorfactory.fabryki.1~1] odwzorowywać nazwę typu magazynu (DEFAULT/DIRECT/MEMORY/POSIX/POSIXSHD/GENERIC/BINFILE/DEVICE/TEXTSOURCE)
///   na konkretną implementację FileInterface (makeAccessor()); nieznany typ, pusta ścieżka danych lub
///   pusty typ kończą się błędem Errc::Config (faza 1, plaster 2a; do 2026-10 rzutem ConfigError),
/// - [accessorfactory.fabryki.2~1] rozstrzygać, czy typ magazynu jest źródłem deklarowanym tylko do odczytu (isDeclaredType()),
/// - [accessorfactory.fabryki.3~1] dobierać wariant indeksu metadanych null (makeMetaIndex()): wariant inertny (pusta ścieżka pliku)
///   dla źródeł deklarowanych, storageShadow gdy accessor utrzymuje plik cienia danych, bazowy metaData
///   dla pozostałych zapisywalnych - posiadanie pliku cienia danych i posiadanie metaindeksu są
///   niezależnymi własnościami magazynu (cień chroni oryginalną zarejestrowaną zawartość danych,
///   metaindeks rejestruje wartości null i przerwy w transmisji),
/// - [accessorfactory.fabryki.4~1] skupiać całą wiedzę o konkretnych typach akcesorów w jednym miejscu - storage zna wyłącznie
///   abstrakcyjny FileInterface.
///   Nietestowalne: zaleznosc strukturalna modulow; przeglad storage.cc i accessorFactory.cc sprawdza miejsce znajomosci klas konkretnych.

/// Funkcja isDeclaredType powinna:
/// - [accessorfactory.is-declared-type.1~1] zwracać true wyłącznie dla typów źródeł deklarowanych tylko do odczytu: BINFILE, DEVICE i TEXTSOURCE.
[[nodiscard]] bool isDeclaredType(std::string_view storageType);

/// Funkcja isWritableType powinna:
/// - [accessorfactory.is-writable-type.1~1] zwracać true wyłącznie dla zapisywalnych profili wyjścia SELECT: DEFAULT, DIRECT, MEMORY, POSIX, POSIXSHD i GENERIC.
[[nodiscard]] bool isWritableType(std::string_view storageType);

/// Funkcja sourceKindMismatch powinna:
/// - [accessorfactory.source-kind-mismatch.1~1] zwracać pusty napis, gdy rodzaj pliku pasuje do typu źródła deklarowanego: urządzenie znakowe albo FIFO dla DEVICE, plik zwykły dla BINFILE i TEXTSOURCE.
/// - [accessorfactory.source-kind-mismatch.2~1] dla rodzaju niepasującego (także katalogu, urządzenia blokowego i gniazda) zwracać odmowę nazywającą słowo RQL (TEXTFILE dla TEXTSOURCE), ścieżkę i rodzaj znalezionego pliku, bez nazwy strumienia, którą dokleja wołający.
/// - [accessorfactory.source-kind-mismatch.3~1] w wariancie bez trybu ustalać rodzaj przez stat(), bez otwierania pliku, a dla ścieżki, której stat() nie widzi, zwracać pusty napis.
///
/// @note Odmowa rodzaju pliku wprowadzona w #346. Wariant przez stat() nie blokuje sie na FIFO bez pisarza;
///       brak pliku nie jest tu odmowa - obsluguje go akcesor.
[[nodiscard]] std::string sourceKindMismatch(std::string_view storageType, const std::string &path, mode_t mode);
[[nodiscard]] std::string sourceKindMismatch(std::string_view storageType, const std::string &path);

/// Funkcja makeAccessor powinna:
/// - [accessorfactory.make-accessor.1~1] tworzyć implementację FileInterface właściwą dla typu magazynu: groupFile z cieniem dla DEFAULT, groupFile bez cienia dla DIRECT, memoryFile dla MEMORY, posixBinaryFile dla POSIX, posixBinaryFileWithShadow dla POSIXSHD, genericBinaryFile dla GENERIC, binaryDeviceRO dla BINFILE i DEVICE oraz textSourceRO dla TEXTSOURCE.
/// - [accessorfactory.make-accessor.2~1] dla MEMORY używać wskazanego Memory Store, a przy memory == nullptr MemoryStore::processDefault().
/// - [accessorfactory.make-accessor.3~1] przekazywać followFinalLink wyłącznie akcesorom zapisywalnym, a źródłom deklarowanym czytanie z zawijaniem, gdy oneShot jest wyłączone.
/// - [accessorfactory.make-accessor.4~1] zwracać Errc::Config dla pustej ścieżki, pustego typu i typu spoza listy, bez kończenia procesu.
///
/// @note Descriptor jest nie-const, bo retention()/storagePolicy() nie są metodami const. followFinalLink
///       dotyczy tylko REF podanego przez wolajacego (#374, storageFile.hpp). Typ bierze sie takze wprost od
///       wolajacego (wiazanie Pythona), wiec nieznany typ musi byc zwyklym bledem.
[[nodiscard]] Result<std::unique_ptr<FileInterface>> makeAccessor(std::string_view storageType,    //
                                                                  const std::string &storageFile,  //
                                                                  Descriptor &descriptor,          //
                                                                  bool oneShot,                    //
                                                                  int percounter,                  //
                                                                  bool followFinalLink = false,    //
                                                                  MemoryStore *memory  = nullptr);

/// Funkcja makeMetaIndex powinna:
/// - [accessorfactory.make-meta-index.1~1] dla źródła deklarowanego zwracać inertny metaData bez pliku (pusta ścieżka, bez persystencji).
/// - [accessorfactory.make-meta-index.2~1] dla magazynu z plikiem cienia danych zwracać storageShadow na metaIndexFile, a dla pozostałych bazowy metaData na metaIndexFile.
[[nodiscard]] std::unique_ptr<metaData> makeMetaIndex(bool declared,                 //
                                                      bool hasShadow,                //
                                                      const Descriptor &descriptor,  //
                                                      const std::string &metaIndexFile);

}  // namespace rdb
