// Scenariusze użycia funkcji descriptorIO
//
// descriptorIO to wydzielona z klasy storage persystencja pliku deskryptora .desc:
// zapis, odczyt (błąd wraca jako komunikat, bez kończenia procesu) i weryfikacja zgodności
// deskryptora dostarczonego z już zapisanym.

#include <gtest/gtest.h>

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
  ASSERT_EQ(rdb::tryLoadDescriptorFile(file, loaded), "");
  EXPECT_TRUE(loaded == saved);
  EXPECT_EQ(loaded.getSizeInBytes(), saved.getSizeInBytes());

  std::filesystem::remove(file);
}

TEST(DescriptorIOTest, bad_syntax_returns_an_error_without_ending_the_process) {
  const std::string file{"dio_bad.desc"};
  std::ofstream(file) << "{\n INTEGER }\n";

  rdb::Descriptor descriptor;
  const std::string error = rdb::tryLoadDescriptorFile(file, descriptor);
  EXPECT_TRUE(error.contains("Fail: line 2:9")) << error;
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

// ---------------------------------------------------------------------------
// Weryfikacja zgodności: identyczne deskryptory przechodzą, niezgodne zwracają
// błąd Errc::Config z nazwą pliku - bez końca procesu i bez wyjątku.
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, verify_match_accepts_equal_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = sampleDescriptor();
  EXPECT_RDB_OK(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"));
}

TEST(DescriptorIOTest, verify_match_rejects_different_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  EXPECT_RDB_ERROR(rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc"), rdb::Errc::Config, "dio_verify.desc");
}
