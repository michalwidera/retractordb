#pragma once

#include <optional>
#include <string>
#include <vector>

#include "serviceDefaults.h"  // kBuildDefaultServiceQueryFile (generowane z CMake)

namespace appcfg {
inline constexpr int kDefaultIpcQueueBufferSeconds{10};
inline constexpr int kDefaultIpcMinQueueElements{100};
inline constexpr int kDefaultIpcClientResponseMaxFails{10};
inline constexpr int kDefaultTimingServerStartupWaitSeconds{30};
inline constexpr int kDefaultTimingServerStartupPollIntervalMs{100};
inline constexpr int kDefaultTimingQueryNoDataTimeoutMs{10'000};
inline constexpr int kDefaultSchedulingRtPriority{50};

inline constexpr int kRtPriorityMin{1};
inline constexpr int kRtPriorityMax{99};

// Kanoniczny plik zapytań serwisu — fallback, gdy działający serwis nie zaraportował
// własnego QUERYFILE w pliku blokady. Jedno źródło prawdy: CMake RETRACTOR_QUERY_FILE
// (to samo zasila ExecStart jednostki i postinst), spójność gwarantowana w build-time.
inline constexpr const char *kDefaultServiceQueryFile{kBuildDefaultServiceQueryFile};
}  // namespace appcfg

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
  int ipcQueueBufferSeconds{appcfg::kDefaultIpcQueueBufferSeconds};

  /// Minimalna pojemność kolejki IPC niezależnie od interwału strumienia.
  int ipcMinQueueElements{appcfg::kDefaultIpcMinQueueElements};

  /// Liczba prób odczytu odpowiedzi z shared memory po wysłaniu komendy przez xqry.
  /// Efektywny timeout to clientResponseMaxFails * ipc::kClientResponsePollInterval.
  int ipcClientResponseMaxFails{appcfg::kDefaultIpcClientResponseMaxFails};

  // === [timing] ===

  /// Maksymalny czas (sekundy) oczekiwania xqry na gotowość serwera (--wait-server).
  int timingServerStartupWaitSeconds{appcfg::kDefaultTimingServerStartupWaitSeconds};

  /// Interwał odpytywania (ms) podczas czekania na start serwera (--wait-server).
  int timingServerStartupPollIntervalMs{appcfg::kDefaultTimingServerStartupPollIntervalMs};

  /// Timeout braku danych (ms) w pętli select xqry, po którym klient uznaje serwer za martwy.
  int timingQueryNoDataTimeoutMs{appcfg::kDefaultTimingQueryNoDataTimeoutMs};

  // === [scheduling] ===

  /// Priorytet schedulera SCHED_FIFO w trybie real-time (zakres 1–99).
  int schedulingRtPriority{appcfg::kDefaultSchedulingRtPriority};

  // === [paths] ===

  /// Katalog na plik blokady singleton (lock). Pusty = system temp dir.
  /// Dla usług systemd zalecane /var/run/retractor lub $XDG_RUNTIME_DIR.
  std::string lockDir;

  // === [service] ===

  /// Plik zapytań nadpisywany przy przekazaniu zestawu działającemu serwisowi (restart).
  /// Używany tylko jako fallback, gdy serwis nie zaraportował własnego QUERYFILE w blokadzie.
  /// Musi być zgodny z argumentem ExecStart jednostki systemd (config nie zmienia ExecStart).
  std::string serviceQueryFile{appcfg::kDefaultServiceQueryFile};

  // === diagnostyka ===

  /// Ścieżki plików konfiguracyjnych faktycznie wczytanych (w kolejności nakładania warstw).
  /// Pusty wektor = żaden plik nie został znaleziony, użyto wartości domyślnych.
  std::vector<std::string> loadedFrom;
};

/// Wczytuje konfigurację.
/// - cliPath != nullopt → czyta WYŁĄCZNIE ten plik; brak pliku lub błąd składni są twarde
///   (rzucają wyjątek) — to jawne żądanie użytkownika.
/// - cliPath == nullopt → wyszukiwanie warstwowe; brak plików → wartości domyślne; błąd
///   składni TOML w którejś warstwie → ostrzeżenie i pominięcie warstwy (usługa nie pada).
AppConfig loadAppConfig(const std::optional<std::string> &cliPath = std::nullopt);
