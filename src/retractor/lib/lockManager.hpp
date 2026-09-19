#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/// Tozsamosc systemd biezacego procesu, ustalona z /proc/self/cgroup.
struct SystemdIdentity {
  std::optional<std::string> unit;  // nazwa jednostki, gdy proces jest jednostka systemd
  bool userScope{false};            // true => user.slice (systemctl --user), false => system
};

/// Ustala wlasna tozsamosc systemd. unit == nullopt => zwykly proces.
///
/// Wyniesione z lockManager.cpp, bo tozsamosci jednostki potrzebuje takze magistrala: bez niej
/// slot nie umie odpowiedziec, ktora jednostke trzeba zatrzymac, zeby zwolnic kolidujaca nazwe.
[[nodiscard]] SystemdIdentity detectSystemdIdentity();

class FlockServiceGuard {
 public:
  // Tryb działania instancji trzymającej blokadę, odczytany z pliku blokady.
  // Service => proces jest jednostką systemd (znana nazwa unitu => możliwy systemctl restart).
  // Process => zwykły proces uruchomiony z linii poleceń.
  // Unknown => starszy/niepełny plik blokady albo brak danych.
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
  int lockFileDescriptor{-1};
  int ipcLockDescriptor{-1};
  bool isLocked{false};

 public:
  explicit FlockServiceGuard(const std::string &serviceName);
  ~FlockServiceGuard();

  // Przejmuje wylaczna blokade i ZERUJE plik. Tresci (PID/MODE/UNIT/...) jeszcze nie zapisuje --
  // robi to publishLockInfo(). Rozdzielenie jest wymogiem poprawnosci startu: wylacznosc musi byc
  // ustalona ZANIM instancja dotknie obiektow IPC, ale pusty plik nie moze udawac gotowego serwera
  // przed klientami, ktorzy czekaja na linie "PID: <pid>".
  bool acquireLock();

  // Druga blokada chroni faktyczna nazwe IPC, takze przy kolizji skrotow nazw.
  // Wspolna dla wszystkich TMPDIR i przestrzeni magistrali. Przed dotknieciem IPC.
  bool acquireIpcLock(std::string_view objectName);

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
struct SweepReport {
  std::size_t serviceLocks{0};   ///< pliki blokad instancji w katalogu blokad
  std::size_t ipcIdentities{0};  ///< blokady tozsamosci IPC razem z globalnymi obiektami IPC
  std::size_t busSegments{0};    ///< segmenty magistrali, ktorych nikt nie mapuje
};

/// Usuwa pozostalosci instancji, ktore nie zyja: pliki blokad, globalne obiekty IPC i segmenty
/// magistrali. Normalny koniec procesu sprzata po sobie sam; tu trafia to, co zostawil proces
/// zabity (SIGKILL, OOM, awaria zasilania). Bezpieczne w kazdej chwili i przy dzialajacych
/// instancjach: usuwa wylacznie to, czego blokade udalo sie zajac wylacznie (lockFile.hpp),
/// a zywa instancja trzyma swoje blokady do konca.
SweepReport sweepAbandonedResources(const std::string &serviceLockDir);
