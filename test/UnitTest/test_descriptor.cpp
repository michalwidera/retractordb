#include <gtest/gtest.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "rdb/descriptor.h"

extern std::string parserDESCString(rdb::Descriptor &desc, const std::string_view inlet);

namespace {

bool test_descriptor() {
  rdb::Descriptor data1{rdb::rField("Name3", 1, 10, rdb::STRING), rdb::rField("Name4", 10, 1, rdb::STRING)};

  data1.append({rdb::rField("Name5z", 1, 10, rdb::STRING)});
  data1.append({rdb::rField("Name6z", 1, 10, rdb::STRING)});

  data1.push_back(rdb::rField("Name", 1, 10, rdb::STRING));
  data1.push_back(rdb::rField("TLen", sizeof(uint), 1, rdb::UINT));

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

  if (data2.position("Control") != 2) return false;
  if (data2.fieldSize("Control") != 1) return false;
  if (strcmp(data2.type("Control").data(), "BYTE") != 0) return false;
  if (data2.offsetBegArr("Control") != 14) return false;

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

TEST(descriptor, read_from_stream) { EXPECT_TRUE(test_descriptor_read()); }

TEST(descriptor, print_and_basic_accessors) { EXPECT_TRUE(test_descriptor()); }

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

TEST(descriptor, compare_ignores_configuration_fields_without_out_of_bounds_access) {
  auto withConfig = rdb::Descriptor("source.dat", 0, 0, rdb::REF) +   //
                    rdb::Descriptor("TEXTSOURCE", 0, 1, rdb::TYPE) +  //
                    rdb::Descriptor("value", 4, 1, rdb::INTEGER);
  auto plain     = rdb::Descriptor("value", 4, 1, rdb::INTEGER);
  auto different = rdb::Descriptor("value", 8, 1, rdb::DOUBLE);

  EXPECT_TRUE(withConfig == plain);
  EXPECT_FALSE(withConfig == different);
}

TEST(descriptor, retention_and_policy_defaults) {
  auto desc = rdb::Descriptor("value", 4, 1, rdb::INTEGER);

  auto retention = desc.retention();
  EXPECT_TRUE(retention.noRetention());

  auto policy = desc.policy();
  EXPECT_TRUE(policy.first.empty());
  EXPECT_EQ(policy.second, 0U);
}

TEST(descriptor, retention_and_policy_values) {
  auto desc = rdb::Descriptor("value", 4, 1, rdb::INTEGER) +  //
              rdb::Descriptor("MEMORY", 0, 1, rdb::TYPE) +    //
              rdb::Descriptor("retention-mem", 7, 1, rdb::RETMEMORY) + rdb::Descriptor("retention", 5, 2, rdb::RETENTION);

  auto retention = desc.retention();
  EXPECT_EQ(retention.segments, 5U);
  EXPECT_EQ(retention.capacity, 2U);

  auto policy = desc.policy();
  EXPECT_EQ(policy.first, "MEMORY");
  EXPECT_EQ(policy.second, 7U);
}

TEST(descriptor, clean_ref_and_flat_fields) {
  auto desc = rdb::Descriptor("src.bin", 0, 0, rdb::REF) +      //
              rdb::Descriptor("TEXTSOURCE", 0, 1, rdb::TYPE) +  //
              rdb::Descriptor("a", 1, 3, rdb::BYTE) +           //
              rdb::Descriptor("s", 8, 99, rdb::STRING);

  EXPECT_TRUE(desc.hasField("src.bin"));
  EXPECT_EQ(desc.flatElementCount(), 4);

  auto flatFields = desc.fieldsFlat();
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

TEST(descriptor, flat_output_resets_after_stream) {
  auto desc = rdb::Descriptor("x", 1, 1, rdb::BYTE);

  std::stringstream flatOut;
  flatOut << rdb::flat << desc;
  EXPECT_EQ(flatOut.str(), "{ BYTE x }");
  EXPECT_FALSE(rdb::Descriptor::getFlat());

  std::stringstream multilineOut;
  multilineOut << desc;
  EXPECT_EQ(multilineOut.str(), "{\tBYTE x\n}");
}

TEST(descriptor, offset_and_convert_for_string_and_arrays) {
  auto desc = rdb::Descriptor("a", 1, 2, rdb::BYTE) +    //
              rdb::Descriptor("s", 5, 9, rdb::STRING) +  //
              rdb::Descriptor("v", 4, 1, rdb::INTEGER);

  EXPECT_EQ(desc.flatElementCount(), 4);

  EXPECT_TRUE(desc.convert(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc.convert(1) == std::make_pair(0, 1));
  EXPECT_TRUE(desc.convert(2) == std::make_pair(1, 0));
  EXPECT_TRUE(desc.convert(3) == std::make_pair(2, 0));

  EXPECT_EQ(desc.offset(0), 0);
  EXPECT_EQ(desc.offset(1), 1);
  EXPECT_EQ(desc.offset(2), 2);
  EXPECT_EQ(desc.offset(3), 47);
}

TEST(descriptor, empty_descriptor_has_empty_flat_mapping) {
  rdb::Descriptor empty;

  EXPECT_EQ(empty.flatElementCount(), 0);
  EXPECT_TRUE(empty.fieldsFlat().empty());
  EXPECT_FALSE(empty.convert(0).has_value());
}

TEST(descriptor, position_conversion_case_1) {
  auto desc1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.convert(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.convert(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.convert(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.convert(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.convert(4) == std::make_pair(2, 0));
}

TEST(descriptor, position_conversion_case_2) {
  auto desc1{rdb::Descriptor("Name", 1, 1, rdb::BYTE) +     //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.convert(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.convert(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.convert(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.convert(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.convert(4) == std::make_pair(2, 0));
}

TEST(descriptor, parser) {
  rdb::Descriptor out;
  EXPECT_TRUE(parserDESCString(out, "{ BYTE a INTEGER b[10] INTEGER c }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a INTEGER b INTEGER c REF \"datafile.txt\" TYPE TEXTSOURCE }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETENTION 10 5 }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETMEMORY 10 TYPE MEMORY }") == "OK");
}

TEST(descriptor, assign_operator) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2;
  data2 = data1;
  EXPECT_TRUE(data2.position("Control") == data1.position("Control"));
  EXPECT_TRUE(data2.fieldSize("Control") == data1.fieldSize("Control"));
  EXPECT_TRUE(data2.position("TLen") == data1.position("TLen"));
}

TEST(descriptor, copy_constructor) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2{data1};
  EXPECT_TRUE(data2.position("Control") == data1.position("Control"));
  EXPECT_TRUE(data2.fieldSize("Control") == data1.fieldSize("Control"));
  EXPECT_TRUE(data2.position("TLen") == data1.position("TLen"));
}
