#pragma once

#include <optional>
#include <string>
#include <vector>

#include "rdb/retention.hpp"
#include "serviceDefaults.h"  // kBuildDefaultServiceQueryFile (generowane z CMake)

namespace appcfg {
inline constexpr int kDefaultIpcQueueBufferSeconds{10};
inline constexpr int kDefaultIpcMinQueueElements{100};
// 300 × ipc::kClientResponsePollInterval = 3 s. Powód wartości: patrz
// kIpcClientDefaultResponseMaxFails w src/qry/ipcClient.hpp.
inline constexpr int kDefaultIpcClientResponseMaxFails{300};
inline constexpr int kDefaultTimingServerStartupWaitSeconds{30};
inline constexpr int kDefaultTimingServerStartupPollIntervalMs{100};
inline constexpr int kDefaultTimingQueryNoDataTimeoutMs{10'000};
inline constexpr int kDefaultSchedulingRtPriority{50};
// Budzet pamieci historii planu. Uzasadnienie wartosci przy AppConfig::historyMemoryMib.
inline constexpr int kDefaultHistoryMemoryMib{1024};

inline constexpr int kRtPriorityMin{1};
inline constexpr int kRtPriorityMax{99};

// Kanoniczny plik zapytań serwisu - fallback, gdy działający serwis nie zaraportował
// własnego QUERYFILE w pliku blokady. Jedno źródło prawdy: CMake RETRACTOR_QUERY_FILE
// (to samo zasila ExecStart jednostki i postinst), spójność gwarantowana w build-time.
inline constexpr const char *kDefaultServiceQueryFile{kBuildDefaultServiceQueryFile};
}  // namespace appcfg

/// Konfiguracja usługi xretractor wczytywana z opcjonalnego pliku TOML (toml++).
///
/// Pliki konfiguracyjne są OPCJONALNE - ich brak to stan poprawny (program startuje
/// z wartościami domyślnymi, nie jest to błąd). Wyszukiwanie warstwowe (styl usług
/// systemu, np. sshd):
///   1. /etc/retractor/retractor.toml            (systemowy)
///   2. $XDG_CONFIG_HOME (lub ~/.config)/retractor/retractor.toml  (użytkownika)
/// Każda kolejna warstwa nadpisuje klucze poprzedniej. Jawnie podana ścieżka (cliPath,
/// flaga --config) wyłącza wyszukiwanie i wymaga istnienia oraz poprawności pliku.
/// @requirements-exempt AppConfig: Nosnik parametrow konfiguracji; odczyt i walidacje wykonuja funkcje modulu.
struct AppConfig {
  // === [storage] ===

  /// Domyślny katalog na artefakty (storage). Pusty = brak ustawienia (zachowanie jak
  /// dziś - bieżący katalog procesu). Jeśli niepusty, gwarantuje końcowy '/'. Stosowany
  /// tylko gdy zestaw RQL nie zdefiniował własnej dyrektywy :STORAGE (RQL ma pierwszeństwo).
  std::string storageDir;

  /// Retencja strumieni plikowych (DEFAULT, DIRECT, takze posrednich) bez klauzuli RETENTION:
  /// `default_retention = [capacity, segments]`, obie liczby > 0. Pusta (domyslnie) = brak retencji,
  /// czyli historia rosnie na dysku - dane kasuje dopiero jawna decyzja operatora (D8). Stosuje ja
  /// kompilator (compiler::setDefaultRetention) na tych samych drogach co history_memory_mib.
  rdb::retention_t defaultRetention{.segments = 0, .capacity = 0};

  /// Katalogi spoza katalogu magazynu, do ktorych REF z wczytanego `.desc` moze przeniesc
  /// zapisywalny plik danych (#278): `ref_dirs = ["/srv/rdb/archive"]`, sciezki bezwzgledne
  /// istniejacych katalogow. Bez klucza REF z pliku pisze i usuwa tylko w katalogu magazynu.
  /// Klucz nie zmienia odczytu zrodel deklarowanych ani REF z planu - te sa dozwolone zawsze.
  /// Czyta go xtrdb: w xretractorze REF zachowanego `.desc` musi byc rowny REF planu.
  std::vector<std::string> storageRefDirs;

  /// Powod odrzucenia `storage.ref_dirs` (pusty = poprawny albo brak klucza). Jak przy
  /// sourcesTimeoutError: loadAppConfig() go tylko zapisuje, a xtrdb konczy start bledem - lista
  /// uprawnien zapisu nie zgaduje.
  std::string storageRefDirsError;

  // === [sources] ===

  /// Termin odczytu zrodel DEVICE bez klauzuli TIMEOUT, w sekundach (#347). Brak klucza = nullopt,
  /// czyli wbudowane 0. Jawna klauzula w RQL wygrywa - takze jawne `TIMEOUT 0`. Klucz nie zmienia
  /// BINFILE, TEXTFILE ani timing.query_no_data_timeout_ms klienta xqry.
  std::optional<double> sourcesTimeoutSeconds;

  /// Powod odrzucenia `[sources] timeout_s` (pusty = wartosc poprawna albo brak klucza). Swiadome
  /// odstepstwo od reguly "ostrzezenie + wartosc domyslna": cicha zamiana zmienilaby semantyke
  /// czekania. loadAppConfig() tylko go zapisuje, bo czytaja go tez xqry i xtrdb, ktorych ten klucz
  /// nie dotyczy; start xretractora konczy sie bledem w launcherze.
  std::string sourcesTimeoutError;

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

  // === [embedded] ===

  /// Katalogi blokad, w ktorych silnik osadzony (Engine/Storage w Pythonie, #450) sprawdza blokady
  /// serwisu lagodnie, jak xtrdb: odmowe daje tylko trzymany flock, a wpis nieczytelny albo brak
  /// katalogu nie. Katalog spoza listy jest sprawdzany scisle. Sciezki bezwzgledne. nullopt = brak
  /// klucza, czyli z definicji katalog tymczasowy systemu (TMPDIR, bez niej /tmp) - ten sam, ktory
  /// serwis bierze przy pustym paths.lock_dir. xretractor i xqry klucza nie czytaja.
  std::optional<std::vector<std::string>> embeddedLenientLockDirs;

  /// Powod odrzucenia `embedded.lenient_lock_dirs` (pusty = poprawny albo brak klucza). Jak przy
  /// storageRefDirsError: loadAppConfig() go tylko zapisuje, a odmawia silnik osadzony - start
  /// serwisu nie zalezy od klucza, ktorego serwis nie uzywa.
  std::string embeddedLenientLockDirsError;

  // === [server] ===

  /// Losowanie nazwy instancji, gdy nie podano ani `--name`, ani `--autoname`. Wartosc `false`
  /// (domyslna) zostawia tryb historyczny: pusta nazwa, blokada i obiekty IPC bez sufiksu.
  /// Jawne `--name` wygrywa nad tym kluczem - wskazanie operatora jest nadrzedne wobec configu.
  bool serverAutoName{false};

  // === [service] ===

  /// Plik zapytań nadpisywany przy przekazaniu zestawu działającemu serwisowi (restart).
  /// Używany tylko jako fallback, gdy serwis nie zaraportował własnego QUERYFILE w blokadzie.
  /// Musi być zgodny z argumentem ExecStart jednostki systemd (config nie zmienia ExecStart).
  std::string serviceQueryFile{appcfg::kDefaultServiceQueryFile};

  /// Dopuszcza regułę `DO SYSTEM` w planie przyjmowanym kanałem `--reset`. Wartość `false`
  /// (domyślna) odrzuca taki plan w całości - patrz executorsm::validatePlanText, gdzie stoi
  /// pełne uzasadnienie granicy.
  ///
  /// Klucz czytany jest przy starcie procesu, więc ustawia go ten sam autorytet, który pisze
  /// plik planu usługi: dla jednostki systemd jest to /etc/retractor/retractor.toml (usługa
  /// biegnie jako `retractor` z HOME=/nonexistent, więc warstwa użytkownika jej nie dotyczy),
  /// a dla serwera uruchomionego z terminala - konfiguracja tego, kto go uruchomił.
  bool serviceUnrestricted{false};

  // === [limits] ===

  /// Budzet pamieci historii planu w MiB: suma po magazynach trzymajacych historie w RAM - pierscienie
  /// zrodel deklarowanych (SourceBuffer) i magazynow MEMORY - z capacity x (sizeof(rdb::payload) +
  /// bajty rekordu). Plan, ktory go przekracza, jest bledem kompilacji: przy starcie, w `-c`, w kanale
  /// ad-hoc i przy `--reset` (compiler::setHistoryMemoryBudget). Bez budzetu wartosc w zakresie int
  /// (`>65536` nad rekordem 1 MiB) konczyla sie std::bad_alloc albo OOM killerem w dzialajacym
  /// serwerze (A2 M11). 1024 MiB to ponad trzy rzedy wielkosci zapasu: najwiecej w repozytorium
  /// (examples/ecg/rec205) trzyma ok. 200 KiB historii.
  int historyMemoryMib{appcfg::kDefaultHistoryMemoryMib};

  // === diagnostyka ===

  /// Ścieżki plików konfiguracyjnych faktycznie wczytanych (w kolejności nakładania warstw).
  /// Pusty wektor = żaden plik nie został znaleziony, użyto wartości domyślnych.
  std::vector<std::string> loadedFrom;
};

/// Funkcja loadAppConfig powinna:
/// - przy podanym cliPath wczytywać wyłącznie ten plik, a brak pliku albo błąd składni TOML zgłaszać wyjątkiem toml::parse_error z nazwą pliku.
/// - bez cliPath nakładać kolejno istniejące warstwy: /etc/retractor/retractor.toml, na Darwinie także /usr/local/etc/retractor/retractor.toml i /opt/homebrew/etc/retractor/retractor.toml, a na końcu plik użytkownika ($XDG_CONFIG_HOME/retractor/retractor.toml, bez niej ~/.config/retractor/retractor.toml), tak że klucz z późniejszej warstwy nadpisuje wcześniejszy.
/// - pomijać brakujące warstwy, a bez żadnej zwracać wartości domyślne; błąd sprawdzenia istnienia warstwy zgłaszać wyjątkiem std::filesystem::filesystem_error.
/// - błąd składni TOML w dowolnej warstwie zgłaszać wyjątkiem toml::parse_error z nazwą pliku.
/// - odrzucać wyjątkiem std::invalid_argument z nazwą pliku niepusty, względny paths.lock_dir w każdej wczytywanej warstwie, także nadpisanej później.
/// - zapisywać ścieżki wczytanych plików w loadedFrom, w kolejności nakładania.
/// - po wczytaniu zastępować wartościami domyślnymi, z ostrzeżeniem w logu, niedodatnie wartości kluczy `[ipc]`, `[timing]` i `limits.history_memory_mib` oraz `scheduling.rt_priority` spoza 1..99, a do niepustego storage.dir dopisywać końcowy '/'.
///
/// @note Jawna sciezka to zadanie uzytkownika, wiec jej blad jest twardy. Warstwa systemowa ma kilka sciezek,
///       bo na macOS /etc nalezy do systemu (SIP), a oprogramowanie spoza systemu trzyma konfiguracje w
///       prefiksie instalacji. Bledne `storage.ref_dirs`, `embedded.lenient_lock_dirs` i `sources.timeout_s`
///       nie rzucaja: zostaje powod odrzucenia, ktory zamienia na blad dopiero wolajacy.
AppConfig loadAppConfig(const std::optional<std::string> &cliPath = std::nullopt);
