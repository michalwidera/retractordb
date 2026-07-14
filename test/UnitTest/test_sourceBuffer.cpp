// Scenariusze użycia klasy SourceBuffer
//
// SourceBuffer to wydzielona z klasy storage komora bieżącego rekordu i bufor
// historii dla źródeł deklarowanych (DEVICE/TEXTSOURCE). readCurrent() jest
// jedynym miejscem fizycznego odczytu ze źródła; wzorzec null pochodzi
// z null-aware read() akcesora (bez dynamic_cast na konkretne typy źródeł).

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "rdb/facctxtsrc.hpp"
#include "rdb/payload.hpp"
#include "rdb/sourceBuffer.hpp"

namespace {

rdb::Descriptor intDescriptor() { return {{"a", static_cast<int>(sizeof(int)), 1, rdb::INTEGER}}; }

void createTextFile(const std::string &name, const std::string &content) {
  std::ofstream ofs(name);
  ofs << content;
}

int valueOf(const rdb::payload &p) {
  int value = 0;
  std::memcpy(&value, p.span().data(), sizeof(int));
  return value;
}

}  // namespace

// ---------------------------------------------------------------------------
// Pojemność bufora historii: źródło deklarowane musi utrzymywać co najmniej
// jeden rekord — setCapacity(0) podnosi pojemność do 1.
// ---------------------------------------------------------------------------
TEST(SourceBufferTest, capacity_is_at_least_one) {
  rdb::SourceBuffer buffer;
  EXPECT_EQ(buffer.capacity(), 0U);

  buffer.setCapacity(0);
  EXPECT_EQ(buffer.capacity(), 1U);

  buffer.setCapacity(3);
  EXPECT_EQ(buffer.capacity(), 3U);
}

// ---------------------------------------------------------------------------
// readCurrent: rekord i wzorzec null pochodzą z null-aware read() źródła;
// poprawny odczyt daje wzorzec all-false i wartość z pliku.
// ---------------------------------------------------------------------------
TEST(SourceBufferTest, read_current_fills_out_from_source) {
  createTextFile("sb_read.txt", "42\n");
  auto desc = intDescriptor();
  rdb::textSourceRO source("sb_read.txt", desc, false);

  rdb::SourceBuffer buffer;
  buffer.attach(desc);

  rdb::payload out(desc);
  buffer.readCurrent(source, out);

  EXPECT_EQ(valueOf(out), 42);
  EXPECT_EQ(out.getNullBitset(), std::vector<bool>(desc.size(), false));

  std::filesystem::remove("sb_read.txt");
}

// ---------------------------------------------------------------------------
// readCurrent po wyczerpaniu źródła (bez zapętlenia): źródło deklarowane samo
// zwraca dane wyzerowane z wzorcem all-null — SourceBuffer przekazuje je dalej.
// ---------------------------------------------------------------------------
TEST(SourceBufferTest, read_current_after_eof_returns_null_row) {
  createTextFile("sb_eof.txt", "7\n");
  auto desc = intDescriptor();
  rdb::textSourceRO source("sb_eof.txt", desc, false);

  rdb::SourceBuffer buffer;
  buffer.attach(desc);
  rdb::payload out(desc);

  buffer.readCurrent(source, out);  // 7
  buffer.readCurrent(source, out);  // EOF

  EXPECT_EQ(valueOf(out), 0);
  EXPECT_EQ(out.getNullBitset(), std::vector<bool>(desc.size(), true));

  std::filesystem::remove("sb_eof.txt");
}

// ---------------------------------------------------------------------------
// fire: kopiuje komorę do payloadu wyjściowego i zasila historię; history(0)
// to najnowszy rekord, starsze rekordy przesuwają się w głąb bufora.
// ---------------------------------------------------------------------------
TEST(SourceBufferTest, fire_pushes_chamber_into_history) {
  createTextFile("sb_fire.txt", "1\n2\n3\n");
  auto desc = intDescriptor();
  rdb::textSourceRO source("sb_fire.txt", desc, false);

  rdb::SourceBuffer buffer;
  buffer.attach(desc);
  buffer.setCapacity(2);
  rdb::payload out(desc);

  for (int expected : {1, 2, 3}) {
    buffer.readCurrent(source, out);
    buffer.fire(out);
    EXPECT_EQ(valueOf(out), expected);
  }

  EXPECT_EQ(buffer.size(), 2U);              // pojemność 2 — najstarszy rekord wypchnięty
  EXPECT_EQ(valueOf(buffer.history(0)), 3);  // najnowszy
  EXPECT_EQ(valueOf(buffer.history(1)), 2);

  std::filesystem::remove("sb_fire.txt");
}
