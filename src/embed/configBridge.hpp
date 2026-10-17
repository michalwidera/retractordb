#pragma once

#include <string>

#include "rdb/error.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
/// Funkcja embeddedConfig powinna:
/// - [configbridge.embedded-config.1~1] wczytywać wyłącznie plik `path` przez loadAppConfig(path) i zwracać wczytaną konfigurację, także z niepustymi powodami odrzucenia pojedynczych kluczy.
///   Nietestowalne: brak warstw /etc i XDG potwierdza wywolanie loadAppConfig(path); test nie podmienia konfiguracji systemowej.
/// - [configbridge.embedded-config.2~1] zamieniać każdy wyjątek std::exception (brak pliku, błąd składni TOML, względny paths.lock_dir) na błąd Errc::Config z jego komunikatem.
/// - [configbridge.embedded-config.3~1] zamieniać wyjątek innego typu na błąd Errc::Config "cannot read embedded configuration: <path>".
/// - [configbridge.embedded-config.4~1] przechwytywać wyjątki z loadAppConfig i zwracać Result, o ile możliwe jest utworzenie diagnostyki błędu.
///   Ograniczenie weryfikacji: catch obejmuje odczyt konfiguracji; awaria alokacji podczas budowy diagnostyki w catch nie ma gwarancji zamiany na Result.
///
/// @note Wyspa wyjatkow: toml++ zglasza nimi niepoprawna konfiguracje, a status nie moze przekroczyc granicy
///       rdzenia w postaci wyjatku.
[[nodiscard]] Result<AppConfig> embeddedConfig(const std::string &path);
}  // namespace rdb::embed
