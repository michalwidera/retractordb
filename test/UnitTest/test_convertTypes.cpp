#include <gtest/gtest.h>

#include <any>
#include <boost/rational.hpp>
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

// ── cast<descFldVT> — NULLTYPE ────────────────────────────────────────────────

TEST(cast_variant, nulltype_returns_monostate) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = int{5};
  auto result       = c(in, rdb::NULLTYPE);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
}

// ── cast<descFldVT> — null input fallback ────────────────────────────────────

TEST(cast_variant, monostate_input_returns_fallback_for_integer) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = std::monostate{};
  auto result       = c(in, rdb::INTEGER);
  EXPECT_EQ(std::get<int>(result), 0);
}

// ── cast<descFldVT> — numeric conversions ────────────────────────────────────

TEST(cast_variant, int_to_byte) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = int{200};
  EXPECT_EQ(std::get<uint8_t>(c(in, rdb::BYTE)), uint8_t(200));
}
TEST(cast_variant, double_to_integer) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = double{3.9};
  EXPECT_EQ(std::get<int>(c(in, rdb::INTEGER)), 3);
}
TEST(cast_variant, float_to_double) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = float{1.5F};
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

// ── cast<descFldVT> — STRING ──────────────────────────────────────────────────

TEST(cast_variant, int_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = int{42};
  EXPECT_EQ(std::get<std::string>(c(in, rdb::STRING)), "42");
}
TEST(cast_variant, double_to_string) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = double{1.0};
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

// ── cast<descFldVT> — RATIONAL ───────────────────────────────────────────────

TEST(cast_variant, int_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = int{5};
  EXPECT_EQ(std::get<boost::rational<int>>(c(in, rdb::RATIONAL)), boost::rational<int>(5));
}
TEST(cast_variant, double_to_rational) {
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = double{0.5};
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

// ── cast<descFldVT> — INTPAIR ────────────────────────────────────────────────

TEST(cast_variant, int_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = int{7};
  P result          = std::get<P>(c(in, rdb::INTPAIR));
  P expected{0, 7};
  EXPECT_EQ(result, expected);
}
TEST(cast_variant, double_to_intpair) {
  using P = std::pair<int, int>;
  cast<rdb::descFldVT> c;
  rdb::descFldVT in = double{0.5};
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

// ── cast<std::any> — numeric conversions ─────────────────────────────────────

TEST(cast_any, nulltype_returns_monostate) {
  cast<std::any> c;
  std::any in = int{5};
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
  std::any in = int{3};
  EXPECT_DOUBLE_EQ(std::any_cast<double>(c(in, rdb::DOUBLE)), 3.0);
}
TEST(cast_any, double_to_integer) {
  cast<std::any> c;
  std::any in = double{4.9};
  EXPECT_EQ(std::any_cast<int>(c(in, rdb::INTEGER)), 4);
}
TEST(cast_any, string_numeric_to_integer) {
  cast<std::any> c;
  std::any in = std::string("77");
  EXPECT_EQ(std::any_cast<int>(c(in, rdb::INTEGER)), 77);
}
TEST(cast_any, int_to_string) {
  cast<std::any> c;
  std::any in = int{9};
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "9");
}
TEST(cast_any, rational_to_string) {
  cast<std::any> c;
  std::any in = boost::rational<int>(1, 4);
  EXPECT_EQ(std::any_cast<std::string>(c(in, rdb::STRING)), "1/4");
}
TEST(cast_any, double_to_rational) {
  cast<std::any> c;
  std::any in = double{0.75};
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
  std::any in = int{8};
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
  std::any in = double{0.5};
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
