#pragma once

#include <string>

#include "descriptor.hpp"
#include "exceptions.hpp"

namespace rdb {

/// @brief Persystencja pliku deskryptora (.desc) - wydzielona z klasy storage.
///
/// Funkcje descriptorIO powinny:
/// - wczytywać Descriptor z istniejącego pliku .desc (loadDescriptorFile()) z pominięciem buforowania
///   strumienia; pusty lub niepoprawny deskryptor w pliku kończy się rzutem CorruptDescriptor,
/// - zapisywać Descriptor do pliku .desc (saveDescriptorFile()) z kontrolą błędów otwarcia i zapisu,
/// - weryfikować zgodność deskryptora dostarczonego z już zapisanym (verifyDescriptorMatch());
///   niezgodność schematów wypisuje oba deskryptory i kończy się rzutem ConfigError,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// @brief Load a Descriptor from an existing .desc file.
/// @throws CorruptDescriptor when the file is missing, empty, or does not parse. This is the
///         phase-1 boundary: the function no longer ends the process, so an embedding host
///         (the Python extension, an iOS app) survives a bad file.
[[nodiscard]] Descriptor loadDescriptorFile(const std::string &descriptorFile);

/// @brief Write the descriptor to a .desc file; still FatalError on open or write failure.
/// @note Deliberately left for sub-slice 2b: every I/O-failure site in this layer (here,
///       storage::read/write, facc*) gets converted together, so the IOError type arrives once
///       with all of its call sites rather than half-introduced here.
void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief Check the provided descriptor against the one already on disk.
/// @throws ConfigError on a schema mismatch - the file is fine, it just describes a different record
///         shape than the plan asks for, which is the plan's author to fix.
void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile);

}  // namespace rdb
