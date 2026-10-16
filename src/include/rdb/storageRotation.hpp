#pragma once

#include <string>

namespace rdb {

/// Funkcja rotateStorageFile powinna:
/// - przemianowywać plik source na archive i utrwalać wpisy katalogu źródła i katalogu archiwum (jednego, gdy jest wspólny), próbując utrwalić drugi katalog także po błędzie pierwszego.
///   Test: `StorageRotationTest.same_directory_is_synced_once_after_rename`, `StorageRotationTest.different_directories_are_both_synced_even_if_first_fails`.
/// - logować jako ERROR nadpisanie istniejącego archiwum, także w Release.
///   Test: `RotationPathsTest.reports_failure_at_error_level_and_syncs_successful_rename`.
/// - zwracać false z diagnostyką ERROR przy błędzie sprawdzenia archiwum, przemianowania albo utrwalenia katalogu, także gdy rename się udał.
///   Test: `RotationPathsTest.reports_failure_at_error_level_and_syncs_successful_rename`, `StorageRotationTest.failed_rename_keeps_source_and_never_syncs_directory`.
///
/// @note Nie jest to transakcja rodziny plikow ani utrwalenie zawartosci samego pliku. Nadpisanie archiwum
///       widoczne w Release od #281.
[[nodiscard]] bool rotateStorageFile(const std::string &source, const std::string &archive);

}  // namespace rdb
