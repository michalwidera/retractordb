#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "logCapture.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/persistentCounter.hpp"
#include "syscallWrap.hpp"

namespace {
enum class CounterFailure { None, Write, ShortWrite, FileSync, Close, Rename, DirectorySync };
CounterFailure counterFailure = CounterFailure::None;
bool counterRecording         = false;
bool interruptCounterRead     = false;
int counterReads              = 0;
int temporaryFd               = -1;
std::string observedCounter;
std::string observedTemporary;
std::vector<std::string> counterCalls;

std::string counterBytes(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}  // namespace

extern "C" ssize_t __real_read(int, void *, size_t);
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fsync(int);
extern "C" int __real_close(int);

extern "C" ssize_t __wrap_read(int fd, void *buffer, size_t size) {
  if (interruptCounterRead) {
    ++counterReads;
    if (counterReads == 1) {
      errno = EINTR;
      return -1;
    }
    return __real_read(fd, buffer, std::min(size, size_t{1}));
  }
  return __real_read(fd, buffer, size);
}

extern "C" ssize_t __wrap_write(int fd, const void *buffer, size_t size) {
  if (counterRecording && std::string_view(static_cast<const char *>(buffer), size) == "99") {
    temporaryFd = fd;
    counterCalls.emplace_back("write");
    if (counterFailure == CounterFailure::Write) {
      errno = ENOSPC;
      return -1;
    }
    if (counterFailure == CounterFailure::ShortWrite) {
      errno = EIO;
      return __real_write(fd, buffer, 1);
    }
  }
  return __real_write(fd, buffer, size);
}

extern "C" int __wrap_fsync(int fd) {
  if (counterRecording) {
    const bool isFile = fd == temporaryFd;
    counterCalls.emplace_back(isFile ? "fsync-file" : "fsync-directory");
    EXPECT_EQ(counterBytes(observedCounter), isFile ? "98" : "99");
    if (isFile) EXPECT_EQ(counterBytes(observedTemporary), "99");
    if ((isFile && counterFailure == CounterFailure::FileSync) || (!isFile && counterFailure == CounterFailure::DirectorySync)) {
      errno = EIO;
      return -1;
    }
  }
  return __real_fsync(fd);
}

extern "C" int __wrap_close(int fd) {
  const bool closeTemporary = counterRecording && fd == temporaryFd;
  if (closeTemporary) {
    counterCalls.emplace_back("close-file");
    temporaryFd = -1;
  }
  const int result = __real_close(fd);
  if (closeTemporary && counterFailure == CounterFailure::Close) {
    errno = EIO;
    return -1;
  }
  return result;
}

namespace {
constexpr bool kRenameNoexcept = noexcept(::rename("", ""));
bool renameProbe               = false;
bool renameProbeHit            = false;
}  // namespace

// filesystem::rename wywoluje libc z biblioteki dzielonej, poza zasiegiem --wrap.
// Na ELF eksport symbolu z testu przechwytuje te droge. Na Darwinie biblioteka dzielona
// rozwiazuje rename dwupoziomowo, prosto w libSystem, wiec podmiana tam nie dziala -
// rozstrzyga o tym sonda renameInterposed(), nie nazwa systemu.
extern "C" int rename(const char *source, const char *target) noexcept(kRenameNoexcept) {
  using Rename         = decltype(&::rename);
  static auto original = reinterpret_cast<Rename>(::dlsym(RTLD_NEXT, "rename"));
  if (renameProbe) {
    renameProbeHit = true;
    errno          = ENOENT;
    return -1;
  }
  if (counterRecording && observedTemporary == source) {
    counterCalls.emplace_back("rename");
    EXPECT_EQ(counterBytes(observedCounter), "98");
    EXPECT_EQ(counterBytes(observedTemporary), "99");
    if (counterFailure == CounterFailure::Rename) {
      errno = EACCES;
      return -1;
    }
  }
  return original(source, target);
}

RDB_WRAP_SYSCALL(ssize_t, read, (int fd, void *buffer, size_t size), (fd, buffer, size));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *buffer, size_t size), (fd, buffer, size));
RDB_WRAP_SYSCALL(int, fsync, (int fd), (fd));
RDB_WRAP_SYSCALL(int, close, (int fd), (fd));

namespace {
// Czy wywolanie std::filesystem::rename trafia na podmiane z tego pliku.
bool renameInterposed() {
  renameProbe    = true;
  renameProbeHit = false;
  std::error_code ec;
  std::filesystem::rename("rdb-rename-probe-source", "rdb-rename-probe-target", ec);
  renameProbe = false;
  return renameProbeHit;
}
}  // namespace

// ctest -R '^ut_persistentCounter' -V

class PersistentCounterTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_persistentCounter";
  const std::string counterFile             = "test_counter";

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
    std::filesystem::current_path(sandBoxFolder);
  }

  void TearDown() override {
    counterRecording     = false;
    interruptCounterRead = false;
    counterFailure       = CounterFailure::None;
    temporaryFd          = -1;
    counterCalls.clear();
    counterReads = 0;
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
  }

  std::string counterPath() { return (sandBoxFolder / counterFile).string(); }
};

// ============================================================
// Pierwszy użycie - brak pliku
// ============================================================

TEST_F(PersistentCounterTest, starts_at_zero_when_no_file) {
  // covers: persistentcounter.persistent-counter.2~1
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 0);
}

// ============================================================
// Konstruktor rezerwuje nastepny numer (#281)
// ============================================================

// Numer nastepnej sesji jest w pliku, zanim ta sesja cokolwiek zarchiwizuje - nie dopiero
// po destrukcji obiektu.
TEST_F(PersistentCounterTest, construction_reserves_next_value) {
  // covers: persistentcounter.persistent-counter.3~1
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 0);

  std::ifstream in(counterPath());
  int saved = -1;
  in >> saved;
  EXPECT_EQ(saved, 1);
}

// ============================================================
// Druga instancja wczytuje zapisaną wartość
// ============================================================

TEST_F(PersistentCounterTest, second_instance_loads_saved_value) {
  // covers: persistentcounter.persistent-counter.7~1
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // zapisuje 1

  auto pc2 = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc2->getCount(), 1);
}

// ============================================================
// Kolejne instancje akumulują licznik
// ============================================================

TEST_F(PersistentCounterTest, count_accumulates_across_instances) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // 0 → zapisuje 1
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // wczytuje 1 → zapisuje 2
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // wczytuje 2 → zapisuje 3

  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 3);
}

// ============================================================
// getCount() nie mutuje stanu przed destrukcją
// ============================================================

TEST_F(PersistentCounterTest, getCount_is_idempotent_before_destruction) {
  // covers: persistentcounter.persistent-counter.7~1
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // zapisuje 1

  auto pc2 = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc2->getCount(), 1);
  EXPECT_EQ(pc2->getCount(), 1);
}

// ============================================================
// Nieczytelny plik - odmowa startu, nie rotacja 0 (#281); od #472 klasa K4, kod EIO
// ============================================================
//
// Odmowa przychodzi od fazy 1 wyjatkiem, nie std::exit: demon konczy sie na nim kodem
// EXIT_FAILURE w main (start) albo w executorsm::run (plan przeladowany), a te testy pilnuja
// typu i komunikatu u zrodla.

// Plik 0-bajtowy to slad procesu zabitego w trakcie dawnego zapisu (ofstream obcinal plik
// przy otwarciu). Czytany jako 0 kazal planowi nadpisywac archiwa .old0, .old1, ...
TEST_F(PersistentCounterTest, empty_file_is_not_rotation_zero) {
  // covers: persistentcounter.persistent-counter.4~1
  std::ofstream(counterPath()).close();
  ASSERT_TRUE(std::filesystem::exists(counterPath()));
  ASSERT_EQ(std::filesystem::file_size(counterPath()), 0U);

  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_EQ(created.error().err(), EIO);
    EXPECT_THAT(created.error().message(), ::testing::ContainsRegex("Rotation counter file .* is unreadable \\(0 bytes"));
  }
}

TEST_F(PersistentCounterTest, non_numeric_file_is_fatal) {
  std::ofstream(counterPath()) << "not_a_number";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_EQ(created.error().err(), EIO);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// Dawny `>>` czytal z "12abc" liczbe 12 i ogon przemilczal.
TEST_F(PersistentCounterTest, trailing_garbage_is_fatal) {
  std::ofstream(counterPath()) << "12abc";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_EQ(created.error().err(), EIO);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// percounter < 0 wylacza w storage rotacje - plik z "-1" nie moze tego zrobic po cichu.
TEST_F(PersistentCounterTest, negative_value_is_fatal) {
  std::ofstream(counterPath()) << "-1";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_EQ(created.error().err(), EIO);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// Kontrola dodatnia: plik poprawiony recznie w edytorze konczy sie znakiem nowej linii.
TEST_F(PersistentCounterTest, trailing_newline_is_accepted) {
  std::ofstream(counterPath()) << "7\n";
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 7);
}

// ============================================================
// Odczyt bez rezerwacji: cztery wyniki rozdzielone przyczyna (#472)
// ============================================================

TEST_F(PersistentCounterTest, read_missing_file_is_missing) {
  // covers: persistentcounter.persistent-counter.1~1
  EXPECT_EQ(PersistentCounter::read(counterPath()).state, PersistentCounter::Reading::State::Missing);
}

TEST_F(PersistentCounterTest, read_valid_file_is_value_and_reserves_nothing) {
  // covers: persistentcounter.persistent-counter.1~1
  std::ofstream(counterPath()) << "7\n";
  const auto reading = PersistentCounter::read(counterPath());
  EXPECT_EQ(reading.state, PersistentCounter::Reading::State::Value);
  EXPECT_EQ(reading.value, 7);
  std::ifstream in(counterPath());
  std::string saved;
  in >> saved;
  EXPECT_EQ(saved, "7");
}

// Odczytana w calosci tresc, ktora nie jest liczba, to wadliwy stan planu (K2), nie srodowisko.
TEST_F(PersistentCounterTest, read_empty_file_is_invalid) {
  // covers: persistentcounter.persistent-counter.1~1
  std::ofstream(counterPath()).close();
  const auto reading = PersistentCounter::read(counterPath());
  EXPECT_EQ(reading.state, PersistentCounter::Reading::State::Invalid);
  EXPECT_EQ(reading.detail, "0 bytes: ''");
}

// Katalog w miejscu pliku: open() sie udaje, read() zwraca EISDIR. Dawny ifstream czytal z niego
// zero bajtow, wiec awaria odczytu udawala pusty plik - czyli blad planu zamiast srodowiska.
// Katalog, a nie chmod 000, bo uprawnienia nie zatrzymuja roota w kontenerze CI.
TEST_F(PersistentCounterTest, read_directory_is_unreadable_not_invalid) {
  // covers: persistentcounter.persistent-counter.1~1
  std::filesystem::create_directory(counterPath());
  const auto reading = PersistentCounter::read(counterPath());
  EXPECT_EQ(reading.state, PersistentCounter::Reading::State::Unreadable);
  EXPECT_NE(reading.detail.find("read: "), std::string::npos) << reading.detail;
}

// Ta sama przyczyna w konstruktorze: EIO z opisem odczytu, nie "is unreadable" od tresci.
TEST_F(PersistentCounterTest, unreadable_file_exits_with_eio) {
  // covers: persistentcounter.persistent-counter.4~1
  std::filesystem::create_directory(counterPath());
  const auto created = PersistentCounter::create(counterPath());
  ASSERT_RDB_ERROR(created, rdb::Errc::IO, "cannot be read (read: ");
  EXPECT_EQ(created.error().err(), EIO);
}

// ============================================================
// Zapis przez plik tymczasowy i rename (#281)
// ============================================================

TEST_F(PersistentCounterTest, save_leaves_no_temp_file) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }
  size_t entries = 0;
  for ([[maybe_unused]] const auto &entry : std::filesystem::directory_iterator(sandBoxFolder))
    ++entries;
  EXPECT_EQ(entries, 1U);
  EXPECT_TRUE(std::filesystem::exists(counterPath()));
}

// Nieudana rezerwacja zatrzymuje start i nie narusza poprzedniej wartosci. Katalog w miejscu
// pliku tymczasowego wymusza porazke open(); dawny ofstream pisal wprost do celu i by tu
// przeszedl. Nazwa pliku tymczasowego niesie pid - tego procesu, bo odmowa jest wyjatkiem, a
// nie smiercia procesu potomnego.
TEST_F(PersistentCounterTest, failed_reservation_returns_io_and_keeps_previous_value) {
  // covers: persistentcounter.persistent-counter.4~1
  std::ofstream(counterPath()) << "5";

  std::filesystem::create_directory(counterPath() + ".tmp." + std::to_string(::getpid()));
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_EQ(created.error().err(), EISDIR);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("Cannot reserve rotation number 6"));
  }

  std::ifstream in(counterPath());
  std::string saved;
  in >> saved;
  EXPECT_EQ(saved, "5");
}

// ============================================================
// Własna nazwa pliku jest respektowana
// ============================================================

TEST_F(PersistentCounterTest, custom_filename_creates_correct_file) {
  std::string custom = (sandBoxFolder / "my_custom_counter").string();
  { auto pc = rdbtest::ok(PersistentCounter::create(custom)); }
  EXPECT_TRUE(std::filesystem::exists(custom));
  EXPECT_FALSE(std::filesystem::exists(counterPath()));
}

TEST_F(PersistentCounterTest, reservation_orders_sync_rename_and_directory_sync) {
  // covers: persistentcounter.persistent-counter.3~1, persistentcounter.persistent-counter.5~1
  if (!renameInterposed()) {
    ASSERT_FALSE(RDB_OS_LINUX) << "podmiana rename musi dzialac na Linuksie";
    GTEST_SKIP() << "std::filesystem::rename omija podmiane rename z testu";
  }
  std::ofstream(counterPath()) << "98";
  observedCounter   = counterPath();
  observedTemporary = counterPath() + ".tmp." + std::to_string(::getpid());
  counterRecording  = true;
  auto counter      = PersistentCounter::create(counterPath());
  counterRecording  = false;
  ASSERT_RDB_OK(counter);
  EXPECT_EQ((*counter)->getCount(), 98);
  EXPECT_EQ(counterCalls, (std::vector<std::string>{"write", "fsync-file", "close-file", "rename", "fsync-directory"}));
  EXPECT_EQ(counterBytes(counterPath()), "99");
  EXPECT_FALSE(std::filesystem::exists(observedTemporary));
}

TEST_F(PersistentCounterTest, failed_write_sync_close_or_rename_keeps_old_bytes_and_removes_temporary) {
  // covers: persistentcounter.persistent-counter.4~1, persistentcounter.persistent-counter.5~1
  // covers: persistentcounter.persistent-counter.6~1
  for (const auto fault : {CounterFailure::Write, CounterFailure::ShortWrite, CounterFailure::FileSync, CounterFailure::Close,
                           CounterFailure::Rename}) {
    SCOPED_TRACE(static_cast<int>(fault));
    if (fault == CounterFailure::Rename && !renameInterposed()) {
      ASSERT_FALSE(RDB_OS_LINUX) << "podmiana rename musi dzialac na Linuksie";
      continue;  // awarii rename nie da sie tu wstrzyknac; pozostale przypadki ida przez --wrap
    }
    std::ofstream(counterPath()) << "98";
    observedCounter   = counterPath();
    observedTemporary = counterPath() + ".tmp." + std::to_string(::getpid());
    counterCalls.clear();
    counterFailure   = fault;
    counterRecording = true;
    auto counter     = PersistentCounter::create(counterPath());
    counterRecording = false;
    ASSERT_RDB_ERROR(counter, rdb::Errc::IO);
    EXPECT_EQ(counter.error().err(), fault == CounterFailure::Write ? ENOSPC : fault == CounterFailure::Rename ? EACCES : EIO);
    EXPECT_EQ(counterBytes(counterPath()), "98");
    EXPECT_FALSE(std::filesystem::exists(observedTemporary));
    EXPECT_EQ(std::count(counterCalls.begin(), counterCalls.end(), "fsync-directory"), 0);
    EXPECT_EQ(std::count(counterCalls.begin(), counterCalls.end(), "rename"), fault == CounterFailure::Rename ? 1 : 0);
  }
}

TEST_F(PersistentCounterTest, directory_sync_failure_logs_but_keeps_the_completed_reservation) {
  // covers: persistentcounter.persistent-counter.6~1
  std::ofstream(counterPath()) << "98";
  observedCounter   = counterPath();
  observedTemporary = counterPath() + ".tmp." + std::to_string(::getpid());
  counterFailure    = CounterFailure::DirectorySync;
  LogCapture log;
  counterRecording = true;
  auto counter     = PersistentCounter::create(counterPath());
  counterRecording = false;
  ASSERT_RDB_OK(counter);
  EXPECT_EQ((*counter)->getCount(), 98);
  EXPECT_EQ(counterBytes(counterPath()), "99");
  EXPECT_FALSE(std::filesystem::exists(observedTemporary));
  EXPECT_NE(log.text().find("[error]"), std::string::npos);
  EXPECT_NE(log.text().find("directory fsync failed"), std::string::npos);
}

TEST_F(PersistentCounterTest, read_retries_interrupted_and_short_reads_without_reserving) {
  // covers: persistentcounter.persistent-counter.1~1
  std::ofstream(counterPath()) << "98\n";
  interruptCounterRead = true;
  const auto value     = PersistentCounter::read(counterPath());
  interruptCounterRead = false;
  EXPECT_EQ(value.state, PersistentCounter::Reading::State::Value);
  EXPECT_EQ(value.value, 98);
  EXPECT_EQ(counterReads, 5);
  EXPECT_EQ(counterBytes(counterPath()), "98\n");
}

TEST_F(PersistentCounterTest, existence_failure_is_unreadable_instead_of_missing) {
  // covers: persistentcounter.persistent-counter.1~1, persistentcounter.persistent-counter.2~1
  std::filesystem::create_symlink(counterFile, counterPath());
  const auto reading = PersistentCounter::read(counterPath());
  EXPECT_EQ(reading.state, PersistentCounter::Reading::State::Unreadable);
  EXPECT_NE(reading.detail.find("cannot check whether it exists"), std::string::npos);
  const auto result = PersistentCounter::create(counterPath());
  ASSERT_RDB_ERROR(result, rdb::Errc::IO);
  EXPECT_EQ(result.error().err(), EIO);
  EXPECT_TRUE(std::filesystem::is_symlink(counterPath()));
}
