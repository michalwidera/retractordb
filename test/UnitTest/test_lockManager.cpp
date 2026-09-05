#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "retractor/lib/lockManager.hpp"
#include "retractor/lib/serviceControl.hpp"

using servicecontrol::deliverQueryFile;
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

TEST(LockManagerFlock, releaseKeepsStableInode) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / ("ut_lockmgr_inode_" + std::to_string(getpid()));
  const std::string serviceName   = "xretractor_service.alfa";
  const std::filesystem::path lockPath = dir / (serviceName + ".lock");
  std::filesystem::create_directories(dir);
  std::filesystem::remove(lockPath);

  FlockServiceGuard first(serviceName);
  first.setLockDir(dir.string());
  ASSERT_TRUE(first.acquireLock());

  // Drugi uczestnik otwiera TEN SAM inode jeszcze przed zwolnieniem pierwszej blokady.
  // Stary kod wykonywal potem unlink: drugi blokowal osierocony inode, a trzeci tworzyl
  // pod ta sama sciezka nowy plik i rowniez zdobywal flock.
  const int secondFd = open(lockPath.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_NE(secondFd, -1);
  struct stat original{};
  ASSERT_EQ(fstat(secondFd, &original), 0);

  first.releaseLock();
  ASSERT_EQ(flock(secondFd, LOCK_EX | LOCK_NB), 0);

  struct stat current{};
  ASSERT_EQ(stat(lockPath.c_str(), &current), 0);
  EXPECT_EQ(current.st_dev, original.st_dev);
  EXPECT_EQ(current.st_ino, original.st_ino);

  FlockServiceGuard third(serviceName);
  third.setLockDir(dir.string());
  EXPECT_FALSE(third.acquireLock());

  EXPECT_EQ(flock(secondFd, LOCK_UN), 0);
  EXPECT_EQ(close(secondFd), 0);
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
  const std::vector<std::string> expected{"systemctl", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
}

TEST(ServiceControlRestart, builds_user_scope_argv) {
  std::vector<std::string> captured;
  restartService(/*userScope=*/true, "xretractor.service", [&](const std::vector<std::string> &argv) {
    captured = argv;
    return 0;
  });

  const std::vector<std::string> expected{"systemctl", "--user", "restart", "xretractor.service"};
  EXPECT_EQ(captured, expected);
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
// pliku zrodlowego po stronie serwera nie ma. Tresc pusta jest zadaniem poprawnym — tak
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
