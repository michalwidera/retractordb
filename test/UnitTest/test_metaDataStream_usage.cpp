// Scenariusze użycia klasy metaDataStream
//
// Każdy TEST_F ilustruje jeden typowy wzorzec interakcji z klasą:
// jak magazyn storage rejestruje informacje o polach null,
// jak działa kompresja RLE, persystencja, detekcja gap
// i mechanizm bezpieczeństwa flushCurrentEntry().
//
// Testy są ułożone od najprostszych do złożonych.

#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

#include <cstdio>
#include <filesystem>

// ---------------------------------------------------------------------------
// Konfiguracja: każdy test dostaje świeży plik meta i descriptor 3-polowy:
//   val  - INTEGER  (np. wartość pomiaru)
//   flag - INTEGER  (np. flaga statusu)
//   temp - DOUBLE   (np. temperatura)
// ---------------------------------------------------------------------------

struct UsageFixture : public ::testing::Test {
  const std::string file = "usage_meta.meta";

  // Descriptor opisuje trzy pola strumienia danych.
  rdb::Descriptor descriptor;

  // Wzorce bitsetów null odpowiadające różnym stanom czujnika.
  const std::vector<bool> allPresent = {false, false, false};  // wszystkie pola mają wartości
  const std::vector<bool> valNull    = {true, false, false};   // pomiar niedostępny
  const std::vector<bool> allNull    = {true, true, true};     // czujnik offline

  void SetUp() override {
    descriptor.append({{"val", 8, 0, rdb::DOUBLE}, {"flag", 4, 0, rdb::INTEGER}, {"temp", 8, 0, rdb::DOUBLE}});
  }
  void TearDown() override { std::remove(file.c_str()); }
};

// ---------------------------------------------------------------------------
// Scenariusz 1: Nagrywanie przychodzących rekordów
//
// storage.write() wywołuje onRecordAppended() po każdym zapisanym rekordzie.
// Klasa zapamiętuje wzorzec null każdego rekordu. getNullBitset(i) zwraca
// ten wzorzec dla dowolnego rekordu po fakcie.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_nagrywanie_rekordow) {
  rdb::metaDataStream meta(descriptor, file);

  // Symulacja 4 rekordów przychodzących z czujnika:
  meta.onRecordAppended(allPresent);  // rekord 0: pomiar OK
  meta.onRecordAppended(allPresent);  // rekord 1: pomiar OK
  meta.onRecordAppended(valNull);     // rekord 2: czujnik nie odpowiedział
  meta.onRecordAppended(allPresent);  // rekord 3: powrót do normy

  EXPECT_EQ(meta.totalRecords(), 4u);

  // Odczyt wzorca null dla dowolnego rekordu po fakcie:
  EXPECT_EQ(meta.getNullBitset(0), allPresent);
  EXPECT_EQ(meta.getNullBitset(1), allPresent);
  EXPECT_EQ(meta.getNullBitset(2), valNull);  // rekord 2 miał null
  EXPECT_EQ(meta.getNullBitset(3), allPresent);

  // Indywidualne pole: czy pole 'val' (indeks 0) rekordu 2 jest null?
  EXPECT_TRUE(meta.getNullBitset(2)[0]);
  // Pole 'flag' (indeks 1) tego samego rekordu nie jest null:
  EXPECT_FALSE(meta.getNullBitset(2)[1]);
}

// ---------------------------------------------------------------------------
// Scenariusz 2: Kompresja RLE — jeden segment dla wielu takich samych rekordów
//
// Kiedy wiele kolejnych rekordów ma identyczny wzorzec null, klasa przechowuje
// jeden wpis (segment) z licznikiem, zamiast N osobnych wpisów. Dzięki temu
// plik meta pozostaje mały nawet przy długich seriach danych.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_kompresja_rle) {
  rdb::metaDataStream meta(descriptor, file);

  // 500 rekordów z czujnika online (allPresent), po nich 200 rekordów offline.
  for (int i = 0; i < 500; ++i)
    meta.onRecordAppended(allPresent);
  for (int i = 0; i < 200; ++i)
    meta.onRecordAppended(allNull);

  EXPECT_EQ(meta.totalRecords(), 700u);

  // Mimo 700 rekordów — tylko 2 segmenty RLE.
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 2u);

  EXPECT_EQ(segs[0].nullBitset, allPresent);
  EXPECT_EQ(segs[0].recordCount, 500u);

  EXPECT_EQ(segs[1].nullBitset, allNull);
  EXPECT_EQ(segs[1].recordCount, 200u);
}

// ---------------------------------------------------------------------------
// Scenariusz 3: Persystencja — odtworzenie stanu po restarcie procesu
//
// Destruktor klasy automatycznie zapisuje buforowany segment do pliku.
// Nowy obiekt otwierający ten sam plik odtwarza pełny indeks null.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_persystencja_po_restarcie) {
  // Pierwsza sesja — program zapisuje dane i kończy pracę.
  {
    rdb::metaDataStream meta(descriptor, file);

    meta.onRecordAppended(allPresent);  // rekord 0
    meta.onRecordAppended(allPresent);  // rekord 1
    meta.onRecordAppended(valNull);     // rekord 2
    meta.onRecordAppended(valNull);     // rekord 3
    meta.onRecordAppended(allPresent);  // rekord 4

    // Destruktor ~metaDataStream() automatycznie zapisuje ostatni segment.
  }

  // Druga sesja — program rusza od nowa, otwiera istniejący plik.
  {
    rdb::metaDataStream meta(descriptor, file);

    // Wszystkie 5 rekordów jest dostępnych po restarcie.
    EXPECT_EQ(meta.totalRecords(), 5u);

    EXPECT_EQ(meta.getNullBitset(0), allPresent);
    EXPECT_EQ(meta.getNullBitset(2), valNull);
    EXPECT_EQ(meta.getNullBitset(4), allPresent);
  }
}

// ---------------------------------------------------------------------------
// Scenariusz 4: Przerwa w transmisji — gap detection
//
// Kiedy źródło danych przestaje dostarczać dane (np. utrata połączenia),
// storage wywołuje onTransmissionGap(). Czytelnik strumienia może sprawdzić
// isGapBefore(i), żeby wiedzieć, że przed rekordem i nastąpiła przerwa.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_przerwa_w_transmisji) {
  rdb::metaDataStream meta(descriptor, file);

  // Normalna praca: 3 rekordy.
  meta.onRecordAppended(allPresent);  // rekord 0
  meta.onRecordAppended(allPresent);  // rekord 1
  meta.onRecordAppended(allPresent);  // rekord 2

  // Wykryto przerwę w transmisji (np. 5 utraconych interwałów próbkowania).
  meta.onTransmissionGap(5);

  // Odbiór wznowiony.
  meta.onRecordAppended(allPresent);  // rekord 3 — pierwszy po przerwie
  meta.onRecordAppended(allPresent);  // rekord 4

  EXPECT_EQ(meta.totalRecords(), 5u);

  // Rekordy 0-2 nie mają przerwy przed sobą.
  EXPECT_FALSE(meta.isGapBefore(0));
  EXPECT_FALSE(meta.isGapBefore(2));

  // Rekord 3 ma przerwę przed sobą — tu nastąpiła utrata danych.
  EXPECT_TRUE(meta.isGapBefore(3));

  // Rekord 4 nie ma nowej przerwy.
  EXPECT_FALSE(meta.isGapBefore(4));

  // Segment gap widoczny w strukturze RLE.
  auto segs       = meta.segments();
  size_t gapCount = 0;
  for (const auto &s : segs)
    if (s.isGap) ++gapCount;
  EXPECT_EQ(gapCount, 1u);
}

// ---------------------------------------------------------------------------
// Scenariusz 5: Bezpieczeństwo przy awarii — flushCurrentEntry()
//
// storage wywołuje flushCurrentEntry() po każdym write(), żeby zapewnić,
// że metadane przeżyją crash. Klasa używa mechanizmu overwrite-in-place
// (tailDirty_), dzięki czemu plik nie rośnie dla serii identycznych wzorców.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_flush_dla_bezpieczenstwa) {
  rdb::metaDataStream meta(descriptor, file);

  // Typowy wzorzec użycia przez storage:
  // onRecordAppended + flushCurrentEntry po każdym zapisie.

  meta.onRecordAppended(allPresent);
  meta.flushCurrentEntry();  // rekord 0 bezpiecznie na dysku

  meta.onRecordAppended(allPresent);
  meta.flushCurrentEntry();  // rekord 1 — nadpisuje wpis [allPresent,1] → [allPresent,2]

  meta.onRecordAppended(allPresent);
  meta.flushCurrentEntry();  // rekord 2 — nadpisuje → [allPresent,3]

  meta.onRecordAppended(valNull);
  meta.flushCurrentEntry();  // rekord 3 — inny wzorzec, nowy wpis na dysku

  EXPECT_EQ(meta.totalRecords(), 4u);
  EXPECT_EQ(meta.getNullBitset(0), allPresent);
  EXPECT_EQ(meta.getNullBitset(3), valNull);

  // Mimo 3 takich samych rekordów — tylko 2 segmenty (kompresja działa).
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 2u);
  EXPECT_EQ(segs[0].recordCount, 3u);
  EXPECT_EQ(segs[1].recordCount, 1u);
}

// ---------------------------------------------------------------------------
// Scenariusz 6: Korekta wzorca null dla istniejącego rekordu
//
// storage.write() z istniejącym indeksem wywołuje onRecordModified().
// Jeśli segment RLE zawiera wiele rekordów, klasa rozdziela go na 3 części:
// przed modyfikowanym rekordem, sam rekord, po nim.
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_modyfikacja_rekordu) {
  rdb::metaDataStream meta(descriptor, file);

  // 5 rekordów: wszystkie allNull (np. czujnik był offline).
  for (int i = 0; i < 5; ++i)
    meta.onRecordAppended(allNull);

  // Okazuje się, że rekord 2 jednak miał dane — korekta po fakcie.
  meta.onRecordModified(2, allPresent);

  EXPECT_EQ(meta.totalRecords(), 5u);

  // Rekordy 0,1 pozostają allNull.
  EXPECT_EQ(meta.getNullBitset(0), allNull);
  EXPECT_EQ(meta.getNullBitset(1), allNull);

  // Rekord 2 ma teraz poprawiony wzorzec.
  EXPECT_EQ(meta.getNullBitset(2), allPresent);

  // Rekordy 3,4 pozostają allNull.
  EXPECT_EQ(meta.getNullBitset(3), allNull);
  EXPECT_EQ(meta.getNullBitset(4), allNull);

  // Segment [allNull,5] został rozbity na 3: [allNull,2], [allPresent,1], [allNull,2].
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 3u);
  EXPECT_EQ(segs[0].recordCount, 2u);
  EXPECT_EQ(segs[1].recordCount, 1u);
  EXPECT_EQ(segs[2].recordCount, 2u);
}

// ---------------------------------------------------------------------------
// Scenariusz 7: Reset — wyczyszczenie indeksu i ponowne użycie
//
// storage.purge() wywołuje reset() klasy, żeby przygotować strumień
// do zapisu od nowa (np. przy rotacji pliku danych).
// ---------------------------------------------------------------------------
TEST_F(UsageFixture, scenariusz_reset_strumienia) {
  rdb::metaDataStream meta(descriptor, file);

  // Pierwsza seria — 10 rekordów.
  for (int i = 0; i < 10; ++i)
    meta.onRecordAppended(allPresent);
  EXPECT_EQ(meta.totalRecords(), 10u);
  EXPECT_FALSE(meta.isEmpty());

  // Rotacja — strumień zaczyna od nowa.
  meta.reset();

  EXPECT_TRUE(meta.isEmpty());
  EXPECT_EQ(meta.totalRecords(), 0u);
  EXPECT_TRUE(meta.segments().empty());

  // Po resecie klasa jest gotowa do ponownego zapisu.
  meta.onRecordAppended(valNull);
  EXPECT_EQ(meta.totalRecords(), 1u);
  EXPECT_EQ(meta.getNullBitset(0), valNull);
}
