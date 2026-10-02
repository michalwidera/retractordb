#include "lockManager.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include <spdlog/spdlog.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "bus.hpp"
#include "constants.hpp"
#include "lockFile.hpp"
#include "osPlatform.hpp"
#include "serverName.hpp"

namespace IPC = boost::interprocess;

namespace {

constexpr std::string_view kLockSuffix = ".lock";

/// Czy napis jest czlonem instancji z nazw IPC: sama nazwa serwera albo jej skrot ("0" i osiem
/// malych cyfr szesnastkowych, patrz ipc::shortServerTag).
bool isInstanceToken(std::string_view token) {
  if (servername::isValid(token)) return true;
  return token.size() == ipc::kShortServerTagLength && token.front() == '0' &&
         std::ranges::all_of(token, [](unsigned char c) { return std::isdigit(c) || (c >= 'a' && c <= 'f'); });
}

/// Czlon instancji odczytany z nazwy pliku blokady tozsamosci IPC, pusty dla instancji
/// bezimiennej; nullopt, gdy plik nie jest taka blokada.
std::optional<std::string_view> ipcTokenOf(std::string_view file) {
  constexpr std::string_view prefix = ipc::kIdentityLockPrefix;
  if (file.size() < prefix.size() + kLockSuffix.size() || !file.starts_with(prefix) || !file.ends_with(kLockSuffix))
    return std::nullopt;
  const std::string_view queue = file.substr(prefix.size(), file.size() - prefix.size() - kLockSuffix.size());
  if (queue == ipc::kQueryQueue) return std::string_view{};
  if (queue.size() <= ipc::kQueryQueue.size() + 1 || !queue.starts_with(ipc::kQueryQueue) ||
      queue[ipc::kQueryQueue.size()] != '.')
    return std::nullopt;
  const std::string_view token = queue.substr(ipc::kQueryQueue.size() + 1);
  if (!isInstanceToken(token)) return std::nullopt;
  return token;
}

/// Czy plik jest blokada instancji: xretractor_service.lock albo xretractor_service.<nazwa>.lock.
bool isServiceLockName(std::string_view file) {
  const std::optional<std::string_view> name = ipc::serviceLockInstance(file);
  return name && (name->empty() || servername::isValid(*name));
}

}  // namespace

// Ustala wlasna tozsamosc w menedzerze uslug systemu. Sam odczyt jest platformowy i siedzi
// w osplat::detectServiceIdentity (Linux: /proc/self/cgroup i jednostka systemd; Darwin:
// etykieta zadania launchd). Tutaj zostaje samo przeniesienie wyniku do typu, ktorego
// uzywa reszta drzewa.
//
// Deklaracja stoi w lockManager.hpp: tozsamosci jednostki potrzebuje takze magistrala xrdbbus.
SystemdIdentity detectSystemdIdentity() {
  const osplat::ServiceIdentity identity = osplat::detectServiceIdentity();
  return SystemdIdentity{.unit = identity.unit, .userScope = identity.userScope};
}

FlockServiceGuard::FlockServiceGuard(const std::string &serviceName)

{
  lockFilePath = ipc::serviceLockDir({}) / ipc::serviceLockFile(serviceName);
}

FlockServiceGuard::~FlockServiceGuard() { releaseLock(); }

void FlockServiceGuard::setLockDir(const std::string &dir) {
  const std::filesystem::path lockName = std::filesystem::path(lockFilePath).filename();
  lockFilePath                         = (ipc::serviceLockDir(dir) / lockName).string();
}

void FlockServiceGuard::setServiceQueryFile(const std::string &queryFile) { serviceQueryFile = queryFile; }

std::string FlockServiceGuard::lockDirectory() const { return std::filesystem::path(lockFilePath).parent_path().string(); }

bool FlockServiceGuard::acquireLock() {
  // Komunikat dla operatora nalezy do wolajacego: tylko on wie, ktora tozsamosc probowal
  // przejac i co odczytal z pliku blokady (patrz launcher.cpp). Tutaj zostaje sam log,
  // zeby ta sama odmowa nie pojawiala sie na konsoli dwa razy.
  switch (lockfile::acquire(lockFilePath, true, true, lockFileDescriptor)) {
    case lockfile::Result::Acquired:
      break;
    case lockfile::Result::Busy:
      SPDLOG_WARN("Other instance is already running, cannot acquire lock on: {}", lockFilePath);
      return false;
    case lockfile::Result::Error:
      SPDLOG_ERROR("Cannot acquire lock file: {} , errno: {}", lockFilePath, strerror(errno));
      return false;
  }

  isLocked = true;

  // Plik zerujemy natychmiast, ale opisu procesu jeszcze nie piszemy (patrz publishLockInfo).
  // Po padzie poprzednika zostaje w pliku JEGO opis, a nikt go nie prostuje az do publikacji --
  // pusty plik czyta sie jako "instancja wstaje", nie jako cudzy, nieaktualny serwer.
  if (ftruncate(lockFileDescriptor, 0) == -1) {
    SPDLOG_WARN("Cannot truncate lock file: {}, errno: {}", lockFilePath, strerror(errno));
  }

  return true;
}

bool FlockServiceGuard::acquireIpcLock(std::string_view objectName) {
  if (!isLockActive() || ipcLockDescriptor != -1) return false;
  // Nie uzywamy TMPDIR ani lock.dir: te same obiekty IPC moga byc osiagalne
  // z roznych katalogow blokad instancji.
  const std::string path = ipc::identityLockPath(objectName);
  int fd                 = -1;
  switch (lockfile::acquire(path, true, false, fd)) {
    case lockfile::Result::Acquired:
      break;
    case lockfile::Result::Busy:
      SPDLOG_ERROR("IPC identity lock {} is held by another instance.", path);
      return false;
    case lockfile::Result::Error:
      SPDLOG_ERROR("Cannot acquire IPC identity lock {}: {}", path, strerror(errno));
      return false;
  }
  ipcLockPath       = path;
  ipcLockDescriptor = fd;
  return true;
}

bool FlockServiceGuard::publishLockInfo() {
  if (!isLockActive()) return false;

  if (!writeLockInfo()) {
    SPDLOG_WARN("Cannot write process info to lock file: {}, bypass.", lockFilePath);
    return false;
  }
  return true;
}

bool FlockServiceGuard::isLockActive() const { return isLocked && lockFileDescriptor != -1; }

void FlockServiceGuard::releaseLock() {
  // Pliki kasuje wlasciciel, jeszcze pod blokada - patrz lockFile.hpp. Wczesniej zostawaly na
  // dysku, po jednym na kazda nazwe instancji, ktora kiedykolwiek wystartowala.
  if (ipcLockDescriptor != -1) {
    lockfile::removeAndRelease(ipcLockPath, ipcLockDescriptor);
    ipcLockDescriptor = -1;
  }
  if (isLocked && lockFileDescriptor != -1) {
    lockfile::removeAndRelease(lockFilePath, lockFileDescriptor);
    lockFileDescriptor = -1;
    isLocked           = false;
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
    if (key == "PID:") {
      ls >> info.pid;
      if (ls.fail()) info.pid = 0;
    } else if (key == "MODE:") {
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
  // z własnego /proc/self/cgroup - pewniej niż z flagi logowania (-j), która nie oznacza,
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

SweepReport sweepAbandonedResources(const std::string &serviceLockDir) {
  SweepReport retVal;
  retVal.serviceLocks = lockfile::sweep(serviceLockDir, isServiceLockName, [](std::string_view) {});
  // Porzucona blokada tozsamosci dowodzi, ze zaden zywy serwer nie uzywa obiektow tego czlonu:
  // serwer zajmuje ja przed ich utworzeniem i zwalnia dopiero po ich skasowaniu. Kolejek
  // odpowiedzi klientow nie da sie tu wyliczyc bez listowania /dev/shm, wiec zostaja - usuwa je
  // serwer przy wyjsciu, a klient, ktory dostanie ten sam identyfikator, zaklada je od nowa.
  retVal.ipcIdentities = lockfile::sweep(
      std::string(ipc::kMachineLockDir), [](std::string_view file) { return ipcTokenOf(file).has_value(); },
      [](std::string_view file) {
        const ipc::ServerNames names = ipc::namesForToken(*ipcTokenOf(file));
        IPC::shared_memory_object::remove(names.shmemSegment.c_str());
        IPC::message_queue::remove(names.queryQueue.c_str());
      });
  retVal.busSegments = bus::sweepAbandonedSegments();
  return retVal;
}
