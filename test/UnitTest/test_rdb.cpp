#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "rdb/fainterface.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"

// ctest -R '^ut-test_rdb' -V

const uint AREA_SIZE = 10;

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
    EXPECT_EQ(std::any_cast<int>(pl->getItem(2).value()), tlenViaOffset);
  }

  dAcc2.write();
  dAcc2.write();
  dAcc2.write();

  pl->setItem(0, std::string("xxxx xxxx"));
  pl->setItem(1, static_cast<uint8_t>(0x33));
  pl->setItem(2, 0x67);

  dAcc2.write(1);

  dAcc2.revRead(dAcc2.getRecordsCount() - 1 - 1);

  EXPECT_EQ(std::any_cast<std::string>(pl->getItem(0).value()), "xxxx xxxx");
  EXPECT_EQ(std::any_cast<int>(pl->getItem(2).value()), 0x67);
  EXPECT_EQ(std::any_cast<uint8_t>(pl->getItem(1).value()), 0x33);
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

TEST(xrdb, storage_persists_null_flags_via_metadata_stream) {
  const std::string streamName = "ut-null-meta-stream";
  const std::string dataFile   = "ut-null-meta-stream.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);
    s.setDisposable(true);

    auto *pl = s.getPayload();

    pl->setItem(0, std::nullopt);
    ASSERT_TRUE(s.write());

    pl->setItem(0, 77);
    ASSERT_TRUE(s.write());

    ASSERT_TRUE(s.read(0));
    EXPECT_FALSE(pl->getItem(0).has_value());

    ASSERT_TRUE(s.read(1));
    ASSERT_TRUE(pl->getItem(0).has_value());
    EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 77);
  }

  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_updates_null_flags_on_record_modify) {
  const std::string streamName = "ut-null-meta-modify";
  const std::string dataFile   = "ut-null-meta-modify.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);
    s.setDisposable(true);

    auto *pl = s.getPayload();

    pl->setItem(0, 123);
    ASSERT_TRUE(s.write());

    pl->setItem(0, std::nullopt);
    ASSERT_TRUE(s.write(0));

    ASSERT_TRUE(s.read(0));
    EXPECT_FALSE(pl->getItem(0).has_value());
  }

  std::filesystem::remove(metaFile);
}
