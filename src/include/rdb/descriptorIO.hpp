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
///   Test: `DescriptorIOTest.all_load_refusals_preserve_a_nonempty_descriptor`.
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
///   Test: `DescriptorIOTest.save_reports_open_write_and_zero_write_failures`.
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodnosc po preflight wypisuje oba deskryptory i wraca jako Errc::Logic,
///   Test: `DescriptorIOTest.mismatch_diagnostics_include_path_and_both_descriptors`.
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.
///   Test: `DescriptorIOTest.load_replaces_an_existing_descriptor_and_only_reads_the_named_file`.

/// Funkcja tryLoadDescriptorFile powinna:
/// - otwierać plik .desc bez podążania za dowiązaniem i przy sukcesie zastępować descriptor odczytanym deskryptorem, zwracając odmowę z pustym komunikatem.
///   Test: `DescriptorIOTest.load_replaces_an_existing_descriptor_and_only_reads_the_named_file`, `DescriptorIOTest.io_refusal_preserves_the_error_class_and_errno`.
/// - błąd otwarcia, sprawdzenia rozmiaru albo odczytu zwracać jako odmowę klasy Environment z errno i ścieżką w komunikacie.
///   Test: `DescriptorIOTest.io_refusal_preserves_the_error_class_and_errno`, `DescriptorIOTest.size_and_read_failures_preserve_descriptor_errno_and_path`.
/// - błąd składni oraz deskryptor o rozmiarze zero bajtów zwracać jako odmowę klasy Plan, bez errno.
///   Test: `DescriptorIOTest.all_load_refusals_preserve_a_nonempty_descriptor`.
/// - przy każdej odmowie pozostawiać descriptor bez zmian i nie kończyć procesu.
///   Test: `DescriptorIOTest.all_load_refusals_preserve_a_nonempty_descriptor`, `DescriptorIOTest.size_and_read_failures_preserve_descriptor_errno_and_path`.
///
/// @note Dowiazanie pod nazwa .desc w katalogu magazynu jest odmowa (#374).
[[nodiscard]] Refusal tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// Funkcja saveDescriptorFile powinna:
/// - zapisywać tekstową postać deskryptora do pliku .desc, tworząc go albo obcinając, bez podążania za dowiązaniem.
///   Test: `DescriptorIOTest.save_truncates_the_file_and_does_not_follow_a_symlink`.
/// - błąd otwarcia albo zapisu zwracać jako Errc::IO z errno (EIO, gdy errno nie jest ustawione).
///   Test: `DescriptorIOTest.save_reports_open_write_and_zero_write_failures`.
///
/// @note O_TRUNC przez dowiazanie obcialby jego cel (#374).
[[nodiscard]] Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// Funkcja verifyDescriptorMatch powinna:
/// - zwracać sukces, gdy deskryptor dostarczony jest równy zapisanemu.
///   Test: `DescriptorIOTest.verify_match_accepts_equal_descriptors`.
/// - przy niezgodności logować błąd, wypisywać na stderr ścieżkę pliku i oba deskryptory oraz zwracać Errc::Logic.
///   Test: `DescriptorIOTest.mismatch_diagnostics_include_path_and_both_descriptors`.
///
/// @note Niezgodnosc to zlamany niezmiennik: granica planu (checkDescriptorFiles, #473) porownala schematy
///       przed publikacja planu.
[[nodiscard]] Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing,
                                             const std::string &descriptorFile);

}  // namespace rdb
