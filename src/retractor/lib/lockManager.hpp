#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "constants.hpp"

/// Tozsamosc systemd biezacego procesu, ustalona z /proc/self/cgroup.
/// @requirements-exempt SystemdIdentity: Dane identyfikujace jednostke systemd i jej zakres.
struct SystemdIdentity {
  std::optional<std::string> unit;  // nazwa jednostki, gdy proces jest jednostka systemd
  bool userScope{false};            // true => user.slice (systemctl --user), false => system
};

/// Funkcja detectSystemdIdentity powinna:
/// - [lockmanager.detect-systemd-identity.1~1] zwracać nazwę jednostki i zakres użytkownika przeniesione wprost z osplat::detectServiceIdentity (Linux: jednostka systemd z /proc/self/cgroup, Darwin: zadanie launchd), z unit == nullopt dla zwykłego procesu.
///   Nietestowalne: test nie uruchamia procesu w kazdym managerze uslug; porownuje caly wynik z warstwa platformowa. Darwin niezweryfikowany.
///
/// @note Deklaracja w naglowku, bo tozsamosci jednostki potrzebuje takze magistrala: bez niej slot nie umie
///       odpowiedziec, ktora jednostke trzeba zatrzymac, zeby zwolnic kolidujaca nazwe.
[[nodiscard]] SystemdIdentity detectSystemdIdentity();

/// Obiekt klasy FlockServiceGuard powinien:
/// - [lockmanager.flock-service-guard.1~1] wyznaczać ścieżkę pliku blokady instancji z nazwy serwera w domyślnym katalogu blokad albo w katalogu ustawionym przez setLockDir.
/// - [lockmanager.flock-service-guard.2~1] w acquireLock przejmować nieblokująco wyłączną blokadę pliku instancji według protokołu lockFile, a po przejęciu
///   podejmować próbę obcięcia pliku do zera bez zapisu opisu procesu; przy błędzie ftruncate zachowywać blokadę,
///   wpisywać ostrzeżenie do logu i zwracać sukces.
/// - [lockmanager.flock-service-guard.3~1] zwracać z acquireLock fałsz z pustym acquireLockError, gdy blokadę trzyma inna instancja, a fałsz z opisem przyczyny, gdy plik nie mógł zostać otwarty lub nie przeszedł kontroli (dowiązanie, nie zwykły plik, obcy właściciel, wiele dowiązań).
/// - [lockmanager.flock-service-guard.4~1] w acquireIpcLock przejmować drugą wyłączną blokadę dla nazwy obiektu IPC w katalogu blokad maszyny, wspólnym dla wszystkich katalogów blokad instancji, wyłącznie gdy blokada instancji jest aktywna i blokada IPC nie jest jeszcze trzymana.
///   Zakres testow: `LockFileSafety.IpcLockRequiresInstanceLockAndCannotBeAcquiredTwice`, `LockManagerFlock.HashCollisionCannotAcquireTheSameIpcIdentity`, `LockManagerFlock.IpcIdentityLockNeedsNoWriteAccess`; integracja `it_ipc_identity_lock`.
/// - [lockmanager.flock-service-guard.5~1] w publishLockInfo zapisywać do trzymanego pliku blokady, od początku, linie PID, PPID, MODE (service lub process),
///   przy usłudze UNIT i SCOPE (system lub user) oraz QUERYFILE, gdy ustawiono plik zapytań, jednym write, a potem fsync;
///   zwracać fałsz przy nieaktywnej blokadzie, błędzie ftruncate, lseek lub fsync albo wyniku write == -1.
///   Krótkiego zapisu, w tym 0 bajtów, nie rozpoznawać jako błędu.
///   Nietestowalne: galezie UNIT/SCOPE wykonywane tylko jesli test dziala w jednostce; kontrola strukturalna writeLockInfo i testy odczytu obu zakresow nie zastepuja takiego wykonania.
/// - [lockmanager.flock-service-guard.6~1] przez readPeerInfo odczytywać z pliku blokady innej instancji PID, tryb, jednostkę, zakres i plik zapytań, zwracając wartości Unknown i pid 0 dla brakującego pliku lub brakujących linii.
/// - [lockmanager.flock-service-guard.7~1] przez isAnotherInstanceRunning sprawdzać, czy blokadę pliku instancji trzyma jakikolwiek proces, a przez isLockActive, czy trzyma ją ten obiekt.
/// - [lockmanager.flock-service-guard.8~1] w releaseLock i przy zniszczeniu usuwać pliki obu trzymanych blokad jeszcze pod blokadą i zwalniać ich deskryptory, idempotentnie.
/// - [lockmanager.flock-service-guard.9~1] uniemożliwiać kopiowanie i przenoszenie, tak aby adres strażnika używany przez obsługę wyjścia pozostawał ważny.
///   Sprawdzenie kompilacji: test/UnitTest/test_lockManager.cpp, cztery static_assert cech kopiowania i przenoszenia FlockServiceGuard.
class FlockServiceGuard {
 public:
  // Tryb działania instancji trzymającej blokadę, odczytany z pliku blokady.
  // Service => proces jest jednostką systemd (znana nazwa unitu => możliwy systemctl restart).
  // Process => zwykły proces uruchomiony z linii poleceń.
  // Unknown => starszy/niepełny plik blokady albo brak danych.
  /// @requirements-exempt PeerInfo: Dane wlasciciela blokady i jego uslugi, bez logiki sprawdzania blokad.
  struct PeerInfo {
    enum class Kind : std::uint8_t { Unknown, Process, Service };
    enum class Scope : std::uint8_t { Unknown, System, User };  // systemctl restart vs systemctl --user restart
    Kind kind{Kind::Unknown};
    Scope scope{Scope::Unknown};
    int pid{0};             // PID właściciela blokady; 0 => nieznany. Do diagnostyki odmowy startu.
    std::string unit;       // nazwa jednostki systemd, gdy kind == Service
    std::string queryFile;  // plik zapytań, z którego serwis wystartował (do nadpisania przy restarcie)
  };

 private:
  std::string lockFilePath;
  std::string ipcLockPath;
  std::string serviceQueryFile;  // plik zapytań tej instancji; zapisywany do locka jako QUERYFILE
  std::string lockError;
  int lockFileDescriptor{-1};
  int ipcLockDescriptor{-1};
  bool isLocked{false};

 public:
  explicit FlockServiceGuard(const std::string &serviceName);
  ~FlockServiceGuard();

  // Bez kopiowania i bez przenoszenia: handler wyjscia zwalnia blokade przez adres straznika
  // (serviceGuardPtr), wiec obiekt nie moze zmienic ani wlasciciela deskryptorow, ani miejsca.
  FlockServiceGuard(const FlockServiceGuard &)            = delete;
  FlockServiceGuard &operator=(const FlockServiceGuard &) = delete;
  FlockServiceGuard(FlockServiceGuard &&)                 = delete;
  FlockServiceGuard &operator=(FlockServiceGuard &&)      = delete;

  // Przejmuje wylaczna blokade i ZERUJE plik. Tresci (PID/MODE/UNIT/...) jeszcze nie zapisuje --
  // robi to publishLockInfo(). Rozdzielenie jest wymogiem poprawnosci startu: wylacznosc musi byc
  // ustalona ZANIM instancja dotknie obiektow IPC, ale pusty plik nie moze udawac gotowego serwera
  // przed klientami, ktorzy czekaja na linie "PID: <pid>".
  bool acquireLock();
  // Pusty przy zajetej blokadzie; blad otwarcia/walidacji nie oznacza zywego serwera.
  [[nodiscard]] const std::string &acquireLockError() const { return lockError; }

  // Druga blokada chroni faktyczna nazwe IPC, takze przy kolizji skrotow nazw.
  // Wspolna dla wszystkich TMPDIR i przestrzeni magistrali. Przed dotknieciem IPC.
  // machineLockDir inny niz domyslny podaja tylko testy.
  bool acquireIpcLock(std::string_view objectName, std::string_view machineLockDir = ipc::kMachineLockDir);

  // Pliki obu blokad kasuje releaseLock(), zgodnie z protokolem z lockFile.hpp.

  // Zapisuje do trzymanej blokady informacje o procesie. Wolac dopiero gdy instancja jest
  // gotowa obsluzyc klientow -- pojawienie sie linii "PID: <pid>" jest dla nich sygnalem startu.
  bool publishLockInfo();

  void setLockDir(const std::string &dir);
  // Katalog pliku blokady instancji (TMPDIR albo paths.lock_dir) - tam sprzata sweepAbandonedResources.
  [[nodiscard]] std::string lockDirectory() const;
  // Ścieżka pliku zapytań tej instancji - zapisywana do locka jako QUERYFILE, by inna instancja
  // wiedziała, który plik nadpisać przed restartem serwisu. Ustawić przed acquireLock().
  void setServiceQueryFile(const std::string &queryFile);
  // Plik zapytań tej instancji (pusty => nie podano). Czyta go przeładowanie w locie:
  // zaakceptowany plan trafia TAM, żeby restart usługi wznowił to, co faktycznie liczy.
  [[nodiscard]] const std::string &getServiceQueryFile() const { return serviceQueryFile; }
  [[nodiscard]] bool isLockActive() const;
  void releaseLock();
  [[nodiscard]] bool isAnotherInstanceRunning() const;

  // Odczytuje tryb (serwis/proces) i nazwę unitu z pliku blokady trzymanego przez inną instancję.
  // Wywoływane przez nową instancję po nieudanym acquireLock().
  [[nodiscard]] PeerInfo readPeerInfo() const;

 private:
  [[nodiscard]] bool writeLockInfo() const;
};

/// Liczba usunietych pozostalosci, osobno dla kazdego rodzaju.
/// @requirements-exempt SweepReport: Liczniki zasobow usunietych przez operacje sprzatania.
struct SweepReport {
  std::size_t serviceLocks{0};   ///< pliki blokad instancji w katalogu blokad
  std::size_t ipcIdentities{0};  ///< blokady tozsamosci IPC razem z globalnymi obiektami IPC
  std::size_t busSegments{0};    ///< segmenty magistrali, ktorych nikt nie mapuje
};

/// Funkcja sweepAbandonedResources powinna:
/// - [lockmanager.sweep-abandoned-resources.1~1] przez lockfile::sweep usuwać w serviceLockDir porzucone pliki blokad instancji (`xretractor_service.lock` albo `xretractor_service.<poprawna nazwa>.lock`) i liczyć je w serviceLocks.
///   Zakres testow: `LockManagerSweep.RemovesOnlyAbandonedLocksAndTheirIpcObjects`, `ServiceLockFamily.NamesRoundTripAndForeignFilesAreRejected`; integracja `it_leftover_sweep`.
/// - [lockmanager.sweep-abandoned-resources.2~1] przez lockfile::sweep usuwać w machineLockDir porzucone blokady tożsamości IPC instancji bezimiennej albo o członie będącym poprawną nazwą serwera lub jej skrótem, a pod tą blokadą usuwać przez ipc::security::removeOwned segment pamięci dzielonej i kolejkę komend tego członu, licząc je w ipcIdentities.
///   Zakres testow: `LockManagerSweep.RemovesOnlyAbandonedLocksAndTheirIpcObjects`; integracje `it_ipc_identity_lock`, `it_leftover_sweep`. Nietestowalne: odmiany czlonu pustego i skroconego sa sprawdzone strukturalnie w ipcTokenOf/isInstanceToken; nie usuwamy globalnych obiektow bezimiennego serwera na maszynie testujacej.
/// - [lockmanager.sweep-abandoned-resources.3~1] usuwać porzucone segmenty magistrali przez bus::sweepAbandonedSegments(machineLockDir) i liczyć je w busSegments.
/// - [lockmanager.sweep-abandoned-resources.4~1] nie ruszać niczego, czego blokady nie da się zająć wyłącznie, plików innych kont ani kolejek odpowiedzi klientów.
///   Zakres testow: `LockManagerSweep.RemovesOnlyAbandonedLocksAndTheirIpcObjects`, `IpcSecurity.SweepDoesNotRemoveForeignLockOrInvokeItsCleanup`; integracje `it_leftover_sweep`, `it_ipc_security-foreign-uid` (root lub jawny skip). Nietestowalne: brak wyliczania kolejek klientow - kontrola strukturalna sweepAbandonedResources, ktora przeglada tylko rodziny blokad.
///
/// @note Normalny koniec procesu sprzata po sobie sam; tu trafia to, co zostawil proces zabity (SIGKILL, OOM,
///       awaria zasilania). Bezpieczne przy dzialajacych instancjach, bo zywa instancja trzyma swoje blokady
///       do konca. Kolejek odpowiedzi nie da sie wyliczyc bez listowania /dev/shm. machineLockDir inny niz
///       domyslny podaja tylko testy.
SweepReport sweepAbandonedResources(const std::string &serviceLockDir, std::string_view machineLockDir = ipc::kMachineLockDir);
