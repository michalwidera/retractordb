#include <gtest/gtest.h>

#include <cstring>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "rdb/descriptor.hpp"

// Tests intentionally validate legacy textual/binary layouts with C-style arrays.
// NOLINTBEGIN(modernize-avoid-c-arrays)

extern std::string parserDESCString(rdb::Descriptor &desc, std::string_view inlet);

namespace {

// Interfejs kontenera nie moze oddawac mutowalnego pola ani drogi do bazy vector.
static_assert(!std::is_base_of_v<std::vector<rdb::rField>, rdb::Descriptor>);
static_assert(!std::is_convertible_v<rdb::Descriptor *, std::vector<rdb::rField> *>);
static_assert(std::is_same_v<decltype(std::declval<rdb::Descriptor &>()[0]), const rdb::rField &>);
static_assert(std::is_same_v<std::ranges::range_reference_t<rdb::Descriptor>, const rdb::rField &>);
static_assert(std::is_same_v<decltype(*std::declval<rdb::Descriptor &>().cbegin()), const rdb::rField &>);
static_assert(std::is_nothrow_move_constructible_v<rdb::Descriptor>);
static_assert(std::is_nothrow_move_assignable_v<rdb::Descriptor>);

template <typename T>
constexpr bool exposesVectorMutation = requires(T & d, rdb::rField f) {
  d.push_back(f);
}
|| requires(T &d, rdb::rField f) { d.emplace_back(f); }
|| requires(T &d, rdb::rField f) { d.insert(d.begin(), f); }
|| requires(T &d) { d.erase(d.begin()); }
|| requires(T &d) { d.clear(); }
|| requires(T &d) { d.resize(1); }
|| requires(T &d, rdb::rField f) { d.assign(1, f); }
|| requires(T &d, rdb::rField f) { d.assign({f}); }
|| requires(T &d) { d.assign(d.begin(), d.end()); }
|| requires(T &d) { d.pop_back(); }
|| requires(T &d) { d.swap(d); }
|| requires(T &d, rdb::rField f) { d.at(0) = f; }
|| requires(T &d, rdb::rField f) { d.front() = f; }
|| requires(T &d, rdb::rField f) { d.back() = f; }
|| requires(T &d, rdb::rField f) { *d.data() = f; };
static_assert(!exposesVectorMutation<rdb::Descriptor>);

void expectLayout(const rdb::Descriptor &desc, std::initializer_list<int> offsets,
                  std::initializer_list<std::pair<int, int>> positions, size_t bytes) {
  EXPECT_EQ(desc.getSizeInBytes(), bytes);
  ASSERT_EQ(desc.flatElementCount(), static_cast<int>(offsets.size()));
  ASSERT_EQ(offsets.size(), positions.size());
  auto position = positions.begin();
  int slot      = 0;
  for (const int offset : offsets) {
    EXPECT_EQ(desc.byteOffsetAtFlatIndex(slot), offset);
    EXPECT_EQ(desc.flatIndexToDescriptorPosition(slot), *position);
    ++position;
    ++slot;
  }
  EXPECT_FALSE(desc.flatIndexToDescriptorPosition(slot).has_value());
}

TEST(Descriptor, NamedLookupsAreConstAndPreserveArrayLayout) {
  const rdb::Descriptor desc{rdb::rField("values", 4, 3, rdb::INTEGER), rdb::rField("label", 1, 5, rdb::STRING),
                             rdb::rField("flag", 1, 1, rdb::BYTE)};
  EXPECT_EQ(desc.fieldIndex("label"), 1U);
  EXPECT_EQ(desc.fieldSize("values"), 12);
  EXPECT_EQ(desc.fieldSize("label"), 5);
  EXPECT_EQ(desc.fieldByteOffset("flag"), 17U);
  EXPECT_EQ(desc.fieldTypeName("label"), "STRING");
  EXPECT_EQ(desc.getSizeInBytes(), 18U);
}

bool test_descriptor() {
  rdb::Descriptor data1{rdb::rField("Name3", 1, 10, rdb::STRING), rdb::rField("Name4", 10, 1, rdb::STRING)};

  data1.append({rdb::rField("Name5z", 1, 10, rdb::STRING)});
  data1.append({rdb::rField("Name6z", 1, 10, rdb::STRING)});

  data1.append({rdb::rField("Name", 1, 10, rdb::STRING)});
  data1.append({rdb::rField("TLen", sizeof(uint), 1, rdb::UINT)});

  data1 += rdb::Descriptor("Name2", 1, 10, rdb::STRING);
  data1 += rdb::Descriptor("Control", 1, 1, rdb::BYTE);
  data1 += rdb::Descriptor("Len3", 4, 1, rdb::UINT);
  {
    std::stringstream coutstring;
    coutstring << data1;
    char test[] =
        "{\tSTRING Name3[10]\n\tSTRING Name4[10]\n\tSTRING "
        "Name5z[10]\n\tSTRING Name6z[10]\n\tSTRING Name[10]\n\tUINT "
        "TLen\n\tSTRING Name2[10]\n\tBYTE Control\n\tUINT Len3\n}";
    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  rdb::Descriptor data2 = rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                          rdb::Descriptor("Len3", 4, 1, rdb::UINT) +     //
                          rdb::Descriptor("Control", 1, 1, rdb::BYTE);
  {
    std::stringstream coutstring;
    char test[] = "{\tSTRING Name[10]\n\tUINT Len3\n\tBYTE Control\n}";
    coutstring << data2;
    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  if (data2.fieldIndex("Control") != 2) return false;
  if (data2.fieldSize("Control") != 1) return false;
  if (strcmp(data2.fieldTypeName("Control").data(), "BYTE") != 0) return false;
  if (data2.fieldByteOffset("Control") != 14) return false;

  return true;
}

bool test_descriptor_read() {
  // start cin test
  // https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
  std::streambuf *orig = std::cin.rdbuf();

  const char *test_string =
      "{ STRING Name3[10]\nSTRING Name[10]\nUINT Len STRING Name2[10] BYTE "
      "Control UINT Len3 }";

  rdb::Descriptor data3;

  std::istringstream input(test_string);
  std::cin.rdbuf(input.rdbuf());
  std::cin >> data3;
  std::cin.rdbuf(orig);

  {
    std::stringstream coutstring;
    const char *test =
        "{\tSTRING Name3[10]\n\tSTRING Name[10]\n\tUINT Len\n\tSTRING "
        "Name2[10]\n\tBYTE Control\n\tUINT Len3\n}";
    coutstring << data3;

    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  return true;
}

}  // namespace

// Sprawdza odczyt deskryptora przez operator>> i jego ponowny zapis tekstowy.
// Pokrywa mieszany uklad napisow, pola BYTE i pol UINT w wejsciach z podzialem na wiersze.
TEST(descriptor, read_from_stream) { EXPECT_TRUE(test_descriptor_read()); }

// Sprawdza budowanie deskryptora przez append, += i + oraz formatowanie i dostep po nazwie.
// Pokrywa napisy o dlugosci zapisanej w rlen lub rarray i pola liczbowe za nimi.
TEST(descriptor, print_and_basic_accessors) { EXPECT_TRUE(test_descriptor()); }

// Sprawdza typ i szerokosc pojedynczego slotu zwracane przez widestFieldType().
// Pokrywa tablice INTEGER, jej polaczenie ze skalarem DOUBLE oraz jeden napis STRING[12].
TEST(descriptor, widest_field_type_reports_flat_element_width) {
  rdb::Descriptor numericArray{{"samples", static_cast<int>(sizeof(int)), 3, rdb::INTEGER}};
  EXPECT_EQ(numericArray.widestFieldType(), std::make_pair(rdb::INTEGER, static_cast<int>(sizeof(int))));

  rdb::Descriptor mixed{{"samples", static_cast<int>(sizeof(int)), 8, rdb::INTEGER},
                        {"value", static_cast<int>(sizeof(double)), 1, rdb::DOUBLE}};
  EXPECT_EQ(mixed.widestFieldType(), std::make_pair(rdb::DOUBLE, static_cast<int>(sizeof(double))));

  rdb::Descriptor text{{"label", 1, 12, rdb::STRING}};
  EXPECT_EQ(text.widestFieldType(), std::make_pair(rdb::STRING, 12));
}

// Sprawdza zgodnosc identycznych ukladow i odmowe dla zmienionej kolejnosci typow pol.
// Pokrywa tez napis w prawym deskryptorze dluzszy niz pole dostepne w lewym.
TEST(descriptor, compare) {
  rdb::Descriptor dataDescriptor1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                                  rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                                  rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor dataDescriptor2{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                                  rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                                  rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor dataDescriptorDiff1{rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                                      rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                                      rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor dataDescriptorDiff2{rdb::Descriptor("Name", 1, 11, rdb::STRING) +  //
                                      rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                                      rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  EXPECT_TRUE(dataDescriptor1 == dataDescriptor2);
  EXPECT_FALSE(dataDescriptor1 == dataDescriptorDiff1);
  EXPECT_FALSE(dataDescriptor1 == dataDescriptorDiff2);
}

// Sprawdza pomijanie REF i TYPE podczas porownania deskryptorow z rozna liczba wpisow.
// Pokrywa zgodne pole INTEGER po konfiguracji oraz odmowe dla szerszego pola DOUBLE.
TEST(descriptor, compare_ignores_configuration_fields_without_out_of_bounds_access) {
  auto withConfig = rdb::Descriptor("source.dat", 0, 0, rdb::REF) +   //
                    rdb::Descriptor("TEXTSOURCE", 0, 1, rdb::TYPE) +  //
                    rdb::Descriptor("value", 4, 1, rdb::INTEGER);
  auto plain     = rdb::Descriptor("value", 4, 1, rdb::INTEGER);
  auto different = rdb::Descriptor("value", 8, 1, rdb::DOUBLE);

  EXPECT_TRUE(withConfig == plain);
  EXPECT_FALSE(withConfig == different);
}

// Sprawdza domyslne wyniki retention() i storagePolicy() bez pol konfiguracyjnych.
// Pokrywa deskryptor z jednym polem danych: brak retencji, pusty typ magazynu i pojemnosc zero.
TEST(descriptor, retention_and_policy_defaults) {
  auto desc = rdb::Descriptor("value", 4, 1, rdb::INTEGER);

  auto retention = desc.retention();
  EXPECT_TRUE(retention.noRetention());

  auto policy = desc.storagePolicy();
  EXPECT_TRUE(policy.first.empty());
  EXPECT_EQ(policy.second, 0U);
}

// Sprawdza odczyt konfiguracji RETENTION, RETMEMORY i TYPE obok pola danych.
// Pokrywa retencje 5 segmentow po 2 rekordy oraz magazyn MEMORY o pojemnosci 7.
TEST(descriptor, retention_and_policy_values) {
  auto desc = rdb::Descriptor("value", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("MEMORY", 0, 1, rdb::TYPE) +    //
              rdb::Descriptor("retention-mem", 7, 1, rdb::RETMEMORY) + rdb::Descriptor("retention", 5, 2, rdb::RETENTION);

  auto retention = desc.retention();
  EXPECT_EQ(retention.segments, 5U);
  EXPECT_EQ(retention.capacity, 2U);

  auto policy = desc.storagePolicy();
  EXPECT_EQ(policy.first, "MEMORY");
  EXPECT_EQ(policy.second, 7U);
}

// Sprawdza wybor pol danych, liczbe slotow plaskich i usuniecie konfiguracji REF oraz TYPE.
// Pokrywa tablice BYTE[3] i jeden napis, ktore musza pozostac po usunieciu konfiguracji.
TEST(descriptor, clean_ref_and_flat_fields) {
  auto desc = rdb::Descriptor("src.bin", 0, 0, rdb::REF) +      //
              rdb::Descriptor("TEXTSOURCE", 0, 1, rdb::TYPE) +  //
              rdb::Descriptor("a", 1, 3, rdb::BYTE) +           //
              rdb::Descriptor("s", 8, 99, rdb::STRING);

  EXPECT_TRUE(desc.hasField("src.bin"));
  EXPECT_EQ(desc.flatElementCount(), 4);

  auto flatFields = desc.dataFields();
  EXPECT_EQ(flatFields.size(), 2U);
  EXPECT_EQ(flatFields[0].rname, "a");
  EXPECT_EQ(flatFields[1].rname, "s");

  desc.removeConfigurationFields();
  EXPECT_FALSE(desc.hasField("src.bin"));
  EXPECT_FALSE(desc.hasField("TEXTSOURCE"));
  EXPECT_TRUE(desc.hasField("a"));
  EXPECT_TRUE(desc.hasField("s"));
  EXPECT_EQ(desc.size(), 2U);
}

// Sprawdza nazwy, typy i szerokosci slotow deskryptora przeplotu po pominieciu REF.
// Pokrywa pary BYTE/INTEGER i UINT/BYTE, wybierajac szerszy typ dla kazdej pozycji.
TEST(descriptor, create_hash_uses_max_len_and_type) {
  auto lhs = rdb::Descriptor("src-left", 0, 0, rdb::REF) +  //
             rdb::Descriptor("a", 1, 1, rdb::BYTE) +        //
             rdb::Descriptor("b", 4, 1, rdb::UINT);
  auto rhs = rdb::Descriptor("src-right", 0, 0, rdb::REF) +  //
             rdb::Descriptor("a", 4, 1, rdb::INTEGER) +      //
             rdb::Descriptor("b", 1, 1, rdb::BYTE);

  rdb::Descriptor out;
  out.composeHashDescriptorFrom("h", lhs, rhs);

  EXPECT_EQ(out.size(), 2U);
  EXPECT_EQ(out[0].rname, "h_0");
  EXPECT_EQ(out[0].rlen, 4);
  EXPECT_EQ(out[0].rarray, 1);
  EXPECT_EQ(out[0].rtype, rdb::INTEGER);

  EXPECT_EQ(out[1].rname, "h_1");
  EXPECT_EQ(out[1].rlen, 4);
  EXPECT_EQ(out[1].rarray, 1);
  EXPECT_EQ(out[1].rtype, rdb::UINT);
}

// Slot przeplotu nad napisem ma CALA dlugosc dluzszego napisu, w obu kolejnosciach skladnikow.
// `STRING[N]` z DECLARE niesie dlugosc w rarray (rlen = 1), wpis z okna AGSE - w rlen (rarray = 1).
// Do 2026-09-27 przeplot bral samo rlen, wiec slot zadeklarowanego `STRING[8]` mial 1 B i pierwsze
// przypisanie rekordu zrodla konczylo proces FatalError-em "schema mismatch".
TEST(descriptor, create_hash_string_slot_takes_the_full_length_of_the_longer_side) {
  const rdb::Descriptor declared8("s", 1, 8, rdb::STRING);
  const rdb::Descriptor declared16("s", 1, 16, rdb::STRING);
  const rdb::Descriptor window12("w", 12, 1, rdb::STRING);

  for (const auto &[lhs, rhs, bytes] : {std::tuple{declared8, declared16, 16}, std::tuple{declared16, declared8, 16},
                                        std::tuple{window12, declared8, 12}, std::tuple{declared8, window12, 12}}) {
    rdb::Descriptor out;
    out.composeHashDescriptorFrom("h", lhs, rhs);
    ASSERT_EQ(out.size(), 1U);
    EXPECT_EQ(out[0].rtype, rdb::STRING);
    EXPECT_EQ(out[0].rlen, 1);
    EXPECT_EQ(out[0].rarray, bytes);
    EXPECT_EQ(out.getSizeInBytes(), static_cast<size_t>(bytes));
  }
}

// Pozycja liczbowa bierze dlugosc pola, ktorego TYP wygrywa. Wsrod typow BYTE..DOUBLE jest to zarazem
// dluzsze pole z jednym wyjatkiem: FLOAT (4 B) nad RATIONAL (8 B). Slot FLOAT o 8 B kazalby zapisowi
// wartosci kopiowac 8 B z czterobajtowej zmiennej.
TEST(descriptor, create_hash_numeric_slot_takes_the_length_of_the_winning_type) {
  for (const auto &[lhs, rhs] : {std::pair{rdb::Descriptor("a", 8, 1, rdb::RATIONAL), rdb::Descriptor("a", 4, 1, rdb::FLOAT)},
                                 std::pair{rdb::Descriptor("a", 4, 1, rdb::FLOAT), rdb::Descriptor("a", 8, 1, rdb::RATIONAL)}}) {
    rdb::Descriptor out;
    out.composeHashDescriptorFrom("h", lhs, rhs);
    ASSERT_EQ(out.size(), 1U);
    EXPECT_EQ(out[0].rtype, rdb::FLOAT);
    EXPECT_EQ(out[0].rlen, 4);
    EXPECT_EQ(out[0].rarray, 1);
  }
}

// Sprawdza jednorazowe dzialanie singleLineFormat i reset flagi po zapisie deskryptora.
// Pokrywa dwa kolejne zapisy: pierwszy w jednym wierszu, drugi w domyslnym formacie wielowierszowym.
TEST(descriptor, flat_output_resets_after_stream) {
  auto desc = rdb::Descriptor("x", 1, 1, rdb::BYTE);

  std::stringstream flatOut;
  flatOut << rdb::singleLineFormat << desc;
  EXPECT_EQ(flatOut.str(), "{ BYTE x }");
  EXPECT_FALSE(rdb::Descriptor::isSingleLineOutput());

  std::stringstream multilineOut;
  multilineOut << desc;
  EXPECT_EQ(multilineOut.str(), "{\tBYTE x\n}");
}

// Sprawdza mapowanie slotow na pola i elementy tablic oraz offsety bajtowe mieszanego rekordu.
// Pokrywa dwa sloty BYTE, jeden napis o rozmiarze 5 * 9 bajtow i INTEGER pod offsetem 47.
TEST(descriptor, offset_and_convert_for_string_and_arrays) {
  auto desc = rdb::Descriptor("a", 1, 2, rdb::BYTE) +    //
              rdb::Descriptor("s", 5, 9, rdb::STRING) +  //
              rdb::Descriptor("v", 4, 1, rdb::INTEGER);

  EXPECT_EQ(desc.flatElementCount(), 4);

  EXPECT_TRUE(desc.flatIndexToDescriptorPosition(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc.flatIndexToDescriptorPosition(1) == std::make_pair(0, 1));
  EXPECT_TRUE(desc.flatIndexToDescriptorPosition(2) == std::make_pair(1, 0));
  EXPECT_TRUE(desc.flatIndexToDescriptorPosition(3) == std::make_pair(2, 0));

  EXPECT_EQ(desc.byteOffsetAtFlatIndex(0), 0);
  EXPECT_EQ(desc.byteOffsetAtFlatIndex(1), 1);
  EXPECT_EQ(desc.byteOffsetAtFlatIndex(2), 2);
  EXPECT_EQ(desc.byteOffsetAtFlatIndex(3), 47);
}

// Sprawdza odczyt ukladu pustego deskryptora utworzonego konstruktorem domyslnym.
// Pokrywa brak pol danych, zero slotow i odmowe mapowania indeksu zero.
TEST(descriptor, empty_descriptor_has_empty_flat_mapping) {
  rdb::Descriptor empty;

  EXPECT_EQ(empty.flatElementCount(), 0);
  EXPECT_TRUE(empty.dataFields().empty());
  EXPECT_FALSE(empty.flatIndexToDescriptorPosition(0).has_value());
}

// #424: sprawdza odswiezenie rozmiaru, offsetow i mapowan po append na juz zbudowanym cache.
// Pokrywa dodanie napisu i INTEGER za tablica BYTE; stare dwa sloty musza rozszerzyc sie do czterech.
TEST(descriptor, append_refreshes_cached_layout) {
  rdb::Descriptor desc{{"a", 1, 2, rdb::BYTE}};
  expectLayout(desc, {0, 1}, {{0, 0}, {0, 1}}, 2);

  desc.append({{"s", 1, 5, rdb::STRING}, {"b", 4, 1, rdb::INTEGER}});
  expectLayout(desc, {0, 1, 2, 7}, {{0, 0}, {0, 1}, {1, 0}, {2, 0}}, 11);
}

// #424: sprawdza aktualny uklad po laczeniu deskryptorow przez + i +=, gdy oba maja zbudowany cache.
// Pokrywa zachowanie lewego argumentu przy + oraz dopisanie napisu i INTEGER za tablica BYTE.
TEST(descriptor, concatenation_refreshes_cached_layout) {
  rdb::Descriptor desc{{"a", 1, 2, rdb::BYTE}};
  const rdb::Descriptor rhs{{"s", 1, 5, rdb::STRING}, {"b", 4, 1, rdb::INTEGER}};
  expectLayout(desc, {0, 1}, {{0, 0}, {0, 1}}, 2);
  expectLayout(rhs, {0, 5}, {{0, 0}, {1, 0}}, 9);

  const auto sum = desc + rhs;
  expectLayout(sum, {0, 1, 2, 7}, {{0, 0}, {0, 1}, {1, 0}, {2, 0}}, 11);
  expectLayout(desc, {0, 1}, {{0, 0}, {0, 1}}, 2);
  desc += rhs;
  expectLayout(desc, {0, 1, 2, 7}, {{0, 0}, {0, 1}, {1, 0}, {2, 0}}, 11);
}

// #424: sprawdza odswiezenie indeksow pol po usunieciu konfiguracji z juz zmapowanego deskryptora.
// Pokrywa REF przed danymi i TYPE miedzy polami: offsety i rozmiar zostaja te same, indeksy pol sie zmieniaja.
TEST(descriptor, removing_configuration_refreshes_cached_positions) {
  rdb::Descriptor desc{
      {"src.bin", 0, 0, rdb::REF}, {"a", 1, 2, rdb::BYTE}, {"MEMORY", 0, 0, rdb::TYPE}, {"b", 4, 1, rdb::INTEGER}};
  expectLayout(desc, {0, 1, 2}, {{1, 0}, {1, 1}, {3, 0}}, 6);

  desc.removeConfigurationFields();
  expectLayout(desc, {0, 1, 2}, {{0, 0}, {0, 1}, {1, 0}}, 6);
  ASSERT_EQ(desc.size(), 2U);
  EXPECT_EQ(desc[1].rname, "b");
}

// #424: sprawdza zastapienie pol i ich cache podczas composeHashDescriptorFrom na niepustym deskryptorze.
// Pokrywa zmiane trzech slotow BYTE na dwa sloty INTEGER z nowymi offsetami i nazwami h_0 oraz h_1.
TEST(descriptor, hash_composition_replaces_cached_layout) {
  rdb::Descriptor desc{{"old", 1, 3, rdb::BYTE}};
  expectLayout(desc, {0, 1, 2}, {{0, 0}, {0, 1}, {0, 2}}, 3);

  desc.composeHashDescriptorFrom("h", rdb::Descriptor{{"a", 4, 2, rdb::INTEGER}}, rdb::Descriptor{{"b", 1, 2, rdb::BYTE}});
  expectLayout(desc, {0, 4}, {{0, 0}, {1, 0}}, 8);
  ASSERT_EQ(desc.size(), 2U);
  EXPECT_EQ(desc[0].rname, "h_0");
  EXPECT_EQ(desc[1].rname, "h_1");
}

// #424: sprawdza odswiezenie cache po dopisaniu pol przez parserDESCString i operator>>.
// Pokrywa kolejne parsowania do niepustego deskryptora: napis po BYTE[2], a nastepnie INTEGER za napisem.
TEST(descriptor, parsing_into_nonempty_descriptor_refreshes_cached_layout) {
  rdb::Descriptor desc{{"a", 1, 2, rdb::BYTE}};
  expectLayout(desc, {0, 1}, {{0, 0}, {0, 1}}, 2);
  ASSERT_EQ(parserDESCString(desc, "{ STRING s[5] }"), "OK");
  expectLayout(desc, {0, 1, 2}, {{0, 0}, {0, 1}, {1, 0}}, 7);

  std::istringstream input("{ INTEGER b }");
  input >> desc;
  expectLayout(desc, {0, 1, 2, 7}, {{0, 0}, {0, 1}, {1, 0}, {2, 0}}, 11);
}

// #424: sprawdza zastapienie cache celu przy przypisaniu kopiujacym i przenoszacym.
// Pokrywa zrodlo z cache zbudowanym lub wymagajacym odbudowy oraz ponowne append do oproznionego zrodla.
TEST(descriptor, assignment_transfers_clean_and_dirty_layouts) {
  // Cel i zrodlo maja rozne uklady. Kopia/przeniesienie musza zastapic takze cache celu.
  for (const bool buildSourceCache : {false, true}) {
    SCOPED_TRACE(buildSourceCache);
    rdb::Descriptor source{{"s", 1, 5, rdb::STRING}, {"b", 4, 1, rdb::INTEGER}};
    if (buildSourceCache) expectLayout(source, {0, 5}, {{0, 0}, {1, 0}}, 9);

    rdb::Descriptor target{{"old", 1, 2, rdb::BYTE}};
    expectLayout(target, {0, 1}, {{0, 0}, {0, 1}}, 2);
    target = source;
    expectLayout(target, {0, 5}, {{0, 0}, {1, 0}}, 9);

    rdb::Descriptor moved{{"old", 1, 2, rdb::BYTE}};
    expectLayout(moved, {0, 1}, {{0, 0}, {0, 1}}, 2);
    moved = std::move(source);
    expectLayout(moved, {0, 5}, {{0, 0}, {1, 0}}, 9);
    expectLayout(source, {}, {}, 0);
    source.append({{"new", 4, 1, rdb::INTEGER}});
    expectLayout(source, {0}, {{0, 0}}, 4);
  }
}

// Sprawdza przypisanie indeksu plaskiego do pola i elementu tablicy w rekordzie zaczynajacym sie od napisu.
// Pokrywa STRING[10] jako jeden slot, BYTE[3] jako trzy sloty i koncowy skalar INTEGER.
TEST(descriptor, position_conversion_case_1) {
  auto desc1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(4) == std::make_pair(2, 0));
}

// Sprawdza mapowanie indeksow plaskich, gdy pole przed tablica jest skalarem liczbowym.
// Pokrywa BYTE, trzy elementy BYTE[3] i koncowy INTEGER, z zachowaniem indeksow elementow tablicy.
TEST(descriptor, position_conversion_case_2) {
  auto desc1{rdb::Descriptor("Name", 1, 1, rdb::BYTE) +     //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.flatIndexToDescriptorPosition(4) == std::make_pair(2, 0));
}

// Sprawdza akceptacje poprawnej skladni deskryptorow przez parserDESCString.
// Pokrywa pola skalarne i tablicowe oraz konfiguracje REF, TYPE TEXTSOURCE, RETENTION i RETMEMORY.
TEST(descriptor, parser) {
  rdb::Descriptor out;
  EXPECT_TRUE(parserDESCString(out, "{ BYTE a INTEGER b[10] INTEGER c }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a INTEGER b INTEGER c REF \"datafile.txt\" TYPE TEXTSOURCE }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETENTION 10 5 }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETMEMORY 10 TYPE MEMORY }") == "OK");
}

// Sprawdza diagnostyke z numerem wiersza i kolumny oraz mozliwosc parsowania po bledzie.
// Pokrywa brak nazwy pola, niedozwolony znak @ i nastepny poprawny deskryptor w tym samym procesie.
TEST(descriptor, syntax_error_returns_location_and_allows_next_parse) {
  rdb::Descriptor bad;
  const std::string parserError = parserDESCString(bad, "{\n INTEGER }\n");
  EXPECT_TRUE(parserError.starts_with("Fail: line 2:9 ")) << parserError;
  EXPECT_TRUE(parserError.contains("missing ID")) << parserError;

  rdb::Descriptor badToken;
  const std::string lexerError = parserDESCString(badToken, "{\n INTEGER @bad\n}");
  EXPECT_TRUE(lexerError.starts_with("Fail: line 2:9 ")) << lexerError;
  EXPECT_TRUE(lexerError.contains("token recognition error")) << lexerError;

  rdb::Descriptor valid;
  EXPECT_EQ(parserDESCString(valid, "{ INTEGER a }"), "OK");
}

// Plik .desc czyta takze serwer (storage::attachDescriptor -> tryLoadDescriptorFile), wiec rozmiar pola
// ma w gramatyce DESC te sama granice co `TYP[N]` i `STRING[N]` w RQL (A2 M11). Literal spoza int
// konczyl proces przez std::terminate: std::stoi rzucal z metody exit* listenera, a ta biegnie
// z noexcept-owego destruktora antlrcpp::FinalAction.
TEST(descriptor, parser_limits_field_size) {
  for (const std::string type : {"BYTE", "INTEGER", "UINT", "FLOAT", "DOUBLE", "RATIONAL", "STRING"}) {
    rdb::Descriptor atLimit;
    EXPECT_EQ(parserDESCString(atLimit, "{ " + type + " a[65536] }"), "OK") << type;
    rdb::Descriptor aboveLimit;
    EXPECT_TRUE(parserDESCString(aboveLimit, "{ " + type + " a[65537] }").contains("field size 65537 exceeds the limit 65536"))
        << type;
    rdb::Descriptor zero;
    EXPECT_TRUE(parserDESCString(zero, "{ " + type + " a[0] }").contains("field size 0 must be greater than zero")) << type;
  }

  for (const std::string text :
       {"{ INTEGER a[99999999999] }", "{ STRING a[99999999999] }", "{ INTEGER a RETENTION 99999999999 5 }",
        "{ INTEGER a RETENTION 5 99999999999 }", "{ INTEGER a RETMEMORY 99999999999 TYPE MEMORY }"}) {
    rdb::Descriptor out;
    EXPECT_TRUE(parserDESCString(out, text).contains("numeric literal 99999999999 is out of range")) << text;
  }

  // Literaly z #279 (S-07): INT_MAX miesci sie w int, wiec odpada na granicy pola - wczesniej konstruktor
  // payloadu zamawial na nim 2 GiB; drugi nie miesci sie w int - wczesniej std::stoi rzucal std::out_of_range.
  rdb::Descriptor intMax;
  EXPECT_TRUE(parserDESCString(intMax, "{ STRING s[2147483647] }").contains("field size 2147483647 exceeds the limit 65536"));
  rdb::Descriptor aboveInt;
  EXPECT_TRUE(
      parserDESCString(aboveInt, "{ INTEGER a[999999999999] }").contains("numeric literal 999999999999 is out of range"));

  // Retencja nic nie alokuje - trzyma pliki na dysku - wiec gornej granicy nie ma.
  rdb::Descriptor retention;
  EXPECT_EQ(parserDESCString(retention, "{ INTEGER a RETENTION 1000000 1000000 }"), "OK");

  // Pojemnosc 0 nie opisuje zadnego magazynu: RETENTION konczyl proces przy pierwszym zapisie,
  // a RETMEMORY dawal pierscien bez granicy. Segmenty 0 znacza "bez limitu segmentow".
  rdb::Descriptor zeroCapacity;
  EXPECT_TRUE(
      parserDESCString(zeroCapacity, "{ INTEGER a RETENTION 0 5 }").contains("RETENTION capacity 0 must be greater than zero"));
  rdb::Descriptor zeroSegments;
  EXPECT_EQ(parserDESCString(zeroSegments, "{ INTEGER a RETENTION 5 0 }"), "OK");
  rdb::Descriptor zeroRing;
  EXPECT_TRUE(parserDESCString(zeroRing, "{ INTEGER a RETMEMORY 0 TYPE MEMORY }")
                  .contains("RETMEMORY capacity 0 must be greater than zero"));

  // Granica pola nie ogranicza liczby pol: 4096 pol po 512 KiB to 2^31 bajtow, o jeden wiecej niz int.
  std::string wide = "{";
  for (int i = 0; i < 4096; ++i)
    wide += " DOUBLE f" + std::to_string(i) + "[65536]";
  rdb::Descriptor tooWide;
  EXPECT_TRUE(
      parserDESCString(tooWide, wide + " }").contains("record of 2147483648 bytes exceeds the descriptor limit 2147483647"));

  // Odmowa nie zostaje w stanie globalnym: nastepne parsowanie w tym samym procesie przechodzi.
  rdb::Descriptor next;
  EXPECT_EQ(parserDESCString(next, "{ INTEGER a }"), "OK");
}

// Sprawdza zachowanie indeksow i rozmiaru pola po przypisaniu kopiujacym do pustego deskryptora.
// Pokrywa mieszany uklad napisu, BYTE i INTEGER odczytywany po nazwach Control i TLen.
TEST(descriptor, assign_operator) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2;
  data2 = data1;
  EXPECT_TRUE(data2.fieldIndex("Control") == data1.fieldIndex("Control"));
  EXPECT_TRUE(data2.fieldSize("Control") == data1.fieldSize("Control"));
  EXPECT_TRUE(data2.fieldIndex("TLen") == data1.fieldIndex("TLen"));
}

// NOLINTEND(modernize-avoid-c-arrays)

// Sprawdza zachowanie indeksow i rozmiaru pola podczas konstrukcji kopii deskryptora.
// Pokrywa kopie mieszanego ukladu STRING/BYTE/INTEGER i odczyt pol Control oraz TLen po nazwie.
TEST(descriptor, copy_constructor) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2{data1};
  EXPECT_TRUE(data2.fieldIndex("Control") == data1.fieldIndex("Control"));
  EXPECT_TRUE(data2.fieldSize("Control") == data1.fieldSize("Control"));
  EXPECT_TRUE(data2.fieldIndex("TLen") == data1.fieldIndex("TLen"));
}

// Zgodnosc deskryptorow idzie po SLOTACH PLASKICH rekordu, nie po wpisach: numeryczne
// `T[N]` i N pol `T` opisuja te same bajty pod tymi samymi offsetami. Do 2026-08-30
// porownanie szlo po wpisach, wiec ta para wychodzila NIEZGODNA i payload::operator=
// konczylo sie bledem krytycznym - tak wywracal sie przeplot `#` nad polem tablicowym.
TEST(descriptor, array_field_is_compatible_with_the_same_flat_scalar_fields) {
  const rdb::Descriptor arrayForm("cells", 4, 3, rdb::INTEGER);
  const auto scalarForm{rdb::Descriptor("c0", 4, 1, rdb::INTEGER) +  //
                        rdb::Descriptor("c1", 4, 1, rdb::INTEGER) +  //
                        rdb::Descriptor("c2", 4, 1, rdb::INTEGER)};

  ASSERT_EQ(arrayForm.flatElementCount(), scalarForm.flatElementCount());
  ASSERT_EQ(arrayForm.getSizeInBytes(), scalarForm.getSizeInBytes());
  EXPECT_TRUE(arrayForm == scalarForm);
  EXPECT_TRUE(scalarForm == arrayForm);
}

// Zluzowanie porownania nie moze uczynic go slepym: rozna szerokosc plaska to nadal
// niezgodnosc, niezaleznie od tego, ze liczba wpisow moglaby sie zgadzac.
TEST(descriptor, different_flat_width_stays_incompatible) {
  const rdb::Descriptor threeElements("cells", 4, 3, rdb::INTEGER);
  const rdb::Descriptor fourElements("cells", 4, 4, rdb::INTEGER);
  const rdb::Descriptor oneScalar("cells", 4, 1, rdb::INTEGER);

  EXPECT_FALSE(threeElements == fourElements);
  EXPECT_FALSE(fourElements == threeElements);
  EXPECT_FALSE(threeElements == oneScalar);
}

// STRING[N] jest JEDNYM slotem o dlugosci N bajtow, a nie N slotami - inaczej niz typy
// liczbowe. Ta sama regula co w Descriptor::rebuildFieldMappings().
TEST(descriptor, string_array_stays_one_flat_slot) {
  const rdb::Descriptor text("name", 1, 8, rdb::STRING);
  const auto eightBytes{rdb::Descriptor("b0", 1, 1, rdb::BYTE) +  //
                        rdb::Descriptor("b1", 1, 1, rdb::BYTE)};

  EXPECT_EQ(text.flatElementCount(), 1);
  EXPECT_FALSE(text == eightBytes);
}

// Przeniesiony deskryptor musi zostac SPOJNY: wektor pol jest pusty, wiec cache mapowan
// nie moze udawac poprzedniego ukladu. Wersja `= default` tej wlasnosci nie ma - przenosi
// wektory cache, ale kopiuje liczniki i flage brudnosci, wiec zrodlo raportowaloby sloty,
// ktorych juz nie ma, i indeksowalo puste fieldByteOffsets_.
TEST(descriptor, moved_from_descriptor_reports_an_empty_layout) {
  auto source{rdb::Descriptor("c0", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("c1", 4, 1, rdb::INTEGER)};

  EXPECT_EQ(source.flatElementCount(), 2);  // cache zbudowany PRZED przeniesieniem
  EXPECT_EQ(source.getSizeInBytes(), 8U);

  const rdb::Descriptor target(std::move(source));

  EXPECT_EQ(target.flatElementCount(), 2);
  EXPECT_EQ(target.getSizeInBytes(), 8U);
  EXPECT_EQ(source.flatElementCount(), 0);
  EXPECT_EQ(source.getSizeInBytes(), 0U);

  // To samo dla przypisania przenoszacego.
  auto secondSource{rdb::Descriptor("cells", 4, 3, rdb::INTEGER)};
  EXPECT_EQ(secondSource.flatElementCount(), 3);

  rdb::Descriptor assigned;
  assigned = std::move(secondSource);

  EXPECT_EQ(assigned.flatElementCount(), 3);
  EXPECT_EQ(assigned.getSizeInBytes(), 12U);
  EXPECT_EQ(secondSource.flatElementCount(), 0);
  EXPECT_EQ(secondSource.getSizeInBytes(), 0U);
}
