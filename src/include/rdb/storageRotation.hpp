#pragma once

#include <string>

namespace rdb {

/// Funkcja rotateStorageFile powinna:
/// - przemianowywać plik source na archive i utrwalać wpisy katalogu źródła i katalogu archiwum (jednego, gdy jest wspólny), próbując utrwalić drugi katalog także po błędzie pierwszego.
/// - logować jako ERROR nadpisanie istniejącego archiwum, także w Release.
/// - zwracać false z diagnostyką ERROR przy błędzie sprawdzenia archiwum, przemianowania albo utrwalenia katalogu, także gdy rename się udał.
///
/// @note Nie jest to transakcja rodziny plikow ani utrwalenie zawartosci samego pliku. Nadpisanie archiwum
///       widoczne w Release od #281.
[[nodiscard]] bool rotateStorageFile(const std::string &source, const std::string &archive);

}  // namespace rdb
