#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define private public
#include "retractor/lib/dumpManager.hpp"
#undef private

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"
#include "syscallWrap.hpp"

extern "C" off_t __real_lseek(int fd, off_t offset, int whence);
extern "C" ssize_t __real_write(int fd, const void *buf, size_t count);
extern "C" int __real_unlink(const char *path);

namespace {
bool wrapEnabled      = false;
int wrappedLseekCalls = 0;
int wrappedWriteCalls = 0;
int wrappedLastFd     = -1;
// Wstrzykniete porazki: errno != 0 konczy lseek bledem; writeResult >= 0 albo writeErrno != 0
// zastepuje wynik write (writeResult == -1 oznacza zapis pelny).
int lseekErrno          = 0;
ssize_t writeResult     = -1;
int writeErrno          = 0;
size_t lastWriteRequest = 0;
// unlink: po prawdziwym usunieciu odtwarza plik pod ta sama nazwa tyle razy (-1 = zawsze), jak
// konkurent tworzacy plik miedzy unlink() a open(). Dziala niezaleznie od wrapEnabled.
int recreateAfterUnlink = 0;
int wrappedUnlinkCalls  = 0;
}  // namespace

extern "C" off_t __wrap_lseek(int fd, off_t offset, int whence) {
  if (!wrapEnabled) return __real_lseek(fd, offset, whence);
  (void)offset;
  (void)whence;
  wrappedLseekCalls++;
  wrappedLastFd = fd;
  if (lseekErrno != 0) {
    errno = lseekErrno;
    return -1;
  }
  return 0;
}

extern "C" ssize_t __wrap_write(int fd, const void *buf, size_t count) {
  if (!wrapEnabled) return __real_write(fd, buf, count);
  (void)buf;
  wrappedWriteCalls++;
  wrappedLastFd    = fd;
  lastWriteRequest = count;
  if (writeResult >= 0 || writeErrno != 0) {
    errno = writeErrno;
    return writeResult;
  }
  return static_cast<ssize_t>(count);
}

extern "C" int __wrap_unlink(const char *path) {
  const int result = __real_unlink(path);
  if (recreateAfterUnlink == 0) return result;
  wrappedUnlinkCalls++;
  if (recreateAfterUnlink > 0) recreateAfterUnlink--;
  const int competitor = ::open(path, O_WRONLY | O_CREAT | O_CLOEXEC, 0600);
  if (competitor >= 0) ::close(competitor);
  return result;
}

// Spiecie __wrap_/__real_ z prawdziwym wywolaniem systemowym - patrz syscallWrap.hpp.
RDB_WRAP_SYSCALL(off_t, lseek, (int fd, off_t offset, int whence), (fd, offset, whence));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *buf, size_t count), (fd, buf, count));
RDB_WRAP_SYSCALL(int, unlink, (const char *path), (path));

// Przywraca wszystkie wstrzykniecia, takze gdy asercja przerwie test w polowie.
struct wrapReset {
  wrapReset()                             = default;
  wrapReset(const wrapReset &)            = delete;
  wrapReset &operator=(const wrapReset &) = delete;
  ~wrapReset() {
    wrapEnabled         = false;
    lseekErrno          = 0;
    writeResult         = -1;
    writeErrno          = 0;
    recreateAfterUnlink = 0;
  }
};

TEST(dumpManager, buildDumpChunk_accepts_fd_zero_as_valid) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 42);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;
  wrappedLastFd     = -1;

  const bool completed = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));

  wrapEnabled = false;

  EXPECT_TRUE(completed);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);
  EXPECT_EQ(wrappedLseekCalls, 1);
  EXPECT_EQ(wrappedWriteCalls, 1);
  EXPECT_EQ(wrappedLastFd, 0);
}

TEST(dumpManager, buildDumpChunk_rejects_negative_fd) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 7);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = -1;

  // Niezmiennik silnika: RDB_ASSERT -> rdb::fatal (FATAL i koniec procesu). Plaster B3 fazy 1
  // zamienil tu smierc procesu na rzut LogicError; rdzen bez wyjatkow wraca do zatrzymania.
  EXPECT_DEATH(static_cast<void>(manager.buildDumpChunk(task, payload.get())), "FATAL: .*file descriptor is not set");
}

// ============================================================
// buildDumpChunk - opóźnienie (delayDumpRecordsToGo > 0)
// ============================================================

TEST(dumpManager, buildDumpChunk_delay_decrements_returns_false) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {2, 4}, 0);
  task.dumpedRecordsToGo    = 2;
  task.delayDumpRecordsToGo = 2;
  task.fd                   = 0;  // wrapping nie jest potrzebne - zapis nie nastąpi

  const bool result = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));

  EXPECT_FALSE(result);
  EXPECT_EQ(task.delayDumpRecordsToGo, 1);
  EXPECT_EQ(task.dumpedRecordsToGo, 2);  // bez zmian
}

TEST(dumpManager, buildDumpChunk_delay_exhausted_then_writes) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);
  payload->setItem(0, 99);

  dumpTask task("task", {1, 2}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = 1;
  task.fd                   = 0;

  // Pierwsze wywołanie: opóźnienie, brak zapisu
  bool r1 = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.delayDumpRecordsToGo, 0);

  // Drugie wywołanie: opóźnienie wyczerpane, zapis
  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;

  bool r2 = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));

  wrapEnabled = false;

  EXPECT_TRUE(r2);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);
  EXPECT_EQ(wrappedLseekCalls, 1);
  EXPECT_EQ(wrappedWriteCalls, 1);
}

// ============================================================
// buildDumpChunk - wiele rekordów
// ============================================================

TEST(dumpManager, buildDumpChunk_multiple_records_not_done_until_last) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 3}, 0);
  task.dumpedRecordsToGo    = 3;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  wrapEnabled       = true;
  wrappedWriteCalls = 0;

  bool r1 = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.dumpedRecordsToGo, 2);

  bool r2 = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));
  EXPECT_FALSE(r2);
  EXPECT_EQ(task.dumpedRecordsToGo, 1);

  bool r3 = rdbtest::ok(manager.buildDumpChunk(task, payload.get()));
  EXPECT_TRUE(r3);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);

  wrapEnabled = false;

  EXPECT_EQ(wrappedWriteCalls, 3);
}

// ============================================================
// buildDumpChunk - FatalError dla ujemnych wartości
// ============================================================

TEST(dumpManager, buildDumpChunk_negative_dumpedRecordsToGo_fatals) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 0}, 0);
  task.dumpedRecordsToGo    = -1;
  task.delayDumpRecordsToGo = 0;
  task.fd                   = 0;

  // Niezmiennik silnika: RDB_ASSERT -> rdb::fatal (FATAL i koniec procesu). Plaster B3 fazy 1
  // zamienil tu smierc procesu na rzut LogicError; rdzen bez wyjatkow wraca do zatrzymania.
  EXPECT_DEATH(static_cast<void>(manager.buildDumpChunk(task, payload.get())), "FATAL: .*dumpedRecordsToGo is negative");
}

TEST(dumpManager, buildDumpChunk_negative_delayDumpRecordsToGo_fatals) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 1}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = -1;
  task.fd                   = 0;

  // Niezmiennik silnika: RDB_ASSERT -> rdb::fatal (FATAL i koniec procesu). Plaster B3 fazy 1
  // zamienil tu smierc procesu na rzut LogicError; rdzen bez wyjatkow wraca do zatrzymania.
  EXPECT_DEATH(static_cast<void>(manager.buildDumpChunk(task, payload.get())), "FATAL: .*delayDumpRecordsToGo is negative");
}

// ============================================================
// dumpTask - semantyka move i destruktor
// ============================================================

TEST(dumpTask, move_constructor_transfers_fd) {
  int fd = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd, 0);

  dumpTask src("src", {0, 1}, 0);
  src.fd = fd;

  dumpTask dst(std::move(src));

  EXPECT_EQ(dst.fd, fd);
  EXPECT_EQ(src.fd, -1);  // NOLINT(bugprone-use-after-move)
}

TEST(dumpTask, move_assignment_closes_old_fd_and_transfers) {
  int fd1 = ::open("/dev/null", O_RDWR);
  int fd2 = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd1, 0);
  ASSERT_GE(fd2, 0);

  dumpTask src("src", {0, 1}, 0);
  src.fd = fd1;

  dumpTask dst("dst", {0, 1}, 0);
  dst.fd = fd2;

  dst = std::move(src);  // zamyka fd2, przejmuje fd1

  EXPECT_EQ(dst.fd, fd1);
  EXPECT_EQ(src.fd, -1);  // NOLINT(bugprone-use-after-move)
  // fd2 powinien być zamknięty
  EXPECT_EQ(fcntl(fd2, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST(dumpTask, destructor_closes_fd) {
  int fd = ::open("/dev/null", O_RDWR);
  ASSERT_GE(fd, 0);

  {
    dumpTask task("t", {0, 1}, 0);
    task.fd = fd;
  }  // destruktor zamyka fd

  EXPECT_EQ(fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST(dumpTask, destructor_with_negative_fd_does_not_crash) {
  dumpTask task("t", {0, 1}, 0);
  // task.fd = -1 domyślnie - destruktor nie wywołuje close
}

// ============================================================
// setDumpStorage + createDumpFile
// ============================================================

class DumpManagerFileTest : public ::testing::Test {
 protected:
  std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_dumpManager_files";

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
  }

  void TearDown() override {
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
  }
};

TEST_F(DumpManagerFileTest, setDumpStorage_sets_path) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  EXPECT_EQ(manager.storagePath, sandBoxFolder.string());
}

TEST_F(DumpManagerFileTest, createDumpFile_without_retention_creates_tmp_suffix) {
  dumpManager manager;
  manager.storagePath = sandBoxFolder.string();
  // retentionSize["stream_t"] = 0 (domyślna wartość mapy)

  auto [filename, fd] = rdbtest::ok(manager.createDumpFile("stream", "t"));
  ASSERT_GE(fd, 0);
  ::close(fd);

  EXPECT_TRUE(std::filesystem::exists(filename));
  EXPECT_TRUE(std::string_view(filename).ends_with("_dump.tmp"));
}

TEST_F(DumpManagerFileTest, createDumpFile_with_retention_creates_numbered_files) {
  dumpManager manager;
  manager.storagePath                  = sandBoxFolder.string();
  manager.retentionSize["stream_task"] = 3;

  auto [f0, fd0] = rdbtest::ok(manager.createDumpFile("stream", "task"));
  ASSERT_GE(fd0, 0);
  ::close(fd0);

  auto [f1, fd1] = rdbtest::ok(manager.createDumpFile("stream", "task"));
  ASSERT_GE(fd1, 0);
  ::close(fd1);

  EXPECT_TRUE(std::string_view(f0).ends_with("_dump_0.tmp"));
  EXPECT_TRUE(std::string_view(f1).ends_with("_dump_1.tmp"));
  EXPECT_TRUE(std::filesystem::exists(f0));
  EXPECT_TRUE(std::filesystem::exists(f1));
}

// Drugi deskryptor na pliku zrzutu. Po unlink i zamknieciu fd zadania tylko przez niego widac,
// czy odlaczony i-wezel jeszcze rosnie.
struct dumpObserver {
  int fd{-1};
  off_t sizeAtOpen{-1};
  explicit dumpObserver(const std::string &path) : fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC)) {
    struct stat st{};
    if (fd >= 0 && ::fstat(fd, &st) == 0) sizeAtOpen = st.st_size;
  }
  dumpObserver(const dumpObserver &)            = delete;
  dumpObserver &operator=(const dumpObserver &) = delete;
  ~dumpObserver() {
    if (fd >= 0) ::close(fd);
  }
};

// Plik odlaczony od katalogu i bez zapisow od otwarcia obserwatora.
void expectDetachedAndFrozen(const dumpObserver &observer) {
  ASSERT_GE(observer.sizeAtOpen, 0);
  struct stat now{};
  ASSERT_EQ(::fstat(observer.fd, &now), 0);
  EXPECT_EQ(now.st_nlink, 0);
  EXPECT_EQ(now.st_size, observer.sizeAtOpen);
}

// Gdyby numer zamknietego fd dostalo kolejne open(), F_GETFD by sie udalo i asercja padnie -
// ponowny przydzial numeru nie ukryje wiec niezamknietego deskryptora.
void expectClosed(int fd) {
  errno = 0;
  EXPECT_EQ(::fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

const dumpTask &taskOf(const dumpManager &manager, const std::string &filename) {
  const auto &tasks = manager.bookOfTasks.at("result");
  const auto task   = std::find_if(tasks.begin(), tasks.end(), [&](const dumpTask &t) { return t.dumpFilename == filename; });
  if (task == tasks.end()) throw std::runtime_error("no dump task for " + filename);
  return *task;
}

class DumpManagerTaskTest : public DumpManagerFileTest, public ::testing::WithParamInterface<size_t> {
 protected:
  qTree plan;
  std::unique_ptr<dataModel> model;

  // Poczatek zakresu DUMP dla parametru: zapis od razu, jeden rekord historii (strumien ma wtedy
  // RETENTION 2) i start opozniony o dwa rekordy.
  [[nodiscard]] long rangeStart() const { return std::array<long, 3>{0, -1, 2}.at(GetParam()); }

  void writeRecord(std::int32_t value) {
    auto &output = *model->qSet.at("result")->outputPayload;
    output.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
  }

  [[nodiscard]] std::string dumpPath(const char *filename) const { return (sandBoxFolder / filename).string(); }

  void expectDump(const char *filename, std::int32_t start, std::int32_t count) {
    std::vector<std::int32_t> expected(count);
    for (std::int32_t i = 0; i < count; ++i)
      expected[i] = start + i;
    std::ifstream dump(sandBoxFolder / filename, std::ios::binary);
    ASSERT_TRUE(dump.is_open()) << filename;
    const std::string actualBytes(std::istreambuf_iterator<char>{dump}, std::istreambuf_iterator<char>{});
    const std::string expectedBytes(reinterpret_cast<const char *>(expected.data()), expected.size() * sizeof(expected[0]));
    EXPECT_EQ(actualBytes, expectedBytes) << filename;
  }

  void SetUp() override {
    DumpManagerFileTest::SetUp();
    const auto inputFile = sandBoxFolder / "input.bin";
    std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
    const auto [status, keyword, streamName] = parserRQLString(
        plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" + inputFile.string() +
                  "'\nSELECT src[0] STREAM result FROM src RETENTION " + std::to_string(GetParam() + 1) + " STORAGE MEMORY\n");
    ASSERT_EQ(status, "OK");
    compiler compilePlan(plan);
    ASSERT_EQ(compilePlan.compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    DumpManagerFileTest::TearDown();
  }
};

// Pierwsza nazwa, zawiniecie retencji i bajty kazdego pozostalego zrzutu.
// Zadania koncza sie przed kolejnym wyzwoleniem - nakladanie zrzutow to osobny kontrakt (#380).
TEST_P(DumpManagerTaskTest, registerTask_retains_completed_dump_files_and_contents) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  const auto retention = GetParam();
  std::map<std::filesystem::path, std::int32_t> expectedDumps;
  auto &output = *model->qSet.at("result")->outputPayload;

  for (int trigger = 0; trigger < 5; ++trigger) {
    const std::int32_t value = 10 + trigger;
    output.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
    ASSERT_RDB_OK(manager.registerTask("result", dumpTask("task", {0, 1}, retention)));

    const auto suffix   = retention == 0 ? "_dump.tmp" : "_dump_" + std::to_string(trigger % retention) + ".tmp";
    const auto filename = sandBoxFolder / ("result_task" + suffix);
    EXPECT_EQ(manager.bookOfTasks.at("result").back().dumpFilename, filename.string());
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
    EXPECT_TRUE(manager.bookOfTasks.at("result").empty());
    expectedDumps[filename] = value;

    size_t dumpCount = 0;
    for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder))
      if (entry.path().extension() == ".tmp") ++dumpCount;
    EXPECT_EQ(dumpCount, expectedDumps.size());
    if (retention > 0) EXPECT_FALSE(std::filesystem::exists(sandBoxFolder / "result_task_dump.tmp"));

    for (const auto &[path, expectedValue] : expectedDumps) {
      std::ifstream dump(path, std::ios::binary);
      ASSERT_TRUE(dump.is_open()) << path;
      std::int32_t actualValue = 0;
      dump.read(reinterpret_cast<char *>(&actualValue), sizeof(actualValue));
      ASSERT_EQ(dump.gcount(), sizeof(actualValue)) << path;
      EXPECT_EQ(actualValue, expectedValue) << path;
      EXPECT_EQ(dump.peek(), std::ifstream::traits_type::eof()) << path;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(BoundarySizes, DumpManagerTaskTest, ::testing::Values(size_t{0}, size_t{1}, size_t{2}));

// #380: regula bez RETENTION odtwarza swoj plik przy kazdym wyzwoleniu. Zastapione zadanie
// konczy sie od razu, choc obca regula z RETENTION 100 trzyma w ksiedze dluzsze okna.
TEST_P(DumpManagerTaskTest, unretained_replacement_closes_only_same_rule_tasks) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  const long first = rangeStart();
  writeRecord(1);
  writeRecord(2);
  writeRecord(3);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("latest", {first, first + 3}, 0)));
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("kept", {0, 7}, 100)));
  const int keptFd = taskOf(manager, dumpPath("result_kept_dump_0.tmp")).fd;
  ASSERT_RDB_OK(manager.processStreamChunk("result"));

  std::list<dumpObserver> observers;
  for (int trigger = 4; trigger <= 5; ++trigger) {
    const int oldFd = taskOf(manager, dumpPath("result_latest_dump.tmp")).fd;
    ASSERT_GE(observers.emplace_back(dumpPath("result_latest_dump.tmp")).sizeAtOpen, 0);

    writeRecord(trigger);
    if (trigger == 4) {
      ASSERT_RDB_OK(manager.registerTask("result", dumpTask("kept", {0, 7}, 100)));
    }
    ASSERT_RDB_OK(manager.registerTask("result", dumpTask("latest", {first, first + 3}, 0)));
    expectClosed(oldFd);
    EXPECT_NE(::fcntl(keptFd, F_GETFD), -1);
    EXPECT_EQ(manager.bookOfTasks.at("result").size(), 3U);
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
  }

  for (int value = 6; value <= 10; ++value) {
    writeRecord(value);
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
  }
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());
  for (const auto &observer : observers)
    expectDetachedAndFrozen(observer);

  expectDump("result_latest_dump.tmp", static_cast<std::int32_t>(5 + first), 3);
  expectDump("result_kept_dump_0.tmp", 3, 7);
  expectDump("result_kept_dump_1.tmp", 4, 7);
}

// #380: regula z RETENTION 2 odtwarza plik slotu 0 przy trzecim wyzwoleniu. Zadanie pierwszego
// wyzwolenia konczy sie wtedy, choc obca regula z RETENTION 100 zostawilaby mu miejsce w ksiedze.
TEST_P(DumpManagerTaskTest, retention_wrap_closes_task_of_reused_slot) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  const long first = rangeStart();
  writeRecord(1);
  writeRecord(2);
  writeRecord(3);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("kept", {0, 9}, 100)));
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2)));
  ASSERT_RDB_OK(manager.processStreamChunk("result"));
  writeRecord(4);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2)));
  ASSERT_RDB_OK(manager.processStreamChunk("result"));

  const int wrappedFd = taskOf(manager, dumpPath("result_ring_dump_0.tmp")).fd;
  const int slot1Fd   = taskOf(manager, dumpPath("result_ring_dump_1.tmp")).fd;
  const int keptFd    = taskOf(manager, dumpPath("result_kept_dump_0.tmp")).fd;
  const dumpObserver observer(dumpPath("result_ring_dump_0.tmp"));
  ASSERT_GE(observer.sizeAtOpen, 0);
  writeRecord(5);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2)));
  expectClosed(wrappedFd);
  EXPECT_NE(::fcntl(slot1Fd, F_GETFD), -1);
  EXPECT_NE(::fcntl(keptFd, F_GETFD), -1);
  EXPECT_EQ(manager.bookOfTasks.at("result").size(), 3U);
  ASSERT_RDB_OK(manager.processStreamChunk("result"));

  for (int value = 6; value <= 11; ++value) {
    writeRecord(value);
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
  }
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());
  expectDetachedAndFrozen(observer);

  expectDump("result_ring_dump_0.tmp", static_cast<std::int32_t>(5 + first), 5);
  expectDump("result_ring_dump_1.tmp", static_cast<std::int32_t>(4 + first), 5);
  expectDump("result_kept_dump_0.tmp", 3, 9);
}

// #380: dwie reguly bez RETENTION na jednym strumieniu maja nakladajace sie okna. Do #380 ksiega
// miala wtedy pojemnosc 1 i wyzwolenie drugiej ucinalo widoczny zrzut pierwszej.
TEST_P(DumpManagerTaskTest, unretained_rules_do_not_evict_each_other) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  const long first = rangeStart();
  writeRecord(1);
  writeRecord(2);
  writeRecord(3);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("a", {first, first + 3}, 0)));
  const int aFd = taskOf(manager, dumpPath("result_a_dump.tmp")).fd;
  ASSERT_RDB_OK(manager.processStreamChunk("result"));
  writeRecord(4);
  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("b", {first, first + 3}, 0)));
  EXPECT_NE(::fcntl(aFd, F_GETFD), -1);
  EXPECT_EQ(manager.bookOfTasks.at("result").size(), 2U);
  ASSERT_RDB_OK(manager.processStreamChunk("result"));

  for (int value = 5; value <= 8; ++value) {
    writeRecord(value);
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
  }
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());

  expectDump("result_a_dump.tmp", static_cast<std::int32_t>(3 + first), 3);
  expectDump("result_b_dump.tmp", static_cast<std::int32_t>(4 + first), 3);
}

// Licznik slotow i RETENTION naleza do pary strumien/regula. Klucz bez separatora sklejal a/bc
// z ab/c (oba "abc"), choc ich pliki sa rozne: druga regula zaczynala od cudzego slotu 1.
TEST_F(DumpManagerFileTest, retention_counters_are_kept_per_stream_and_rule) {
  const auto inputFile = sandBoxFolder / "input.bin";
  std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
  qTree plan;
  const auto [status, keyword, streamName] =
      parserRQLString(plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" +
                                inputFile.string() + "'\nSELECT src[0] STREAM a FROM src\nSELECT src[0] STREAM ab FROM src\n");
  ASSERT_EQ(status, "OK");
  compiler compilePlan(plan);
  ASSERT_EQ(compilePlan.compile(), "OK");
  auto modelOwner  = rdbtest::ok(dataModel::create(plan));
  dataModel &model = *modelOwner;
  pProc            = &model;

  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  ASSERT_RDB_OK(manager.registerTask("a", dumpTask("bc", {0, 1}, 3)));
  ASSERT_RDB_OK(manager.registerTask("ab", dumpTask("c", {0, 1}, 2)));
  ASSERT_RDB_OK(manager.registerTask("a", dumpTask("bc", {0, 1}, 3)));
  EXPECT_EQ(manager.bookOfTasks.at("a").front().dumpFilename, (sandBoxFolder / "a_bc_dump_0.tmp").string());
  EXPECT_EQ(manager.bookOfTasks.at("a").back().dumpFilename, (sandBoxFolder / "a_bc_dump_1.tmp").string());
  EXPECT_EQ(manager.bookOfTasks.at("ab").back().dumpFilename, (sandBoxFolder / "ab_c_dump_0.tmp").string());
  pProc = nullptr;
}

struct DumpHistoryCase {
  int historyDepth;
  const char *storageClause;
};

void PrintTo(const DumpHistoryCase &testCase, std::ostream *out) {
  *out << "H=" << testCase.historyDepth << ", " << testCase.storageClause;
}

class DumpManagerHistoryTest : public DumpManagerFileTest, public ::testing::WithParamInterface<DumpHistoryCase> {
 protected:
  qTree plan;
  std::unique_ptr<dataModel> model;

  void SetUp() override {
    DumpManagerFileTest::SetUp();
    const auto inputFile = sandBoxFolder / "input.bin";
    std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
    const auto [status, keyword, streamName] =
        parserRQLString(plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" +
                                  inputFile.string() + "'\nSELECT src[0] STREAM result FROM src " + GetParam().storageClause +
                                  "\nRULE capture ON result WHEN result[0] >= 10 DO DUMP -" +
                                  std::to_string(GetParam().historyDepth) + " TO 1\n");
    ASSERT_EQ(status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    DumpManagerFileTest::TearDown();
  }
};

// #419: prawdziwy magazyn i regula skompilowanego planu, bez recznego powiekszania
// pierscienia. Zrzut porownujemy bajtowo po kilku obrotach pierscienia/segmentow.
TEST_P(DumpManagerHistoryTest, compiled_rule_dumps_history_and_current_record_bytes) {
  auto &runtime       = *model->qSet.at("result");
  auto &output        = *runtime.outputPayload;
  const auto dumpPath = sandBoxFolder / "result_capture_dump.tmp";

  for (std::int32_t value = 1; value <= 12; ++value) {
    output.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
    ASSERT_EQ(output.getRecordsCount(), static_cast<size_t>(value));
    ASSERT_RDB_OK(runtime.constructRulesAndUpdate(plan.getQuery("result")));
    if (value < 10) continue;

    std::vector<std::int32_t> expected;
    for (auto previous = value - GetParam().historyDepth; previous <= value; ++previous)
      expected.push_back(previous);
    const std::string expectedBytes(reinterpret_cast<const char *>(expected.data()), expected.size() * sizeof(expected[0]));
    std::ifstream dump(dumpPath, std::ios::binary);
    ASSERT_TRUE(dump.is_open());
    const std::string actualBytes(std::istreambuf_iterator<char>{dump}, std::istreambuf_iterator<char>{});
    EXPECT_EQ(actualBytes, expectedBytes) << "trigger=" << value << ", H=" << GetParam().historyDepth;
  }
}

INSTANTIATE_TEST_SUITE_P(StorageBoundaries, DumpManagerHistoryTest,
                         ::testing::Values(DumpHistoryCase{1, "STORAGE MEMORY"}, DumpHistoryCase{3, "STORAGE MEMORY"},
                                           DumpHistoryCase{7, "STORAGE MEMORY"},
                                           DumpHistoryCase{3, "RETENTION 3 STORAGE MEMORY"},
                                           DumpHistoryCase{3, "RETENTION 4 STORAGE MEMORY"},
                                           DumpHistoryCase{3, "RETENTION 1 4 STORAGE DEFAULT"},
                                           DumpHistoryCase{3, "RETENTION 1 4 STORAGE DIRECT"}));

struct DumpPastRangeCase {
  int left;
  int right;
  const char *storageClause;
};

void PrintTo(const DumpPastRangeCase &testCase, std::ostream *out) {
  *out << "DUMP " << testCase.left << " TO " << testCase.right << ", " << testCase.storageClause;
}

class DumpManagerPastRangeTest : public DumpManagerFileTest, public ::testing::WithParamInterface<DumpPastRangeCase> {
 protected:
  qTree plan;
  std::unique_ptr<dataModel> model;

  void SetUp() override {
    DumpManagerFileTest::SetUp();
    const auto inputFile = sandBoxFolder / "input.bin";
    std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
    const auto [status, keyword, streamName] =
        parserRQLString(plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" +
                                  inputFile.string() + "'\nSELECT src[0] STREAM result FROM src " + GetParam().storageClause +
                                  "\nRULE capture ON result WHEN result[0] >= 10 DO DUMP " + std::to_string(GetParam().left) +
                                  " TO " + std::to_string(GetParam().right) + "\n");
    ASSERT_EQ(status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    DumpManagerFileTest::TearDown();
  }
};

// Zakres DUMP jest polotwarty [left, right) wzgledem rekordu biezacego (0). Przy prawej granicy
// <= 0 lezy w calosci w historii i jest gotowy w chwili wyzwolenia: `DUMP -5 TO -2` to rekordy
// -5, -4, -3. Do 2026-10 registerTask zrzucal zawsze historie do -1, a dla prawej granicy ujemnej
// konczyl proces przez rdb::fatal przy pierwszym wyzwoleniu reguly.
TEST_P(DumpManagerPastRangeTest, past_only_range_dumps_exactly_its_history_records) {
  auto &runtime       = *model->qSet.at("result");
  auto &output        = *runtime.outputPayload;
  const auto dumpPath = sandBoxFolder / "result_capture_dump.tmp";

  for (std::int32_t value = 1; value <= 14; ++value) {
    output.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
    ASSERT_RDB_OK(runtime.constructRulesAndUpdate(plan.getQuery("result")));
    if (value < 10) continue;

    // Regula bez RETENTION odtwarza plik przy kazdym wyzwoleniu, wiec plik niesie zakres
    // ostatniego - i nic wiecej: zadanie zakonczone przy wyzwoleniu nie dopisuje kolejnych slotow.
    std::vector<std::int32_t> expected;
    for (auto offset = GetParam().left; offset < GetParam().right; ++offset)
      expected.push_back(value + offset);
    const std::string expectedBytes(reinterpret_cast<const char *>(expected.data()), expected.size() * sizeof(expected[0]));
    std::ifstream dump(dumpPath, std::ios::binary);
    ASSERT_TRUE(dump.is_open());
    const std::string actualBytes(std::istreambuf_iterator<char>{dump}, std::istreambuf_iterator<char>{});
    EXPECT_EQ(actualBytes, expectedBytes) << "trigger=" << value;
  }
}

INSTANTIATE_TEST_SUITE_P(PastRanges, DumpManagerPastRangeTest,
                         ::testing::Values(DumpPastRangeCase{-5, -2, "STORAGE MEMORY"},
                                           DumpPastRangeCase{-5, 0, "STORAGE MEMORY"},
                                           DumpPastRangeCase{-1, 0, "STORAGE MEMORY"},
                                           DumpPastRangeCase{-3, -2, "RETENTION 1 4 STORAGE DIRECT"}));

// ============================================================
// Kontrakt etapu 9 (#521): konstrukcja, tworzenie pliku, porazki I/O
// ============================================================

static_assert(!std::is_copy_constructible_v<dumpTask> && !std::is_copy_assignable_v<dumpTask>);
static_assert(std::is_nothrow_move_constructible_v<dumpTask> && std::is_nothrow_move_assignable_v<dumpTask>);

TEST(dumpTask, constructor_keeps_rule_and_starts_closed_with_zero_counters) {
  const dumpTask task("capture", {-3, 4}, 7);
  EXPECT_EQ(task.taskName, "capture");
  EXPECT_EQ(task.range, (std::pair<long, long>{-3, 4}));
  EXPECT_EQ(task.retentionSize, 7U);
  EXPECT_EQ(task.fd, -1);
  EXPECT_EQ(task.dumpedRecordsToGo, 0);
  EXPECT_EQ(task.delayDumpRecordsToGo, 0);
  EXPECT_TRUE(task.dumpFilename.empty());
}

// Poprzedni plik pod ta nazwa zostaje odlaczony, a nie obciety: deskryptor otwarty na nim przed
// wywolaniem widzi stara tresc i nlink 0. Tryb liczony z umask procesu, nie odczytany z silnika.
TEST_F(DumpManagerFileTest, createDumpFile_replaces_previous_file_with_a_fresh_inode) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  const auto path = sandBoxFolder / "stream_t_dump.tmp";
  std::ofstream(path, std::ios::binary) << "old contents";
  const dumpObserver previous(path.string());
  ASSERT_EQ(previous.sizeAtOpen, 12);
  struct stat before{};
  ASSERT_EQ(::fstat(previous.fd, &before), 0);

  const mode_t mask = ::umask(0);
  ::umask(mask);
  auto [filename, fd] = rdbtest::ok(manager.createDumpFile("stream", "t"));
  ASSERT_GE(fd, 0);
  EXPECT_EQ(filename, path.string());

  struct stat created{};
  ASSERT_EQ(::fstat(fd, &created), 0);
  EXPECT_NE(created.st_ino, before.st_ino);
  EXPECT_EQ(created.st_size, 0);
  EXPECT_EQ(created.st_mode & 0777, 0644 & ~mask);
  EXPECT_NE(::fcntl(fd, F_GETFD) & FD_CLOEXEC, 0);
  EXPECT_EQ(::fcntl(fd, F_GETFL) & O_ACCMODE, O_RDWR);
  expectDetachedAndFrozen(previous);
  ::close(fd);
}

// Konkurent odtwarza plik miedzy unlink() a open(): O_EXCL odmawia, a createDumpFile kasuje
// i probuje ponownie - do skutku, gdy konkurent ustapi, i najwyzej kMaxRecreates (100) razy.
TEST_F(DumpManagerFileTest, createDumpFile_retries_a_competing_creator_at_most_100_times) {
  const wrapReset reset;
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());

  wrappedUnlinkCalls  = 0;
  recreateAfterUnlink = 3;
  auto [filename, fd] = rdbtest::ok(manager.createDumpFile("stream", "won"));
  EXPECT_EQ(wrappedUnlinkCalls, 3);
  EXPECT_EQ(recreateAfterUnlink, 0);
  struct stat created{};
  ASSERT_EQ(::fstat(fd, &created), 0);
  EXPECT_EQ(
      created.st_mode & 0777, 0644 & ~[] {
        const mode_t mask = ::umask(0);
        ::umask(mask);
        return mask;
      }());
  ::close(fd);

  wrappedUnlinkCalls  = 0;
  recreateAfterUnlink = -1;
  const auto refused  = manager.createDumpFile("stream", "lost");
  recreateAfterUnlink = 0;
  ASSERT_RDB_ERROR(refused, rdb::Errc::IO, "failed to create");
  EXPECT_EQ(refused.error().err(), EEXIST);
  EXPECT_EQ(wrappedUnlinkCalls, 100);
  // Plik konkurenta zostaje nietkniety - silnik do niego nie pisal.
  EXPECT_EQ(std::filesystem::file_size(sandBoxFolder / "stream_lost_dump.tmp"), 0U);
}

// Porazki usuniecia i utworzenia wracaja jako Errc::IO z errno, ktore system zwraca dla tej
// samej operacji wykonanej niezaleznie przez test.
TEST_F(DumpManagerFileTest, createDumpFile_reports_remove_and_create_failures_with_errno) {
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  std::filesystem::create_directory(sandBoxFolder / "stream_dir_dump.tmp");
  std::filesystem::create_directory(sandBoxFolder / "probe");
  errno = 0;
  ASSERT_NE(::unlink((sandBoxFolder / "probe").c_str()), 0);
  const int unlinkDirectoryErrno = errno;

  const auto removeFailed = manager.createDumpFile("stream", "dir");
  ASSERT_RDB_ERROR(removeFailed, rdb::Errc::IO, "failed to remove");
  EXPECT_EQ(removeFailed.error().err(), unlinkDirectoryErrno);
  EXPECT_TRUE(std::filesystem::is_directory(sandBoxFolder / "stream_dir_dump.tmp"));

  manager.setDumpStorage((sandBoxFolder / "missing").string());
  const auto createFailed = manager.createDumpFile("stream", "t");
  ASSERT_RDB_ERROR(createFailed, rdb::Errc::IO, "failed to create");
  EXPECT_EQ(createFailed.error().err(), ENOENT);
}

// Jeden write o rozmiarze deskryptora. Niepelny, ale dodatni wynik liczy sie jako rekord i nie
// jest ponawiany; zero i -1 to Errc::IO z errno (EIO, gdy write nie ustawil errno).
TEST(dumpManager, buildDumpChunk_counts_any_positive_write_and_reports_failures) {
  const wrapReset reset;
  dumpManager manager;
  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER) + rdb::Descriptor("w", 8, 1, rdb::DOUBLE);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 2}, 0);
  task.dumpedRecordsToGo = 2;
  task.fd                = 0;
  wrapEnabled            = true;
  wrappedWriteCalls      = 0;
  wrappedLseekCalls      = 0;
  writeResult            = 1;
  EXPECT_FALSE(rdbtest::ok(manager.buildDumpChunk(task, payload.get())));
  EXPECT_EQ(wrappedLseekCalls, 1);
  EXPECT_EQ(wrappedWriteCalls, 1);
  EXPECT_EQ(lastWriteRequest, descriptor.getSizeInBytes());
  EXPECT_EQ(task.dumpedRecordsToGo, 1);

  writeResult = 0;
  writeErrno  = 0;
  ASSERT_RDB_ERROR(manager.buildDumpChunk(task, payload.get()), rdb::Errc::IO, "write to");
  EXPECT_EQ(manager.buildDumpChunk(task, payload.get()).error().err(), EIO);
  EXPECT_EQ(task.dumpedRecordsToGo, 1);

  writeResult        = -1;
  writeErrno         = ENOSPC;
  const auto noSpace = manager.buildDumpChunk(task, payload.get());
  ASSERT_RDB_ERROR(noSpace, rdb::Errc::IO, "write to");
  EXPECT_EQ(noSpace.error().err(), ENOSPC);

  writeErrno            = 0;
  lseekErrno            = ESPIPE;
  wrappedWriteCalls     = 0;
  const auto seekFailed = manager.buildDumpChunk(task, payload.get());
  ASSERT_RDB_ERROR(seekFailed, rdb::Errc::IO, "lseek to end");
  EXPECT_EQ(seekFailed.error().err(), ESPIPE);
  EXPECT_EQ(wrappedWriteCalls, 0);
  EXPECT_EQ(task.dumpedRecordsToGo, 1);
}

class DumpManagerModelTest : public DumpManagerFileTest {
 protected:
  qTree plan;
  std::unique_ptr<dataModel> model;

  void writeRecord(std::int32_t value) {
    auto &output = *model->qSet.at("result")->outputPayload;
    output.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
  }

  void SetUp() override {
    DumpManagerFileTest::SetUp();
    const auto inputFile = sandBoxFolder / "input.bin";
    std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
    const auto [status, keyword, streamName] =
        parserRQLString(plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" +
                                  inputFile.string() + "'\nSELECT src[0] STREAM result FROM src RETENTION 3 STORAGE MEMORY\n");
    ASSERT_EQ(status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    DumpManagerFileTest::TearDown();
  }
};

// Strumien bez zadan nie siega nawet po model (pProc == nullptr to dowodzi), a strumien bez
// rekordow nie pisze i nie przesuwa licznikow zadania. Po wyczerpaniu zakresu plik zadania jest
// zamkniety, a zadanie znika z ksiegi.
TEST_F(DumpManagerModelTest, processStreamChunk_does_no_work_without_tasks_or_records) {
  const wrapReset reset;
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());

  pProc = nullptr;
  ASSERT_RDB_OK(manager.processStreamChunk("result"));
  EXPECT_FALSE(manager.bookOfTasks.contains("result"));
  pProc = model.get();

  ASSERT_RDB_OK(manager.registerTask("result", dumpTask("t", {1, 3}, 0)));
  const int fd      = manager.bookOfTasks.at("result").front().fd;
  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;
  ASSERT_RDB_OK(manager.processStreamChunk("result"));
  EXPECT_EQ(wrappedLseekCalls, 0);
  EXPECT_EQ(wrappedWriteCalls, 0);
  ASSERT_EQ(manager.bookOfTasks.at("result").size(), 1U);
  EXPECT_EQ(manager.bookOfTasks.at("result").front().delayDumpRecordsToGo, 1);
  EXPECT_EQ(manager.bookOfTasks.at("result").front().dumpedRecordsToGo, 2);
  wrapEnabled = false;

  for (std::int32_t value = 1; value <= 3; ++value) {
    writeRecord(value);
    ASSERT_RDB_OK(manager.processStreamChunk("result"));
  }
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());
  expectClosed(fd);
  std::ifstream dump(sandBoxFolder / "result_t_dump.tmp", std::ios::binary);
  const std::string bytes(std::istreambuf_iterator<char>{dump}, std::istreambuf_iterator<char>{});
  const std::array<std::int32_t, 2> expected{2, 3};
  EXPECT_EQ(bytes, std::string(reinterpret_cast<const char *>(expected.data()), sizeof(expected)));
}

// Zrzut historii przy rejestracji ma te same porazki co zapis w slocie: Errc::IO z errno,
// a zadanie nie trafia do ksiegi.
TEST_F(DumpManagerModelTest, registerTask_history_dump_reports_seek_and_write_failures) {
  const wrapReset reset;
  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  writeRecord(1);
  writeRecord(2);

  wrapEnabled            = true;
  writeErrno             = EFBIG;
  const auto writeFailed = manager.registerTask("result", dumpTask("w", {-2, 1}, 0));
  ASSERT_RDB_ERROR(writeFailed, rdb::Errc::IO, "failed during history dump");
  EXPECT_EQ(writeFailed.error().err(), EFBIG);
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());

  writeErrno            = 0;
  lseekErrno            = EBADF;
  const auto seekFailed = manager.registerTask("result", dumpTask("s", {-1, 1}, 0));
  ASSERT_RDB_ERROR(seekFailed, rdb::Errc::IO, "lseek on");
  EXPECT_EQ(seekFailed.error().err(), EBADF);
  EXPECT_TRUE(manager.bookOfTasks.at("result").empty());
}
