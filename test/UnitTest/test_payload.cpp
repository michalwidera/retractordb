#include <gtest/gtest.h>

#include <any>
#include <sstream>
#include <string>

#include "rdb/descriptor.h"
#include "rdb/payload.h"

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
