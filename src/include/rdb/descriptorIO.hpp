#pragma once

#include <string>

#include "descriptor.hpp"
#include "exceptions.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc bez kończenia procesu: tryLoadDescriptorFile()
///   zwraca błąd otwarcia, składni, wartości i pusty deskryptor jako komunikat, a loadDescriptorFile()
///   rzuca z tym samym komunikatem CorruptDescriptor,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodność schematów wypisuje oba deskryptory i kończy się rzutem ConfigError,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// @brief Read a descriptor without ending the process; return an empty string on success.
[[nodiscard]] std::string tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// @brief Load a Descriptor from an existing .desc file.
/// @throws CorruptDescriptor when the file is missing, unreadable, empty, or does not parse - with the
///         reason tryLoadDescriptorFile() returns. This is the phase-1 boundary: the function no longer
///         ends the process, so an embedding host (the Python extension, an iOS app) survives a bad file.
[[nodiscard]] Descriptor loadDescriptorFile(const std::string &descriptorFile);

/// @brief Write the descriptor to a .desc file.
/// @throws IOError when the file cannot be opened for writing or the write fails.
void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief Check the provided descriptor against the one already on disk.
/// @throws ConfigError on a schema mismatch - the file is fine, it just describes a different record
///         shape than the plan asks for, which is the plan's author to fix.
void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile);

}  // namespace rdb
