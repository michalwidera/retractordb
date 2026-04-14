#include <gtest/gtest.h>

#include <any>
#include <cstring>
#include <sstream>
#include <string>

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"

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

  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(0).value()) == 145);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(1).value()) == 24);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(2).value()) == 25);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(3).value()) == 26);
  EXPECT_TRUE(std::any_cast<int>(payload.getItem(4).value()) == 2000);
  EXPECT_TRUE(std::any_cast<std::string>(payload.getItem(5).value()).c_str() == std::string("test"));

  std::stringstream coutstring;
  coutstring << rdb::flat << payload;
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
  out << rdb::flat << payload;
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
  out << rdb::flat << p;
  EXPECT_EQ(out.str(), "{ x:null }");
}

TEST(payload, operator_ostream_mixed_null_and_value) {
  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("b", 4, 1, rdb::INTEGER);
  rdb::payload p(desc);

  p.setItem(0, std::nullopt);
  p.setItem(1, 42);

  std::stringstream out;
  out << rdb::flat << p;
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
  out << rdb::flat << p;
  EXPECT_EQ(out.str(), "{ name:null }");
}
