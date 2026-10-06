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
#include <vector>

#define private public
#include "retractor/lib/dumpManager.hpp"
#undef private

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"
#include "syscallWrap.hpp"

extern "C" off_t __real_lseek(int fd, off_t offset, int whence);
extern "C" ssize_t __real_write(int fd, const void *buf, size_t count);

namespace {
bool wrapEnabled      = false;
int wrappedLseekCalls = 0;
int wrappedWriteCalls = 0;
int wrappedLastFd     = -1;
}  // namespace

extern "C" off_t __wrap_lseek(int fd, off_t offset, int whence) {
  if (!wrapEnabled) return __real_lseek(fd, offset, whence);
  (void)offset;
  (void)whence;
  wrappedLseekCalls++;
  wrappedLastFd = fd;
  return 0;
}

extern "C" ssize_t __wrap_write(int fd, const void *buf, size_t count) {
  if (!wrapEnabled) return __real_write(fd, buf, count);
  (void)buf;
  wrappedWriteCalls++;
  wrappedLastFd = fd;
  return static_cast<ssize_t>(count);
}

// Spiecie __wrap_/__real_ z prawdziwym wywolaniem systemowym - patrz syscallWrap.hpp.
RDB_WRAP_SYSCALL(off_t, lseek, (int fd, off_t offset, int whence), (fd, offset, whence));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *buf, size_t count), (fd, buf, count));

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

  const bool completed = manager.buildDumpChunk(task, payload.get());

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

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "file descriptor is not set");
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

  const bool result = manager.buildDumpChunk(task, payload.get());

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
  bool r1 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.delayDumpRecordsToGo, 0);

  // Drugie wywołanie: opóźnienie wyczerpane, zapis
  wrapEnabled       = true;
  wrappedLseekCalls = 0;
  wrappedWriteCalls = 0;

  bool r2 = manager.buildDumpChunk(task, payload.get());

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

  bool r1 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r1);
  EXPECT_EQ(task.dumpedRecordsToGo, 2);

  bool r2 = manager.buildDumpChunk(task, payload.get());
  EXPECT_FALSE(r2);
  EXPECT_EQ(task.dumpedRecordsToGo, 1);

  bool r3 = manager.buildDumpChunk(task, payload.get());
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

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "dumpedRecordsToGo is negative");
}

TEST(dumpManager, buildDumpChunk_negative_delayDumpRecordsToGo_fatals) {
  dumpManager manager;

  auto descriptor = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  auto payload    = std::make_unique<rdb::payload>(descriptor);

  dumpTask task("task", {0, 1}, 0);
  task.dumpedRecordsToGo    = 1;
  task.delayDumpRecordsToGo = -1;
  task.fd                   = 0;

  EXPECT_DEATH({ (void)manager.buildDumpChunk(task, payload.get()); }, "delayDumpRecordsToGo is negative");
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

  auto [filename, fd] = manager.createDumpFile("stream", "t");
  ASSERT_GE(fd, 0);
  ::close(fd);

  EXPECT_TRUE(std::filesystem::exists(filename));
  EXPECT_TRUE(std::string_view(filename).ends_with("_dump.tmp"));
}

TEST_F(DumpManagerFileTest, createDumpFile_with_retention_creates_numbered_files) {
  dumpManager manager;
  manager.storagePath                  = sandBoxFolder.string();
  manager.retentionSize["stream_task"] = 3;

  auto [f0, fd0] = manager.createDumpFile("stream", "task");
  ASSERT_GE(fd0, 0);
  ::close(fd0);

  auto [f1, fd1] = manager.createDumpFile("stream", "task");
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
    static_cast<void>(output.write());
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
    model = std::make_unique<dataModel>(plan);
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
    static_cast<void>(output.write());
    manager.registerTask("result", dumpTask("task", {0, 1}, retention));

    const auto suffix   = retention == 0 ? "_dump.tmp" : "_dump_" + std::to_string(trigger % retention) + ".tmp";
    const auto filename = sandBoxFolder / ("result_task" + suffix);
    EXPECT_EQ(manager.bookOfTasks.at("result").back().dumpFilename, filename.string());
    manager.processStreamChunk("result");
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
  manager.registerTask("result", dumpTask("latest", {first, first + 3}, 0));
  manager.registerTask("result", dumpTask("kept", {0, 7}, 100));
  const int keptFd = taskOf(manager, dumpPath("result_kept_dump_0.tmp")).fd;
  manager.processStreamChunk("result");

  std::list<dumpObserver> observers;
  for (int trigger = 4; trigger <= 5; ++trigger) {
    const int oldFd = taskOf(manager, dumpPath("result_latest_dump.tmp")).fd;
    ASSERT_GE(observers.emplace_back(dumpPath("result_latest_dump.tmp")).sizeAtOpen, 0);

    writeRecord(trigger);
    if (trigger == 4) manager.registerTask("result", dumpTask("kept", {0, 7}, 100));
    manager.registerTask("result", dumpTask("latest", {first, first + 3}, 0));
    expectClosed(oldFd);
    EXPECT_NE(::fcntl(keptFd, F_GETFD), -1);
    EXPECT_EQ(manager.bookOfTasks.at("result").size(), 3U);
    manager.processStreamChunk("result");
  }

  for (int value = 6; value <= 10; ++value) {
    writeRecord(value);
    manager.processStreamChunk("result");
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
  manager.registerTask("result", dumpTask("kept", {0, 9}, 100));
  manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2));
  manager.processStreamChunk("result");
  writeRecord(4);
  manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2));
  manager.processStreamChunk("result");

  const int wrappedFd = taskOf(manager, dumpPath("result_ring_dump_0.tmp")).fd;
  const int slot1Fd   = taskOf(manager, dumpPath("result_ring_dump_1.tmp")).fd;
  const int keptFd    = taskOf(manager, dumpPath("result_kept_dump_0.tmp")).fd;
  const dumpObserver observer(dumpPath("result_ring_dump_0.tmp"));
  ASSERT_GE(observer.sizeAtOpen, 0);
  writeRecord(5);
  manager.registerTask("result", dumpTask("ring", {first, first + 5}, 2));
  expectClosed(wrappedFd);
  EXPECT_NE(::fcntl(slot1Fd, F_GETFD), -1);
  EXPECT_NE(::fcntl(keptFd, F_GETFD), -1);
  EXPECT_EQ(manager.bookOfTasks.at("result").size(), 3U);
  manager.processStreamChunk("result");

  for (int value = 6; value <= 11; ++value) {
    writeRecord(value);
    manager.processStreamChunk("result");
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
  manager.registerTask("result", dumpTask("a", {first, first + 3}, 0));
  const int aFd = taskOf(manager, dumpPath("result_a_dump.tmp")).fd;
  manager.processStreamChunk("result");
  writeRecord(4);
  manager.registerTask("result", dumpTask("b", {first, first + 3}, 0));
  EXPECT_NE(::fcntl(aFd, F_GETFD), -1);
  EXPECT_EQ(manager.bookOfTasks.at("result").size(), 2U);
  manager.processStreamChunk("result");

  for (int value = 5; value <= 8; ++value) {
    writeRecord(value);
    manager.processStreamChunk("result");
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
  dataModel model(plan);
  pProc = &model;

  dumpManager manager;
  manager.setDumpStorage(sandBoxFolder.string());
  manager.registerTask("a", dumpTask("bc", {0, 1}, 3));
  manager.registerTask("ab", dumpTask("c", {0, 1}, 2));
  manager.registerTask("a", dumpTask("bc", {0, 1}, 3));
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
    model = std::make_unique<dataModel>(plan);
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
    static_cast<void>(output.write());
    ASSERT_EQ(output.getRecordsCount(), static_cast<size_t>(value));
    runtime.constructRulesAndUpdate(plan.getQuery("result"));
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
