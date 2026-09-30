#pragma once

#include <string>

#include "descriptor.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc (tryLoadDescriptorFile()) bez kończenia procesu;
///   błąd otwarcia, składni, wartości i pusty deskryptor wracają jako komunikat,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodność schematów wypisuje oba deskryptory i kończy się przez FatalError,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// @brief Read a descriptor without ending the process; return an empty string on success.
[[nodiscard]] std::string tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// @brief Write the descriptor to a .desc file; FatalError on open or write failure.
void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief FatalError when @p provided does not match @p existing (schema mismatch in @p descriptorFile).
void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile);

}  // namespace rdb
