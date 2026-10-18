#include <gtest/gtest.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pthread.h>
#include <signal.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "logCapture.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccbindev.hpp"
#include "rdbResult.hpp"
#include "syscallWrap.hpp"

namespace {
enum class ReadFault { None, InterruptedThenShort, Error };
ReadFault readFault = ReadFault::None;
int readFaultFd     = -1;
int injectedReads   = 0;
bool failPoll       = false;
int injectedPolls   = 0;
}  // namespace

extern "C" ssize_t __real_read(int, void *, size_t);
extern "C" int __real_poll(struct pollfd *, nfds_t, int);
extern "C" ssize_t __wrap_read(int fd, void *buffer, size_t size) {
  if (fd == readFaultFd && readFault != ReadFault::None) {
    ++injectedReads;
    if (readFault == ReadFault::Error || injectedReads == 1) {
      errno = readFault == ReadFault::Error ? EIO : EINTR;
      return -1;
    }
    return __real_read(fd, buffer, std::min(size, size_t{2}));
  }
  return __real_read(fd, buffer, size);
}
extern "C" int __wrap_poll(struct pollfd *fds, nfds_t count, int timeout) {
  if (failPoll) {
    ++injectedPolls;
    errno = EINVAL;
    return -1;
  }
  return __real_poll(fds, count, timeout);
}
RDB_WRAP_SYSCALL(ssize_t, read, (int fd, void *buffer, size_t size), (fd, buffer, size));
RDB_WRAP_SYSCALL(int, poll, (struct pollfd * fds, nfds_t count, int timeout), (fds, count, timeout));

// Tests intentionally use raw byte buffers for binary device I/O coverage.
// NOLINTBEGIN(modernize-avoid-c-arrays)

// Akcesor trzyma deskryptor, wiec nie wolno go ani kopiowac, ani przenosic - R-01, #274.
static_assert(!std::is_copy_constructible_v<rdb::binaryDeviceRO>);
static_assert(!std::is_copy_assignable_v<rdb::binaryDeviceRO>);
static_assert(!std::is_move_constructible_v<rdb::binaryDeviceRO>);
static_assert(!std::is_move_assignable_v<rdb::binaryDeviceRO>);

namespace {

using clock_type = std::chrono::steady_clock;

// Otwarcie do zapisu bez czekania - FIFO ma juz czytelnika (akcesor DEVICE).
int openWriter(const std::string &path) {
  const int fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);
  EXPECT_GE(fd, 0) << path << ": " << std::strerror(errno);
  return fd;
}

void writeAll(int fd, const std::vector<uint8_t> &bytes) {
  EXPECT_EQ(::write(fd, bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
}

// Jeden slot z punktu widzenia modelu: read() akcesora. nullopt = rekord all-null.
std::optional<std::vector<uint8_t>> readRecord(rdb::binaryDeviceRO &dev) {
  std::vector<uint8_t> out(4, 0xFF);
  std::vector<bool> nullBitset;
  EXPECT_EQ(dev.read(out.data(), nullBitset, 0), EXIT_SUCCESS);
  if (std::ranges::all_of(nullBitset, [](bool isNull) { return isNull; })) return std::nullopt;
  return out;
}

// Czas jednej fazy DEVICE dla podanych terminow wzgledem jej poczatku.
std::chrono::milliseconds timedAwait(std::vector<std::pair<rdb::binaryDeviceRO *, std::chrono::milliseconds>> sources) {
  const auto start = clock_type::now();
  std::vector<rdb::deviceWait> waits;
  for (const auto &[source, timeout] : sources)
    waits.push_back({.source = source, .deadline = start + timeout});
  rdb::awaitRecords(waits);
  return std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - start);
}

class BinaryDeviceROTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_faccbindev";

  void SetUp() override {
    std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override {
    readFault     = ReadFault::None;
    readFaultFd   = -1;
    injectedReads = 0;
    failPoll      = false;
    injectedPolls = 0;
    std::filesystem::remove_all(sandBoxFolder);
  }

  [[nodiscard]] std::string sandboxPath(const std::string &name) const { return (sandBoxFolder / name).string(); }

  void writeBinaryFile(const std::string &path, const std::vector<uint8_t> &bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    if (!bytes.empty()) {
      out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      ASSERT_TRUE(out.good());
    }
  }
};

}  // namespace

static rdb::Descriptor fixedIntDescriptor() { return rdb::Descriptor{{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}}; }

TEST_F(BinaryDeviceROTest, read_exact_record_and_count) {
  // covers: faccbindev.binary-device-ro.1~1, faccbindev.binary-device-ro.6~1, faccbindev.binary-device-ro.11~1
  // covers: fainterface.file-interface.6~1
  auto path = sandboxPath("device.bin");
  writeBinaryFile(path, {0x11, 0x22, 0x33, 0x44});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x11);
  EXPECT_EQ(out[1], 0x22);
  EXPECT_EQ(out[2], 0x33);
  EXPECT_EQ(out[3], 0x44);
  EXPECT_EQ(rdbtest::ok(dev.count()), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_FALSE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, count_starts_at_zero_before_any_read) {
  // covers: faccbindev.binary-device-ro.11~1, fainterface.file-interface.6~1
  auto path = sandboxPath("position.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  EXPECT_EQ(rdbtest::ok(dev.count()), 0U);

  uint8_t out[4] = {0, 0, 0, 0};
  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(rdbtest::ok(dev.count()), 1U);
}

TEST_F(BinaryDeviceROTest, read_loops_to_beginning_on_eof_when_enabled) {
  // covers: accessorfactory.make-accessor.3~1, faccbindev.binary-device-ro.8~1
  auto path = sandboxPath("loop.bin");
  writeBinaryFile(path, {0x01, 0x02, 0x03, 0x04});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x01);
  EXPECT_EQ(out[1], 0x02);
  EXPECT_EQ(out[2], 0x03);
  EXPECT_EQ(out[3], 0x04);
  EXPECT_EQ(rdbtest::ok(dev.count()), 2U);
}

TEST_F(BinaryDeviceROTest, read_zero_fills_on_eof_when_loop_disabled) {
  // covers: faccbindev.binary-device-ro.7~1, faccbindev.binary-device-ro.9~1, faccbindev.binary-device-ro.10~1
  auto path = sandboxPath("noloop.bin");
  writeBinaryFile(path, {0x10, 0x20, 0x30, 0x40});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, false, "BINFILE");
  uint8_t out[4] = {0, 0, 0, 0};

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  out[0] = 0xFF;
  out[1] = 0xFF;
  out[2] = 0xFF;
  out[3] = 0xFF;

  EXPECT_EQ(dev.read(out, 0), EXIT_SUCCESS);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(rdbtest::ok(dev.count()), 2U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
  EXPECT_TRUE(dev.exhausted());
}

TEST_F(BinaryDeviceROTest, read_fails_on_empty_file_when_loop_enabled) {
  auto path = sandboxPath("empty.bin");
  writeBinaryFile(path, {});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};

  EXPECT_EQ(dev.read(out, 0), EIO);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(rdbtest::ok(dev.count()), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, read_fails_on_missing_file_with_null_metadata) {
  // covers: faccbindev.binary-device-ro.7~1
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(sandboxPath("missing.bin"), desc, true, "BINFILE");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};

  EXPECT_EQ(dev.read(out, 0), EBADF);
  EXPECT_EQ(out[0], 0x00);
  EXPECT_EQ(out[1], 0x00);
  EXPECT_EQ(out[2], 0x00);
  EXPECT_EQ(out[3], 0x00);
  EXPECT_EQ(rdbtest::ok(dev.count()), 1U);

  auto nulls = dev.lastNullBitset();
  ASSERT_EQ(nulls.size(), 1U);
  EXPECT_TRUE(nulls[0]);
}

TEST_F(BinaryDeviceROTest, name_and_write_contract) {
  // covers: faccbindev.binary-device-ro.4~1
  auto path = sandboxPath("name.bin");
  writeBinaryFile(path, {0xAA, 0xBB, 0xCC, 0xDD});

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
  uint8_t out[4] = {0x10, 0x20, 0x30, 0x40};

  EXPECT_EQ(dev.name(), path);
  EXPECT_EQ(dev.write(out, 0), ENOTSUP);
}

// Krotki odczyt nie gubi rekordu (dawny regres: krotki ::read na FIFO bral sie za koniec strumienia).
// Od #347 DEVICE nie czeka w read(): bajty zebrane w jednym slocie czekaja w prywatnym buforze,
// a rekord dokonczony pozniej trafia do pierwszego kolejnego slotu.
TEST_F(BinaryDeviceROTest, device_keeps_a_short_read_until_the_record_completes) {
  // covers: faccbindev.binary-device-ro.13~1
  auto path = sandboxPath("short-read.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_GE(writer, 0);

  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");
  writeAll(writer, {0x11, 0x22});
  EXPECT_EQ(dev.fill(), rdb::binaryDeviceRO::fillResult::wouldBlock);
  EXPECT_EQ(readRecord(dev), std::nullopt) << "niepelny rekord to all-null w tym slocie";

  writeAll(writer, {0x33, 0x44});
  EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44}));
  ::close(writer);
}

// FIFO bez pisarza nie wiesza otwarcia (O_NONBLOCK) ani odczytu. EOF przed pierwszymi danymi to
// pisarz, ktory sie jeszcze nie podlaczyl - takze z ONESHOT nie jest wyczerpaniem.
TEST_F(BinaryDeviceROTest, device_opens_a_fifo_without_a_writer_without_blocking) {
  // covers: faccbindev.binary-device-ro.13~1
  auto path = sandboxPath("nowriter.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();

  for (const bool loop : {true, false}) {
    auto pending = std::async(std::launch::async, [&] {
      rdb::binaryDeviceRO dev(path, desc, loop, "DEVICE");
      const auto fill   = dev.fill();
      const auto record = readRecord(dev);
      return std::make_tuple(dev.initializationError(), fill, record, dev.exhausted());
    });
    if (pending.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
      std::cerr << "binaryDeviceRO(DEVICE) blocked on a FIFO without a writer\n";
      std::_Exit(EXIT_FAILURE);
    }
    const auto [error, fill, record, exhausted] = pending.get();
    EXPECT_EQ(error, "") << loop;
    EXPECT_EQ(fill, rdb::binaryDeviceRO::fillResult::endOfFile) << loop;
    EXPECT_EQ(record, std::nullopt) << loop;
    EXPECT_FALSE(exhausted) << loop;
  }
}

// Bez ONESHOT EOF znaczy "teraz nie ma pisarza": all-null, zrodlo zostaje otwarte, a nowy pisarz
// wznawia dane. Z ONESHOT pierwszy EOF po danych to wyczerpanie i dalsze dane juz nie wracaja.
TEST_F(BinaryDeviceROTest, device_eof_means_no_writer_and_with_oneshot_exhaustion_after_data) {
  // covers: faccbindev.binary-device-ro.10~1
  auto path = sandboxPath("eof.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();

  for (const bool oneShot : {false, true}) {
    rdb::binaryDeviceRO dev(path, desc, !oneShot, "DEVICE");
    int writer = openWriter(path);
    writeAll(writer, {0x01, 0x02, 0x03, 0x04});
    ::close(writer);
    EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04})) << oneShot;
    EXPECT_EQ(dev.fill(), rdb::binaryDeviceRO::fillResult::endOfFile) << oneShot;
    EXPECT_EQ(readRecord(dev), std::nullopt) << oneShot;
    EXPECT_EQ(dev.exhausted(), oneShot);

    writer = openWriter(path);
    writeAll(writer, {0x05, 0x06, 0x07, 0x08});
    const auto resumed = readRecord(dev);
    ::close(writer);
    if (oneShot)
      EXPECT_EQ(resumed, std::nullopt) << "po wyczerpaniu zrodlo milczy jak BINFILE ONESHOT";
    else
      EXPECT_EQ(resumed, (std::vector<uint8_t>{0x05, 0x06, 0x07, 0x08}));
  }
}

// Niepelny rekord w chwili EOF jest odrzucany: kolejny pisarz zaczyna od nowego rekordu, zamiast
// sklejac sie z resztka poprzedniego.
TEST_F(BinaryDeviceROTest, device_drops_an_incomplete_record_at_eof) {
  auto path = sandboxPath("torn.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  int writer = openWriter(path);
  writeAll(writer, {0xAA, 0xBB});
  ::close(writer);
  EXPECT_EQ(dev.fill(), rdb::binaryDeviceRO::fillResult::endOfFile);
  EXPECT_EQ(readRecord(dev), std::nullopt);

  writer = openWriter(path);
  writeAll(writer, {0x01, 0x02, 0x03, 0x04});
  EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}));
  ::close(writer);
}

// read() pod blokada modelu nie ponawia proby, ktora faza DEVICE juz wykonala w tym slocie: dane,
// ktore przyszly po terminie, naleza do nastepnego slotu.
TEST_F(BinaryDeviceROTest, device_read_does_not_retry_after_the_device_phase) {
  // covers: faccbindev.binary-device-ro.13~1, sourcebuffer.source-buffer.2~1
  auto path = sandboxPath("late.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_GE(writer, 0);
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  EXPECT_EQ(dev.fill(), rdb::binaryDeviceRO::fillResult::wouldBlock);
  writeAll(writer, {0x01, 0x02, 0x03, 0x04});
  EXPECT_EQ(readRecord(dev), std::nullopt);
  EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04})) << "zrodlo spoza fazy dostaje jedna probe w read()";
  ::close(writer);
}

// BINFILE czyta wylacznie plik zwykly i sprawdza to PRZED otwarciem (#346): open(O_RDONLY) na FIFO
// bez pisarza wisi w samym wywolaniu. Konstrukcja idzie w osobnym watku z limitem czasu - gdyby
// akcesor otworzyl FIFO, test zakonczylby proces zamiast wisiec do limitu ctest.
TEST_F(BinaryDeviceROTest, binfile_refuses_a_fifo_without_blocking) {
  // covers: faccbindev.binary-device-ro.12~1
  auto path = sandboxPath("feed.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();

  auto pending = std::async(std::launch::async, [&] {
    rdb::binaryDeviceRO dev(path, desc, true, "BINFILE");
    uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    return std::make_pair(dev.initializationError(), dev.read(out, 0));
  });
  if (pending.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    std::cerr << "binaryDeviceRO(BINFILE) blocked on a FIFO without a writer\n";
    std::_Exit(EXIT_FAILURE);
  }
  const auto [error, readResult] = pending.get();
  EXPECT_EQ(error, "BINFILE '" + path + "' is a FIFO, not a regular file");
  EXPECT_EQ(readResult, EBADF);
}

TEST_F(BinaryDeviceROTest, binfile_refuses_a_character_device_and_device_a_regular_file) {
  // covers: faccbindev.binary-device-ro.12~1
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO null(std::string("/dev/null"), desc, true, "BINFILE");
  EXPECT_EQ(null.initializationError(), "BINFILE '/dev/null' is a character device, not a regular file");

  auto path = sandboxPath("plain.bin");
  writeBinaryFile(path, {0x01, 0x02, 0x03, 0x04});
  rdb::binaryDeviceRO plain(path, desc, true, "DEVICE");
  EXPECT_EQ(plain.initializationError(), "DEVICE '" + path + "' is a regular file, not a character device or FIFO");
  uint8_t out[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  EXPECT_EQ(plain.read(out, 0), EBADF) << "odrzucona sciezka nie moze byc czytana";

  rdb::binaryDeviceRO device(std::string("/dev/zero"), desc, true, "DEVICE");
  EXPECT_EQ(device.initializationError(), "");
  EXPECT_EQ(device.read(out, 0), EXIT_SUCCESS);
}

// ---- Faza DEVICE (#347): jedno poll() na wszystkie zrodla, terminy od poczatku slotu ----

// TIMEOUT 0: jedna proba nieblokujaca, bez czekania.
TEST_F(BinaryDeviceROTest, await_with_zero_timeout_does_not_wait) {
  // covers: faccbindev.await-records.1~1
  auto path = sandboxPath("empty.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  auto desc        = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  EXPECT_LT(timedAwait({{&dev, std::chrono::milliseconds(0)}}), std::chrono::milliseconds(50));
  EXPECT_EQ(readRecord(dev), std::nullopt);
  ::close(writer);
}

// Pisarz spozniony w granicach terminu: rekord trafia do tego slotu, a faza konczy sie razem z nim,
// nie z terminem.
TEST_F(BinaryDeviceROTest, await_returns_when_a_late_writer_completes_the_record) {
  // covers: faccbindev.await-records.1~1, faccbindev.await-records.4~1
  auto path = sandboxPath("late-writer.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  auto desc        = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  std::thread late([writer] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    writeAll(writer, {0x01, 0x02, 0x03, 0x04});
  });
  const auto elapsed = timedAwait({{&dev, std::chrono::seconds(3)}});
  late.join();
  EXPECT_GE(elapsed, std::chrono::milliseconds(90));
  EXPECT_LT(elapsed, std::chrono::milliseconds(2000));
  EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}));
  ::close(writer);
}

// Niepelny rekord po terminie: all-null w tym slocie, rekord dokonczony trafia do nastepnego.
TEST_F(BinaryDeviceROTest, await_partial_record_hits_the_deadline_and_completes_later) {
  // covers: faccbindev.binary-device-ro.13~1
  auto path = sandboxPath("partial.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  auto desc        = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  writeAll(writer, {0x0A, 0x0B});
  const auto elapsed = timedAwait({{&dev, std::chrono::milliseconds(200)}});
  EXPECT_GE(elapsed, std::chrono::milliseconds(200));
  EXPECT_LT(elapsed, std::chrono::milliseconds(600));
  EXPECT_EQ(readRecord(dev), std::nullopt);

  writeAll(writer, {0x0C, 0x0D});
  EXPECT_LT(timedAwait({{&dev, std::chrono::milliseconds(200)}}), std::chrono::milliseconds(150));
  EXPECT_EQ(readRecord(dev), (std::vector<uint8_t>{0x0A, 0x0B, 0x0C, 0x0D}));
  ::close(writer);
}

// Dwa DEVICE w jednym slocie czekaja rownolegle: laczny czas to maksimum terminow, nie suma.
TEST_F(BinaryDeviceROTest, await_of_two_sources_takes_the_longer_deadline_not_the_sum) {
  // covers: faccbindev.await-records.2~1
  auto first  = sandboxPath("first.fifo");
  auto second = sandboxPath("second.fifo");
  ASSERT_EQ(::mkfifo(first.c_str(), 0600), 0);
  ASSERT_EQ(::mkfifo(second.c_str(), 0600), 0);
  const int firstWriter  = ::open(first.c_str(), O_RDWR | O_CLOEXEC);
  const int secondWriter = ::open(second.c_str(), O_RDWR | O_CLOEXEC);
  auto desc              = fixedIntDescriptor();
  rdb::binaryDeviceRO a(first, desc, true, "DEVICE");
  rdb::binaryDeviceRO b(second, desc, true, "DEVICE");

  const auto elapsed = timedAwait({{&a, std::chrono::milliseconds(300)}, {&b, std::chrono::milliseconds(600)}});
  EXPECT_GE(elapsed, std::chrono::milliseconds(600));
  EXPECT_LT(elapsed, std::chrono::milliseconds(850)) << "suma terminow to 900 ms";
  ::close(firstWriter);
  ::close(secondWriter);
}

// EOF konczy czekanie zrodla od razu: FIFO bez pisarza nie trzyma slotu do terminu.
TEST_F(BinaryDeviceROTest, await_ends_on_eof_without_waiting_for_the_deadline) {
  // covers: faccbindev.await-records.4~1
  auto path = sandboxPath("eof-wait.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto desc = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  EXPECT_LT(timedAwait({{&dev, std::chrono::seconds(2)}}), std::chrono::milliseconds(200));
  int writer = openWriter(path);
  writeAll(writer, {0x01, 0x02});
  ::close(writer);
  EXPECT_LT(timedAwait({{&dev, std::chrono::seconds(2)}}), std::chrono::milliseconds(200)) << "pisarz odszedl w polowie rekordu";
  EXPECT_EQ(readRecord(dev), std::nullopt);
}

namespace {
std::atomic<int> signalsDelivered{0};
void countSignal(int /*signal*/) { signalsDelivered.fetch_add(1, std::memory_order_relaxed); }
}  // namespace

// EINTR z poll() nie odnawia terminu. Watek pomocniczy przerywa czekanie co 5 ms prawdziwym sygnalem
// (handler bez SA_RESTART); przy odnawianym terminie faza trwalaby, dopoki leca sygnaly (2 s).
// Odnowienie lapie gorna granica 700 ms; kontrola liczby sygnalow sprawdza tylko, ze przerwanie zaszlo
// wiecej niz raz. Nie zalezy od ziarna budzika: na macOS 5 ms wychodzi pod obciazeniem ok. 37 ms (#408).
TEST_F(BinaryDeviceROTest, await_deadline_survives_eintr) {
  // covers: faccbindev.await-records.3~1
  auto path = sandboxPath("eintr.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  auto desc        = fixedIntDescriptor();
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  struct sigaction action{};
  struct sigaction previous{};
  action.sa_handler = countSignal;
  sigemptyset(&action.sa_mask);
  ASSERT_EQ(::sigaction(SIGUSR1, &action, &previous), 0);
  signalsDelivered = 0;

  std::atomic<bool> stop{false};
  const pthread_t waiter = ::pthread_self();
  std::thread interrupter([&] {
    const auto limit = clock_type::now() + std::chrono::seconds(2);
    while (!stop.load() && clock_type::now() < limit) {
      ::pthread_kill(waiter, SIGUSR1);
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });
  const auto elapsed = timedAwait({{&dev, std::chrono::milliseconds(300)}});
  stop               = true;
  interrupter.join();
  ::sigaction(SIGUSR1, &previous, nullptr);

  EXPECT_GE(elapsed, std::chrono::milliseconds(300));
  EXPECT_LT(elapsed, std::chrono::milliseconds(700));
  EXPECT_GE(signalsDelivered.load(), 2) << "czekanie nie zostalo przerwane - test niczego nie sprawdzil";
  ::close(writer);
}

// EAGAIN po przebudzeniu nie odnawia terminu. Pisarz kapie po bajcie co 20 ms do rekordu 64 B, ktory
// skompletowalby sie po ok. 1,3 s; kazdy bajt budzi poll(), a read() po nim konczy sie EAGAIN.
// Jak wyzej: odnowienie lapie granica 700 ms, kontrola kropel nie zalezy od ziarna budzika (na macOS
// 20 ms wychodzi ok. 100 ms, #408).
TEST_F(BinaryDeviceROTest, await_deadline_survives_eagain) {
  // covers: faccbindev.await-records.3~1
  auto path = sandboxPath("drip.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  const int writer = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  const rdb::Descriptor desc{"a", 64, 1, rdb::BYTE};
  rdb::binaryDeviceRO dev(path, desc, true, "DEVICE");

  std::atomic<bool> stop{false};
  std::atomic<int> dripped{0};
  std::thread drip([&] {
    while (!stop.load() && dripped.load() < 63) {
      writeAll(writer, {0x55});
      dripped.fetch_add(1);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });
  const auto elapsed = timedAwait({{&dev, std::chrono::milliseconds(300)}});
  stop               = true;
  drip.join();

  EXPECT_GE(elapsed, std::chrono::milliseconds(300));
  EXPECT_LT(elapsed, std::chrono::milliseconds(700));
  EXPECT_GE(dripped.load(), 2) << "pisarz nie budzil czekania - test niczego nie sprawdzil";
  ::close(writer);
}

// Niezmiennik fazy DEVICE: akcesor nalezacy do migawki nie moze zginac w jej trakcie. Kontrola stoi
// w destruktorze i tylko w Debug.
#ifndef NDEBUG
TEST_F(BinaryDeviceROTest, device_destroyed_during_the_device_phase_is_fatal) {
  // covers: faccbindev.await-records.5~1
  auto desc = fixedIntDescriptor();
  EXPECT_DEATH(
      {
        auto dev = std::make_unique<rdb::binaryDeviceRO>(std::string("/dev/zero"), desc, true, "DEVICE");
        dev->markAwaited(true);
        dev.reset();
      },
      "destroyed during the DEVICE phase");

  rdb::binaryDeviceRO dev(std::string("/dev/zero"), desc, true, "DEVICE");
  dev.markAwaited(true);
  rdb::awaitRecords(std::vector<rdb::deviceWait>{{.source = &dev, .deadline = clock_type::now()}});
  // Faza zdjela znacznik - zniszczenie po niej jest legalne.
}
#endif

TEST_F(BinaryDeviceROTest, binfile_retries_eintr_and_short_reads_without_interpreting_bytes) {
  // covers: faccbindev.binary-device-ro.2~1, faccbindev.binary-device-ro.3~1, faccbindev.binary-device-ro.6~1
  const auto path = sandboxPath("short.bin");
  writeBinaryFile(path, {0xFF, 0, 0x7F, 0x80});
  const rdb::Descriptor descriptor{{"opaque", 3, 1, rdb::STRING}, {"flag", 1, 1, rdb::BYTE}};
  rdb::binaryDeviceRO source(path, descriptor, false, "BINFILE");
  readFaultFd = source.pollDescriptor();
  readFault   = ReadFault::InterruptedThenShort;
  std::vector<uint8_t> bytes(4, 0xAA);
  std::vector<bool> nulls{true};
  EXPECT_EQ(source.read(bytes.data(), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(injectedReads, 3);
  EXPECT_EQ(bytes, (std::vector<uint8_t>{0xFF, 0, 0x7F, 0x80}));
  EXPECT_EQ(nulls, (std::vector<bool>{false, false}));
  EXPECT_EQ(rdbtest::ok(source.count()), 1U);
}

TEST_F(BinaryDeviceROTest, binfile_invalid_position_and_read_error_return_counted_zero_null_rows) {
  // covers: faccbindev.binary-device-ro.5~1, faccbindev.binary-device-ro.7~1, faccbindev.binary-device-ro.11~1
  // covers: fainterface.file-interface.5~1
  const auto path = sandboxPath("error.bin");
  writeBinaryFile(path, {1, 2, 3, 4});
  auto descriptor = fixedIntDescriptor();
  rdb::binaryDeviceRO source(path, descriptor, false, "BINFILE");
  std::vector<uint8_t> bytes(4, 0xFF);
  std::vector<bool> nulls{false};
  EXPECT_EQ(source.read(bytes.data(), nulls, 1), EINVAL);
  EXPECT_EQ(bytes, (std::vector<uint8_t>{0, 0, 0, 0}));
  EXPECT_EQ(nulls, std::vector<bool>{true});
  readFaultFd = source.pollDescriptor();
  readFault   = ReadFault::Error;
  EXPECT_EQ(source.read(bytes.data(), nulls, 0), EIO);
  EXPECT_EQ(bytes, (std::vector<uint8_t>{0, 0, 0, 0}));
  EXPECT_EQ(nulls, std::vector<bool>{true});
  EXPECT_FALSE(source.exhausted());
  readFault = ReadFault::None;
  EXPECT_EQ(source.read(bytes.data(), nulls, 0), EXIT_SUCCESS);
  EXPECT_EQ(bytes, (std::vector<uint8_t>{1, 2, 3, 4}));
  EXPECT_EQ(nulls, std::vector<bool>{false});
  EXPECT_EQ(rdbtest::ok(source.count()), 3U);
}

TEST_F(BinaryDeviceROTest, poll_failure_ends_all_waits_and_clears_every_awaited_mark) {
  // covers: faccbindev.await-records.4~1, faccbindev.await-records.5~1
  const auto first  = sandboxPath("poll-first.fifo");
  const auto second = sandboxPath("poll-second.fifo");
  ASSERT_EQ(::mkfifo(first.c_str(), 0600), 0);
  ASSERT_EQ(::mkfifo(second.c_str(), 0600), 0);
  const int firstWriter  = ::open(first.c_str(), O_RDWR | O_CLOEXEC);
  const int secondWriter = ::open(second.c_str(), O_RDWR | O_CLOEXEC);
  ASSERT_GE(firstWriter, 0);
  ASSERT_GE(secondWriter, 0);
  auto descriptor = fixedIntDescriptor();
  rdb::binaryDeviceRO a(first, descriptor, true, "DEVICE");
  rdb::binaryDeviceRO b(second, descriptor, true, "DEVICE");
  a.markAwaited(true);
  b.markAwaited(true);
  LogCapture log;
  failPoll = true;
  EXPECT_LT(timedAwait({{&a, std::chrono::seconds(30)}, {&b, std::chrono::seconds(30)}}), std::chrono::seconds(1));
  EXPECT_EQ(injectedPolls, 1);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
  EXPECT_NE(log.text().find("awaitRecords: poll failed"), std::string::npos);
#endif
  EXPECT_EQ(readRecord(a), std::nullopt);
  EXPECT_EQ(readRecord(b), std::nullopt);
  // Destruktory w Debug sprawdzaja zdjecie obu znacznikow takze po bledzie poll().
  ::close(firstWriter);
  ::close(secondWriter);
}

TEST_F(BinaryDeviceROTest, device_read_error_does_not_exhaust_and_next_slot_recovers) {
  // covers: faccbindev.await-records.4~1
  const auto path = sandboxPath("read-error.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto descriptor = fixedIntDescriptor();
  rdb::binaryDeviceRO source(path, descriptor, false, "DEVICE");
  const int writer = openWriter(path);
  readFaultFd      = source.pollDescriptor();
  readFault        = ReadFault::Error;
  source.markAwaited(true);
  EXPECT_LT(timedAwait({{&source, std::chrono::seconds(30)}}), std::chrono::seconds(1));
  EXPECT_EQ(readRecord(source), std::nullopt);
  EXPECT_FALSE(source.exhausted());
  EXPECT_EQ(injectedReads, 1);
  readFault = ReadFault::None;
  writeAll(writer, {1, 2, 3, 4});
  EXPECT_EQ(readRecord(source), (std::vector<uint8_t>{1, 2, 3, 4}));
  EXPECT_EQ(rdbtest::ok(source.count()), 2U);
  ::close(writer);
}

TEST_F(BinaryDeviceROTest, device_fill_retries_eintr_and_short_reads) {
  // covers: faccbindev.binary-device-ro.3~1
  const auto path = sandboxPath("interrupted.fifo");
  ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
  auto descriptor = fixedIntDescriptor();
  rdb::binaryDeviceRO source(path, descriptor, true, "DEVICE");
  const int writer = openWriter(path);
  writeAll(writer, {0x11, 0x22, 0x33, 0x44});
  readFaultFd = source.pollDescriptor();
  readFault   = ReadFault::InterruptedThenShort;
  EXPECT_EQ(readRecord(source), (std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44}));
  EXPECT_EQ(injectedReads, 3);
  ::close(writer);
}

TEST_F(BinaryDeviceROTest, missing_sources_report_initialization_error_and_errno) {
  // covers: faccbindev.binary-device-ro.14~1
  const auto regular = sandboxPath("parent.bin");
  writeBinaryFile(regular, {1, 2, 3, 4});
  const auto desc = fixedIntDescriptor();
  for (const std::string type : {"BINFILE", "DEVICE"}) {
    for (const auto &[path, expected] :
         std::vector<std::pair<std::string, int>>{{sandboxPath("absent"), ENOENT}, {regular + "/input", ENOTDIR}}) {
      rdb::binaryDeviceRO source(path, desc, true, type);
      EXPECT_EQ(source.initializationErrno(), expected);
      EXPECT_TRUE(source.initializationError().contains(type));
      EXPECT_TRUE(source.initializationError().contains(path));
      EXPECT_TRUE(source.initializationError().contains(std::strerror(expected)));
      EXPECT_FALSE(source.exhausted());
    }
  }
}

// NOLINTEND(modernize-avoid-c-arrays)
