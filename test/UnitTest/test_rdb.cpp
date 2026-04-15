#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "rdb/descriptor.hpp"
#include "rdb/fainterface.hpp"
#include "rdb/payload.hpp"
#include "rdb/storageacc.hpp"

// ctest -R '^ut-test_rdb' -V

const uint AREA_SIZE = 10;

// Helper: build a single-field Descriptor matching AREA_SIZE bytes
static rdb::Descriptor makeDesc(size_t size) {
  return rdb::Descriptor("f", static_cast<int>(size), 1, rdb::BYTE);
}

template <typename T, typename K>
bool test_1() {
  const int noPerCounter = -1;
  K binary1("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
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
  K dataStore("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
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
  K dataStore("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
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

TEST(xrdb, storage_purge_resets_metadata_stream_state) {
  const std::string streamName = "ut-purge-meta-reset";
  const std::string dataFile   = "ut-purge-meta-reset.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  // Clean up any stale files from a previous failed run
  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
  std::filesystem::remove("./" + dataFile + ".shadow");

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    auto *pl = s.getPayload();

    pl->setItem(0, std::nullopt);
    ASSERT_TRUE(s.write());
    ASSERT_EQ(s.getRecordsCount(), 1u);

    s.purge();
    ASSERT_EQ(s.getRecordsCount(), 0u);

    pl->setItem(0, 1234);
    ASSERT_TRUE(s.write());
    ASSERT_EQ(s.getRecordsCount(), 1u);

    ASSERT_TRUE(s.read(0));
    ASSERT_TRUE(pl->getItem(0).has_value());
    EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 1234);
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
  std::filesystem::remove("./" + dataFile + ".shadow");
}

TEST(xrdb, storage_first_write_persists_first_meta_record) {
  const std::string streamName = "ut-meta-first-record";
  const std::string dataFile   = "ut-meta-first-record.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    auto *pl = s.getPayload();
    pl->setItem(0, std::nullopt);
    ASSERT_TRUE(s.write());
    // Meta index is flushed to disk at destructor (lazy-write per spec):
    // "zapis pierwszego rekordu do pliku indeksu nie jest wymagany natychmiast"
  }

  // After storage is closed, meta file must contain at least one RLE entry
  ASSERT_TRUE(std::filesystem::exists(metaFile));
  constexpr uintmax_t headerSize = sizeof(int64_t) + sizeof(int32_t) + sizeof(int32_t);
  EXPECT_GT(std::filesystem::file_size(metaFile), headerSize);

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_detection_marks_gap_after_null_records) {
  const std::string streamName = "ut-auto-gap";
  const std::string dataFile   = "ut-auto-gap.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    // nullFillCount=2: first 2 consecutive all-null appends go to storage (nullfill phase)
    // gap phase starts when consecutiveNullCount_ > nullFillCount_
    s.configureGapDetection(boost::rational<int>(1, 100), 2, 2);

    auto *pl = s.getPayload();
    const std::vector<bool> allNull(desc.size(), true);
    const std::vector<bool> nonNull(desc.size(), false);

    // Nullfill phase: 2 all-null records written to storage as records 0 and 1
    pl->setNullBitset(allNull);
    ASSERT_TRUE(s.write());
    ASSERT_TRUE(s.write());

    // Gap phase: 2 more all-null records NOT written to storage (activeGapDuration_ = 2)
    ASSERT_TRUE(s.write());
    ASSERT_TRUE(s.write());

    // Non-null record: flushPendingGap(2) marks gap, then written as record 2
    pl->setNullBitset(nonNull);
    pl->setItem(0, 42);
    ASSERT_TRUE(s.write());

    EXPECT_TRUE(s.hasGapBefore(2));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_gap_flushed_on_destructor) {
  const std::string streamName = "ut-auto-gap-dtor";
  const std::string dataFile   = "ut-auto-gap-dtor.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);
    s.configureGapDetection(boost::rational<int>(1, 100), 1, 2);

    auto *pl = s.getPayload();
    const std::vector<bool> allNull(desc.size(), true);

    // Nullfill phase: 1 all-null record written to storage as record 0
    pl->setNullBitset(allNull);
    ASSERT_TRUE(s.write());

    // Gap phase: 1 more all-null record NOT written (activeGapDuration_ = 1)
    ASSERT_TRUE(s.write());

    // Destructor fires here — flushPendingGap() must persist the gap to meta file
  }

  // Reopen and verify gap was saved
  {
    rdb::storage s2(streamName, dataFile, ".");
    s2.attachDescriptor(&desc);

    // Record 0 was written; gap marker should be before record 1 (which doesn't exist yet,
    // but isGapBefore is based on meta entries, not record count)
    EXPECT_TRUE(s2.hasGapBefore(1));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_not_triggered_on_fast_writes) {
  const std::string streamName = "ut-auto-gap-fast";
  const std::string dataFile   = "ut-auto-gap-fast.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    // rInterval=1 (1 second), nullFillCount=2, gapDetectionThreshold=3 → threshold = 3000ms
    // All writes happen within milliseconds so no gap should be detected
    s.configureGapDetection(boost::rational<int>(1), 2, 3);

    auto *pl = s.getPayload();

    for (int i = 0; i < 5; ++i) {
      pl->setItem(0, i);
      ASSERT_TRUE(s.write());
    }

    EXPECT_FALSE(s.hasGapBefore(0));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_not_triggered_on_modify) {
  const std::string streamName = "ut-auto-gap-modify";
  const std::string dataFile   = "ut-auto-gap-modify.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    // rInterval=1/100 (10ms), nullFillCount=2, gapDetectionThreshold=2 → threshold = 20ms
    s.configureGapDetection(boost::rational<int>(1, 100), 2, 2);

    auto *pl = s.getPayload();

    pl->setItem(0, 10);
    ASSERT_TRUE(s.write());

    // Wait longer than threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Modify existing record — auto-gap should NOT be triggered for modifications
    pl->setItem(0, 99);
    ASSERT_TRUE(s.write(0));

    EXPECT_FALSE(s.hasGapBefore(0));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_setSamplingInterval_propagates_to_meta) {
  const std::string streamName = "ut-interval-prop";
  const std::string dataFile   = "ut-interval-prop.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    s.attachDescriptor(&desc);

    s.configureGapDetection(boost::rational<int>(1, 10));

    auto *pl = s.getPayload();

    // Write 3 records and verify meta tracks them
    for (int i = 0; i < 3; ++i) {
      pl->setItem(0, i * 10);
      ASSERT_TRUE(s.write());
    }

    EXPECT_FALSE(s.isMetaIndexEmpty());
    EXPECT_EQ(s.getRecordsCount(), 3u);
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}
