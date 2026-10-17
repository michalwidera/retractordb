#pragma once

#include <string>

#include "descriptor.hpp"
#include "error.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - [descriptorio.descriptor-io.1~1] wczytywać Descriptor z istniejącego pliku .desc bez kończenia procesu: tryLoadDescriptorFile()
///   zwraca błąd otwarcia, składni, wartości i pusty deskryptor jako komunikat,
/// - [descriptorio.descriptor-io.2~1] zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - [descriptorio.descriptor-io.3~1] weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodnosc po preflight wypisuje oba deskryptory i wraca jako Errc::Logic,
/// - [descriptorio.descriptor-io.4~1] nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// Funkcja tryLoadDescriptorFile powinna:
/// - [descriptorio.try-load-descriptor-file.1~1] otwierać plik .desc bez podążania za dowiązaniem i przy sukcesie zastępować descriptor odczytanym deskryptorem, zwracając odmowę z pustym komunikatem.
/// - [descriptorio.try-load-descriptor-file.2~1] błąd otwarcia, sprawdzenia rozmiaru albo odczytu zwracać jako odmowę klasy Environment z errno i ścieżką w komunikacie.
/// - [descriptorio.try-load-descriptor-file.3~1] błąd składni oraz deskryptor o rozmiarze zero bajtów zwracać jako odmowę klasy Plan, bez errno.
/// - [descriptorio.try-load-descriptor-file.4~1] przy każdej odmowie pozostawiać descriptor bez zmian i nie kończyć procesu.
///
/// @note Dowiazanie pod nazwa .desc w katalogu magazynu jest odmowa (#374).
[[nodiscard]] Refusal tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// Funkcja saveDescriptorFile powinna:
/// - [descriptorio.save-descriptor-file.1~1] zapisywać tekstową postać deskryptora do pliku .desc, tworząc go albo obcinając, bez podążania za dowiązaniem.
/// - [descriptorio.save-descriptor-file.2~1] błąd otwarcia albo zapisu zwracać jako Errc::IO z errno (EIO, gdy errno nie jest ustawione).
///
/// @note O_TRUNC przez dowiazanie obcialby jego cel (#374).
[[nodiscard]] Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// Funkcja verifyDescriptorMatch powinna:
/// - [descriptorio.verify-descriptor-match.1~1] zwracać sukces, gdy deskryptor dostarczony jest równy zapisanemu.
/// - [descriptorio.verify-descriptor-match.2~1] przy niezgodności logować błąd, wypisywać na stderr ścieżkę pliku i oba deskryptory oraz zwracać Errc::Logic.
///
/// @note Niezgodnosc to zlamany niezmiennik: granica planu (checkDescriptorFiles, #473) porownala schematy
///       przed publikacja planu.
[[nodiscard]] Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing,
                                             const std::string &descriptorFile);

}  // namespace rdb
