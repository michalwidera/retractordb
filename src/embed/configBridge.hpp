#pragma once

#include <string>

#include "rdb/error.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
/// Funkcja embeddedConfig powinna:
/// - wczytywać wyłącznie plik `path` przez loadAppConfig(path) i zwracać wczytaną konfigurację, także z niepustymi powodami odrzucenia pojedynczych kluczy.
///   Test: `embedConfig.explicit_file_preserves_values_and_rejected_key_reasons`.
///   Nietestowalne: brak warstw /etc i XDG potwierdza wywolanie loadAppConfig(path); test nie podmienia konfiguracji systemowej.
/// - zamieniać każdy wyjątek std::exception (brak pliku, błąd składni TOML, względny paths.lock_dir) na błąd Errc::Config z jego komunikatem.
///   Test: `embedConfig.missing_malformed_and_relative_lock_paths_are_config_errors`, `embedConfig.standard_and_unknown_loader_exceptions_return_config_results`.
/// - zamieniać wyjątek innego typu na błąd Errc::Config "cannot read embedded configuration: <path>".
///   Test: `embedConfig.standard_and_unknown_loader_exceptions_return_config_results`.
/// - przechwytywać wyjątki z loadAppConfig i zwracać Result, o ile możliwe jest utworzenie diagnostyki błędu.
///   Test: `embedConfig.standard_and_unknown_loader_exceptions_return_config_results`.
///   Ograniczenie weryfikacji: catch obejmuje odczyt konfiguracji; awaria alokacji podczas budowy diagnostyki w catch nie ma gwarancji zamiany na Result.
///
/// @note Wyspa wyjatkow: toml++ zglasza nimi niepoprawna konfiguracje, a status nie moze przekroczyc granicy
///       rdzenia w postaci wyjatku.
[[nodiscard]] Result<AppConfig> embeddedConfig(const std::string &path);
}  // namespace rdb::embed
