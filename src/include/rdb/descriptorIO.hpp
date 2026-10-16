#pragma once

#include <string>

#include "descriptor.hpp"
#include "error.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc bez kończenia procesu: tryLoadDescriptorFile()
///   zwraca błąd otwarcia, składni, wartości i pusty deskryptor jako komunikat,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodnosc po preflight wypisuje oba deskryptory i wraca jako Errc::Logic,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// Funkcja tryLoadDescriptorFile powinna:
/// - otwierać plik .desc bez podążania za dowiązaniem i przy sukcesie zastępować descriptor odczytanym deskryptorem, zwracając odmowę z pustym komunikatem.
/// - błąd otwarcia, sprawdzenia rozmiaru albo odczytu zwracać jako odmowę klasy Environment z errno i ścieżką w komunikacie.
/// - błąd składni oraz deskryptor o rozmiarze zero bajtów zwracać jako odmowę klasy Plan, bez errno.
/// - przy każdej odmowie pozostawiać descriptor bez zmian i nie kończyć procesu.
///
/// @note Dowiazanie pod nazwa .desc w katalogu magazynu jest odmowa (#374).
[[nodiscard]] Refusal tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// Funkcja saveDescriptorFile powinna:
/// - zapisywać tekstową postać deskryptora do pliku .desc, tworząc go albo obcinając, bez podążania za dowiązaniem.
/// - błąd otwarcia albo zapisu zwracać jako Errc::IO z errno (EIO, gdy errno nie jest ustawione).
///
/// @note O_TRUNC przez dowiazanie obcialby jego cel (#374).
[[nodiscard]] Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// Funkcja verifyDescriptorMatch powinna:
/// - zwracać sukces, gdy deskryptor dostarczony jest równy zapisanemu.
/// - przy niezgodności logować błąd, wypisywać na stderr ścieżkę pliku i oba deskryptory oraz zwracać Errc::Logic.
///
/// @note Niezgodnosc to zlamany niezmiennik: granica planu (checkDescriptorFiles, #473) porownala schematy
///       przed publikacja planu.
[[nodiscard]] Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing,
                                             const std::string &descriptorFile);

}  // namespace rdb
