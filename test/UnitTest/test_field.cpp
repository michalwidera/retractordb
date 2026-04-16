#include <gtest/gtest.h>

#include <sstream>

#include "retractor/lib/field.hpp"

TEST(field, single_token_constructor_exposes_first_token) {
  token firstToken(PUSH_VAL, 42);
  field tested(rdb::rField("value", sizeof(int), 1, rdb::INTEGER), firstToken);

  auto first = tested.getFirstFieldToken();

  EXPECT_EQ(tested.field_.rname, "value");
  EXPECT_EQ(tested.field_.rlen, static_cast<int>(sizeof(int)));
  EXPECT_EQ(tested.field_.rarray, 1);
  EXPECT_EQ(tested.field_.rtype, rdb::INTEGER);
  EXPECT_EQ(tested.lProgram.size(), 1U);
  EXPECT_EQ(first.getCommandID(), PUSH_VAL);
  EXPECT_EQ(std::get<int>(first.getVT()), 42);
}

TEST(field, list_constructor_preserves_program_order) {
  std::list<token> program = {token(PUSH_ID, std::pair<std::string, int>("src", 0)), token(PUSH_VAL, 7), token(ADD, 0)};
  field tested(rdb::rField("sum", sizeof(int), 1, rdb::INTEGER), program);

  auto first      = tested.getFirstFieldToken();
  auto firstValue = std::get<std::pair<std::string, int>>(first.getVT());

  EXPECT_EQ(tested.lProgram.size(), 3U);
  EXPECT_EQ(first.getCommandID(), PUSH_ID);
  EXPECT_EQ(firstValue.first, "src");
  EXPECT_EQ(firstValue.second, 0);
}

TEST(field, stream_output_serializes_metadata_and_program) {
  std::list<token> program = {token(PUSH_VAL, 3), token(PUSH_VAL, 4), token(ADD, 0)};
  field tested(rdb::rField("sum", sizeof(int), 1, rdb::INTEGER), program);

  std::ostringstream out;
  out << tested;

  EXPECT_EQ(out.str(), "FLD/name:sum,type:1,prog:PUSH_VAL(3),PUSH_VAL(4),ADD(0),");
}

TEST(field, stream_output_handles_empty_program) {
  field tested(rdb::rField("decl", sizeof(uint8_t), 4, rdb::BYTE), std::list<token>{});

  std::ostringstream out;
  out << tested;

  EXPECT_EQ(out.str(), "FLD/name:decl,type:0,prog:");
  EXPECT_TRUE(tested.lProgram.empty());
}
