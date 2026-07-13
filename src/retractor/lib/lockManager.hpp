#pragma once

#include <cstdint>
#include <string>

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

  bool acquireLock();
  void setLockDir(const std::string &dir);
  // Ścieżka pliku zapytań tej instancji — zapisywana do locka jako QUERYFILE, by inna instancja
  // wiedziała, który plik nadpisać przed restartem serwisu. Ustawić przed acquireLock().
  void setServiceQueryFile(const std::string &queryFile);
  [[nodiscard]] bool isLockActive() const;
  void releaseLock();
  [[nodiscard]] bool isAnotherInstanceRunning() const;

  // Odczytuje tryb (serwis/proces) i nazwę unitu z pliku blokady trzymanego przez inną instancję.
  // Wywoływane przez nową instancję po nieudanym acquireLock().
  [[nodiscard]] PeerInfo readPeerInfo() const;

 private:
  [[nodiscard]] bool writeLockInfo() const;
  void cleanupLockFile();
};
