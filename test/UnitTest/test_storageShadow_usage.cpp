// Scenariusze użycia klasy storageShadow
//
// storageShadow to wariant metaData wstrzykiwany do storage dla magazynów,
// których accessor utrzymuje plik cienia danych (FileInterface::hasShadow()):
// aktualizacje rekordów trafiają do cienia indeksu (.meta.shadow, obiekt
// metaShadow) zamiast do głównego indeksu, a przy odczycie nadpisanie z cienia
// wygrywa. Cień danych zachowuje oryginalną zarejestrowaną zawartość; metaindeks
// rejestruje wartości null i przerwy w transmisji — to niezależne mechanizmy.

#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/fagrp.hpp"
#include "rdb/storageShadow.hpp"

#include <cstdio>
#include <filesystem>

// ── Warunek wstrzyknięcia: cień danych raportowany przez accessor ────

TEST(StorageShadowTest, hasShadow_raportowany_przez_accessor) {
  rdb::Descriptor desc("a", 4, 1, rdb::INTEGER);

  rdb::posixBinaryFileWithShadow shd("ut_ss_shd.bin", desc);
  EXPECT_TRUE(shd.hasShadow());

  rdb::posixBinaryFile plain("ut_ss_plain.bin", desc);
  EXPECT_FALSE(plain.hasShadow());

  // groupFile raportuje mechanizm cienia swoich segmentów.
  rdb::groupFile<rdb::posixBinaryFileWithShadow> grpShd("ut_ss_grp_shd.bin", desc, {}, -1);
  EXPECT_TRUE(grpShd.hasShadow());
  rdb::groupFile<rdb::posixBinaryFile> grpPlain("ut_ss_grp_plain.bin", desc, {}, -1);
  EXPECT_FALSE(grpPlain.hasShadow());

  for (const auto &f : {"ut_ss_shd.bin", "ut_ss_shd.bin.shadow", "ut_ss_plain.bin", "ut_ss_grp_shd.bin",
                        "ut_ss_grp_shd.bin.shadow", "ut_ss_grp_plain.bin"})
    std::remove(f);
}

TEST(StorageShadowTest, metaShadowFilePath_dopisuje_sufiks_shadow) {
  EXPECT_EQ(rdb::storageShadow::metaShadowFilePath("data.meta"), "data.meta.shadow");
  EXPECT_EQ(rdb::storageShadow::metaShadowFilePath(""), "");
}

// ── Wariant metaData z cieniem indeksu ─────────────────────────

struct StorageShadowFixture : public ::testing::Test {
  const std::string file       = "usage_storageShadow.meta";
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
// Scenariusz 1: Aktualizacja rekordu trafia do cienia, główny indeks nietknięty
//
// onRecordModified() nie modyfikuje głównego indeksu, lecz dopisuje nadpisanie
// wzorca null do cienia indeksu (.meta.shadow). Główny indeks pozostaje spójny
// z głównym plikiem danych; cień indeksu jest spójny z cieniem danych.
// ---------------------------------------------------------------------------
TEST_F(StorageShadowFixture, scenariusz_nadpisanie_w_cieniu) {
  rdb::storageShadow meta(descriptor, file);

  // 5 rekordów allNull trafia do głównego indeksu (append nie używa cienia).
  for (int i = 0; i < 5; ++i)
    meta.onRecordAppended(allNull);

  // Korekta rekordu 2 → nadpisanie w .meta.shadow.
  meta.onRecordModified(2, allPresent);

  // Widok logiczny uwzględnia nadpisanie...
  EXPECT_EQ(meta.getNullBitset(2), allPresent);
  EXPECT_EQ(meta.getNullBitset(1), allNull);

  // ...ale główny indeks pozostaje nietknięty (1 segment RLE [allNull,5]).
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 1U);
  EXPECT_EQ(segs[0].recordCount, 5U);

  // Plik cienia indeksu powstał obok głównego.
  EXPECT_TRUE(std::filesystem::exists(shadowFile));

  // Aktualizacja poza zakresem głównego indeksu jest odrzucana.
  EXPECT_THROW(meta.onRecordModified(5, allPresent), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Scenariusz 2: Persystencja cienia po restarcie i mergeShadow()
//
// Nowa instancja wczytuje nadpisania z pliku cienia w konstruktorze.
// mergeShadow() scala nadpisania do głównego indeksu i usuwa cień
// (jak merge() pliku cienia danych).
// ---------------------------------------------------------------------------
TEST_F(StorageShadowFixture, scenariusz_persystencja_i_merge) {
  {
    rdb::storageShadow meta(descriptor, file);
    for (int i = 0; i < 5; ++i)
      meta.onRecordAppended(allNull);
    meta.onRecordModified(2, allPresent);
  }

  // Persystencja cienia po restarcie: nadpisanie nadal widoczne.
  rdb::storageShadow meta(descriptor, file);
  EXPECT_EQ(meta.getNullBitset(2), allPresent);

  // Scalenie cienia do głównego indeksu — jak merge() pliku cienia danych.
  meta.mergeShadow();

  // Teraz główny indeks jest rozbity na 3 segmenty, a cień usunięty.
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 3U);
  EXPECT_EQ(segs[1].nullBitset, allPresent);
  EXPECT_FALSE(std::filesystem::exists(shadowFile));
  EXPECT_EQ(meta.getNullBitset(2), allPresent);
}

// ---------------------------------------------------------------------------
// Scenariusz 3: discardShadow() odrzuca cień bez scalania; reset() czyści oba
//
// discardShadow() odzwierciedla usunięcie pliku .shadow danych bez merge —
// widok wraca do głównego indeksu. reset() (purge/rotacja w storage) czyści
// główny indeks i odrzuca cień.
// ---------------------------------------------------------------------------
TEST_F(StorageShadowFixture, scenariusz_discard_i_reset) {
  rdb::storageShadow meta(descriptor, file);
  for (int i = 0; i < 3; ++i)
    meta.onRecordAppended(allNull);
  meta.onRecordModified(1, allPresent);
  ASSERT_TRUE(std::filesystem::exists(shadowFile));

  meta.discardShadow();
  EXPECT_EQ(meta.getNullBitset(1), allNull);  // nadpisanie odrzucone — widok z głównego indeksu
  EXPECT_FALSE(std::filesystem::exists(shadowFile));

  meta.onRecordModified(1, allPresent);
  meta.reset();
  EXPECT_TRUE(meta.isEmpty());
  EXPECT_FALSE(std::filesystem::exists(shadowFile));
}
