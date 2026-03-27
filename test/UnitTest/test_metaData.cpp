#include <gtest/gtest.h>

#include "rdb/descriptor.h"
#include "rdb/metaDataStream.h"

#include <boost/rational.hpp>
#include <chrono>
#include <cstdio>

// Helper: remove test file if it exists
struct MetaTestFixture : public ::testing::Test {
  const std::string metaFile = "test_metaDataStream.meta";
  void TearDown() override { std::remove(metaFile.c_str()); }
};

TEST_F(MetaTestFixture, test_metaDataStream_construction) {
  rdb::Descriptor descriptor;
  descriptor.append({{"field1", 4, 0, rdb::INTEGER}, {"field2", 10, 0, rdb::STRING}});

  rdb::metaDataStream metaIndex(descriptor, metaFile);
  EXPECT_EQ(metaIndex.totalRecords(), 0u);
}

TEST(MetaDataIndexRecordTest, test_IndexRecord_serialization) {
  rdb::metaDataStream::IndexRecord original;
  original.recordCount = 42;
  original.nullBitset  = {true, false, true, true, false};
  original.isGap       = false;

  std::vector<std::byte> serialized             = original.serialize();
  rdb::metaDataStream::IndexRecord deserialized = rdb::metaDataStream::IndexRecord::deserialize(serialized);
  EXPECT_EQ(deserialized.recordCount, original.recordCount);
  EXPECT_EQ(deserialized.nullBitset, original.nullBitset);
  EXPECT_EQ(deserialized.isGap, original.isGap);
}

TEST(MetaDataIndexRecordTest, test_IndexRecord_gap_serialization) {
  rdb::metaDataStream::IndexRecord gap;
  gap.recordCount = 0;
  gap.nullBitset  = {false, false};
  gap.isGap       = true;

  auto serialized   = gap.serialize();
  auto deserialized = rdb::metaDataStream::IndexRecord::deserialize(serialized);
  EXPECT_EQ(deserialized.isGap, true);
  EXPECT_EQ(deserialized.recordCount, 0u);
}

TEST_F(MetaTestFixture, test_append_and_query) {
  rdb::Descriptor descriptor;
  descriptor.append({{"a", 4, 0, rdb::INTEGER}, {"b", 4, 0, rdb::INTEGER}});

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    std::vector<bool> allNull = {true, true};
    std::vector<bool> noNull  = {false, false};
    std::vector<bool> mixed   = {true, false};

    meta.onRecordAppended(allNull);  // rec 0
    meta.onRecordAppended(allNull);  // rec 1
    meta.onRecordAppended(noNull);   // rec 2
    meta.onRecordAppended(mixed);    // rec 3

    EXPECT_EQ(meta.totalRecords(), 4u);
    EXPECT_EQ(meta.getNullBitset(0), allNull);
    EXPECT_EQ(meta.getNullBitset(1), allNull);
    EXPECT_EQ(meta.getNullBitset(2), noNull);
    EXPECT_EQ(meta.getNullBitset(3), mixed);
    EXPECT_TRUE(meta.isFieldNull(0, 0));
    EXPECT_FALSE(meta.isFieldNull(2, 0));
  }

  // Verify persistence: reload from file
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    EXPECT_EQ(meta.totalRecords(), 4u);
    std::vector<bool> allNull = {true, true};
    EXPECT_EQ(meta.getNullBitset(0), allNull);
  }
}

TEST_F(MetaTestFixture, test_modify_splits_rle) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> null_   = {true};
  std::vector<bool> noNull_ = {false};

  // Build a run of 5 identical records
  for (int i = 0; i < 5; ++i)
    meta.onRecordAppended(null_);

  // Modify the middle one
  meta.onRecordModified(2, noNull_);

  EXPECT_EQ(meta.totalRecords(), 5u);
  EXPECT_EQ(meta.getNullBitset(0), null_);
  EXPECT_EQ(meta.getNullBitset(1), null_);
  EXPECT_EQ(meta.getNullBitset(2), noNull_);  // modified
  EXPECT_EQ(meta.getNullBitset(3), null_);
  EXPECT_EQ(meta.getNullBitset(4), null_);
}

TEST_F(MetaTestFixture, test_transmission_gap) {
  rdb::Descriptor descriptor;
  descriptor.append({{"v", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false};

  meta.onRecordAppended(pat);  // rec 0
  meta.onRecordAppended(pat);  // rec 1
  meta.onTransmissionGap();
  meta.onRecordAppended(pat);  // rec 2

  EXPECT_EQ(meta.totalRecords(), 3u);
  EXPECT_FALSE(meta.isGapBefore(0));
  EXPECT_FALSE(meta.isGapBefore(1));
  EXPECT_TRUE(meta.isGapBefore(2));
}

TEST_F(MetaTestFixture, test_timestamps) {
  rdb::Descriptor descriptor;
  descriptor.append({{"t", 4, 0, rdb::INTEGER}});

  auto before = std::chrono::system_clock::now();
  rdb::metaDataStream meta(descriptor, metaFile, boost::rational<int>(1, 10));  // 0.1s interval
  auto after = std::chrono::system_clock::now();
  std::vector<bool> pat = {false};

  meta.onRecordAppended(pat);

  // Creation time should be between before and after construction
  auto creationTime = meta.getCreationTime();
  EXPECT_GE(creationTime, before);
  EXPECT_LE(creationTime, after);

  // Sampling interval should be 1/10
  EXPECT_EQ(meta.getSamplingInterval(), boost::rational<int>(1, 10));

  // Record 0 timestamp == creation time
  auto ts0 = meta.calculateRecordTimestamp(0);
  EXPECT_EQ(ts0, creationTime);

  // Record 10 timestamp == creation time + 1 second (10 * 0.1s)
  auto ts10 = meta.calculateRecordTimestamp(10);
  auto expected = creationTime + std::chrono::seconds(1);
  EXPECT_EQ(ts10, expected);
}
