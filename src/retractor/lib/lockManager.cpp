#include "lockManager.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

#include <spdlog/spdlog.h>

namespace {

struct SystemdIdentity {
  std::optional<std::string> unit;  // nazwa jednostki, gdy proces jest jednostką systemd
  bool userScope{false};            // true => user.slice (systemctl --user), false => system
};

// Ustala własną tożsamość systemd na podstawie /proc/self/cgroup. systemd umieszcza jednostkę
// w ścieżce cgroup typu ".../system.slice/xretractor.service" lub (dla --user)
// ".../user.slice/user@1000.service/.../xretractor.service". Zwraca nazwę unitu, gdy proces jest
// jednostką systemd; unit == nullopt gdy to zwykły proces.
SystemdIdentity detectSystemdIdentity() {
  SystemdIdentity id;

  std::ifstream cgroup("/proc/self/cgroup");
  if (!cgroup.is_open()) return id;

  std::string line;
  while (std::getline(cgroup, line)) {
    // Format: "hierarchy:controllers:path" (v2: "0::/...path"). Interesuje nas ostatnie pole.
    const auto lastColon = line.rfind(':');
    if (lastColon == std::string::npos) continue;
    std::string_view path(line);
    path.remove_prefix(lastColon + 1);

    // Zakres user, gdy ścieżka cgroup biegnie przez user.slice / user@<uid>.service.
    const bool userScope = path.find("/user.slice") != std::string_view::npos || path.find("/user@") != std::string_view::npos;

    // Ostatni (najgłębszy) segment ścieżki kończący się na ".service" jest nazwą naszego unitu;
    // pomijamy user@<uid>.service, który jest menedżerem sesji, nie naszą jednostką.
    std::string_view scan = path;
    while (!scan.empty()) {
      const auto slash     = scan.rfind('/');
      std::string_view seg = (slash == std::string_view::npos) ? scan : scan.substr(slash + 1);
      if (seg.ends_with(".service") && !seg.starts_with("user@")) {
        id.unit      = std::string(seg);
        id.userScope = userScope;
        return id;
      }
      if (slash == std::string_view::npos) break;
      scan = scan.substr(0, slash);
    }
  }
  return id;
}

}  // namespace

FlockServiceGuard::FlockServiceGuard(const std::string &serviceName)

{
  lockFilePath = std::filesystem::temp_directory_path() / (serviceName + ".lock");
}

FlockServiceGuard::~FlockServiceGuard() { releaseLock(); }

void FlockServiceGuard::setLockDir(const std::string &dir) {
  if (dir.empty()) return;
  const std::filesystem::path lockName = std::filesystem::path(lockFilePath).filename();
  lockFilePath                         = (std::filesystem::path(dir) / lockName).string();
}

void FlockServiceGuard::setServiceQueryFile(const std::string &queryFile) { serviceQueryFile = queryFile; }

bool FlockServiceGuard::acquireLock() {
  // Open or create the lock file
  lockFileDescriptor = open(lockFilePath.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (lockFileDescriptor == -1) {
    SPDLOG_ERROR("Failed to open lock file: {} ,errno: {}", lockFilePath, strerror(errno));
    return false;
  }

  // Non-blocking exclusive lock
  int flockResult = flock(lockFileDescriptor, LOCK_EX | LOCK_NB);

  if (flockResult == -1) {
    std::cerr << "Another instance is running, errno: " << strerror(errno) << '\n';
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      SPDLOG_WARN("Other instance is already running, cannot acquire lock on: {}", lockFilePath);
    } else {
      SPDLOG_ERROR("Unexpected error while locking: {} , errno: {}", lockFilePath, strerror(errno));
    }

    close(lockFileDescriptor);
    lockFileDescriptor = -1;
    return false;
  }

  isLocked = true;

  if (!writeLockInfo()) {
    SPDLOG_WARN("Cannot write process info to lock file: {}, bypass.", lockFilePath);
  }

  return true;
}

bool FlockServiceGuard::isLockActive() const { return isLocked && lockFileDescriptor != -1; }

void FlockServiceGuard::releaseLock() {
  if (isLocked && lockFileDescriptor != -1) {
    if (flock(lockFileDescriptor, LOCK_UN) == -1) {
      SPDLOG_WARN("Failed to release lock: {} , errno: {}", lockFilePath, strerror(errno));
    }

    if (close(lockFileDescriptor) == -1) {
      SPDLOG_WARN("Failed to close lock file descriptor: {} , errno: {}", lockFilePath, strerror(errno));
    }

    lockFileDescriptor = -1;
    isLocked           = false;

    cleanupLockFile();
  }
}

bool FlockServiceGuard::isAnotherInstanceRunning() const {
  int testFd = open(lockFilePath.c_str(), O_RDONLY);
  if (testFd == -1) {
    return false;
  }

  int result     = flock(testFd, LOCK_SH | LOCK_NB);
  bool isRunning = (result == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));

  close(testFd);
  return isRunning;
}

FlockServiceGuard::PeerInfo FlockServiceGuard::readPeerInfo() const {
  PeerInfo info;

  std::ifstream lockFile(lockFilePath);
  if (!lockFile.is_open()) return info;  // brak pliku => Unknown

  std::string line;
  while (std::getline(lockFile, line)) {
    std::istringstream ls(line);
    std::string key;
    ls >> key;
    if (key == "MODE:") {
      std::string value;
      ls >> value;
      if (value == "service")
        info.kind = PeerInfo::Kind::Service;
      else if (value == "process")
        info.kind = PeerInfo::Kind::Process;
    } else if (key == "UNIT:") {
      ls >> info.unit;
    } else if (key == "SCOPE:") {
      std::string value;
      ls >> value;
      if (value == "system")
        info.scope = PeerInfo::Scope::System;
      else if (value == "user")
        info.scope = PeerInfo::Scope::User;
    } else if (key == "QUERYFILE:") {
      ls >> info.queryFile;
    }
  }
  return info;
}

bool FlockServiceGuard::writeLockInfo() const {
  if (lockFileDescriptor == -1) {
    return false;
  }

  // Clear the file first
  if (ftruncate(lockFileDescriptor, 0) == -1) {
    return false;
  }

  // Move to the beginning of the file
  if (lseek(lockFileDescriptor, 0, SEEK_SET) == -1) {
    return false;
  }

  // Process information
  std::string processInfo = "PID: " + std::to_string(getpid()) + "\n";
  processInfo += "PPID: " + std::to_string(getppid()) + "\n";

  // Tryb działania: serwis (jednostka systemd) vs zwykły proces. Tożsamość systemd ustalamy
  // z własnego /proc/self/cgroup — pewniej niż z flagi logowania (-j), która nie oznacza,
  // że proces jest restartowalną jednostką. UNIT/SCOPE są potrzebne do systemctl [--user] restart,
  // a QUERYFILE wskazuje plik zapytań do nadpisania przez inną instancję przed restartem.
  const SystemdIdentity id = detectSystemdIdentity();
  processInfo += "MODE: " + std::string(id.unit ? "service" : "process") + "\n";
  if (id.unit) {
    processInfo += "UNIT: " + *id.unit + "\n";
    processInfo += "SCOPE: " + std::string(id.userScope ? "user" : "system") + "\n";
  }
  if (!serviceQueryFile.empty()) processInfo += "QUERYFILE: " + serviceQueryFile + "\n";

  // Write to the file
  ssize_t written = write(lockFileDescriptor, processInfo.c_str(), processInfo.length());
  if (written == -1) {
    return false;
  }

  // Ensure data is written to disk
  if (fsync(lockFileDescriptor) == -1) {
    return false;
  }

  return true;
}

void FlockServiceGuard::cleanupLockFile() { unlink(lockFilePath.c_str()); }
