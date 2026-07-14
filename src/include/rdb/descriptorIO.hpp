#pragma once

#include <string>

#include "descriptor.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) — wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc (loadDescriptorFile()) z pominięciem buforowania
///   strumienia; pusty deskryptor w pliku kończy się przez FatalError,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodność schematów wypisuje oba deskryptory i kończy się przez FatalError,
/// - nie znać pozostałych plików magazynu — operują wyłącznie na wskazanej ścieżce .desc.

/// @brief Load a Descriptor from an existing .desc file; FatalError when the file holds an empty descriptor.
[[nodiscard]] Descriptor loadDescriptorFile(const std::string &descriptorFile);

/// @brief Write the descriptor to a .desc file; FatalError on open or write failure.
void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief FatalError when @p provided does not match @p existing (schema mismatch in @p descriptorFile).
void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile);

}  // namespace rdb
