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

  rdb::saveDescriptorFile(file, saved);
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
  rdb::storage storage("dio_storage", "dio_storage", "", "DEFAULT", false, false, -1);
  const std::string error = storage.attachDescriptor(&planned);
  EXPECT_TRUE(error.contains("Fail: line 2:9")) << error;

  std::filesystem::remove(file);
}

// ---------------------------------------------------------------------------
// Weryfikacja zgodności: identyczne deskryptory przechodzą bez efektów
// ubocznych (niezgodność kończy proces przez FatalError - poza zasięgiem
// testu jednostkowego, tak jak pozostałe ścieżki FatalError w repo).
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, verify_match_accepts_equal_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = sampleDescriptor();
  rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc");  // brak FatalError == sukces
}
