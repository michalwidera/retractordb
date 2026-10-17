#pragma once

#include <string>

namespace rdb {

/// Funkcja rotateStorageFile powinna:
/// - [storagerotation.rotate-storage-file.1~1] przemianowywać plik source na archive i utrwalać wpisy katalogu źródła i katalogu archiwum (jednego, gdy jest wspólny), próbując utrwalić drugi katalog także po błędzie pierwszego.
/// - [storagerotation.rotate-storage-file.2~1] logować jako ERROR nadpisanie istniejącego archiwum, także w Release.
/// - [storagerotation.rotate-storage-file.3~1] zwracać false z diagnostyką ERROR przy błędzie sprawdzenia archiwum, przemianowania albo utrwalenia katalogu, także gdy rename się udał.
///
/// @note Nie jest to transakcja rodziny plikow ani utrwalenie zawartosci samego pliku. Nadpisanie archiwum
///       widoczne w Release od #281.
[[nodiscard]] bool rotateStorageFile(const std::string &source, const std::string &archive);

}  // namespace rdb
