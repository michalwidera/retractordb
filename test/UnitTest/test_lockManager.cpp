#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "constants.hpp"
#include "platformConfig.h"
#include "retractor/lib/lockFile.hpp"
#include "retractor/lib/lockManager.hpp"
#include "retractor/lib/serviceControl.hpp"

using servicecontrol::deliverQueryFile;
using servicecontrol::restartCommand;
using servicecontrol::restartService;

namespace {

// Zapisuje syntetyczny plik blokady o znanej ścieżce i zwraca skonfigurowany guard,
// który tę ścieżkę odczyta (setLockDir + nazwa serwisu wyznaczają lockFilePath).
std::filesystem::path writeLockFile(const std::string &dir, const std::string &serviceName, const std::string &content) {
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = std::filesystem::path(dir) / (serviceName + ".lock");
  std::ofstream out(path, std::ios::trunc);
  out << content;
  return path;
}

}  // namespace

// --- readPeerInfo: parsowanie MODE/UNIT/SCOPE/QUERYFILE ---

TEST(LockManagerPeerInfo, parses_service_with_unit_scope_and_queryfile) {
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_svc").string();
  const std::string svc = "ut_svc";
  writeLockFile(dir, svc,
                "PID: 1234\nPPID: 1\nMODE: service\nUNIT: xretractor.service\nSCOPE: system\n"
                "QUERYFILE: /etc/retractor/startup.rql\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Service);
  EXPECT_EQ(info.scope, FlockServiceGuard::PeerInfo::Scope::System);
  EXPECT_EQ(info.unit, "xretractor.service");
  EXPECT_EQ(info.queryFile, "/etc/retractor/startup.rql");

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, parses_user_scope) {
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_user").string();
  const std::string svc = "ut_user";
  writeLockFile(dir, svc, "PID: 5\nMODE: service\nUNIT: xretractor.service\nSCOPE: user\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Service);
  EXPECT_EQ(info.scope, FlockServiceGuard::PeerInfo::Scope::User);

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, plain_process_lock_yields_process_kind) {
  const std::string dir = (std::filesystem::temp_directory_path() / "ut_lockmgr_proc").string();
  const std::string svc = "ut_proc";
  writeLockFile(dir, svc, "PID: 9\nPPID: 1\nMODE: process\n");

  FlockServiceGuard guard(svc);
  guard.setLockDir(dir);
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Process);
  EXPECT_TRUE(info.unit.empty());
  EXPECT_TRUE(info.queryFile.empty());

  std::filesystem::remove_all(dir);
}

TEST(LockManagerPeerInfo, missing_lock_file_yields_unknown) {
  FlockServiceGuard guard("ut_absent");
  guard.setLockDir((std::filesystem::temp_directory_path() / "ut_lockmgr_absent").string());
  const FlockServiceGuard::PeerInfo info = guard.readPeerInfo();

  EXPECT_EQ(info.kind, FlockServiceGuard::PeerInfo::Kind::Unknown);
}

TEST(LockManagerFlock, releaseRemovesFileAndOrphanedOpenerCannotWin) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / ("ut_lockmgr_inode_" + std::to_string(getpid()));
  const std::string serviceName   = "xretractor_service.alfa";
  const std::filesystem::path lockPath = dir / (serviceName + ".lock");
  std::filesystem::create_directories(dir);
  std::filesystem::remove(lockPath);

  FlockServiceGuard first(serviceName);
  first.setLockDir(dir.string());
  ASSERT_TRUE(first.acquireLock());

  // Drugi uczestnik otwiera TEN SAM i-wezel jeszcze przed zwolnieniem pierwszej blokady. Po
  // skasowaniu pliku zajmie flock na i-wezle bez nazwy. Dawne kasowanie przy zwolnieniu dawalo
  // wtedy dwoch wlascicieli, bo trzeci tworzyl pod ta sciezka nowy plik; wyscig zamyka
  // sprawdzenie i-wezla po zajeciu blokady (lockFile.hpp).
  const int secondFd = open(lockPath.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_NE(secondFd, -1);

  first.releaseLock();
  EXPECT_FALSE(std::filesystem::exists(lockPath)) << "wlasciciel nie skasowal pliku przy zwolnieniu";
  ASSERT_EQ(flock(secondFd, LOCK_EX | LOCK_NB), 0);
  EXPECT_FALSE(lockfile::stillLinked(secondFd, lockPath.string())) << "blokada na osieroconym i-wezle wyglada na wazna";

  // Uczestnicy protokolu widza jednego wlasciciela: trzeci zajmuje nowy plik, czwarty odpada.
  FlockServiceGuard third(serviceName);
  third.setLockDir(dir.string());
  EXPECT_TRUE(third.acquireLock());
  FlockServiceGuard fourth(serviceName);
  fourth.setLockDir(dir.string());
  EXPECT_FALSE(fourth.acquireLock());

  EXPECT_EQ(close(secondFd), 0);
  third.releaseLock();
  std::filesystem::remove_all(dir);
}

// --- restartService: składanie argv przez wstrzykiwalny runner ---

TEST(ServiceControlRestart, builds_system_scope_argv) {
  std::vector<std::string> captured;
  const int rc = restartService(/*userScope=*/false, "xretractor.service", [&](const std::vector<std::string> &argv) {
    captured = argv;
    return 0;
  });

  EXPECT_EQ(rc, 0);
  // Program i skladnia zaleza od menedzera uslug systemu, wiec oczekiwanie bierzemy
  // z restartCommand - jedynego miejsca, w ktorym ta roznica jest zapisana. Test
  // pilnuje tego, co go naprawde dotyczy: ze restartService NIE ZMIENIA polecenia
  // po drodze i podaje je runnerowi w calosci.
  EXPECT_EQ(captured, restartCommand(/*userScope=*/false, "xretractor.service"));
#if RDB_HAS_SYSTEMD
  const std::vector<std::string> expected{"systemctl", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
#elif RDB_HAS_LAUNCHD
  ASSERT_GE(captured.size(), 4U);
  EXPECT_EQ(captured[0], "launchctl");
  EXPECT_EQ(captured[1], "kickstart");
  EXPECT_EQ(captured[2], "-k");
  EXPECT_EQ(captured[3], "system/xretractor.service");
#endif
}

TEST(ServiceControlRestart, builds_user_scope_argv) {
  std::vector<std::string> captured;
  restartService(/*userScope=*/true, "xretractor.service", [&](const std::vector<std::string> &argv) {
    captured = argv;
    return 0;
  });

  EXPECT_EQ(captured, restartCommand(/*userScope=*/true, "xretractor.service"));
#if RDB_HAS_SYSTEMD
  const std::vector<std::string> expected{"systemctl", "--user", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
#elif RDB_HAS_LAUNCHD
  // Zakres uzytkownika to u launchd INNA DOMENA, a nie dodatkowy przelacznik.
  ASSERT_GE(captured.size(), 4U);
  EXPECT_EQ(captured[0], "launchctl");
  EXPECT_TRUE(captured[3].starts_with("gui/")) << "polecenie nie trafia w domene uzytkownika: " << captured[3];
  EXPECT_TRUE(captured[3].ends_with("/xretractor.service"));
#endif
}

TEST(ServiceControlRestart, propagates_runner_exit_code) {
  const int rc = restartService(false, "x.service", [](const std::vector<std::string> &) { return 5; });
  EXPECT_EQ(rc, 5);
}

// --- deliverQueryFile: atomowe nadpisanie pliku docelowego ---

TEST(ServiceControlDeliver, overwrites_target_atomically) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_deliver";
  std::filesystem::create_directories(dir);
  const std::filesystem::path src = dir / "src.rql";
  const std::filesystem::path dst = dir / "startup.rql";

  {
    std::ofstream s(src);
    s << "SELECT a FROM b;\n";
  }
  {
    std::ofstream d(dst);
    d << "OLD CONTENT\n";
  }

  EXPECT_TRUE(deliverQueryFile(src.string(), dst.string()));

  std::ifstream check(dst);
  std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "SELECT a FROM b;\n");

  std::filesystem::remove_all(dir);
}

// --- writeQueryFile: ta sama atomowa droga, ale dla TRESCI, nie pliku zrodlowego ---
//
// Uzywa jej przeladowanie planu w locie (`xqry --reset`): plan przychodzi kanalem IPC, wiec
// pliku zrodlowego po stronie serwera nie ma. Tresc pusta jest zadaniem poprawnym - tak
// sprowadza sie plik zapytan uslugi do stanu zerowego po bledzie krytycznym.

TEST(ServiceControlWrite, writes_content_over_existing_target) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery";
  std::filesystem::create_directories(dir);
  const std::filesystem::path dst = dir / "startup.rql";
  {
    std::ofstream d(dst);
    d << "OLD CONTENT\n";
  }

  EXPECT_TRUE(servicecontrol::writeQueryFile("SELECT a STREAM d FROM b\n", dst.string()));

  std::ifstream check(dst);
  const std::string content((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "SELECT a STREAM d FROM b\n");

  std::filesystem::remove_all(dir);
}

TEST(ServiceControlWrite, empty_content_truncates_the_target) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery_empty";
  std::filesystem::create_directories(dir);
  const std::filesystem::path dst = dir / "startup.rql";
  {
    std::ofstream d(dst);
    d << "SELECT a STREAM d FROM b\n";
  }

  EXPECT_TRUE(servicecontrol::writeQueryFile("", dst.string()));

  EXPECT_TRUE(std::filesystem::exists(dst));
  EXPECT_EQ(std::filesystem::file_size(dst), 0U);

  std::filesystem::remove_all(dir);
}

TEST(ServiceControlWrite, fails_when_the_target_directory_does_not_exist) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_writequery_nodir";
  std::filesystem::remove_all(dir);
  EXPECT_FALSE(servicecontrol::writeQueryFile("x\n", (dir / "startup.rql").string()));
}

TEST(ServiceControlDeliver, fails_on_missing_source) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_deliver_missing";
  std::filesystem::create_directories(dir);
  EXPECT_FALSE(deliverQueryFile((dir / "nope.rql").string(), (dir / "out.rql").string()));
  std::filesystem::remove_all(dir);
}

TEST(LockManagerFlock, HashCollisionCannotAcquireTheSameIpcIdentity) {
  // Dwie rozne poprawne nazwy, ten sam skrot FNV. Kolizja wymuszona danymi,
  // nie podmiana funkcji haszujacej; test dziala tez na Linuksie.
  const std::string firstName  = "review_collision_65273";
  const std::string secondName = "review_collision_108288";
  ASSERT_EQ(ipc::shortServerTag(firstName), ipc::shortServerTag(secondName));
  const std::string object = "ut_ipc_" + std::to_string(getpid()) + "." + ipc::shortServerTag(firstName);
  const auto dir           = std::filesystem::temp_directory_path() / ("ut_ipc_guard_" + std::to_string(getpid()));
  std::filesystem::create_directories(dir / "a");
  std::filesystem::create_directories(dir / "b");
  {
    FlockServiceGuard first(firstName);
    FlockServiceGuard second(secondName);
    first.setLockDir((dir / "a").string());
    second.setLockDir((dir / "b").string());
    ASSERT_TRUE(first.acquireLock());
    ASSERT_TRUE(second.acquireLock());
    ASSERT_TRUE(first.acquireIpcLock(object));
    EXPECT_FALSE(second.acquireIpcLock(object));
    second.releaseLock();
    ASSERT_TRUE(second.acquireLock());
    EXPECT_FALSE(second.acquireIpcLock(object)) << "odrzucony uczestnik zwolnil cudza blokade";
    first.releaseLock();
    EXPECT_TRUE(second.acquireIpcLock(object));
  }
  std::filesystem::remove_all(dir);
  std::filesystem::remove("/tmp/xretractor_ipc." + object + ".lock");
}

TEST(IpcIdentity, HashedNamesCannotAliasLiteralNames) {
  EXPECT_EQ(ipc::shortServerTag("review_long_name_10"), "0cb235fc4");
  EXPECT_NE(ipc::serverNameToken("review_long_name_10"), ipc::serverNameToken("cb235fc4"));
  const auto names = ipc::names("review_long_name_10");
  EXPECT_LE(names.shmemSegment.size() + 1, ipc::kMaxObjectNameLength);
  EXPECT_LE(names.queryQueue.size() + 1, ipc::kMaxObjectNameLength);
  EXPECT_LE(names.responseQueue(2147483647).size() + 1, ipc::kMaxObjectNameLength);
}

// --- Protokol kasowalnych blokad (lockFile.hpp) i sprzatanie pozostalosci ---

namespace IPC = boost::interprocess;

namespace {
bool shmExists(const std::string &name) {
  try {
    IPC::shared_memory_object probe(IPC::open_only, name.c_str(), IPC::read_only);
    return true;
  } catch (const IPC::interprocess_exception &) {
    return false;
  }
}
}  // namespace

TEST(LockFile, ExclusiveAcquireWaitsOutTransientHolder) {
  // Sprzatacz i proces kasujacy plik trzymaja blokade przez chwile. Startujaca instancja nie moze
  // wtedy odpasc z "juz dziala" - czeka krotko i dostaje nowy plik.
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_transient_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  std::thread releaser([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lockfile::removeAndRelease(path, holder);
  });
  int fd = -1;
  EXPECT_EQ(lockfile::acquire(path, true, false, fd), lockfile::Result::Acquired);
  releaser.join();
  ASSERT_NE(fd, -1);
  EXPECT_TRUE(lockfile::stillLinked(fd, path));
  lockfile::removeAndRelease(path, fd);
  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(LockFile, LiveHolderMeansBusyAndIsVisible) {
  const std::string path =
      (std::filesystem::temp_directory_path() / ("ut_lockfile_live_" + std::to_string(getpid()) + ".lock")).string();
  int holder = -1;
  ASSERT_EQ(lockfile::acquire(path, true, false, holder), lockfile::Result::Acquired);
  int fd = -1;
  EXPECT_EQ(lockfile::acquire(path, true, false, fd), lockfile::Result::Busy);
  EXPECT_TRUE(lockfile::isHeld(path));
  EXPECT_EQ(lockfile::claimAbandoned(path), -1) << "sprzatacz zajal blokade zywego wlasciciela";
  lockfile::removeAndRelease(path, holder);
  EXPECT_FALSE(lockfile::isHeld(path));
}

TEST(LockManagerFlock, IpcIdentityLockNeedsNoWriteAccess) {
  // Blokada tozsamosci lezy w /tmp i moze nalezec do innego uzytkownika: 0644, a przy
  // fs.protected_regular jadro odrzuca nawet O_CREAT na takim pliku. Plik bez prawa zapisu
  // odtwarza to bez drugiego konta (pod rootem test przechodzi trywialnie).
  const std::string object = "ut_ipc_ro_" + std::to_string(getpid());
  const std::string path   = ipc::identityLockPath(object);
  { std::ofstream touch(path); }
  ASSERT_EQ(chmod(path.c_str(), S_IRUSR | S_IRGRP | S_IROTH), 0);
  const auto dir = std::filesystem::temp_directory_path() / ("ut_ipc_ro_" + std::to_string(getpid()));
  std::filesystem::create_directories(dir);
  {
    FlockServiceGuard guard("ut_ipc_ro");
    guard.setLockDir(dir.string());
    ASSERT_TRUE(guard.acquireLock());
    EXPECT_TRUE(guard.acquireIpcLock(object));
  }
  EXPECT_FALSE(std::filesystem::exists(path)) << "wlasciciel nie skasowal pliku przy zwolnieniu";
  std::filesystem::remove(path);
  std::filesystem::remove_all(dir);
}

TEST(LockManagerSweep, RemovesOnlyAbandonedLocksAndTheirIpcObjects) {
  const std::string pid            = std::to_string(getpid());
  const auto dir                   = std::filesystem::temp_directory_path() / ("ut_sweep_" + pid);
  const std::string dead           = "utdead" + pid;
  const std::string live           = "utlive" + pid;
  const ipc::ServerNames deadNames = ipc::names(dead);
  const ipc::ServerNames liveNames = ipc::names(live);
  std::filesystem::create_directories(dir);

  // Instancja zabita SIGKILL-em: pliki blokad bez wlasciciela i komplet globalnych obiektow IPC.
  { std::ofstream touch(dir / ("xretractor_service." + dead + ".lock")); }
  { std::ofstream touch(ipc::identityLockPath(deadNames.queryQueue)); }
  IPC::shared_memory_object(IPC::create_only, deadNames.shmemSegment.c_str(), IPC::read_write);
  IPC::message_queue(IPC::create_only, deadNames.queryQueue.c_str(), 1, 16);

  // Instancja zywa: te same rodzaje zasobow, ale blokady trzyma jej straznik.
  FlockServiceGuard liveGuard("xretractor_service." + live);
  liveGuard.setLockDir(dir.string());
  ASSERT_TRUE(liveGuard.acquireLock());
  ASSERT_TRUE(liveGuard.acquireIpcLock(liveNames.queryQueue));
  IPC::shared_memory_object(IPC::create_only, liveNames.shmemSegment.c_str(), IPC::read_write);

  const SweepReport swept = sweepAbandonedResources(dir.string());
  EXPECT_EQ(swept.serviceLocks, 1U);
  EXPECT_GE(swept.ipcIdentities, 1U);  // /tmp jest wspolny: moga trafic sie cudze porzucone

  EXPECT_FALSE(std::filesystem::exists(dir / ("xretractor_service." + dead + ".lock")));
  EXPECT_FALSE(std::filesystem::exists(ipc::identityLockPath(deadNames.queryQueue)));
  EXPECT_FALSE(shmExists(deadNames.shmemSegment));
  EXPECT_FALSE(IPC::message_queue::remove(deadNames.queryQueue.c_str())) << "kolejka komend przetrwala sprzatanie";

  EXPECT_TRUE(std::filesystem::exists(dir / ("xretractor_service." + live + ".lock")));
  EXPECT_TRUE(std::filesystem::exists(ipc::identityLockPath(liveNames.queryQueue)));
  EXPECT_TRUE(shmExists(liveNames.shmemSegment)) << "sprzatacz skasowal obiekty zywej instancji";

  liveGuard.releaseLock();
  IPC::shared_memory_object::remove(liveNames.shmemSegment.c_str());
  std::filesystem::remove_all(dir);
}
