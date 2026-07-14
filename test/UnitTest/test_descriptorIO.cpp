// Scenariusze użycia funkcji descriptorIO
//
// descriptorIO to wydzielona z klasy storage persystencja pliku deskryptora .desc:
// zapis, odczyt (z pominięciem buforowania strumienia) i weryfikacja zgodności
// deskryptora dostarczonego z już zapisanym.

#include <gtest/gtest.h>

#include <filesystem>

#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"

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

  const auto loaded = rdb::loadDescriptorFile(file);
  EXPECT_TRUE(loaded == saved);
  EXPECT_EQ(loaded.getSizeInBytes(), saved.getSizeInBytes());

  std::filesystem::remove(file);
}

// ---------------------------------------------------------------------------
// Weryfikacja zgodności: identyczne deskryptory przechodzą bez efektów
// ubocznych (niezgodność kończy proces przez FatalError — poza zasięgiem
// testu jednostkowego, tak jak pozostałe ścieżki FatalError w repo).
// ---------------------------------------------------------------------------
TEST(DescriptorIOTest, verify_match_accepts_equal_descriptors) {
  const auto lhs = sampleDescriptor();
  const auto rhs = sampleDescriptor();
  rdb::verifyDescriptorMatch(lhs, rhs, "dio_verify.desc");  // brak FatalError == sukces
}
