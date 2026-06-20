#include "appConfig.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

namespace {

// Normalizuje katalog storage: niepusty bez końcowego '/' dostaje '/'
// — spójnie z konwencją dyrektywy :STORAGE w RQLParser.
std::string normalizeStorageDir(std::string dir) {
  if (!dir.empty() && dir.back() != '/') dir.push_back('/');
  return dir;
}

// Nakłada ustawienia z jednej tabeli TOML na akumulowaną konfigurację.
// Klucze nieobecne w tabeli pozostawiają dotychczasową wartość (warstwowość).
void applyTable(const toml::table &tbl, AppConfig &cfg) {
  if (auto v = tbl.at_path("storage.dir").value<std::string>(); v) cfg.storageDir = *v;
}

// Ścieżka pliku konfiguracyjnego użytkownika wg XDG ($XDG_CONFIG_HOME lub ~/.config).
std::optional<std::filesystem::path> userConfigPath() {
  if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0')
    return std::filesystem::path(xdg) / "retractor" / "retractor.toml";
  if (const char *home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    return std::filesystem::path(home) / ".config" / "retractor" / "retractor.toml";
  return std::nullopt;
}

}  // namespace

AppConfig loadAppConfig(const std::optional<std::string> &cliPath) {
  AppConfig cfg;

  if (cliPath) {
    // Jawnie podana ścieżka: plik musi istnieć i być poprawny — błąd jest twardy
    // (toml::parse_file rzuca toml::parse_error). Wywołujący raportuje go użytkownikowi.
    const toml::table tbl = toml::parse_file(*cliPath);
    applyTable(tbl, cfg);
    cfg.storageDir = normalizeStorageDir(cfg.storageDir);
    return cfg;
  }

  // Wyszukiwanie warstwowe: system → użytkownik (kolejne nadpisują klucze).
  std::vector<std::filesystem::path> candidates{"/etc/retractor/retractor.toml"};
  if (auto up = userConfigPath(); up) candidates.push_back(*up);

  for (const auto &path : candidates) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) continue;  // brak pliku = stan poprawny
    try {
      const toml::table tbl = toml::parse_file(path.string());
      applyTable(tbl, cfg);
    } catch (const toml::parse_error &e) {
      SPDLOG_WARN("Config parse error in {}: {} — skipping this layer", path.string(), e.what());
    }
  }

  cfg.storageDir = normalizeStorageDir(cfg.storageDir);
  return cfg;
}
