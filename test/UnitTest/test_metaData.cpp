#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

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
  auto after            = std::chrono::system_clock::now();
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
  auto ts10     = meta.calculateRecordTimestamp(10);
  auto expected = creationTime + std::chrono::seconds(1);
  EXPECT_EQ(ts10, expected);
}

// R4: Modify a record already committed to disk (rewriteFile path)
TEST_F(MetaTestFixture, test_modify_committed_entry_on_disk) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> null_   = {true};
  std::vector<bool> noNull_ = {false};

  // Append 3 null records, then switch pattern to force flush to disk
  meta.onRecordAppended(null_);    // rec 0
  meta.onRecordAppended(null_);    // rec 1
  meta.onRecordAppended(null_);    // rec 2
  meta.onRecordAppended(noNull_);  // rec 3 — forces recs 0-2 to disk

  // recs 0-2 are now committed on disk, rec 3 is in currentEntry_
  EXPECT_EQ(meta.entries().size(), 1u);  // one committed segment {null_, 3}

  // Modify rec 1 (committed on disk) — triggers rewriteFile
  meta.onRecordModified(1, noNull_);

  EXPECT_EQ(meta.totalRecords(), 4u);
  EXPECT_EQ(meta.getNullBitset(0), null_);
  EXPECT_EQ(meta.getNullBitset(1), noNull_);  // modified on disk
  EXPECT_EQ(meta.getNullBitset(2), null_);
  EXPECT_EQ(meta.getNullBitset(3), noNull_);

  // Verify persistence after rewriteFile
  EXPECT_EQ(meta.entries().size(), 3u);  // split: {null_,1}, {noNull_,1}, {null_,1}
}

// R6: All data in file except last entry buffered in memory
TEST_F(MetaTestFixture, test_committed_on_disk_current_in_memory) {
  rdb::Descriptor descriptor;
  descriptor.append({{"a", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {true};
  std::vector<bool> patB = {false};

  // Same pattern — stays only in currentEntry_, nothing on disk
  meta.onRecordAppended(patA);                    // rec 0
  meta.onRecordAppended(patA);                    // rec 1
  EXPECT_EQ(meta.entries().size(), 0u);           // nothing committed yet
  EXPECT_EQ(meta.currentEntry_.recordCount, 2u);  // buffered in memory

  // Different pattern — flushes patA to disk, patB becomes currentEntry_
  meta.onRecordAppended(patB);           // rec 2
  EXPECT_EQ(meta.entries().size(), 1u);  // patA committed to disk
  EXPECT_EQ(meta.entries()[0].recordCount, 2u);
  EXPECT_EQ(meta.entries()[0].nullBitset, patA);
  EXPECT_EQ(meta.currentEntry_.recordCount, 1u);  // patB in memory only
  EXPECT_EQ(meta.currentEntry_.nullBitset, patB);
}

// R10: Handle large amounts of data
TEST_F(MetaTestFixture, test_large_data_volume) {
  rdb::Descriptor descriptor;
  descriptor.append({{"f1", 4, 0, rdb::INTEGER}, {"f2", 4, 0, rdb::INTEGER}});

  constexpr size_t N = 100'000;
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    std::vector<bool> patA = {true, false};
    std::vector<bool> patB = {false, true};

    for (size_t i = 0; i < N; ++i)
      meta.onRecordAppended((i % 100 < 50) ? patA : patB);

    EXPECT_EQ(meta.totalRecords(), N);
    // With alternating blocks of 50, we expect ~2000 RLE segments
    // Verify boundary records
    EXPECT_EQ(meta.getNullBitset(0), patA);
    EXPECT_EQ(meta.getNullBitset(49), patA);
    EXPECT_EQ(meta.getNullBitset(50), patB);
    EXPECT_EQ(meta.getNullBitset(99), patB);
    EXPECT_EQ(meta.getNullBitset(N - 1), (((N - 1) % 100 < 50) ? patA : patB));
  }

  // Verify persistence with large file
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    EXPECT_EQ(meta.totalRecords(), N);
    std::vector<bool> patA = {true, false};
    EXPECT_EQ(meta.getNullBitset(0), patA);
  }
}

// R12: RLE compression — same pattern counted, not stored individually
TEST_F(MetaTestFixture, test_rle_compression_structure) {
  rdb::Descriptor descriptor;
  descriptor.append({{"r", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {true};

  // 1000 records with the same pattern
  for (int i = 0; i < 1000; ++i)
    meta.onRecordAppended(pat);

  // All 1000 should be in a single currentEntry_ (no committed entries)
  EXPECT_EQ(meta.entries().size(), 0u);
  EXPECT_EQ(meta.currentEntry_.recordCount, 1000u);
  EXPECT_EQ(meta.totalRecords(), 1000u);

  // Force commit by switching pattern
  std::vector<bool> pat2 = {false};
  meta.onRecordAppended(pat2);

  // Now the 1000-record run should be one committed entry
  auto committed = meta.entries();
  EXPECT_EQ(committed.size(), 1u);
  EXPECT_EQ(committed[0].recordCount, 1000u);
  EXPECT_EQ(committed[0].nullBitset, pat);
  EXPECT_EQ(meta.currentEntry_.recordCount, 1u);
  EXPECT_EQ(meta.currentEntry_.nullBitset, pat2);
}

// ── Integration tests: null data arrival → IndexRecord aggregation ───

// Stream of all-null records must be aggregated into one IndexRecord
// with the correct recordCount, not stored individually.
TEST_F(MetaTestFixture, integration_all_null_stream_aggregated_into_single_index_record) {
  rdb::Descriptor descriptor;
  descriptor.append({{"a", 4, 0, rdb::INTEGER}, {"b", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> allNull = {true, true};

  for (int i = 0; i < 10; ++i)
    meta.onRecordAppended(allNull);

  // All 10 records share the same pattern — must stay in one in-memory entry
  EXPECT_EQ(meta.totalRecords(), 10u);
  EXPECT_EQ(meta.entries().size(), 0u);           // nothing committed to disk yet
  EXPECT_EQ(meta.currentEntry_.recordCount, 10u); // single RLE run
  EXPECT_EQ(meta.currentEntry_.nullBitset, allNull);
  EXPECT_TRUE(meta.currentEntry_.nullBitset[0]);
  EXPECT_TRUE(meta.currentEntry_.nullBitset[1]);
}

// When the null pattern changes after all-null records, the all-null
// run must be committed as one IndexRecord before a new entry starts.
TEST_F(MetaTestFixture, integration_all_null_run_committed_when_pattern_changes) {
  rdb::Descriptor descriptor;
  descriptor.append({{"a", 4, 0, rdb::INTEGER}, {"b", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> allNull = {true, true};
  std::vector<bool> mixed   = {true, false};

  meta.onRecordAppended(allNull);  // rec 0
  meta.onRecordAppended(allNull);  // rec 1
  meta.onRecordAppended(allNull);  // rec 2
  meta.onRecordAppended(mixed);    // rec 3 — triggers flush of all-null run

  auto committed = meta.entries();
  ASSERT_EQ(committed.size(), 1u);
  EXPECT_EQ(committed[0].recordCount, 3u);
  EXPECT_EQ(committed[0].nullBitset, allNull);
  EXPECT_FALSE(committed[0].isGap);

  // Current in-memory entry holds the mixed record
  EXPECT_EQ(meta.currentEntry_.recordCount, 1u);
  EXPECT_EQ(meta.currentEntry_.nullBitset, mixed);
}

// A realistic data sequence: all-null, mixed-null, no-null records
// must each produce a separate IndexRecord with correct counts.
TEST_F(MetaTestFixture, integration_realistic_sequence_produces_correct_index_records) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}, {"y", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> allNull = {true, true};   // e.g. "NULL NULL" rows
  std::vector<bool> mixed   = {true, false};  // e.g. "10 NULL" rows
  std::vector<bool> noNull  = {false, false}; // e.g. "20 30" rows

  // Simulate a stream: 2× all-null, 3× mixed, 4× no-null
  meta.onRecordAppended(allNull);  // rec 0
  meta.onRecordAppended(allNull);  // rec 1
  meta.onRecordAppended(mixed);    // rec 2 — flushes all-null run
  meta.onRecordAppended(mixed);    // rec 3
  meta.onRecordAppended(mixed);    // rec 4
  meta.onRecordAppended(noNull);   // rec 5 — flushes mixed run
  meta.onRecordAppended(noNull);   // rec 6
  meta.onRecordAppended(noNull);   // rec 7
  meta.onRecordAppended(noNull);   // rec 8

  EXPECT_EQ(meta.totalRecords(), 9u);

  // Two runs are committed; the last (noNull) is still in currentEntry_
  auto committed = meta.entries();
  ASSERT_EQ(committed.size(), 2u);

  EXPECT_EQ(committed[0].nullBitset, allNull);
  EXPECT_EQ(committed[0].recordCount, 2u);

  EXPECT_EQ(committed[1].nullBitset, mixed);
  EXPECT_EQ(committed[1].recordCount, 3u);

  EXPECT_EQ(meta.currentEntry_.nullBitset, noNull);
  EXPECT_EQ(meta.currentEntry_.recordCount, 4u);

  // Null-bitset queries must be consistent with the RLE structure
  EXPECT_EQ(meta.getNullBitset(0), allNull);
  EXPECT_EQ(meta.getNullBitset(1), allNull);
  EXPECT_EQ(meta.getNullBitset(2), mixed);
  EXPECT_EQ(meta.getNullBitset(4), mixed);
  EXPECT_EQ(meta.getNullBitset(5), noNull);
  EXPECT_EQ(meta.getNullBitset(8), noNull);
}

// IndexRecord entries must survive a full serialize → deserialize round-trip
// that includes all-null, mixed, and no-null patterns.
TEST_F(MetaTestFixture, integration_persistence_restores_aggregated_index_records) {
  rdb::Descriptor descriptor;
  descriptor.append({{"p", 4, 0, rdb::INTEGER}, {"q", 4, 0, rdb::INTEGER}});

  std::vector<bool> allNull = {true, true};
  std::vector<bool> mixed   = {true, false};
  std::vector<bool> noNull  = {false, false};

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    for (int i = 0; i < 5; ++i) meta.onRecordAppended(allNull);
    meta.onRecordAppended(mixed);
    meta.onRecordAppended(noNull);
    // destructor flushes currentEntry_ to disk
  }

  // Reload from disk and verify the full index is intact
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    EXPECT_EQ(meta.totalRecords(), 7u);

    auto committed = meta.entries();
    // After reload all entries are committed (currentEntry_ is the last one
    // restored as the in-memory accumulator with count=0 or the last segment)
    // Total must account for all 7 records across the segments
    size_t total = 0;
    for (const auto &e : committed) total += e.recordCount;
    total += meta.currentEntry_.recordCount;
    EXPECT_EQ(total, 7u);

    EXPECT_EQ(meta.getNullBitset(0), allNull);
    EXPECT_EQ(meta.getNullBitset(4), allNull);
    EXPECT_EQ(meta.getNullBitset(5), mixed);
    EXPECT_EQ(meta.getNullBitset(6), noNull);
  }
}

// isFieldNull must correctly reflect per-field null status inside an
// aggregated RLE entry that covers multiple consecutive records.
TEST_F(MetaTestFixture, integration_is_field_null_inside_aggregated_rle_entry) {
  rdb::Descriptor descriptor;
  descriptor.append({{"c1", 4, 0, rdb::INTEGER}, {"c2", 4, 0, rdb::INTEGER}, {"c3", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false, true, false}; // only field 1 is null

  for (int i = 0; i < 8; ++i)
    meta.onRecordAppended(pat);

  // All 8 records share the same pattern in one RLE accumulator
  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(meta.isFieldNull(i, 0)) << "rec " << i << " field 0";
    EXPECT_TRUE(meta.isFieldNull(i, 1))  << "rec " << i << " field 1";
    EXPECT_FALSE(meta.isFieldNull(i, 2)) << "rec " << i << " field 2";
  }
}
