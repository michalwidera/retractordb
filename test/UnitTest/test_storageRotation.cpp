#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "logCapture.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/faccposixshd.hpp"
#include "rdb/metaData.hpp"
#include "rdb/metaShadow.hpp"
#include "rdb/storageRotation.hpp"
#include "rdb/storageShadow.hpp"
#include "syscallWrap.hpp"

namespace {

enum class Failure { None, Rename, Open, Sync, Close };
bool recording  = false;
Failure failure = Failure::None;
std::string failurePath;
std::map<int, std::string> directories;
std::vector<std::pair<std::string, std::string>> calls;

bool fails(Failure operation, const std::string &path) {
  if (!recording || failure != operation || (!failurePath.empty() && failurePath != path)) return false;
  errno = EIO;
  return true;
}

}  // namespace

extern "C" {
int __real_rename(const char *source, const char *archive);
int __real_open(const char *path, int flags, ...);
int __real_fsync(int fd);
int __real_close(int fd);

int __wrap_rename(const char *source, const char *archive) {
  if (recording) calls.emplace_back("rename", source);
  if (fails(Failure::Rename, source)) return -1;
  return __real_rename(source, archive);
}

int __wrap_open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  if (recording && (flags & O_DIRECTORY)) {
    calls.emplace_back("open", path);
    if (fails(Failure::Open, path)) return -1;
  }
  const int fd = __real_open(path, flags, mode);
  if (recording && (flags & O_DIRECTORY) && fd >= 0) directories.emplace(fd, path);
  return fd;
}

int __wrap_fsync(int fd) {
  const auto it = directories.find(fd);
  if (recording && it != directories.end()) {
    calls.emplace_back("fsync", it->second);
    if (fails(Failure::Sync, it->second)) return -1;
  }
  return __real_fsync(fd);
}

int __wrap_close(int fd) {
  const auto it = directories.find(fd);
  bool inject   = false;
  if (recording && it != directories.end()) {
    calls.emplace_back("close", it->second);
    inject = fails(Failure::Close, it->second);
    directories.erase(it);
  }
  // Symulujemy blad close po zwolnieniu deskryptora, bez wycieku zasobu testu.
  const int result = __real_close(fd);
  if (inject) {
    errno = EIO;
    return -1;
  }
  return result;
}
}

RDB_WRAP_SYSCALL(int, rename, (const char *source, const char *archive), (source, archive));
RDB_WRAP_SYSCALL(int, fsync, (int fd), (fd));
RDB_WRAP_SYSCALL(int, close, (int fd), (fd));

#if !RDB_HAS_LD_WRAP
// open jest wariadyczne: przekazujemy tryb O_CREAT, takze dla zapisow metadanych.
extern "C" int __real_open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  using Open       = int (*)(const char *, int, ...);
  static Open next = nullptr;
  if (next == nullptr) next = reinterpret_cast<Open>(::dlsym(RTLD_NEXT, "open"));
  return next(path, flags, mode);
}
extern "C" int open(const char *path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list arguments;
    va_start(arguments, flags);
    mode = static_cast<mode_t>(va_arg(arguments, int));
    va_end(arguments);
  }
  return __wrap_open(path, flags, mode);
}
#endif

namespace {

class StorageRotationTest : public ::testing::Test {
 protected:
  std::filesystem::path root;
  std::filesystem::path originalDirectory;
  rdb::Descriptor descriptor;

  void SetUp() override {
    recording = false;
    failure   = Failure::None;
    failurePath.clear();
    calls.clear();
    directories.clear();
    originalDirectory = std::filesystem::current_path();
    root              = std::filesystem::temp_directory_path() / ("test_storageRotation_" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "source");
    std::filesystem::create_directories(root / "archive");
    descriptor.append({{"v", 4, 1, rdb::INTEGER}});
  }

  void TearDown() override {
    recording = false;
    EXPECT_TRUE(directories.empty());
    std::filesystem::current_path(originalDirectory);
    std::filesystem::remove_all(root);
  }

  std::string source() const { return (root / "source" / "stream").string(); }

  void expectError(const LogCapture &log, const std::string &path, const std::string &operation) {
    const auto text = log.text();
    EXPECT_NE(text.find("[error]"), std::string::npos) << text;
    EXPECT_NE(text.find(path), std::string::npos) << text;
    EXPECT_NE(text.find(operation), std::string::npos) << text;
    EXPECT_NE(text.find(std::strerror(EIO)), std::string::npos) << text;
  }

  void writeFile(const std::string &path) { std::ofstream(path) << "archive contents"; }

  std::string readFile(const std::string &path) {
    std::ifstream file(path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  }
};

TEST_F(StorageRotationTest, same_directory_is_synced_once_after_rename) {
  const auto path = source();
  writeFile(path);
  recording = true;
  ASSERT_TRUE(rdb::rotateStorageFile(path, path + ".old7"));
  EXPECT_EQ(calls, (std::vector<std::pair<std::string, std::string>>{{"rename", path},
                                                                     {"open", (root / "source").string()},
                                                                     {"fsync", (root / "source").string()},
                                                                     {"close", (root / "source").string()}}));
  EXPECT_FALSE(std::filesystem::exists(path));
  EXPECT_EQ(readFile(path + ".old7"), "archive contents");
}

TEST_F(StorageRotationTest, relative_filename_syncs_current_directory) {
  std::filesystem::current_path(root);
  writeFile("stream");
  recording = true;
  ASSERT_TRUE(rdb::rotateStorageFile("stream", "stream.old7"));
  EXPECT_EQ(calls, (std::vector<std::pair<std::string, std::string>>{
                       {"rename", "stream"}, {"open", "."}, {"fsync", "."}, {"close", "."}}));
}

TEST_F(StorageRotationTest, different_directories_are_both_synced_even_if_first_fails) {
  for (const auto injected : {Failure::None, Failure::Open, Failure::Sync, Failure::Close}) {
    SCOPED_TRACE(static_cast<int>(injected));
    const auto path    = source();
    const auto archive = (root / "archive" / "stream.old7").string();
    writeFile(path);
    failure     = injected;
    failurePath = (root / "source").string();
    calls.clear();
    LogCapture log;
    spdlog::default_logger()->set_level(spdlog::level::err);
    recording = true;
    EXPECT_EQ(rdb::rotateStorageFile(path, archive), injected == Failure::None);
    EXPECT_NE(std::ranges::find(calls, std::pair{std::string("fsync"), (root / "archive").string()}), calls.end());
    EXPECT_TRUE(directories.empty());
    if (injected != Failure::None) EXPECT_NE(log.text().find(failurePath), std::string::npos);
    std::filesystem::remove(archive);
  }
}

TEST_F(StorageRotationTest, second_directory_failure_is_not_success) {
  const auto path    = source();
  const auto archive = (root / "archive" / "stream.old7").string();
  writeFile(path);
  failure     = Failure::Sync;
  failurePath = (root / "archive").string();
  LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::err);
  recording = true;
  EXPECT_FALSE(rdb::rotateStorageFile(path, archive));
  expectError(log, failurePath, "fsync directory");
  EXPECT_NE(std::ranges::find(calls, std::pair{std::string("fsync"), (root / "source").string()}), calls.end());
}

TEST_F(StorageRotationTest, failed_rename_keeps_source_and_never_syncs_directory) {
  const auto path = source();
  writeFile(path);
  failure = Failure::Rename;
  LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::err);
  recording = true;
  EXPECT_FALSE(rdb::rotateStorageFile(path, path + ".old7"));
  EXPECT_EQ(calls, (std::vector<std::pair<std::string, std::string>>{{"rename", path}}));
  EXPECT_EQ(readFile(path), "archive contents");
  EXPECT_FALSE(std::filesystem::exists(path + ".old7"));
  expectError(log, path + ".old7", "Failed to rotate");
}

TEST_F(StorageRotationTest, metadata_reopen_reports_failure_and_keeps_unarchived_index) {
  rdb::metaData meta(descriptor, source());
  meta.onRecordAppended({true});
  meta.flushCurrentEntry();
  const auto bytes = readFile(source());
  failure          = Failure::Rename;
  recording        = true;
  EXPECT_THROW(meta.rotate(7), std::runtime_error);
  EXPECT_EQ(readFile(source()), bytes);
  EXPECT_EQ(meta.totalRecords(), 1U);
  EXPECT_EQ(meta.getNullBitset(0), (std::vector<bool>{true}));
}

TEST_F(StorageRotationTest, metadata_sync_failure_never_recreates_active_file) {
  rdb::metaData meta(descriptor, source());
  meta.onRecordAppended({true});
  meta.flushCurrentEntry();
  const auto bytes = readFile(source());
  failure          = Failure::Sync;
  recording        = true;
  EXPECT_THROW(meta.rotate(7), std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(source()));
  EXPECT_EQ(readFile(source() + ".old7"), bytes);
}

TEST_F(StorageRotationTest, shadow_failure_does_not_discard_unarchived_overrides) {
  rdb::storageShadow meta(descriptor, source());
  meta.onRecordAppended({false});
  meta.onRecordModified(0, {true});
  const auto shadow = source() + ".shadow";
  const auto bytes  = readFile(shadow);
  failure           = Failure::Rename;
  failurePath       = shadow;
  recording         = true;
  EXPECT_THROW(meta.rotate(7), std::runtime_error);
  EXPECT_EQ(readFile(shadow), bytes);
  EXPECT_EQ(meta.getNullBitset(0), (std::vector<bool>{true}));
  EXPECT_FALSE(std::filesystem::exists(shadow + ".old7"));
}

TEST_F(StorageRotationTest, inaccessible_metadata_path_is_logged_without_throwing_on_shutdown) {
  rdb::metaData meta(descriptor, source());
  meta.onRecordAppended({true});
  meta.flushCurrentEntry();
  const auto bytes = readFile(source());
  std::filesystem::rename(root / "source", root / "saved");
  std::filesystem::create_directory_symlink("source", root / "source");
  LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::err);
  EXPECT_NO_THROW(meta.rotate(7, false));
  EXPECT_EQ(readFile((root / "saved" / "stream").string()), bytes);
  EXPECT_NE(log.text().find("[error]"), std::string::npos);
  EXPECT_NE(log.text().find(source()), std::string::npos);
  EXPECT_NE(log.text().find(std::strerror(ELOOP)), std::string::npos);
}

TEST_F(StorageRotationTest, real_rename_failure_keeps_original_file) {
  const auto path    = source();
  const auto archive = path + ".old7";
  writeFile(path);
  std::filesystem::create_directory(archive);
  writeFile(archive + "/occupied");
  LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::err);
  recording = true;
  EXPECT_FALSE(rdb::rotateStorageFile(path, archive));
  EXPECT_EQ(readFile(path), "archive contents");
  EXPECT_EQ(calls, (std::vector<std::pair<std::string, std::string>>{{"rename", path}}));
  EXPECT_NE(log.text().find("[error]"), std::string::npos);
  EXPECT_NE(log.text().find(archive), std::string::npos);
}

// Kazda sciezka produkcyjna, takze oba przemianowania POSIXSHD, musi przejsc helper.
class RotationPathsTest : public StorageRotationTest, public ::testing::WithParamInterface<std::tuple<int, Failure>> {
 protected:
  std::string rotatedSource(int kind) const { return source() + (kind == 3 || kind == 5 || kind == 6 ? ".shadow" : ""); }

  void rotate(int kind) {
    const uint8_t data[4]{1, 2, 3, 4};
    if (kind <= 3) {
      std::unique_ptr<rdb::FileInterface> file;
      if (kind == 0) file = std::make_unique<rdb::genericBinaryFile>(source(), descriptor, 7);
      if (kind == 1) file = std::make_unique<rdb::posixBinaryFile>(source(), descriptor, 7);
      if (kind == 2 || kind == 3) file = std::make_unique<rdb::posixBinaryFileWithShadow>(source(), descriptor, 7);
      ASSERT_EQ(file->write(data), 0);
      file.reset();
    } else if (kind == 4) {
      rdb::metaData meta(descriptor, source());
      meta.onRecordAppended({true});
      EXPECT_NO_THROW(meta.rotate(7, false));
    } else if (kind == 5) {
      rdb::metaShadow meta(descriptor, source());
      meta.appendOverride(0, {true});
      EXPECT_EQ(meta.rotate(7), failure == Failure::None);
    } else {
      rdb::storageShadow meta(descriptor, source());
      meta.onRecordAppended({false});
      meta.onRecordModified(0, {true});
      EXPECT_NO_THROW(meta.rotate(7, false));
    }
  }
};

TEST_P(RotationPathsTest, reports_failure_at_error_level_and_syncs_successful_rename) {
  const auto [kind, injected] = GetParam();
  failure                     = injected;
  failurePath                 = injected == Failure::Rename ? rotatedSource(kind) : (root / "source").string();
  LogCapture log;
  spdlog::default_logger()->set_level(spdlog::level::err);
  recording = true;
  rotate(kind);
  const auto path = rotatedSource(kind);
  EXPECT_EQ(std::filesystem::exists(path + ".old7"), injected != Failure::Rename);
  EXPECT_EQ(std::filesystem::exists(path), injected == Failure::Rename);
  if (injected == Failure::None) {
    EXPECT_TRUE(log.text().empty()) << log.text();
    const auto renames = std::ranges::count_if(calls, [](const auto &call) { return call.first == "rename"; });
    const auto syncs   = std::ranges::count_if(calls, [](const auto &call) { return call.first == "fsync"; });
    EXPECT_EQ(renames, syncs);
    EXPECT_EQ(renames, kind == 2 || kind == 3 || kind == 6 ? 2 : 1);
  } else {
    const auto operation = injected == Failure::Rename ? "Failed to rotate"
                           : injected == Failure::Open ? "open directory"
                           : injected == Failure::Sync ? "fsync directory"
                                                       : "close directory";
    expectError(log, path + ".old7", operation);
  }
  // Cien nieprzeniesiony trzyma .meta przy sobie; cien przeniesiony nie moze zostawic .meta pod aktywna nazwa.
  if (kind == 6) {
    EXPECT_EQ(std::filesystem::exists(source() + ".old7"), injected != Failure::Rename);
    EXPECT_EQ(std::filesystem::exists(source()), injected == Failure::Rename);
  }
}

INSTANTIATE_TEST_SUITE_P(AllBackends, RotationPathsTest,
                         ::testing::Combine(::testing::Range(0, 7),
                                            ::testing::Values(Failure::None, Failure::Rename, Failure::Open, Failure::Sync,
                                                              Failure::Close)));

}  // namespace
