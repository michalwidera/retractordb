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
  // === [storage] ===

  /// Domyślny katalog na artefakty (storage). Pusty = brak ustawienia (zachowanie jak
  /// dziś — bieżący katalog procesu). Jeśli niepusty, gwarantuje końcowy '/'. Stosowany
  /// tylko gdy zestaw RQL nie zdefiniował własnej dyrektywy :STORAGE (RQL ma pierwszeństwo).
  std::string storageDir;

  // === [ipc] ===

  /// Liczba sekund bufora kolejki IPC per strumień (elementy = 1/interwał × sekundy).
  int ipcQueueBufferSeconds{10};

  /// Minimalna pojemność kolejki IPC niezależnie od interwału strumienia.
  int ipcMinQueueElements{100};

  /// Liczba prób odczytu odpowiedzi z shared memory po wysłaniu komendy przez xqry.
  /// Efektywny timeout to clientResponseMaxFails * ipc::kClientResponsePollInterval.
  int ipcClientResponseMaxFails{10};

  // === [timing] ===

  /// Maksymalny czas (sekundy) oczekiwania xqry na gotowość serwera (--wait-server).
  int timingServerStartupWaitSeconds{30};

  /// Interwał odpytywania (ms) podczas czekania na start serwera (--wait-server).
  int timingServerStartupPollIntervalMs{100};

  /// Timeout braku danych (ms) w pętli select xqry, po którym klient uznaje serwer za martwy.
  int timingQueryNoDataTimeoutMs{10000};

  // === [scheduling] ===

  /// Priorytet schedulera SCHED_FIFO w trybie real-time (zakres 1–99).
  int schedulingRtPriority{50};

  // === [paths] ===

  /// Katalog na plik blokady singleton (lock). Pusty = system temp dir.
  /// Dla usług systemd zalecane /var/run/retractor lub $XDG_RUNTIME_DIR.
  std::string lockDir;
};

/// Wczytuje konfigurację.
/// - cliPath != nullopt → czyta WYŁĄCZNIE ten plik; brak pliku lub błąd składni są twarde
///   (rzucają wyjątek) — to jawne żądanie użytkownika.
/// - cliPath == nullopt → wyszukiwanie warstwowe; brak plików → wartości domyślne; błąd
///   składni TOML w którejś warstwie → ostrzeżenie i pominięcie warstwy (usługa nie pada).
AppConfig loadAppConfig(const std::optional<std::string> &cliPath = std::nullopt);
