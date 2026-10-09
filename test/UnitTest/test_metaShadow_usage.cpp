// Scenariusze użycia klasy metaShadow
//
// metaShadow przechowuje nadpisania wzorca null (.meta.shadow) dla magazynów
// posiadających plik cienia danych (.shadow). Testy weryfikują bezpośrednio jej API:
// append/lookup, persystencję po przeładowaniu, kolejność nadpisań oraz discard().

#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaShadow.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Konfiguracja: każdy test dostaje świeży plik meta i descriptor 3-polowy,
// analogicznie do test_metaData_usage.cpp.
// ---------------------------------------------------------------------------

struct ShadowUsageFixture : public ::testing::Test {
  const std::string file       = "usage_shadow.meta";
  const std::string shadowFile = file + ".shadow";

  rdb::Descriptor descriptor;

  const std::vector<bool> allPresent = {false, false, false};
  const std::vector<bool> allNull    = {true, true, true};

  void SetUp() override {
    descriptor.append({{"val", 8, 0, rdb::DOUBLE}, {"flag", 4, 0, rdb::INTEGER}, {"temp", 8, 0, rdb::DOUBLE}});
  }
  void TearDown() override {
    std::remove(file.c_str());
    std::remove(shadowFile.c_str());
  }
};

// ---------------------------------------------------------------------------
// Scenariusz 1: Brak nadpisania → lookup() zwraca std::nullopt.
// ---------------------------------------------------------------------------
TEST_F(ShadowUsageFixture, scenariusz_brak_nadpisania) {
  rdb::metaShadow shadow(descriptor, file);
  EXPECT_FALSE(shadow.lookup(0).has_value());
  EXPECT_TRUE(shadow.overrides().empty());
}

// ---------------------------------------------------------------------------
// Scenariusz 2: Nadpisanie rekordu jest widoczne przez lookup() i tworzy plik cienia.
// ---------------------------------------------------------------------------
TEST_F(ShadowUsageFixture, scenariusz_nadpisanie_widoczne) {
  rdb::metaShadow shadow(descriptor, file);

  shadow.appendOverride(2, allPresent);

  auto ov = shadow.lookup(2);
  ASSERT_TRUE(ov.has_value());
  EXPECT_EQ(*ov, allPresent);
  EXPECT_FALSE(shadow.lookup(1).has_value());
  EXPECT_TRUE(std::filesystem::exists(shadowFile));
}

// ---------------------------------------------------------------------------
// Scenariusz 3: Wielokrotne nadpisania tej samej pozycji - ostatnie wygrywa.
// ---------------------------------------------------------------------------
TEST_F(ShadowUsageFixture, scenariusz_ostatnie_nadpisanie_wygrywa) {
  rdb::metaShadow shadow(descriptor, file);

  shadow.appendOverride(2, allNull);
  shadow.appendOverride(2, allPresent);

  auto ov = shadow.lookup(2);
  ASSERT_TRUE(ov.has_value());
  EXPECT_EQ(*ov, allPresent);
  EXPECT_EQ(shadow.overrides().size(), 2U);  // obie wersje zachowane w kolejności zapisu
}

// ---------------------------------------------------------------------------
// Scenariusz 4: Persystencja - nowa instancja odczytuje nadpisania z pliku cienia po load().
// ---------------------------------------------------------------------------
TEST_F(ShadowUsageFixture, scenariusz_persystencja_po_restarcie) {
  {
    rdb::metaShadow shadow(descriptor, file);
    shadow.appendOverride(0, allNull);
    shadow.appendOverride(2, allPresent);
  }

  rdb::metaShadow reopened(descriptor, file);
  EXPECT_FALSE(reopened.lookup(0).has_value());  // przed load() nic nie jest wczytane

  reopened.load();
  EXPECT_EQ(reopened.overrides().size(), 2U);
  ASSERT_TRUE(reopened.lookup(0).has_value());
  EXPECT_EQ(*reopened.lookup(0), allNull);
  ASSERT_TRUE(reopened.lookup(2).has_value());
  EXPECT_EQ(*reopened.lookup(2), allPresent);
}

// ---------------------------------------------------------------------------
// Scenariusz 5: discard() czyści pamięć i usuwa plik cienia.
// ---------------------------------------------------------------------------
TEST_F(ShadowUsageFixture, scenariusz_discard) {
  rdb::metaShadow shadow(descriptor, file);
  shadow.appendOverride(2, allPresent);
  ASSERT_TRUE(std::filesystem::exists(shadowFile));

  shadow.discard();

  EXPECT_FALSE(shadow.lookup(2).has_value());
  EXPECT_TRUE(shadow.overrides().empty());
  EXPECT_FALSE(std::filesystem::exists(shadowFile));
}

// ---------------------------------------------------------------------------
// Uszkodzony bitsetSize w .meta.shadow (#421): długość z pliku porównywana z ramką
// wpisu przed zaokrągleniem w packedByteCount.
// ---------------------------------------------------------------------------
namespace {

// Offset pola bitsetSize we wpisie: flaga (1 B) + recordIndex (8 B).
constexpr size_t kBitsetSizeOffset = sizeof(uint8_t) + sizeof(size_t);

}  // namespace

TEST(MetaShadowOverrideTest, deserialize_odrzuca_bitsetSize_spoza_ramki) {
  const rdb::metaShadow::ShadowOverride original{.recordIndex = 4, .nullBitset = {true, false, true}};
  const auto serialized = original.serialize();  // 1 bajt bitsetu = miejsce na 8 bitow

  const auto same = rdb::metaShadow::ShadowOverride::deserialize(serialized);  // kontrola dodatnia
  EXPECT_EQ(same.recordIndex, 4U);
  EXPECT_EQ(same.nullBitset, original.nullBitset);

  for (const size_t bitsetSize : {SIZE_MAX, SIZE_MAX - 6, size_t{1} << 40, size_t{9}}) {
    SCOPED_TRACE(bitsetSize);
    auto raw = serialized;
    std::memcpy(raw.data() + kBitsetSizeOffset, &bitsetSize, sizeof(bitsetSize));
    try {
      (void)rdb::metaShadow::ShadowOverride::deserialize(raw);
      ADD_FAILURE() << "expected std::runtime_error";
    } catch (const std::runtime_error &e) {
      EXPECT_NE(std::string(e.what()).find("exceeds remaining buffer"), std::string::npos) << e.what();
    }
  }
}

TEST_F(ShadowUsageFixture, load_uszkodzonego_cienia_daje_runtime_error) {
  { rdb::metaShadow(descriptor, file).appendOverride(1, allNull); }
  {
    rdb::metaShadow shadow(descriptor, file);  // kontrola dodatnia
    shadow.load();
    EXPECT_EQ(shadow.lookup(1), allNull);
  }
  {
    std::fstream f(shadowFile, std::ios::in | std::ios::out | std::ios::binary);
    const size_t corrupt = SIZE_MAX;
    f.seekp(static_cast<std::streamoff>(kBitsetSizeOffset));
    f.write(reinterpret_cast<const char *>(&corrupt), sizeof(corrupt));
  }
  rdb::metaShadow shadow(descriptor, file);
  EXPECT_THROW(shadow.load(), std::runtime_error);
}

/// @brief Ścieżka pliku cienia jest zgodna z konwencją używaną przez storageShadow::metaShadowFilePath().
TEST(MetaShadowStaticTest, shadowFilePathFor_dopisuje_sufiks) {
  EXPECT_EQ(rdb::metaShadow::shadowFilePathFor("foo.meta"), "foo.meta.shadow");
  EXPECT_EQ(rdb::metaShadow::shadowFilePathFor(""), "");
}
