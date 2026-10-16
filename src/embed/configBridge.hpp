#pragma once

#include <string>

#include "rdb/error.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
/// Funkcja embeddedConfig powinna:
/// - wczytywać wyłącznie plik `path` przez loadAppConfig(path) i zwracać wczytaną konfigurację, także z niepustymi powodami odrzucenia pojedynczych kluczy.
/// - zamieniać każdy wyjątek std::exception (brak pliku, błąd składni TOML, względny paths.lock_dir) na błąd Errc::Config z jego komunikatem.
/// - zamieniać wyjątek innego typu na błąd Errc::Config "cannot read embedded configuration: <path>".
/// - nie przepuszczać żadnego wyjątku.
///
/// @note Wyspa wyjatkow: toml++ zglasza nimi niepoprawna konfiguracje, a status nie moze przekroczyc granicy
///       rdzenia w postaci wyjatku.
[[nodiscard]] Result<AppConfig> embeddedConfig(const std::string &path);
}  // namespace rdb::embed
