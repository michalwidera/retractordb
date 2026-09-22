#include <gtest/gtest.h>

#include <any>
#include <boost/rational.hpp>
#include <limits>
#include <string>
#include <utility>

#include "fldType.hpp"
#include "rdb/convertTypes.hpp"

// ── Rationalize ──────────────────────────────────────────────────────────────

TEST(Rationalize, zero) { EXPECT_EQ(Rationalize(0.0), boost::rational<int>(0, 1)); }
TEST(Rationalize, half) { EXPECT_EQ(Rationalize(0.5), boost::rational<int>(1, 2)); }
TEST(Rationalize, third) { EXPECT_EQ(Rationalize(1.0 / 3.0), boost::rational<int>(1, 3)); }
TEST(Rationalize, threequarters) { EXPECT_EQ(Rationalize(0.75), boost::rational<int>(3, 4)); }
TEST(Rationalize, whole_number) { EXPECT_EQ(Rationalize(3.0), boost::rational<int>(3, 1)); }

// --- wartosci UJEMNE ---
//
// Petla ulamka lancuchowego konwertowala `startx` na `unsigned int`. Dla ujemnego wejscia
// jest to zachowanie nieokreslone i architektury rozstrzygaly je roznie: x86-64 (cvttsd2si)
// bral mlodsze 32 bity i oddawal 4294967294 dla -2.0, arm64 (fcvtzu) nasycal do 0. Ta sama
// baza dawala wiec -2/1 na jednej maszynie i 0/1 na drugiej. Na obu bylo to zreszta zle,
// bo `diff = startx - val` liczylo sie na wartosci bez znaku: petla urywala sie po pierwszej
// cyfrze i ulamek nigdy nie powstawal - -2.5 dawalo -2/1, nie -5/2.
//
// Testy sa architektonicznie neutralne - ta sama oczekiwana wartosc na kazdej maszynie -
// wiec ich zadaniem jest padac na tej, ktora by sie wylamala.

TEST(Rationalize, negative_half) { EXPECT_EQ(Rationalize(-2.5), boost::rational<int>(-5, 2)); }
TEST(Rationalize, negative_whole_number) { EXPECT_EQ(Rationalize(-2.0), boost::rational<int>(-2, 1)); }
TEST(Rationalize, negative_third) { EXPECT_NEAR(boost::rational_cast<double>(Rationalize(-10.0 / 3.0)), -10.0 / 3.0, 1E-6); }
// Symetria znaku: rozwiniecie |x| daje p/q, a -|x| ma dac dokladnie -p/q.
TEST(Rationalize, sign_is_symmetric) {
  EXPECT_EQ(Rationalize(-1.0 / 3.0), -Rationalize(1.0 / 3.0));
  EXPECT_EQ(Rationalize(-0.75), -Rationalize(0.75));
  EXPECT_EQ(Rationalize(-1.5), -Rationalize(1.5));
}
TEST(Rationalize, negative_zero_is_zero) { EXPECT_EQ(Rationalize(-0.0), boost::rational<int>(0, 1)); }

// --- wejscie bez przyblizenia wymiernego ---
//
// Rationalize zwraca boost::rational<int> przez wartosc, wiec NULL-a nie ma czym wyrazic:
// NaN, nieskonczonosc i wartosc poza zakresem `int` oddaja {0,1}. NULL stoi o poziom wyzej,
// na sciezce konwersji (ponizej: cast_variant / cast_any do RATIONAL i INTPAIR).

TEST(Rationalize, nan_is_zero) { EXPECT_EQ(Rationalize(std::numeric_limits<double>::quiet_NaN()), boost::rational<int>(0, 1)); }
TEST(Rationalize, infinity_is_zero) {
  EXPECT_EQ(Rationalize(std::numeric_limits<double>::infinity()), boost::rational<int>(0, 1));
  EXPECT_EQ(Rationalize(-std::numeric_limits<double>::infinity()), boost::rational<int>(0, 1));
}
TEST(Rationalize, above_integer_range_is_zero) {
  EXPECT_EQ(Rationalize(1e30), boost::rational<int>(0, 1));
  EXPECT_EQ(Rationalize(-1e30), boost::rational<int>(0, 1));
}
// Granica jest ta sama co w narrowFloatTo - 2^31 wylacznie, sprawdzana na wartosci obcietej.
TEST(Rationalize, at_integer_bound_is_exact) {
  EXPECT_EQ(Rationalize(2147483647.0), boost::rational<int>(2147483647, 1));
  EXPECT_EQ(Rationalize(-2147483647.0), boost::rational<int>(-2147483647, 1));
}
// Jedyna wartosc, ktora na wyniesieniu znaku traci: -2^31. |INT_MIN| to 2^31, wiec wypada za
// gorna granice i wychodzi {0,1}, choc sam INT_MIN da sie w `int` zapisac. Cena jest swiadoma:
// droga przez wartosc bezwzgledna wymagalaby tu negacji INT_MIN, czyli nowego przepelnienia w
// miejsce usunietego zachowania nieokreslonego. `boost::rational<int>` z licznikiem INT_MIN
// jest zreszta nie do uzycia dalej - kazda negacja takiego ulamka przepelnia sie tak samo.
TEST(Rationalize, at_negative_integer_bound_is_zero) { EXPECT_EQ(Rationalize(-2147483648.0), boost::rational<int>(0, 1)); }

// ── nullFallbackValue ─────────────────────────────────────────────────────────

TEST(nullFallbackValue, byte) { EXPECT_EQ(std::get<uint8_t>(nullFallbackValue(rdb::BYTE)), 0); }
TEST(nullFallbackValue, integer) { EXPECT_EQ(std::get<int>(nullFallbackValue(rdb::INTEGER)), 0); }
TEST(nullFallbackValue, uint) { EXPECT_EQ(std::get<unsigned>(nullFallbackValue(rdb::UINT)), 0U); }
TEST(nullFallbackValue, rational) {
  EXPECT_EQ(std::get<boost::rational<int>>(nullFallbackValue(rdb::RATIONAL)), boost::rational<int>(0, 1));
}
TEST(nullFallbackValue, float_type) { EXPECT_EQ(std::get<float>(nullFallbackValue(rdb::FLOAT)), 0.0F); }
TEST(nullFallbackValue, double_type) { EXPECT_EQ(std::get<double>(nullFallbackValue(rdb::DOUBLE)), 0.0); }
TEST(nullFallbackValue, intpair) {
  using P  = std::pair<int, int>;
  P result = std::get<P>(nullFallbackValue(rdb::INTPAIR));
  P expected{0, 0};
  EXPECT_EQ(result, expected);
}
TEST(nullFallbackValue, idxpair) {
  using P  = std::pair<std::string, int>;
  P result = std::get<P>(nullFallbackValue(rdb::IDXPAIR));
  P expected{"", 0};
  EXPECT_EQ(result, expected);
}
TEST(nullFallbackValue, string) { EXPECT_EQ(std::get<std::string>(nullFallbackValue(rdb::STRING)), ""); }
TEST(nullFallbackValue, nulltype) { EXPECT_TRUE(std::holds_alternative<std::monostate>(nullFallbackValue(rdb::NULLTYPE))); }

// ── any_to_variant_cast ───────────────────────────────────────────────────────

TEST(any_to_variant_cast, empty_any_returns_monostate) {
  EXPECT_TRUE(std::holds_alternative<std::monostate>(any_to_variant_cast(std::any{})));
}
TEST(any_to_variant_cast, monostate_input) {
  EXPECT_TRUE(std::holds_alternative<std::monostate>(any_to_variant_cast(std::any(std::monostate{}))));
}
TEST(any_to_variant_cast, int_input) {
  auto result = any_to_variant_cast(std::any(42));
  EXPECT_EQ(std::get<int>(result), 42);
}
TEST(any_to_variant_cast, uint8_input) {
  auto result = any_to_variant_cast(std::any(uint8_t(7)));
  EXPECT_EQ(std::get<uint8_t>(result), 7);
}
TEST(any_to_variant_cast, unsigned_input) {
  auto result = any_to_variant_cast(std::any(unsigned(99)));
  EXPECT_EQ(std::get<unsigned>(result), 99U);
}
TEST(any_to_variant_cast, double_input) {
  auto result = any_to_variant_cast(std::any(3.14));
  EXPECT_DOUBLE_EQ(std::get<double>(result), 3.14);
}
TEST(any_to_variant_cast, float_input) {
  auto result = any_to_variant_cast(std::any(1.5F));
  EXPECT_FLOAT_EQ(std::get<float>(result), 1.5F);
}
TEST(any_to_variant_cast, string_input) {
  auto result = any_to_variant_cast(std::any(std::string("hello")));
  EXPECT_EQ(std::get<std::string>(result), "hello");
}
TEST(any_to_variant_cast, rational_input) {
  auto result = any_to_variant_cast(std::any(boost::rational<int>(1, 3)));
  EXPECT_EQ(std::get<boost::rational<int>>(result), boost::rational<int>(1, 3));
}
TEST(any_to_variant_cast, unsupported_type_throws) {
  EXPECT_THROW(any_to_variant_cast(std::any(std::vector<int>{1, 2})), std::bad_any_cast);
}

// ── cast<descFldVT> - NULLTYPE ────────────────────────────────────────────────

TEST(cast_variant, nulltype_returns_monostate) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 5;
  auto result       = c(in, rdb::NULLTYPE);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// ── cast<descFldVT> - null input fallback ────────────────────────────────────

TEST(cast_variant, monostate_input_returns_fallback_for_integer) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::monostate{};
  auto result       = c(in, rdb::INTEGER);
  EXPECT_EQ(std::get<int>(result), 0);
}

// ── cast<descFldVT> - numeric conversions ────────────────────────────────────

TEST(cast_variant, int_to_byte) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 200;
  EXPECT_EQ(std::get<uint8_t>(c(in, rdb::BYTE)), uint8_t(200));
}
TEST(cast_variant, double_to_integer) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 3.9;
  EXPECT_EQ(std::get<int>(c(in, rdb::INTEGER)), 3);
}
TEST(cast_variant, float_to_double) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 1.5F;
  EXPECT_DOUBLE_EQ(std::get<double>(c(in, rdb::DOUBLE)), double(1.5F));
}
TEST(cast_variant, uint8_to_uint) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = uint8_t{42};
  EXPECT_EQ(std::get<unsigned>(c(in, rdb::UINT)), 42U);
}
TEST(cast_variant, string_numeric_to_integer) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::string("123");
  EXPECT_EQ(std::get<int>(c(in, rdb::INTEGER)), 123);
}
TEST(cast_variant, string_float_to_double) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::string("2.5");
  EXPECT_DOUBLE_EQ(std::get<double>(c(in, rdb::DOUBLE)), 2.5);
}
TEST(cast_variant, rational_to_float) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = boost::rational<int>(1, 2);
  EXPECT_FLOAT_EQ(std::get<float>(c(in, rdb::FLOAT)), 0.5F);
}

// Zaokraglenie przy rzutowaniu na INTEGER - OBCIECIE W STRONE ZERA, nie podloga.
// Regula jest udokumentowana (operatory-agregujace.md, sekcja "Zaokraglenie") i korpus
// UC04/UC06/UC08 opiera na niej swoje modele w Pythonie, gdzie `//` PODLOGUJE. Roznica
// widac tylko na wartosciach ujemnych, wiec bez tych przypadkow zmiana rational_cast na
// floor przeszlaby przez cala baterie niezauwazona.
TEST(cast_variant, rational_to_integer_truncates_toward_zero) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT positive      = boost::rational<int>(8, 3);
  rdb::descFldVT negative      = boost::rational<int>(-8, 3);
  rdb::descFldVT negativeSmall = boost::rational<int>(-4, 3);

  EXPECT_EQ(std::get<int>(c(positive, rdb::INTEGER)), 2);
  EXPECT_EQ(std::get<int>(c(negative, rdb::INTEGER)), -2);       // podloga dalaby -3
  EXPECT_EQ(std::get<int>(c(negativeSmall, rdb::INTEGER)), -1);  // podloga dalaby -2
}

// Ta sama regula dla argumentu zmiennoprzecinkowego - to_integer nie rozroznia zrodla.
TEST(cast_variant, double_to_integer_truncates_toward_zero) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -2.6666666666666665;
  EXPECT_EQ(std::get<int>(c(in, rdb::INTEGER)), -2);  // podloga dalaby -3
}

// ── cast<descFldVT> - STRING ──────────────────────────────────────────────────

TEST(cast_variant, int_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 42;
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "42");
}
TEST(cast_variant, double_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 1.0;
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), std::to_string(1.0));
}
TEST(cast_variant, string_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::string("hello");
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "hello");
}
TEST(cast_variant, intpair_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::make_pair(3, 4);
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "3,4");
}
TEST(cast_variant, idxpair_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::make_pair(std::string("key"), 7);
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "key,7");
}
TEST(cast_variant, rational_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = boost::rational<int>(2, 3);
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "2/3");
}

// ── cast<descFldVT> - RATIONAL ───────────────────────────────────────────────

TEST(cast_variant, int_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 5;
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(5));
}
TEST(cast_variant, double_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 0.5;
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(1, 2));
}
TEST(cast_variant, intpair_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::make_pair(1, 3);
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(1, 3));
}
TEST(cast_variant, string_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::string("3/4");
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(3, 4));
}

// ── cast<descFldVT> - INTPAIR ────────────────────────────────────────────────

TEST(cast_variant, int_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 7;
  P result          = std::get<P>(c(in, rdb::INTPAIR));
  P expected{0, 7};
  EXPECT_EQ(result, expected);
}
TEST(cast_variant, double_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 0.5;
  P result          = std::get<P>(c(in, rdb::INTPAIR));
  P expected{1, 2};
  EXPECT_EQ(result, expected);
}
TEST(cast_variant, intpair_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = P{2, 5};
  P result          = std::get<P>(c(in, rdb::INTPAIR));
  P expected{2, 5};
  EXPECT_EQ(result, expected);
}
TEST(cast_variant, string_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::string("3,7");
  P result          = std::get<P>(c(in, rdb::INTPAIR));
  P expected{3, 7};
  EXPECT_EQ(result, expected);
}

// ── cast<std::any> - numeric conversions ─────────────────────────────────────

TEST(cast_any, nulltype_returns_monostate) {
  cast<std::any> c;
  std::any in = 5;
  auto result = c(in, rdb::NULLTYPE);
  EXPECT_EQ(result.type(), typeid(std::monostate));
}
TEST(cast_any, monostate_input_returns_integer_fallback) {
  cast<std::any> c;
  std::any in = std::monostate{};
  auto result = c(in, rdb::INTEGER);
  EXPECT_EQ(std::any_cast<int>(result), 0);
}
TEST(cast_any, int_to_double) {
  cast<std::any> c;
  std::any in = 3;
  EXPECT_DOUBLE_EQ(std::any_cast<double>(c(in, rdb::DOUBLE)), 3.0);
}
TEST(cast_any, double_to_integer) {
  cast<std::any> c;
  std::any in = 4.9;
  EXPECT_EQ(std::any_cast<int>(c(in, rdb::INTEGER)), 4);
}
TEST(cast_any, string_numeric_to_integer) {
  cast<std::any> c;
  std::any in = std::string("77");
  EXPECT_EQ(std::any_cast<int>(c(in, rdb::INTEGER)), 77);
}
TEST(cast_any, int_to_string) {
  cast<std::any> c;
  std::any in = 9;
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "9");
}
TEST(cast_any, rational_to_string) {
  cast<std::any> c;
  std::any in = boost::rational<int>(1, 4);
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "1/4");
}
TEST(cast_any, double_to_rational) {
  cast<std::any> c;
  std::any in = 0.75;
  EXPECT_EQ(std::any_cast<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(3, 4));
}
TEST(cast_any, string_to_rational) {
  cast<std::any> c;
  std::any in = std::string("2/5");
  EXPECT_EQ(std::any_cast<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(2, 5));
}
TEST(cast_any, int_to_intpair) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = 8;
  P result    = std::any_cast<P>(c(in, rdb::INTPAIR));
  P expected{0, 8};
  EXPECT_EQ(result, expected);
}
TEST(cast_any, string_to_intpair) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = std::string("5,6");
  P result    = std::any_cast<P>(c(in, rdb::INTPAIR));
  P expected{5, 6};
  EXPECT_EQ(result, expected);
}
TEST(cast_any, intpair_to_intpair) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = P{3, 9};
  P result    = std::any_cast<P>(c(in, rdb::INTPAIR));
  P expected{3, 9};
  EXPECT_EQ(result, expected);
}
TEST(cast_any, double_to_intpair) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = 0.5;
  P result    = std::any_cast<P>(c(in, rdb::INTPAIR));
  P expected{1, 2};
  EXPECT_EQ(result, expected);
}
TEST(cast_any, rational_to_intpair) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = boost::rational<int>(3, 5);
  P result    = std::any_cast<P>(c(in, rdb::INTPAIR));
  P expected{3, 5};
  EXPECT_EQ(result, expected);
}
TEST(cast_any, intpair_to_string) {
  using P = std::pair<int, int>;
  cast<std::any> c;
  std::any in = P{2, 7};
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "2,7");
}
TEST(cast_any, idxpair_to_string) {
  using P = std::pair<std::string, int>;
  cast<std::any> c;
  std::any in = P{"abc", 3};
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "abc,3");
}

// --- zwezenie float/double -> typ calkowity POZA ZAKRESEM ---
//
// `static_cast<int>(1e30)` jest w C++ zachowaniem nieokreslonym i architektury rozstrzygaja
// je roznie: x86-64 (cvttsd2si) oddaje INT_MIN, arm64 (fcvtzs) nasyca do INT_MAX; dla NaN
// odpowiednio INT_MIN i 0. Baza dawala wiec na dwoch maszynach rozne liczby, bez bledu i bez
// sladu w logu. narrowFloatTo() w convertTypes.cc przypina regule: wartosc, ktorej typ
// docelowy nie pomiesci (NaN i nieskonczonosc wlacznie), daje NULL (std::monostate).
//
// Testy sa architektonicznie neutralne - to ta sama oczekiwana wartosc na kazdej maszynie -
// wiec ich zadaniem jest padac na tej, ktora by sie wylamala.

TEST(cast_variant, double_above_integer_range_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 1e30;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::INTEGER)));
}
TEST(cast_variant, double_below_integer_range_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -1e30;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::INTEGER)));
}
TEST(cast_variant, double_infinity_to_integer_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT plus  = std::numeric_limits<double>::infinity();
  rdb::descFldVT minus = -std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(plus, rdb::INTEGER)));
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(minus, rdb::INTEGER)));
}
TEST(cast_variant, double_nan_to_integer_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::INTEGER)));
}
// Granica DOKLADNA: INT_MAX i INT_MIN da sie zapisac w double, wiec maja przejsc bez zmiany,
// a nie wpasc w NULL o jeden krok za wczesnie.
TEST(cast_variant, double_at_integer_bounds_is_exact) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT hi = static_cast<double>(std::numeric_limits<int>::max());
  rdb::descFldVT lo = static_cast<double>(std::numeric_limits<int>::lowest());
  EXPECT_EQ(std::get<int>(c(hi, rdb::INTEGER)), std::numeric_limits<int>::max());
  EXPECT_EQ(std::get<int>(c(lo, rdb::INTEGER)), std::numeric_limits<int>::lowest());
}
// Pierwsza liczba calkowita za kazda granica jest juz NULL-em.
TEST(cast_variant, double_one_past_integer_bounds_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT hi = 2147483648.0;
  rdb::descFldVT lo = -2147483649.0;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(hi, rdb::INTEGER)));
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(lo, rdb::INTEGER)));
}
// Zakres sprawdza sie na wartosci OBCIETEJ w strone zera, tak jak konwertuje jezyk: czesc
// ulamkowa za granica nie wyprowadza wartosci poza zakres.
TEST(cast_variant, fraction_past_integer_bounds_truncates_into_range) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT hi = 2147483647.5;
  rdb::descFldVT lo = -2147483648.5;
  EXPECT_EQ(std::get<int>(c(hi, rdb::INTEGER)), std::numeric_limits<int>::max());
  EXPECT_EQ(std::get<int>(c(lo, rdb::INTEGER)), std::numeric_limits<int>::lowest());
}
// FLOAT nie umie zapisac INT_MAX - `static_cast<float>(INT_MAX)` to 2^31, juz poza zakresem.
// Najwieksza liczba FLOAT ponizej 2^31 to 2147483520 i ta ma przejsc bez zmiany; 2^31 nie.
TEST(cast_variant, float_just_below_integer_range_is_exact) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 2147483520.0F;
  EXPECT_EQ(std::get<int>(c(in, rdb::INTEGER)), 2147483520);
}
TEST(cast_variant, float_at_2p31_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 2147483648.0F;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::INTEGER)));
}
TEST(cast_variant, negative_double_to_uint_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -1.0;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::UINT)));
}
// -0.5 obcina sie do 0, a 0 miesci sie w UINT.
TEST(cast_variant, negative_fraction_to_uint_truncates_to_zero) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -0.5;
  EXPECT_EQ(std::get<unsigned>(c(in, rdb::UINT)), 0U);
}
TEST(cast_variant, double_above_uint_range_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 5e9;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::UINT)));
}
// Zmiana zachowania takze na arm64: surowy rzut zwezal 300.0 do 8 bitow (44), a nasycenie
// z a0082a34 dawalo 255. Obie liczby byly zmyslone; teraz jest NULL.
TEST(cast_variant, double_above_byte_range_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 300.0;
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::BYTE)));
}
TEST(cast_variant, double_at_byte_bound_is_exact) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = 255.9;
  EXPECT_EQ(std::get<uint8_t>(c(in, rdb::BYTE)), 255);
}
TEST(cast_any, double_above_integer_range_is_null) {
  cast<std::any> c;
  std::any in = 1e30;
  EXPECT_EQ(c(in, rdb::INTEGER).type(), typeid(std::monostate));
}

// --- zwezenie float/double -> RATIONAL / INTPAIR bez reprezentacji ---
//
// Ta sama regula co dla typow calkowitych wyzej: wartosc, ktorej typ docelowy nie pomiesci,
// daje NULL. NaN i nieskonczonosc nie maja przyblizenia wymiernego, wiec ida jako monostate
// zamiast jako {0,1} - zero w polu bylo liczba, ktorej nikt nie policzyl.

TEST(cast_variant, double_nan_to_rational_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::RATIONAL)));
}
TEST(cast_variant, double_infinity_to_rational_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT plus  = std::numeric_limits<double>::infinity();
  rdb::descFldVT minus = -std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(plus, rdb::RATIONAL)));
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(minus, rdb::RATIONAL)));
}
TEST(cast_variant, float_nan_to_rational_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::numeric_limits<float>::quiet_NaN();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::RATIONAL)));
}
TEST(cast_variant, double_nan_to_intpair_is_null) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::holds_alternative<std::monostate>(c(in, rdb::INTPAIR)));
}
TEST(cast_any, double_infinity_to_rational_is_null) {
  cast<std::any> c;
  std::any in = std::numeric_limits<double>::infinity();
  EXPECT_EQ(c(in, rdb::RATIONAL).type(), typeid(std::monostate));
}
TEST(cast_any, double_nan_to_intpair_is_null) {
  cast<std::any> c;
  std::any in = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(c(in, rdb::INTPAIR).type(), typeid(std::monostate));
}
// Wartosc skonczona ma przejsc ta sama droga bez zmiany - takze ujemna.
TEST(cast_variant, negative_double_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -2.5;
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(-5, 2));
}
TEST(cast_variant, negative_double_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = -2.5;
  P expected{-5, 2};
  EXPECT_EQ(std::get<P>(c(in, rdb::INTPAIR)), expected);
}
