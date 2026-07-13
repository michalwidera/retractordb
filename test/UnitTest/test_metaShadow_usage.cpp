// Scenariusze użycia klasy metaShadow
//
// metaShadow przechowuje nadpisania wzorca null (.meta.shadow) dla magazynów
// posiadających plik cienia danych (.shadow). Testy weryfikują bezpośrednio jej API:
// append/lookup, persystencję po przeładowaniu, kolejność nadpisań oraz discard().

#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaShadow.hpp"

#include <cstdio>
#include <filesystem>

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
// Scenariusz 3: Wielokrotne nadpisania tej samej pozycji — ostatnie wygrywa.
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
// Scenariusz 4: Persystencja — nowa instancja odczytuje nadpisania z pliku cienia po load().
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

/// @brief Ścieżka pliku cienia jest zgodna z konwencją używaną przez storageShadow::metaShadowFilePath().
TEST(MetaShadowStaticTest, shadowFilePathFor_dopisuje_sufiks) {
  EXPECT_EQ(rdb::metaShadow::shadowFilePathFor("foo.meta"), "foo.meta.shadow");
  EXPECT_EQ(rdb::metaShadow::shadowFilePathFor(""), "");
}
