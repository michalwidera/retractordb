#include <gtest/gtest.h>

#include <algorithm>
#include <any>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "rdb/convertTypes.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"

// Tests intentionally access optionals and raw buffers directly to validate low-level payload behavior.
// NOLINTBEGIN(bugprone-unchecked-optional-access,modernize-avoid-c-arrays)

static_assert(std::is_same_v<decltype(std::declval<rdb::payload &>().span()), std::span<uint8_t>>);
static_assert(std::is_same_v<decltype(std::declval<const rdb::payload &>().span()), std::span<const uint8_t>>);

TEST(payload, position_conversion_case_3_with_payload) {
  auto desc1{rdb::Descriptor("ByteW", 1, 1, rdb::BYTE) +    //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER) +  //
             rdb::Descriptor("Name", 1, 10, rdb::STRING)};

  rdb::payload payload(desc1);

  payload.setItem(0, 145);
  payload.setItem(1, static_cast<uint8_t>(24));
  payload.setItem(2, static_cast<uint8_t>(25));
  payload.setItem(3, static_cast<uint8_t>(26));
  payload.setItem(4, 2000);
  payload.setItem(5, std::string("test"));

  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(0).value()) == 145);  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(1).value()) == 24);   // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(2).value()) == 25);   // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(3).value()) == 26);   // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_TRUE(std::any_cast<int>(payload.getItem(4).value()) == 2000);     // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_TRUE(std::any_cast<std::string>(payload.getItem(5).value()) ==
              std::string("test"));  // NOLINT(bugprone-unchecked-optional-access)

  std::stringstream coutstring;
  coutstring << rdb::singleLineFormat << payload;
  EXPECT_TRUE("{ ByteW:145 Control:24 25 26 TLen:2000 Name:test }" == coutstring.str());
}

TEST(payload, null_can_be_cleared_by_writing_value) {
  auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  rdb::payload payload(desc);

  payload.setItem(0, std::nullopt);
  EXPECT_FALSE(payload.getItem(0).has_value());

  payload.setItem(0, 123);
  ASSERT_TRUE(payload.getItem(0).has_value());
  EXPECT_EQ(std::any_cast<int>(payload.getItem(0).value()), 123);
}

TEST(payload, nullopt_writes_integer_fallback_to_memory) {
  auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  rdb::payload payload(desc);

  payload.setItem(0, 123456);
  payload.setItem(0, std::nullopt);

  int raw = -1;
  std::memcpy(&raw, payload.span().data(), sizeof(int));
  EXPECT_EQ(raw, 0);
  EXPECT_FALSE(payload.getItem(0).has_value());
}

TEST(payload, nullopt_writes_string_fallback_to_memory) {
  auto desc = rdb::Descriptor("name", 1, 6, rdb::STRING);
  rdb::payload payload(desc);

  payload.setItem(0, std::string("abcde"));
  payload.setItem(0, std::nullopt);

  auto bytes = payload.span().subspan(0, 6);
  EXPECT_TRUE(std::all_of(bytes.begin(), bytes.end(), [](uint8_t b) { return b == 0; }));
  EXPECT_FALSE(payload.getItem(0).has_value());
}

TEST(payload, null_type_round_trips_as_monostate) {
  auto desc = rdb::Descriptor("nothing", 0, 1, rdb::NULLTYPE);
  rdb::payload payload(desc);

  auto value = payload.getItem(0);
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value->type(), typeid(std::monostate));

  payload.setItem(0, std::any(std::monostate{}));
  auto rewritten = payload.getItem(0);
  ASSERT_TRUE(rewritten.has_value());
  EXPECT_EQ(rewritten->type(), typeid(std::monostate));

  std::stringstream out;
  out << rdb::singleLineFormat << payload;
  EXPECT_EQ(out.str(), "{ nothing:null }");
}

TEST(payload, add_preserves_null_flags) {
  auto leftDesc  = rdb::Descriptor("a", 4, 1, rdb::INTEGER);
  auto rightDesc = rdb::Descriptor("b", 4, 1, rdb::INTEGER);

  rdb::payload left(leftDesc);
  rdb::payload right(rightDesc);

  left.setItem(0, std::nullopt);
  right.setItem(0, 777);

  auto merged = left + right;

  EXPECT_FALSE(merged.getItem(0).has_value());
  ASSERT_TRUE(merged.getItem(1).has_value());
  EXPECT_EQ(std::any_cast<int>(merged.getItem(1).value()), 777);
}

TEST(payload, add_operator) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("ll", 4, 1, rdb::INTEGER) +    //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  union dataPayload {
    uint8_t ptr[19];
    struct __attribute__((packed)) {
      char Name[10];    // 10
      uint8_t Control;  // 1
      int ll;           // 4
      int TLen;         // 4
    };
  };

  auto data2{rdb::Descriptor("TLen2", 4, 1, rdb::INTEGER)};

  rdb::payload data1Payload(data1);

  data1Payload.setItem(0, std::string("test"));
  data1Payload.setItem(1, static_cast<uint8_t>(24));
  data1Payload.setItem(2, 2000);
  data1Payload.setItem(3, 3333);

  std::any Name_    = data1Payload.getItem(0).value();
  std::any Control_ = data1Payload.getItem(1).value();
  std::any ll_      = data1Payload.getItem(2).value();
  std::any TLen_    = data1Payload.getItem(3).value();

  dataPayload var;

  std::memcpy(&var, data1Payload.span().data(), 19);

  EXPECT_TRUE(var.ll == std::any_cast<int>(ll_));
  EXPECT_TRUE(var.TLen == std::any_cast<int>(TLen_));

  rdb::payload data2Payload(data2);
  data2Payload.setItem(0, 4004);

  EXPECT_TRUE(std::any_cast<int>(data1Payload.getItem(2).value()) == 2000);

  rdb::payload data3Payload;

  data3Payload = data1Payload + data2Payload;

  EXPECT_TRUE(std::any_cast<std::string>(data3Payload.getItem(0).value()) == "test");
  EXPECT_TRUE(std::any_cast<uint8_t>(data3Payload.getItem(1).value()) == 24);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(2).value()) == 2000);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(3).value()) == 3333);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(4).value()) == 4004);
}

TEST(payload, operator_ostream_emits_null_for_null_field) {
  auto desc = rdb::Descriptor("x", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);

  std::stringstream out;
  out << rdb::singleLineFormat << p;
  EXPECT_EQ(out.str(), "{ x:null }");
}

TEST(payload, operator_ostream_mixed_null_and_value) {
  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("b", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);
  p.setItem(1, 42);

  std::stringstream out;
  out << rdb::singleLineFormat << p;
  EXPECT_EQ(out.str(), "{ a:null b:42 }");
}

TEST(payload, get_and_set_null_bitset_round_trips) {
  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("b", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);

  p.setItem(0, 10);
  p.setItem(1, 20);

  const std::vector<bool> nulls = {true, false};
  p.setNullBitset(nulls);

  EXPECT_EQ(p.getNullBitset(), nulls);
  EXPECT_FALSE(p.getItem(0).has_value());
  ASSERT_TRUE(p.getItem(1).has_value());
  EXPECT_EQ(std::any_cast<int>(p.getItem(1).value()), 20);
}

TEST(payload, copy_constructor_preserves_null_bitset) {
  auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  rdb::payload original(desc);
  original.setItem(0, std::nullopt);

  rdb::payload copy(original);

  EXPECT_FALSE(copy.getItem(0).has_value());
  EXPECT_EQ(copy.getNullBitset(), original.getNullBitset());
}

TEST(payload, copy_assignment_preserves_null_bitset) {
  auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  rdb::payload original(desc);
  original.setItem(0, std::nullopt);

  rdb::payload assigned;
  assigned = original;

  EXPECT_FALSE(assigned.getItem(0).has_value());
  EXPECT_EQ(assigned.getNullBitset(), original.getNullBitset());
}

TEST(payload, operator_ostream_null_string_field) {
  auto desc = rdb::Descriptor("name", 1, 8, rdb::STRING);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);

  std::stringstream out;
  out << rdb::singleLineFormat << p;
  EXPECT_EQ(out.str(), "{ name:null }");
}

TEST(payload, operator_istream_clears_null_flag_for_integer_field) {
  auto desc = rdb::Descriptor("value", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);
  EXPECT_FALSE(p.getItem(0).has_value());

  std::stringstream in;
  in << "value 123";
  in >> p;

  ASSERT_TRUE(p.getItem(0).has_value());
  EXPECT_EQ(std::any_cast<int>(p.getItem(0).value()), 123);
  EXPECT_EQ(p.getNullBitset(), std::vector<bool>({false}));
}

TEST(payload, operator_istream_clears_null_flag_for_string_field) {
  auto desc = rdb::Descriptor("name", 1, 8, rdb::STRING);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);
  EXPECT_FALSE(p.getItem(0).has_value());

  std::stringstream in;
  in << "name abc";
  in >> p;

  ASSERT_TRUE(p.getItem(0).has_value());
  EXPECT_EQ(std::any_cast<std::string>(p.getItem(0).value()), "abc");
  EXPECT_EQ(p.getNullBitset(), std::vector<bool>({false}));
}

TEST(payload, stream_operators_support_rational_fields) {
  auto desc = rdb::Descriptor("ratio", static_cast<int>(sizeof(boost::rational<int>)), 1, rdb::RATIONAL);
  rdb::payload p(desc);

  std::stringstream in;
  in << "ratio 3/4";
  in >> p;

  ASSERT_TRUE(p.getItem(0).has_value());
  EXPECT_EQ(std::any_cast<boost::rational<int>>(p.getItem(0).value()), boost::rational<int>(3, 4));

  std::stringstream out;
  out << rdb::singleLineFormat << p;
  EXPECT_EQ(out.str(), "{ ratio:3/4 }");
}

// P1-E0 (speed_improvement): parytet wariantowego interfejsu getItemVT/setItemVT
// z any-owym getItem/setItem. getItemVT musi dawac dokladnie any_to_variant_cast(
// getItem), a setItemVT musi zapisac te same bajty co setItem. To bramka
// bezpieczenstwa przed przelaczeniem goracych sciezek na interfejs wariantowy.
TEST(payload, vt_interface_roundtrip_matches_any_interface) {
  auto desc = rdb::Descriptor("b", 1, 1, rdb::BYTE) +      //
              rdb::Descriptor("i", 4, 1, rdb::INTEGER) +   //
              rdb::Descriptor("u", 4, 1, rdb::UINT) +      //
              rdb::Descriptor("d", 8, 1, rdb::DOUBLE) +    //
              rdb::Descriptor("f", 4, 1, rdb::FLOAT) +     //
              rdb::Descriptor("r", 8, 1, rdb::RATIONAL) +  //
              rdb::Descriptor("s", 1, 8, rdb::STRING);

  // 1. Zapis any-owy, odczyt oboma -> getItemVT == any_to_variant_cast(getItem).
  rdb::payload a(desc);
  a.setItem(0, static_cast<uint8_t>(200));
  a.setItem(1, -12345);
  a.setItem(2, 54321U);
  a.setItem(3, 3.14159);
  a.setItem(4, 2.5F);
  a.setItem(5, boost::rational<int>(7, 3));
  a.setItem(6, std::string("hello"));

  for (int i = 0; i <= 6; ++i) {
    ASSERT_TRUE(a.getItem(i).has_value());
    ASSERT_TRUE(a.getItemVT(i).has_value());
    EXPECT_EQ(a.getItemVT(i).value(), any_to_variant_cast(a.getItem(i).value())) << "pole " << i;
  }

  // 2. Zapis wariantowy tych samych wartosci -> te same bajty i null-bitset co any-owy.
  rdb::payload v(desc);
  v.setItemVT(0, rdb::descFldVT{static_cast<uint8_t>(200)});
  v.setItemVT(1, rdb::descFldVT{-12345});
  v.setItemVT(2, rdb::descFldVT{54321U});
  v.setItemVT(3, rdb::descFldVT{3.14159});
  v.setItemVT(4, rdb::descFldVT{2.5F});
  v.setItemVT(5, rdb::descFldVT{boost::rational<int>(7, 3)});
  v.setItemVT(6, rdb::descFldVT{std::string("hello")});

  ASSERT_EQ(v.span().size(), a.span().size());
  EXPECT_TRUE(std::equal(a.span().begin(), a.span().end(), v.span().begin()));
  EXPECT_EQ(v.getNullBitset(), a.getNullBitset());
}

TEST(payload, vt_interface_null_matches_any_interface) {
  auto desc = rdb::Descriptor("i", 4, 1, rdb::INTEGER) + rdb::Descriptor("s", 1, 6, rdb::STRING);

  rdb::payload a(desc);
  a.setItem(0, std::nullopt);
  a.setItem(1, std::nullopt);

  rdb::payload v(desc);
  v.setItemVT(0, std::nullopt);
  v.setItemVT(1, std::nullopt);

  // nullopt: oba odczyty zwracaja brak wartosci, a fallback-bajty i null-bitset identyczne.
  EXPECT_FALSE(v.getItemVT(0).has_value());
  EXPECT_FALSE(v.getItemVT(1).has_value());
  EXPECT_FALSE(v.getItem(0).has_value());
  EXPECT_EQ(v.getNullBitset(), a.getNullBitset());
  ASSERT_EQ(v.span().size(), a.span().size());
  EXPECT_TRUE(std::equal(a.span().begin(), a.span().end(), v.span().begin()));
}

// NOLINTEND(bugprone-unchecked-optional-access,modernize-avoid-c-arrays)
