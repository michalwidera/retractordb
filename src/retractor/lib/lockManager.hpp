#pragma once

#include <cstdint>
#include <optional>
#include <string>

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
  std::string serviceQueryFile;  // plik zapytań tej instancji; zapisywany do locka jako QUERYFILE
  int lockFileDescriptor{-1};
  bool isLocked{false};

 public:
  explicit FlockServiceGuard(const std::string &serviceName);
  ~FlockServiceGuard();

  // Przejmuje wylaczna blokade i ZERUJE plik. Tresci (PID/MODE/UNIT/...) jeszcze nie zapisuje --
  // robi to publishLockInfo(). Rozdzielenie jest wymogiem poprawnosci startu: wylacznosc musi byc
  // ustalona ZANIM instancja dotknie obiektow IPC, ale pusty plik nie moze udawac gotowego serwera
  // przed klientami, ktorzy czekaja na linie "PID: <pid>".
  bool acquireLock();

  // Zapisuje do trzymanej blokady informacje o procesie. Wolac dopiero gdy instancja jest
  // gotowa obsluzyc klientow -- pojawienie sie linii "PID: <pid>" jest dla nich sygnalem startu.
  bool publishLockInfo();

  void setLockDir(const std::string &dir);
  // Ścieżka pliku zapytań tej instancji — zapisywana do locka jako QUERYFILE, by inna instancja
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
