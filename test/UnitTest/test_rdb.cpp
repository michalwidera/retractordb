#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <locale>
#include <string>

#include "rdb/storageacc.h"
#include "rdb/fainterface.h"
#include "rdb/payloadacc.h"

const uint AREA_SIZE = 10;

template <typename T, typename K>
bool test_1() {
  K binaryAccessor1("testfile-fstream");
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    binaryAccessor1.write(xData, AREA_SIZE);

    if (strcmp(reinterpret_cast<char *>(xData), "test data") != 0) return false;

    T yData[AREA_SIZE];
    binaryAccessor1.read(yData, AREA_SIZE, 0);

    if (strcmp(reinterpret_cast<char *>(yData), "test data") != 0) return false;
  }
  auto statusRemove1 = remove(binaryAccessor1.fileName().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

template <typename T, typename K>
bool test_2() {
  K dataStore("testfile-fstream");
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    dataStore.write(xData, AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);  // Add one extra record

    if (strcmp(reinterpret_cast<char *>(xData), "test data") != 0) return false;

    T yData[AREA_SIZE];
    dataStore.read(yData, AREA_SIZE, 0);

    if (strcmp(reinterpret_cast<char *>(yData), "test data") != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.fileName().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

template <typename T, typename K>
bool test_3() {
  K dataStore("testfile-fstream");
  {
    T xData[AREA_SIZE];

    std::memcpy(xData, "test aaaa", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);

    std::memcpy(xData, "test bbbb", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);

    std::memcpy(xData, "test cccc", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);

    std::memcpy(xData, "test xxxx", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE, AREA_SIZE);  // <- Update

    std::memcpy(xData, "test dddd", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);

    T yData[AREA_SIZE];

    dataStore.read(yData, AREA_SIZE, 0);

    if (strcmp(reinterpret_cast<char *>(yData), "test aaaa") != 0) return false;

    dataStore.read(yData, AREA_SIZE, AREA_SIZE);

    if (strcmp(reinterpret_cast<char *>(yData), "test xxxx") != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.fileName().c_str());
  if (statusRemove1 != 0) return false;

  return true;
}

bool test_descriptor() {
  rdb::Descriptor data1{rdb::rfield("Name3", 10, rdb::STRING), rdb::rfield("Name4", 10, rdb::STRING)};

  data1.append({rdb::rfield("Name5z", 10, rdb::STRING)});
  data1.append({rdb::rfield("Name6z", 10, rdb::STRING)});

  data1.push_back(rdb::rfield("Name", 10, rdb::STRING));
  data1.push_back(rdb::rfield("TLen", sizeof(uint), rdb::UINT));

  data1 | rdb::Descriptor("Name2", 10, rdb::STRING) | rdb::Descriptor("Control", rdb::BYTE) | rdb::Descriptor("Len3", rdb::UINT);
  {
    std::stringstream coutstring;
    coutstring << data1;
    char test[] =
        "{\tSTRING Name3[10]\n\tSTRING Name4[10]\n\tSTRING "
        "Name5z[10]\n\tSTRING Name6z[10]\n\tSTRING Name[10]\n\tUINT "
        "TLen\n\tSTRING Name2[10]\n\tBYTE Control\n\tUINT Len3\n}";
    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  rdb::Descriptor data2 =
      rdb::Descriptor("Name", 10, rdb::STRING) | rdb::Descriptor("Len3", rdb::UINT) | rdb::Descriptor("Control", rdb::BYTE);
  {
    std::stringstream coutstring;
    char test[] = "{\tSTRING Name[10]\n\tUINT Len3\n\tBYTE Control\n}";
    coutstring << data2;
    if (strcmp(coutstring.str().c_str(), test) != 0) return false;
  }

  if (data2.position("Control") != 2) return false;
  if (data2.len("Control") != 1) return false;
  if (strcmp(data2.type("Control").c_str(), "BYTE") != 0) return false;
  if (data2.offset("Control") != 14) return false;

  return true;
}

bool test_descriptor_read() {
  // start cin test
  // https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
  std::streambuf *orig = std::cin.rdbuf();
  // Note: This mess is intended here in test

  const char *test_string =
      "{ STRING Name3[10]\nSTRING Name[10]\nUINT Len STRING Name2 10 BYTE "
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

bool test_storage() {
  // This structure is tricky
  // If not aligned - size is 15
  // If aligned - size is 16
  // Note that this should be packed and size should be 15

  union dataPayload {
    std::byte ptr[15];
    struct __attribute__((packed)) {
      char Name[10];    // 10
      uint8_t Control;  // 1
      int TLen;         // 4
    };
  };

  static_assert(sizeof(dataPayload) == 15);

  auto dataDescriptor{rdb::Descriptor("Name", 10, rdb::STRING) |  //
                      rdb::Descriptor("Control", rdb::BYTE) |     //
                      rdb::Descriptor("TLen", rdb::INTEGER)};

  // This assert will fail is structure is not packed.
  if (dataDescriptor.getSizeInBytes() != sizeof(dataPayload)) return false;

  dataPayload payload1;

  rdb::storageAccessor dAcc2("datafile-fstream2");
  dAcc2.createDescriptor(dataDescriptor);

  std::memcpy(payload1.Name, "test data", AREA_SIZE);
  payload1.TLen = 0x66;
  payload1.Control = 0x22;

  if (payload1.TLen != dAcc2.getDescriptor().cast<int>("TLen", payload1.ptr)) return false;

  dAcc2.write();
  dAcc2.write();
  dAcc2.write();

  dataPayload payload2;
  std::memcpy(payload2.Name, "xxxx xxxx", AREA_SIZE);
  payload2.TLen = 0x67;
  payload2.Control = 0x33;
  std::memcpy(dAcc2.payload.get(), &payload2, 15);

  dAcc2.write(1);

  dAcc2.read(1);

  {
    std::stringstream coutstring;
    coutstring << dAcc2.getDescriptor().toString("Name", dAcc2.payload.get());
    if (strcmp(coutstring.str().c_str(), "xxxx xxxx") != 0) return false;
  }

  {
    std::stringstream coutstring;
    coutstring << std::hex;
    coutstring << dAcc2.getDescriptor().cast<int>("TLen", dAcc2.payload.get());
    coutstring << std::dec;
    if (strcmp(coutstring.str().c_str(), "67") != 0) return false;
  }

  {
    std::stringstream coutstring;
    coutstring << std::hex;
    coutstring << (uint)dAcc2.getDescriptor().cast<uint8_t>("Control", dAcc2.payload.get());
    coutstring << std::dec;
    if (strcmp(coutstring.str().c_str(), "33") != 0) return false;
  }

  auto statusRemove1 = remove("datafile-fstream2");
  if (statusRemove1 != 0) return false;

  auto statusRemove2 = remove("datafile-fstream2.desc");
  if (statusRemove2 != 0) return false;

  return true;
}

TEST(xrdb, test_descriptor_read) { ASSERT_TRUE(test_descriptor_read()); }

TEST(xrdb, test_descriptor) { ASSERT_TRUE(test_descriptor()); }

TEST(xrdb, test_storage) { ASSERT_TRUE(test_storage()); }

TEST(xrdb, test_descriptor_compare) {
  auto dataDescriptor1{rdb::Descriptor("Name", 10, rdb::STRING) | rdb::Descriptor("Control", rdb::BYTE) |
                       rdb::Descriptor("TLen", rdb::INTEGER)};
  auto dataDescriptor2{rdb::Descriptor("Name", 10, rdb::STRING) | rdb::Descriptor("Control", rdb::BYTE) |
                       rdb::Descriptor("TLen", rdb::INTEGER)};
  auto dataDescriptorDiff1{rdb::Descriptor("Control", rdb::BYTE) | rdb::Descriptor("Name", 10, rdb::STRING) |
                           rdb::Descriptor("TLen", rdb::INTEGER)};
  auto dataDescriptorDiff2{rdb::Descriptor("Name", 11, rdb::STRING) | rdb::Descriptor("Control", rdb::BYTE) |
                           rdb::Descriptor("TLen", rdb::INTEGER)};
  ASSERT_TRUE(dataDescriptor1 == dataDescriptor2);
  ASSERT_FALSE(dataDescriptor1 == dataDescriptorDiff1);
  ASSERT_FALSE(dataDescriptor1 == dataDescriptorDiff2);
}

TEST(crdb, genericBinaryFileAccessor_byte) {
  auto result1 = test_1<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result3);
}

TEST(crdb, genericBinaryFileAccessor_char) {
  auto result1 = test_1<char, rdb::genericBinaryFileAccessor<char>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<char, rdb::genericBinaryFileAccessor<char>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<char, rdb::genericBinaryFileAccessor<char>>();
  ASSERT_TRUE(result3);
}

TEST(crdb, posixBinaryFileAccessor_byte) {
  auto result1 = test_1<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result3);
}

TEST(crdb, posixBinaryFileAccessor_char) {
  auto result1 = test_1<char, rdb::posixBinaryFileAccessor<char>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<char, rdb::posixBinaryFileAccessor<char>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<char, rdb::posixBinaryFileAccessor<char>>();
  ASSERT_TRUE(result3);
}

TEST(crdb, posixPrmBinaryFileAccessor_byte) {
  auto result1 = test_1<std::byte, rdb::posixPrmBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<std::byte, rdb::posixPrmBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<std::byte, rdb::posixPrmBinaryFileAccessor<std::byte>>();
  ASSERT_TRUE(result3);
}

TEST(crdb, posixPrmBinaryFileAccessor_char) {
  auto result1 = test_1<char, rdb::posixPrmBinaryFileAccessor<char>>();
  ASSERT_TRUE(result1);
  auto result2 = test_2<char, rdb::posixPrmBinaryFileAccessor<char>>();
  ASSERT_TRUE(result2);
  auto result3 = test_3<char, rdb::posixPrmBinaryFileAccessor<char>>();
  ASSERT_TRUE(result3);
}
