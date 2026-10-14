// Scenariusze użycia funkcji descriptorIO
//
// descriptorIO to wydzielona z klasy storage persystencja pliku deskryptora .desc:
// zapis, odczyt (błąd wraca jako komunikat, bez kończenia procesu) i weryfikacja zgodności
// deskryptora dostarczonego z już zapisanym.

#include <gtest/gtest.h>

#include <cerrno>
#include <filesystem>
#include <fstream>

#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/storage.hpp"
#include "rdbResult.hpp"

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
  const std::string file{"dio_roundtrip.desc"};
  const auto saved = sampleDescriptor();

  ASSERT_RDB_OK(rdb::saveDescriptorFile(file, saved));
  ASSERT_TRUE(std::filesystem::exists(file));

  rdb::Descriptor loaded;
  ASSERT_EQ(rdb::tryLoadDescriptorFile(file, loaded).message, "");
  EXPECT_TRUE(loaded == saved);
  EXPECT_EQ(loaded.getSizeInBytes(), saved.getSizeInBytes());

  const auto loadedResult = rdb::loadDescriptorFile(file);
  ASSERT_RDB_OK(loadedResult);
  EXPECT_TRUE(*loadedResult == saved);

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
  const auto loadedResult = rdb::loadDescriptorFile(file);
  ASSERT_RDB_ERROR(loadedResult, rdb::Errc::CorruptDescriptor, "Fail: line 2:9");
  EXPECT_EQ(loadedResult.error().message(), error.message);
  EXPECT_EQ(loadedResult.error().err(), error.err);
  EXPECT_TRUE(descriptor.empty());

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, storage_returns_a_bad_descriptor_error) {
  const std::string file{"dio_storage.desc"};
  std::ofstream(file) << "{\n INTEGER }\n";

  const rdb::Descriptor planned = sampleDescriptor();
  auto storageOwner     = rdbtest::ok(rdb::storage::create("dio_storage", "dio_storage", "", "DEFAULT", false, false, -1));
  rdb::storage &storage = *storageOwner;
  EXPECT_RDB_ERROR(storage.attachDescriptor(&planned), rdb::Errc::CorruptDescriptor, "Fail: line 2:9");

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, io_refusal_preserves_the_error_class_and_errno) {
  const std::filesystem::path file = "dio_missing.desc";
  const std::filesystem::path link = "dio_link.desc";
  std::filesystem::remove(file);
  std::filesystem::remove(link);
  rdb::Descriptor descriptor;
  const auto missing = rdb::tryLoadDescriptorFile(file.string(), descriptor);
  EXPECT_EQ(missing.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(missing.err, ENOENT);
  EXPECT_FALSE(missing.message.empty());
  const auto missingResult = rdb::loadDescriptorFile(file.string());
  ASSERT_RDB_ERROR(missingResult, rdb::Errc::IO, "cannot open descriptor file");
  EXPECT_EQ(missingResult.error().err(), ENOENT);
  EXPECT_EQ(missingResult.error().message(), missing.message);
  ASSERT_RDB_OK(rdb::saveDescriptorFile(file.string(), sampleDescriptor()));
  std::filesystem::create_symlink(file, link);
  const auto linked = rdb::tryLoadDescriptorFile(link.string(), descriptor);
  EXPECT_EQ(linked.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(linked.err, ELOOP);
  EXPECT_TRUE(descriptor.empty());
  const auto linkedResult = rdb::loadDescriptorFile(link.string());
  ASSERT_RDB_ERROR(linkedResult, rdb::Errc::IO, "cannot open descriptor file");
  EXPECT_EQ(linkedResult.error().err(), ELOOP);
  EXPECT_EQ(linkedResult.error().message(), linked.message);
  std::filesystem::remove(link);
  std::filesystem::remove(file);
}

// ---------------------------------------------------------------------------
// Weryfikacja zgodności: identyczne deskryptory przechodzą, niezgodne zwracają
// błąd Errc::Logic po kontroli wstępnej - bez końca procesu i bez wyjątku.
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, verify_match_accepts_equal_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = sampleDescriptor();
  EXPECT_RDB_OK(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"));
}

TEST(DescriptorIOTest, verify_match_rejects_different_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  EXPECT_RDB_ERROR(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"), rdb::Errc::Logic, "descriptor schema mismatch");
}
