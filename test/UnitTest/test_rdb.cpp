#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <locale>
#include <string>

#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"

// ctest -R '^ut-test_rdb' -V

const uint AREA_SIZE = 10;

extern std::string parserDESCString(rdb::Descriptor &desc, const std::string_view inlet);

template <typename T, typename K>
bool test_1() {
  const int noPerCounter = -1;
  K binary1("testfile-fstream", AREA_SIZE, noPerCounter);
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    binary1.write(xData);

    if (std::memcmp(xData, "test data", AREA_SIZE) != 0) return false;

    T yData[AREA_SIZE];
    binary1.read(yData, 0);

    if (std::memcmp(yData, "test data", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(binary1.name().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

template <typename T, typename K>
bool test_2() {
  const int noPerCounter = -1;
  K dataStore("testfile-fstream", AREA_SIZE, noPerCounter);
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    dataStore.write(xData);
    dataStore.write(xData);  // Add one extra record

    if (std::memcmp(xData, "test data", AREA_SIZE) != 0) return false;

    T yData[AREA_SIZE];
    dataStore.read(yData, 0);

    if (std::memcmp(yData, "test data", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.name().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

template <typename T, typename K>
bool test_3() {
  const int noPerCounter = -1;
  K dataStore("testfile-fstream", AREA_SIZE, noPerCounter);
  {
    T xData[AREA_SIZE];

    std::memcpy(xData, "test aaaa", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test bbbb", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test cccc", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test xxxx", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);  // <- Update

    std::memcpy(xData, "test dddd", AREA_SIZE);
    dataStore.write(xData);

    T yData[AREA_SIZE];

    dataStore.read(yData, 0);

    if (std::memcmp(yData, "test aaaa", AREA_SIZE) != 0) return false;

    dataStore.read(yData, AREA_SIZE);

    if (std::memcmp(yData, "test xxxx", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.name().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

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
  if (data2.len("Control") != 1) return false;
  if (strcmp(data2.type("Control").data(), "BYTE") != 0) return false;
  if (data2.offsetBegArr("Control") != 14) return false;

  return true;
}

bool test_descriptor_read() {
  // start cin test
  // https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
  std::streambuf *orig = std::cin.rdbuf();
  // Note: This mess is intended here in test

  const char *test_string =
      "{ STRING Name3[10]\nSTRING Name[10]\nUINT Len STRING Name2[10] BYTE "
      "Control UINT Len3 }";

  // Test goes here

  rdb::Descriptor data3;

  std::istringstream input(test_string);
  std::cin.rdbuf(input.rdbuf());
  std::cin >> data3;
  // Revert to orginal cin
  std::cin.rdbuf(orig);

  {
    std::stringstream coutstring;
    const char *test =
        "{\tSTRING Name3[10]\n\tSTRING Name[10]\n\tUINT Len\n\tSTRING "
        "Name2[10]\n\tBYTE Control\n\tUINT Len3\n}";
    coutstring << data3;

    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  // end cin test

  return true;
}

TEST(xrdb, test_descriptor_read) { EXPECT_TRUE(test_descriptor_read()); }

TEST(xrdb, test_descriptor) { EXPECT_TRUE(test_descriptor()); }

TEST(xrdb, test_storage) {
  // This structure is tricky
  // If not aligned - size is 15
  // If aligned - size is 16
  // Note that this should be packed and size should be 15

  union dataPayload {
    uint8_t ptr[15];
    struct __attribute__((packed)) {
      char Name[10];    // 10
      uint8_t Control;  // 1
      int TLen;         // 4
    };
  };

  static_assert(sizeof(dataPayload) == 15);

  auto dataDescriptor{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                      rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                      rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  // This assert will fail is structure is not packed.
  EXPECT_TRUE(dataDescriptor.getSizeInBytes() == sizeof(dataPayload));

  rdb::storage dAcc2("datafile-fstream2", "datafile-fstream2", "");

  dAcc2.attachDescriptor(&dataDescriptor);
  dAcc2.setDisposable(true);

  auto *pl = dAcc2.getPayload();

  pl->setItem(0, std::string("test data"));
  pl->setItem(1, static_cast<uint8_t>(0x22));
  pl->setItem(2, 0x66);

  // Verify offsetBegArr("TLen") points to the correct location in raw memory
  {
    int tlenViaOffset;
    std::memcpy(&tlenViaOffset, pl->span().data() + dAcc2.descriptor.offsetBegArr("TLen"), sizeof(int));
    EXPECT_EQ(std::any_cast<int>(pl->getItem(2)), tlenViaOffset);
  }

  dAcc2.write();
  dAcc2.write();
  dAcc2.write();

  pl->setItem(0, std::string("xxxx xxxx"));
  pl->setItem(1, static_cast<uint8_t>(0x33));
  pl->setItem(2, 0x67);

  dAcc2.write(1);

  dAcc2.revRead(dAcc2.getRecordsCount() - 1 - 1);

  EXPECT_EQ(std::any_cast<std::string>(pl->getItem(0)), "xxxx xxxx");
  EXPECT_EQ(std::any_cast<int>(pl->getItem(2)), 0x67);
  EXPECT_EQ(std::any_cast<uint8_t>(pl->getItem(1)), 0x33);
}

TEST(xrdb, test_descriptor_compare) {
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

TEST(crdb, genericBinaryFile_byte) {
  auto result1 = test_1<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result1);
  auto result2 = test_2<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result2);
  auto result3 = test_3<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result3);
}

TEST(crdb, posixBinaryFile_byte) {
  auto result1 = test_1<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result1);
  auto result2 = test_2<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result2);
  auto result3 = test_3<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result3);
}

TEST(crdb, payload_assign_operator) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2;
  data2 = data1;
  EXPECT_TRUE(data2.position("Control") == data1.position("Control"));
  EXPECT_TRUE(data2.len("Control") == data1.len("Control"));
  EXPECT_TRUE(data2.position("TLen") == data1.position("TLen"));
}

TEST(crdb, payload_copy_constructor) {
  auto data1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};
  rdb::Descriptor data2{data1};
  EXPECT_TRUE(data2.position("Control") == data1.position("Control"));
  EXPECT_TRUE(data2.len("Control") == data1.len("Control"));
  EXPECT_TRUE(data2.position("TLen") == data1.position("TLen"));
}

TEST(crdb, payload_add_operator) {
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

  std::any Name_    = data1Payload.getItem(0);
  std::any Control_ = data1Payload.getItem(1);
  std::any ll_      = data1Payload.getItem(2);
  std::any TLen_    = data1Payload.getItem(3);

  dataPayload var;

  std::memcpy(&var, data1Payload.span().data(), 19);

  EXPECT_TRUE(var.ll == std::any_cast<int>(ll_));
  EXPECT_TRUE(var.TLen == std::any_cast<int>(TLen_));

  rdb::payload data2Payload(data2);
  data2Payload.setItem(0, 4004);

  EXPECT_TRUE(std::any_cast<int>(data1Payload.getItem(2)) == 2000);

  rdb::payload data3Payload;

  data3Payload = data1Payload + data2Payload;

  EXPECT_TRUE(std::any_cast<std::string>(data3Payload.getItem(0)) == "test");
  EXPECT_TRUE(std::any_cast<uint8_t>(data3Payload.getItem(1)) == 24);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(2)) == 2000);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(3)) == 3333);
  EXPECT_TRUE(std::any_cast<int>(data3Payload.getItem(4)) == 4004);
}

TEST(crdb, position_conversion_test1) {
  auto desc1{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.convert(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.convert(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.convert(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.convert(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.convert(4) == std::make_pair(2, 0));
}

TEST(crdb, position_conversion_test2) {
  auto desc1{rdb::Descriptor("Name", 1, 1, rdb::BYTE) +     //
             rdb::Descriptor("Control", 1, 3, rdb::BYTE) +  //
             rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  EXPECT_TRUE(desc1.convert(0) == std::make_pair(0, 0));
  EXPECT_TRUE(desc1.convert(1) == std::make_pair(1, 0));
  EXPECT_TRUE(desc1.convert(2) == std::make_pair(1, 1));
  EXPECT_TRUE(desc1.convert(3) == std::make_pair(1, 2));
  EXPECT_TRUE(desc1.convert(4) == std::make_pair(2, 0));
}

TEST(crdb, position_conversion_test3) {
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

  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(0)) == 145);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(1)) == 24);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(2)) == 25);
  EXPECT_TRUE(std::any_cast<uint8_t>(payload.getItem(3)) == 26);
  EXPECT_TRUE(std::any_cast<int>(payload.getItem(4)) == 2000);
  EXPECT_TRUE(std::any_cast<std::string>(payload.getItem(5)).c_str() == std::string("test"));

  std::stringstream coutstring;
  coutstring << rdb::flat << payload;
  EXPECT_TRUE("{ ByteW:145 Control:24 25 26 TLen:2000 Name:test }" == coutstring.str());
}

TEST(crdb, descriptor_parser_test) {
  rdb::Descriptor out;
  EXPECT_TRUE(parserDESCString(out, "{ BYTE a INTEGER b[10] INTEGER c }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a INTEGER b INTEGER c REF \"datafile.txt\" TYPE TEXTSOURCE }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETENTION 10 5 }") == "OK");
  EXPECT_TRUE(parserDESCString(out, "{ INTEGER a RETMEMORY 10 TYPE MEMORY }") == "OK");
}
