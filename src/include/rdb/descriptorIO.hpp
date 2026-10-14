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
///   niezgodnosc po preflight wypisuje oba deskryptory i wraca jako Errc::Logic,
/// - nie znać pozostałych plików magazynu - operują wyłącznie na wskazanej ścieżce .desc.

/// Odczyt bez konczenia procesu: pusty message to sukces, K2 to skladnia/schemat, K4 to I/O z errno.
[[nodiscard]] Refusal tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor);

/// @brief Load a Descriptor from an existing .desc file.
/// @return Errc::CorruptDescriptor when the file is missing, unreadable, empty, or does not parse - with the
///         reason tryLoadDescriptorFile() returns. The function never ends the process and never throws, so an
///         embedding host (the Python extension, an iOS app) survives a bad file.
[[nodiscard]] Result<Descriptor> loadDescriptorFile(const std::string &descriptorFile);

/// @brief Write the descriptor to a .desc file.
/// @return Errc::IO when the file cannot be opened for writing or the write fails.
[[nodiscard]] Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor);

/// @brief Check the provided descriptor against the one already on disk.
/// @return Errc::Logic przy niezgodnosci po preflight: granica planu juz porownala schematy.
[[nodiscard]] Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing,
                                             const std::string &descriptorFile);

}  // namespace rdb
