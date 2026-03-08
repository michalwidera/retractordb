#include <gtest/gtest.h>

#include "rdb/descriptor.h"
#include "rdb/metaDataStream.h"

TEST(MetaDataStreamTest, test_metaDataStream_construction) {
  rdb::Descriptor descriptor;
  descriptor.append({{"field1", 4, 0, rdb::INTEGER}, {"field2", 10, 0, rdb::STRING}});

  rdb::metaDataStream metaIndex(descriptor, "test_metaDataStream.meta");

  /*
  EXPECT_EQ(metaIndex.entries().size(), 0);
  EXPECT_EQ(metaIndex.totalRecords(), 0);
  EXPECT_EQ(metaIndex.size(), 0);
  EXPECT_FALSE(metaIndex.hasAnyNull());
  */
}


TEST(MetaDataIndexRecordTest, test_IndexRecord_serialization) {

  rdb::metaDataStream::IndexRecord original;
  original.recordCount = 42;
  original.nullBitset = {true, false, true, true, false};

  std::vector<std::byte> serialized = original.serialize();
  rdb::metaDataStream::IndexRecord deserialized = rdb::metaDataStream::IndexRecord::deserialize(serialized);
  EXPECT_EQ(deserialized.recordCount, original.recordCount);
  EXPECT_EQ(deserialized.nullBitset, original.nullBitset);

  /*
  EXPECT_EQ(metaIndex.entries().size(), 0);
  EXPECT_EQ(metaIndex.totalRecords(), 0);
  EXPECT_EQ(metaIndex.size(), 0);
  EXPECT_FALSE(metaIndex.hasAnyNull());
  */
}