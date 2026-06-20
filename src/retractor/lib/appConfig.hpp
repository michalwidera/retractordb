#pragma once

#include <optional>
#include <string>

/// Konfiguracja usługi xretractor wczytywana z opcjonalnego pliku TOML (toml++).
///
/// Pliki konfiguracyjne są OPCJONALNE — ich brak to stan poprawny (program startuje
/// z wartościami domyślnymi, nie jest to błąd). Wyszukiwanie warstwowe (styl usług
/// systemu, np. sshd):
///   1. /etc/retractor/retractor.toml            (systemowy)
///   2. $XDG_CONFIG_HOME (lub ~/.config)/retractor/retractor.toml  (użytkownika)
/// Każda kolejna warstwa nadpisuje klucze poprzedniej. Jawnie podana ścieżka (cliPath,
/// flaga --config) wyłącza wyszukiwanie i wymaga istnienia oraz poprawności pliku.
struct AppConfig {
  /// Domyślny katalog na artefakty (storage). Pusty = brak ustawienia (zachowanie jak
  /// dziś — bieżący katalog procesu). Jeśli niepusty, gwarantuje końcowy '/'. Stosowany
  /// tylko gdy zestaw RQL nie zdefiniował własnej dyrektywy :STORAGE (RQL ma pierwszeństwo).
  std::string storageDir;
};

/// Wczytuje konfigurację.
/// - cliPath != nullopt → czyta WYŁĄCZNIE ten plik; brak pliku lub błąd składni są twarde
///   (rzucają wyjątek) — to jawne żądanie użytkownika.
/// - cliPath == nullopt → wyszukiwanie warstwowe; brak plików → wartości domyślne; błąd
///   składni TOML w którejś warstwie → ostrzeżenie i pominięcie warstwy (usługa nie pada).
AppConfig loadAppConfig(const std::optional<std::string> &cliPath = std::nullopt);
