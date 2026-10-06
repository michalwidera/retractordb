#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
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
  // retentionSize["streamt"] = 0 (domyślna wartość mapy)

  auto [filename, fd] = manager.createDumpFile("stream", "t");
  ASSERT_GE(fd, 0);
  ::close(fd);

  EXPECT_TRUE(std::filesystem::exists(filename));
  EXPECT_TRUE(std::string_view(filename).ends_with("_dump.tmp"));
}

TEST_F(DumpManagerFileTest, createDumpFile_with_retention_creates_numbered_files) {
  dumpManager manager;
  manager.storagePath                 = sandBoxFolder.string();
  manager.retentionSize["streamtask"] = 3;

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

class DumpManagerTaskTest : public DumpManagerFileTest, public ::testing::WithParamInterface<size_t> {
 protected:
  qTree plan;
  std::unique_ptr<dataModel> model;

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
