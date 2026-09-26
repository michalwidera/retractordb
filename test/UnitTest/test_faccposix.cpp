#include <gtest/gtest.h>

#include <sys/stat.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "logCapture.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccposix.hpp"
#include "syscallWrap.hpp"

using BYTE = unsigned char;

// Helper: create a single-field Descriptor of given byte size (BYTE type)
static rdb::Descriptor makeDesc(size_t size) { return {"f", static_cast<int>(size), 1, rdb::BYTE}; }

// ctest -R '^ut-test_faccposix' -V

// Source under test:
// src/rdb/lib/faccposix.cc

// --- Linker-level interposition via --wrap ---
// g_pread_eintr_count / g_write_eintr_count control how many
// consecutive EINTR errors the wrapped syscall will produce
// before delegating to the real implementation.

static thread_local int g_pread_eintr_count = 0;
static thread_local int g_write_eintr_count = 0;

extern "C" {
ssize_t __real_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t __real_write(int fd, const void *buf, size_t count);

ssize_t __wrap_pread(int fd, void *buf, size_t count, off_t offset) {
  if (g_pread_eintr_count > 0) {
    --g_pread_eintr_count;
    errno = EINTR;
    return -1;
  }
  return __real_pread(fd, buf, count, offset);
}

ssize_t __wrap_write(int fd, const void *buf, size_t count) {
  if (g_write_eintr_count > 0) {
    --g_write_eintr_count;
    errno = EINTR;
    return -1;
  }
  return __real_write(fd, buf, count);
}
}

// Spiecie __wrap_/__real_ z prawdziwym wywolaniem systemowym. Na konsolidatorze
// z --wrap nie generuje niczego; bez niego dostarcza definicje __real_* i symbol
// o nazwie systemowej - patrz syscallWrap.hpp.
RDB_WRAP_SYSCALL(ssize_t, pread, (int fd, void *buf, size_t count, off_t offset), (fd, buf, count, offset));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *buf, size_t count), (fd, buf, count));

// Wstrzykniecie awarii stat() - uzywane przez test kontraktu count(). Licznik jest
// jednorazowy i uzbrajany tuz przed badanym wywolaniem: --wrap dziala na CALY plik
// wykonywalny, wiec uzbrojony na dluzej przechwycilby takze cudze stat().

static thread_local int g_stat_fail_count = 0;
static thread_local int g_stat_fail_errno = 0;

extern "C" {
int __real_stat(const char *path, struct stat *buf);

int __wrap_stat(const char *path, struct stat *buf) {
  if (g_stat_fail_count > 0) {
    --g_stat_fail_count;
    errno = g_stat_fail_errno;
    return -1;
  }
  return __real_stat(path, buf);
}
}

RDB_WRAP_SYSCALL(int, stat, (const char *path, struct stat *buf), (path, buf));

// --- Helpers ---

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

std::vector<BYTE> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

struct fileInfo {
  int sizeFromSystem;
  std::vector<BYTE> fileContents;
};

// --- Test fixture ---
// Creates a clean sandbox directory before each test and removes it after

class PosixFileTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccposix";
  const std::string filename                = "test_file";
  const size_t recsize                      = sizeof(BYTE);
  const rdb::Descriptor desc                = makeDesc(sizeof(BYTE));

  void SetUp() override {
    g_pread_eintr_count = 0;
    g_write_eintr_count = 0;
    g_stat_fail_count   = 0;
    g_stat_fail_errno   = 0;
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
    std::filesystem::permissions(              //
        sandBoxFolder,                         //
        std::filesystem::perms::others_all,    //
        std::filesystem::perm_options::remove  //
    );
    std::filesystem::current_path(sandBoxFolder);
  }

  void TearDown() override {
    g_pread_eintr_count = 0;
    g_write_eintr_count = 0;
    g_stat_fail_count   = 0;
    g_stat_fail_errno   = 0;
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
  }

  std::string sandboxPath(const std::string &name) { return (sandBoxFolder / name).string(); }
};

// ============================================================
// posixBinaryFile tests
// ============================================================

// Verify basic write (append) and read of records
TEST_F(PosixFileTest, test_faccposix_write_and_read) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_test"), desc);

  record = 0xAA;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  record = 0xBB;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  record = 0xCC;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(pfa->count(), 3);

  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xAA);

  GTEST_ASSERT_EQ(pfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xBB);

  GTEST_ASSERT_EQ(pfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0xCC);
}

// Verify read beyond EOF returns EXIT_FAILURE (partial read handling)
TEST_F(PosixFileTest, test_faccposix_read_beyond_eof) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_eof"), desc);

  record = 0x11;
  GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(pfa->count(), 1);

  // Reading at position beyond file size should fail
  GTEST_ASSERT_EQ(pfa->read(&record, 99), EXIT_FAILURE);
}

// Verify read from empty file returns EXIT_FAILURE
TEST_F(PosixFileTest, test_faccposix_read_empty_file) {
  BYTE record = 0xFF;

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_empty"), desc);

  GTEST_ASSERT_EQ(pfa->count(), 0);
  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_FAILURE);
}

// Verify truncate (write nullptr at position 0) clears the file
TEST_F(PosixFileTest, test_faccposix_truncate) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_trunc"), desc);

  record = 0x01;
  pfa->write(&record);
  record = 0x02;
  pfa->write(&record);
  GTEST_ASSERT_EQ(pfa->count(), 2);

  // Truncate
  pfa->write(nullptr, 0);
  GTEST_ASSERT_EQ(pfa->count(), 0);

  // Read after truncate should fail
  GTEST_ASSERT_NE(pfa->read(&record, 0), 0);
}

// Purge oproznia plik W MIEJSCU: deskryptor zostaje wazny, a zapis po purge trafia do
// widocznego pliku. Dawniej purge kasowal plik po nazwie przy otwartym deskryptorze - zapisy
// szly do skasowanego i-wezla, count() (stat po nazwie) zwracal 0, odczyt oddawal dane sprzed
// purge, a wszystko ginelo przy zamknieciu.
TEST_F(PosixFileTest, test_faccposix_purge_keeps_file_usable) {
  const std::string path = sandboxPath("posix_purge");
  {
    rdb::posixBinaryFile pfa(path, desc);
    BYTE record = 0x01;
    GTEST_ASSERT_EQ(pfa.write(&record), EXIT_SUCCESS);
    record = 0x02;
    GTEST_ASSERT_EQ(pfa.write(&record), EXIT_SUCCESS);
    GTEST_ASSERT_EQ(pfa.write(nullptr, 0), EXIT_SUCCESS);
    record = 0x42;
    GTEST_ASSERT_EQ(pfa.write(&record), EXIT_SUCCESS);
    EXPECT_EQ(pfa.count(), 1U);
    record = 0;
    EXPECT_EQ(pfa.read(&record, 0), EXIT_SUCCESS);
    EXPECT_EQ(record, 0x42);
  }
  std::ifstream in(path, std::ios::binary);
  EXPECT_EQ(std::vector<char>(std::istreambuf_iterator<char>(in), {}), std::vector<char>({0x42}));
}

// Verify update-in-place overwrites record at given byte position
TEST_F(PosixFileTest, test_faccposix_update_in_place) {
  BYTE record;

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_update"), desc);

  record = 10;
  pfa->write(&record);
  record = 20;
  pfa->write(&record);
  record = 30;
  pfa->write(&record);

  GTEST_ASSERT_EQ(pfa->count(), 3);

  // Update record at byte position 1
  record = 99;
  GTEST_ASSERT_EQ(pfa->write(&record, 1), EXIT_SUCCESS);

  // Count should remain unchanged
  GTEST_ASSERT_EQ(pfa->count(), 3);

  // Verify updated record
  GTEST_ASSERT_EQ(pfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 99);

  // Verify other records are untouched
  GTEST_ASSERT_EQ(pfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 10);

  GTEST_ASSERT_EQ(pfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 30);
}

// Verify name() returns the path passed at construction
TEST_F(PosixFileTest, test_faccposix_name) {
  auto path = sandboxPath("posix_name");
  auto pfa  = std::make_unique<rdb::posixBinaryFile>(path, desc);

  EXPECT_EQ(pfa->name(), path);
}

// Verify multi-byte record write and read
TEST_F(PosixFileTest, test_faccposix_multibyte_record) {
  int record;
  auto recsize_int = sizeof(int);

  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_multi"),
                                                    rdb::Descriptor("f", static_cast<int>(recsize_int), 1, rdb::INTEGER));

  record = 1000;
  GTEST_ASSERT_EQ(pfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 2000;
  GTEST_ASSERT_EQ(pfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(pfa->count(), 2);

  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), 0 * recsize_int), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 1000);

  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), 1 * recsize_int), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 2000);

  // Partial read: position aligned to half a record from end
  GTEST_ASSERT_EQ(pfa->read(reinterpret_cast<uint8_t *>(&record), recsize_int + recsize_int / 2), EXIT_FAILURE);
}

// Verify data persists on disk after destructor (durability via reopen)
TEST_F(PosixFileTest, test_faccposix_persistence) {
  BYTE record;
  auto path = sandboxPath("posix_persist");

  {
    auto pfa = std::make_unique<rdb::posixBinaryFile>(path, desc);
    record   = 0x42;
    pfa->write(&record);
    record = 0x43;
    pfa->write(&record);
  }  // destructor closes file

  // Reopen and verify data persists
  auto pfa2 = std::make_unique<rdb::posixBinaryFile>(path, desc);
  GTEST_ASSERT_EQ(pfa2->count(), 2);

  GTEST_ASSERT_EQ(pfa2->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x42);

  GTEST_ASSERT_EQ(pfa2->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x43);
}

// read(): EINTR within retry limit (4 failures, 5th attempt succeeds)
TEST_F(PosixFileTest, test_faccposix_read_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_eintr_r_ok"), makeDesc(sizeof(uint8_t)));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  g_pread_eintr_count = 4;  // maxRetries is 5 -> 4 failures + 1 success
  uint8_t result      = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xAA);
}

// read(): EINTR exceeding retry limit (all 5 attempts are EINTR)
TEST_F(PosixFileTest, test_faccposix_read_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_eintr_r_fail"), makeDesc(sizeof(uint8_t)));

  uint8_t data = 0xAA;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  g_pread_eintr_count = 10;  // well above maxRetries
  uint8_t result      = 0;
  ASSERT_NE(pfa->read(&result, 0), EXIT_SUCCESS);
}

// write(): EINTR within retry limit (5 failures, 6th attempt succeeds)
TEST_F(PosixFileTest, test_faccposix_write_eintr_within_limit_succeeds) {
  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_eintr_w_ok"), makeDesc(sizeof(uint8_t)));

  g_write_eintr_count = 5;  // maxRetries is 5 -> retries 1..5 allowed, 6th succeeds
  uint8_t data        = 0xBB;
  ASSERT_EQ(pfa->write(&data), EXIT_SUCCESS);

  uint8_t result = 0;
  ASSERT_EQ(pfa->read(&result, 0), EXIT_SUCCESS);
  ASSERT_EQ(result, 0xBB);
}

// write(): EINTR exceeding retry limit (6+ failures -> gives up)
TEST_F(PosixFileTest, test_faccposix_write_eintr_exceeds_limit_fails) {
  auto pfa = std::make_unique<rdb::posixBinaryFile>(sandboxPath("posix_eintr_w_fail"), makeDesc(sizeof(uint8_t)));

  g_write_eintr_count = 10;  // well above maxRetries
  uint8_t data        = 0xCC;
  ASSERT_NE(pfa->write(&data), EXIT_SUCCESS);
}

// count(): awaria stat() inna niz ENOENT zatrzymuje proces, zamiast oddac blad jako liczbe.
// Pilnowana regresja: `return -1` z count() dociera do storage::recordsCount_ jako SIZE_MAX
// (uzasadnienie kontraktu przy FileInterface::count). Dlatego sprawdzamy nie tylko to, ZE
// proces ginie, ale i to, ze ginie wewnatrz count() - po komunikacie tej wlasnie funkcji.
TEST_F(PosixFileTest, test_faccposix_count_stat_failure_is_fatal) {
  const std::string path = sandboxPath("posix_count_stat_fail");
  {
    auto pfa    = std::make_unique<rdb::posixBinaryFile>(path, desc);
    BYTE record = 0xAA;
    GTEST_ASSERT_EQ(pfa->write(&record), EXIT_SUCCESS);
    // Kontrola dodatnia: z rozbrojonym licznikiem stat() dziala i count() liczy normalnie.
    GTEST_ASSERT_EQ(pfa->count(), 1);
  }

  EXPECT_DEATH(
      {
        rdb::posixBinaryFile pfa(path, desc);
        g_stat_fail_count = 1;
        g_stat_fail_errno = EACCES;
        // Nieosiagalne przy poprawnym kontrakcie. Gdyby count() wrocilo do oddawania bledu
        // jako liczby, instrukcja dobiegnie konca, gtest zglosi "did not die", a wypisana
        // wartosc pokaze, co zwrocila.
        std::cerr << "count() zwrocilo " << pfa.count() << "\n";
      },
      "posixBinaryFile::count: ::stat");
}

// Rotacja pod numerem, ktory ma juz archiwum: rename() nadpisuje je bez pytania, wiec jedynym
// sladem jest log (#281). Pierwsza rotacja pod tym numerem jest kontrola - nie nadpisuje
// niczego i komunikatu nie ma.
TEST_F(PosixFileTest, test_faccposix_rotation_overwrite_is_logged) {
  const std::string path    = sandboxPath("posix_rotate");
  const std::string archive = path + ".old7";
  BYTE record               = 0xAA;
  for (int session = 0; session < 2; ++session) {
    LogCapture log;
    {
      rdb::posixBinaryFile pfa(path, desc, 7);
      GTEST_ASSERT_EQ(pfa.write(&record), EXIT_SUCCESS);
    }
    EXPECT_TRUE(std::filesystem::exists(archive));
    const bool logged = log.text().find("overwrote existing archive " + archive) != std::string::npos;
    EXPECT_EQ(logged, session == 1) << "session " << session << ", log: " << log.text();
  }
}
