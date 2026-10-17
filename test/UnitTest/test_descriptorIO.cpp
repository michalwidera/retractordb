// Scenariusze użycia funkcji descriptorIO
//
// descriptorIO to wydzielona z klasy storage persystencja pliku deskryptora .desc:
// zapis, odczyt (błąd wraca jako komunikat, bez kończenia procesu) i weryfikacja zgodności
// deskryptora dostarczonego z już zapisanym.

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <gtest/gtest.h>

#include "logCapture.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/storage.hpp"
#include "rdb/storageFile.hpp"
#include "rdbResult.hpp"
#include "syscallWrap.hpp"

namespace {
enum class IoFault {
  none,
  size,
  read,
  write,
  zeroWrite,
  interruptedRead,
  partialRead,
  interruptedWrite,
  partialWrite,
  interruptedWriteAt,
  partialWriteAt,
  zeroWriteAt
};
IoFault ioFault = IoFault::none;
}  // namespace

extern "C" int __real_fstat(int, struct stat *);
extern "C" ssize_t __real_pread(int, void *, size_t, off_t);
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" ssize_t __real_pwrite(int, const void *, size_t, off_t);

extern "C" int __wrap_fstat(int fd, struct stat *st) {
  if (ioFault == IoFault::size) {
    ioFault = IoFault::none;
    errno   = EIO;
    return -1;
  }
  return __real_fstat(fd, st);
}
extern "C" ssize_t __wrap_pread(int fd, void *buffer, size_t size, off_t offset) {
  if (ioFault == IoFault::interruptedRead) {
    ioFault = IoFault::none;
    errno   = EINTR;
    return -1;
  }
  if (ioFault == IoFault::partialRead) {
    ioFault = IoFault::none;
    return __real_pread(fd, buffer, 1, offset);
  }
  if (ioFault == IoFault::read) {
    ioFault = IoFault::none;
    errno   = EIO;
    return -1;
  }
  return __real_pread(fd, buffer, size, offset);
}
extern "C" ssize_t __wrap_write(int fd, const void *buffer, size_t size) {
  if (ioFault == IoFault::interruptedWrite) {
    ioFault = IoFault::none;
    errno   = EINTR;
    return -1;
  }
  if (ioFault == IoFault::partialWrite) {
    ioFault = IoFault::none;
    return __real_write(fd, buffer, 1);
  }
  if (ioFault == IoFault::write || ioFault == IoFault::zeroWrite) {
    const auto fault = std::exchange(ioFault, IoFault::none);
    errno            = fault == IoFault::write ? ENOSPC : 0;
    return fault == IoFault::write ? -1 : 0;
  }
  return __real_write(fd, buffer, size);
}
extern "C" ssize_t __wrap_pwrite(int fd, const void *buffer, size_t size, off_t offset) {
  if (ioFault == IoFault::interruptedWriteAt || ioFault == IoFault::zeroWriteAt) {
    const auto fault = std::exchange(ioFault, IoFault::none);
    errno            = fault == IoFault::interruptedWriteAt ? EINTR : 0;
    return fault == IoFault::interruptedWriteAt ? -1 : 0;
  }
  if (ioFault == IoFault::partialWriteAt) {
    ioFault = IoFault::none;
    return __real_pwrite(fd, buffer, 1, offset);
  }
  return __real_pwrite(fd, buffer, size, offset);
}
RDB_WRAP_SYSCALL(int, fstat, (int fd, struct stat *st), (fd, st));
RDB_WRAP_SYSCALL(ssize_t, pread, (int fd, void *buffer, size_t size, off_t offset), (fd, buffer, size, offset));
RDB_WRAP_SYSCALL(ssize_t, write, (int fd, const void *buffer, size_t size), (fd, buffer, size));
RDB_WRAP_SYSCALL(ssize_t, pwrite, (int fd, const void *buffer, size_t size, off_t offset), (fd, buffer, size, offset));

namespace {
rdb::Descriptor sampleDescriptor() {
  return rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER) +  //
         rdb::Descriptor("b", sizeof(double), 1, rdb::DOUBLE);
}
}  // namespace

// ---------------------------------------------------------------------------
// Zapis i odczyt: deskryptor odczytany z pliku jest identyczny z zapisanym.
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, save_and_load_round_trip) {
  // covers: storage.storage.4~1
  const std::string file{"dio_roundtrip.desc"};
  const auto saved = sampleDescriptor();

  ASSERT_RDB_OK(rdb::saveDescriptorFile(file, saved));
  ASSERT_TRUE(std::filesystem::exists(file));

  rdb::Descriptor loaded;
  ASSERT_EQ(rdb::tryLoadDescriptorFile(file, loaded).message, "");
  EXPECT_TRUE(loaded == saved);
  EXPECT_EQ(loaded.getSizeInBytes(), saved.getSizeInBytes());

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, bad_syntax_returns_an_error_without_ending_the_process) {
  const std::string file{"dio_bad.desc"};
  std::ofstream(file) << "{\n INTEGER }\n";

  rdb::Descriptor descriptor;
  const auto error = rdb::tryLoadDescriptorFile(file, descriptor);
  EXPECT_EQ(error.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(error.err, 0);
  EXPECT_TRUE(error.message.contains("Fail: line 2:9")) << error.message;
  EXPECT_TRUE(descriptor.empty());

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, storage_returns_a_bad_descriptor_error) {
  // covers: storage.storage.4~1
  const std::string file{"dio_storage.desc"};
  std::ofstream(file) << "{\n INTEGER }\n";

  const rdb::Descriptor planned = sampleDescriptor();
  auto storageOwner     = rdbtest::ok(rdb::storage::create("dio_storage", "dio_storage", "", "DEFAULT", false, false, -1));
  rdb::storage &storage = *storageOwner;
  EXPECT_RDB_ERROR(storage.attachDescriptor(&planned), rdb::Errc::CorruptDescriptor, "Fail: line 2:9");

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, io_refusal_preserves_the_error_class_and_errno) {
  // covers: descriptorio.try-load-descriptor-file.1~1, descriptorio.try-load-descriptor-file.2~1
  const std::filesystem::path file = "dio_missing.desc";
  const std::filesystem::path link = "dio_link.desc";
  std::filesystem::remove(file);
  std::filesystem::remove(link);
  rdb::Descriptor descriptor;
  const auto missing = rdb::tryLoadDescriptorFile(file.string(), descriptor);
  EXPECT_EQ(missing.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(missing.err, ENOENT);
  EXPECT_FALSE(missing.message.empty());
  ASSERT_RDB_OK(rdb::saveDescriptorFile(file.string(), sampleDescriptor()));
  std::filesystem::create_symlink(file, link);
  const auto linked = rdb::tryLoadDescriptorFile(link.string(), descriptor);
  EXPECT_EQ(linked.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(linked.err, ELOOP);
  EXPECT_TRUE(descriptor.empty());
  std::filesystem::remove(link);
  std::filesystem::remove(file);
}

// ---------------------------------------------------------------------------
// Weryfikacja zgodności: identyczne deskryptory przechodzą, niezgodne zwracają
// błąd Errc::Logic po kontroli wstępnej - bez końca procesu i bez wyjątku.
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, verify_match_accepts_equal_descriptors) {
  // covers: descriptorio.verify-descriptor-match.1~1, storage.storage.4~1
  const auto lhs = sampleDescriptor();
  const auto rhs = sampleDescriptor();
  EXPECT_RDB_OK(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"));
}

TEST(DescriptorIOTest, verify_match_rejects_different_descriptors) {
  // covers: storage.storage.4~1
  const auto lhs = sampleDescriptor();
  const auto rhs = rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  EXPECT_RDB_ERROR(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"), rdb::Errc::Logic, "descriptor schema mismatch");
}

TEST(DescriptorIOTest, load_replaces_an_existing_descriptor_and_only_reads_the_named_file) {
  // covers: descriptorio.descriptor-io.4~1, descriptorio.try-load-descriptor-file.1~1
  const std::string file = "dio_replace.desc";
  const auto expected    = sampleDescriptor();
  ASSERT_RDB_OK(rdb::saveDescriptorFile(file, expected));
  auto descriptor   = rdb::Descriptor("old", 1, 20, rdb::STRING);
  const auto result = rdb::tryLoadDescriptorFile(file, descriptor);
  EXPECT_TRUE(result.message.empty());
  EXPECT_EQ(result.err, 0);
  EXPECT_TRUE(descriptor == expected);
  EXPECT_FALSE(std::filesystem::exists("dio_replace.meta"));
  EXPECT_FALSE(std::filesystem::exists("dio_replace"));
  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, all_load_refusals_preserve_a_nonempty_descriptor) {
  // covers: descriptorio.descriptor-io.1~1, descriptorio.try-load-descriptor-file.3~1, descriptorio.try-load-descriptor-file.4~1
  const std::string file = "dio_preserved.desc";
  const auto original    = sampleDescriptor();
  for (const std::string content : {"{\n INTEGER }\n", "{}", "{ INTEGER a[0] }"}) {
    SCOPED_TRACE(content);
    std::ofstream(file) << content;
    auto descriptor    = original;
    const auto refusal = rdb::tryLoadDescriptorFile(file, descriptor);
    EXPECT_EQ(refusal.cls, rdb::ErrorClass::Plan);
    EXPECT_EQ(refusal.err, 0);
    EXPECT_TRUE(refusal.message.contains(file));
    EXPECT_TRUE(descriptor == original);
  }
  std::filesystem::remove(file);
  auto descriptor    = original;
  const auto missing = rdb::tryLoadDescriptorFile(file, descriptor);
  EXPECT_EQ(missing.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(missing.err, ENOENT);
  EXPECT_TRUE(missing.message.contains(file));
  EXPECT_TRUE(descriptor == original);
}

TEST(DescriptorIOTest, size_and_read_failures_preserve_descriptor_errno_and_path) {
  // covers: descriptorio.try-load-descriptor-file.2~1, descriptorio.try-load-descriptor-file.4~1
  const std::string file = "dio_injected.desc";
  const auto original    = sampleDescriptor();
  ASSERT_RDB_OK(rdb::saveDescriptorFile(file, original));
  for (const auto fault : {IoFault::size, IoFault::read}) {
    auto descriptor    = original;
    ioFault            = fault;
    const auto refusal = rdb::tryLoadDescriptorFile(file, descriptor);
    ioFault            = IoFault::none;
    EXPECT_EQ(refusal.cls, rdb::ErrorClass::Environment);
    EXPECT_EQ(refusal.err, EIO);
    EXPECT_TRUE(refusal.message.contains(file));
    EXPECT_TRUE(descriptor == original);
  }
  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, save_truncates_the_file_and_does_not_follow_a_symlink) {
  // covers: descriptorio.save-descriptor-file.1~1
  const std::string file = "dio_truncated.desc";
  const std::string link = "dio_write_link.desc";
  const auto descriptor  = sampleDescriptor();
  std::ostringstream expected;
  expected << descriptor;
  std::ofstream(file) << std::string(expected.str().size() + 100, 'x');
  ASSERT_RDB_OK(rdb::saveDescriptorFile(file, descriptor));
  EXPECT_EQ(std::filesystem::file_size(file), expected.str().size());
  std::filesystem::create_symlink(file, link);
  const auto refused = rdb::saveDescriptorFile(link, rdb::Descriptor("other", 4, 1, rdb::INTEGER));
  ASSERT_RDB_ERROR(refused, rdb::Errc::IO);
  EXPECT_EQ(refused.error().err(), ELOOP);
  std::ifstream input(file);
  const std::string actual{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  EXPECT_EQ(actual, expected.str());
  std::filesystem::remove(link);
  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, save_reports_open_write_and_zero_write_failures) {
  // covers: descriptorio.descriptor-io.2~1, descriptorio.save-descriptor-file.2~1
  const std::string file = "dio_save_failure.desc";
  const auto descriptor  = sampleDescriptor();
  const auto opened      = rdb::saveDescriptorFile("dio_absent_directory/entry.desc", descriptor);
  ASSERT_RDB_ERROR(opened, rdb::Errc::IO);
  EXPECT_EQ(opened.error().err(), ENOENT);
  for (const auto fault : {IoFault::write, IoFault::zeroWrite}) {
    ioFault          = fault;
    const auto saved = rdb::saveDescriptorFile(file, descriptor);
    ioFault          = IoFault::none;
    ASSERT_RDB_ERROR(saved, rdb::Errc::IO);
    EXPECT_EQ(saved.error().err(), fault == IoFault::write ? ENOSPC : EIO);
    EXPECT_TRUE(saved.error().message().contains(file));
  }
  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, mismatch_diagnostics_include_path_and_both_descriptors) {
  // covers: descriptorio.descriptor-io.3~1, descriptorio.verify-descriptor-match.2~1
  const auto provided = sampleDescriptor();
  const auto existing = rdb::Descriptor("other", 4, 1, rdb::INTEGER);
  std::ostringstream providedText, existingText;
  providedText << provided;
  existingText << existing;
  LogCapture log;
  testing::internal::CaptureStderr();
  const auto result     = rdb::verifyDescriptorMatch(provided, existing, "dio_diagnostic.desc");
  const auto diagnostic = testing::internal::GetCapturedStderr();
  EXPECT_RDB_ERROR(result, rdb::Errc::Logic);
  EXPECT_TRUE(diagnostic.contains("dio_diagnostic.desc"));
  EXPECT_TRUE(diagnostic.contains(providedText.str()));
  EXPECT_TRUE(diagnostic.contains(existingText.str()));
  EXPECT_TRUE(log.text().contains("Descriptors do not match"));
}

TEST(DescriptorIOTest, storage_fd_retries_partial_and_interrupted_io) {
  // covers: storagefile.storage-fd.3~1, storagefile.storage-fd.4~1, storagefile.storage-fd.5~1, storagefile.storage-fd.6~1
  const std::string file = "dio_fd_retry.desc";
  const std::vector<std::byte> bytes{std::byte{1}, std::byte{2}, std::byte{3}};
  for (const auto fault : {IoFault::interruptedWrite, IoFault::partialWrite}) {
    rdb::StorageFd fd(file, O_RDWR | O_CREAT | O_TRUNC);
    ASSERT_TRUE(fd.isOpen());
    ioFault            = fault;
    const bool written = fd.write(bytes);
    ioFault            = IoFault::none;
    ASSERT_TRUE(written);
    EXPECT_EQ(fd.size(), 3);
    for (const auto readFault : {IoFault::interruptedRead, IoFault::partialRead}) {
      std::vector<std::byte> read(3);
      ioFault         = readFault;
      const auto size = fd.readAt(read, 0);
      ioFault         = IoFault::none;
      EXPECT_EQ(size, 3);
      EXPECT_EQ(read, bytes);
    }
    for (const auto writeFault : {IoFault::interruptedWriteAt, IoFault::partialWriteAt}) {
      ioFault               = writeFault;
      const bool positioned = fd.writeAt(bytes, 0);
      ioFault               = IoFault::none;
      EXPECT_TRUE(positioned);
    }
    // pwrite nie przesuwa pozycji: nastepny write ma dopisac za pierwszym rekordem.
    ASSERT_TRUE(fd.write(bytes));
    EXPECT_EQ(fd.size(), 6);
    ioFault         = IoFault::zeroWriteAt;
    const bool zero = fd.writeAt(bytes, 0);
    ioFault         = IoFault::none;
    EXPECT_FALSE(zero);
    ioFault = IoFault::size;
    EXPECT_EQ(fd.size(), -1);
    ioFault = IoFault::none;
  }
  std::filesystem::remove(file);
}
