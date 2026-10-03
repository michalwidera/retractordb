#pragma once

#include <string>

#include "descriptor.hpp"
#include "error.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc bez kończenia procesu: tryLoadDescriptorFile()
///   zwraca błąd otwarcia, składni, wartości i pusty deskryptor jako komunikat, a loadDescriptorFile()
///   zwraca z tym samym komunikatem błąd Errc::CorruptDescriptor,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodność schematów wypisuje oba deskryptory i kończy się błędem Errc::Config,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// @brief Read a descriptor without ending the process; return an empty string on success.
[[nodiscard]] std::string tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// @brief Load a Descriptor from an existing .desc file.
/// @return Errc::CorruptDescriptor when the file is missing, unreadable, empty, or does not parse - with the
///         reason tryLoadDescriptorFile() returns. The function never ends the process and never throws, so an
///         embedding host (the Python extension, an iOS app) survives a bad file.
[[nodiscard]] Result<Descriptor> loadDescriptorFile(const std::string &descriptorFile);

/// @brief Write the descriptor to a .desc file.
/// @return Errc::IO when the file cannot be opened for writing or the write fails.
[[nodiscard]] Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief Check the provided descriptor against the one already on disk.
/// @return Errc::Config on a schema mismatch - the file is fine, it just describes a different record
///         shape than the plan asks for, which is the plan's author to fix.
[[nodiscard]] Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing,
                                             const std::string &descriptorFile);

}  // namespace rdb
