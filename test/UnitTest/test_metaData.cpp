#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>

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

  std::vector<std::byte> serialized             = original.serialize();
  rdb::metaDataStream::IndexRecord deserialized = rdb::metaDataStream::IndexRecord::deserialize(serialized);
  EXPECT_EQ(deserialized.recordCount, original.recordCount);
  EXPECT_EQ(deserialized.nullBitset, original.nullBitset);
}

TEST(MetaDataIndexRecordTest, test_IndexRecord_gap_serialization) {
  rdb::metaDataStream::IndexRecord gap;
  gap.isGap       = true;
  gap.recordCount = 5;
  gap.nullBitset  = {true, true};

  auto serialized   = gap.serialize();
  auto deserialized = rdb::metaDataStream::IndexRecord::deserialize(serialized);
  EXPECT_TRUE(deserialized.isGap);
  EXPECT_EQ(deserialized.recordCount, 5u);
  EXPECT_EQ(deserialized.nullBitset, gap.nullBitset);
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
    EXPECT_TRUE(meta.getNullBitset(0)[0]);
    EXPECT_FALSE(meta.getNullBitset(2)[0]);
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

TEST_F(MetaTestFixture, test_transmission_gap_positioning) {
  rdb::Descriptor descriptor;
  descriptor.append({{"t", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false};

  meta.onRecordAppended(pat);

  // Gap inserted after one record should be at record index 1.
  meta.onTransmissionGap();
  size_t gapCount = 0, gapPos = 0, cumulative = 0;
  for (const auto &e : meta.segments()) {
    if (e.isGap) {
      gapPos = cumulative;
      ++gapCount;
    } else
      cumulative += e.recordCount;
  }
  ASSERT_EQ(gapCount, 1u);
  EXPECT_EQ(gapPos, 1u);
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

  // recs 0-2 are now committed on disk, rec 3 is pending in memory
  EXPECT_EQ(meta.segments().size(), 2u);

  // Modify rec 1 (committed on disk) — triggers rewriteFile
  meta.onRecordModified(1, noNull_);

  EXPECT_EQ(meta.totalRecords(), 4u);
  EXPECT_EQ(meta.getNullBitset(0), null_);
  EXPECT_EQ(meta.getNullBitset(1), noNull_);  // modified on disk
  EXPECT_EQ(meta.getNullBitset(2), null_);
  EXPECT_EQ(meta.getNullBitset(3), noNull_);

  // Verify: 3 committed segments + 1 pending = 4 total
  EXPECT_EQ(meta.segments().size(), 4u);
}

// R6: All data in file except last entry buffered in memory
TEST_F(MetaTestFixture, test_committed_on_disk_current_in_memory) {
  rdb::Descriptor descriptor;
  descriptor.append({{"a", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {true};
  std::vector<bool> patB = {false};

  // Same pattern — only one segment visible
  meta.onRecordAppended(patA);                    // rec 0
  meta.onRecordAppended(patA);                    // rec 1
  EXPECT_EQ(meta.segments().size(), 1u);
  EXPECT_EQ(meta.segments()[0].recordCount, 2u);

  // Different pattern — patA flushed, patB becomes current
  meta.onRecordAppended(patB);           // rec 2
  EXPECT_EQ(meta.segments().size(), 2u);
  EXPECT_EQ(meta.segments()[0].recordCount, 2u);
  EXPECT_EQ(meta.segments()[0].nullBitset, patA);
  EXPECT_EQ(meta.segments().back().recordCount, 1u);
  EXPECT_EQ(meta.segments().back().nullBitset, patB);
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

  // All 1000 in a single segment
  EXPECT_EQ(meta.segments().size(), 1u);
  EXPECT_EQ(meta.segments()[0].recordCount, 1000u);
  EXPECT_EQ(meta.totalRecords(), 1000u);

  // Force commit by switching pattern
  std::vector<bool> pat2 = {false};
  meta.onRecordAppended(pat2);

  // Now: 1000-record run + pat2 run = 2 segments
  auto segs = meta.segments();
  EXPECT_EQ(segs.size(), 2u);
  EXPECT_EQ(segs[0].recordCount, 1000u);
  EXPECT_EQ(segs[0].nullBitset, pat);
  EXPECT_EQ(segs[1].recordCount, 1u);
  EXPECT_EQ(segs[1].nullBitset, pat2);
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

  // All 10 records share the same pattern — one segment
  EXPECT_EQ(meta.totalRecords(), 10u);
  EXPECT_EQ(meta.segments().size(), 1u);
  EXPECT_EQ(meta.segments()[0].recordCount, 10u);
  EXPECT_EQ(meta.segments()[0].nullBitset, allNull);
  EXPECT_TRUE(meta.segments()[0].nullBitset[0]);
  EXPECT_TRUE(meta.segments()[0].nullBitset[1]);
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

  auto segs = meta.segments();
  ASSERT_EQ(segs.size(), 2u);
  EXPECT_EQ(segs[0].recordCount, 3u);
  EXPECT_EQ(segs[0].nullBitset, allNull);
  EXPECT_EQ(segs[1].recordCount, 1u);
  EXPECT_EQ(segs[1].nullBitset, mixed);
}

// A realistic data sequence: all-null, mixed-null, no-null records
// must each produce a separate IndexRecord with correct counts.
TEST_F(MetaTestFixture, integration_realistic_sequence_produces_correct_index_records) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}, {"y", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> allNull = {true, true};    // e.g. "NULL NULL" rows
  std::vector<bool> mixed   = {true, false};   // e.g. "10 NULL" rows
  std::vector<bool> noNull  = {false, false};  // e.g. "20 30" rows

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

  auto segs = meta.segments();
  ASSERT_EQ(segs.size(), 3u);

  EXPECT_EQ(segs[0].nullBitset, allNull);
  EXPECT_EQ(segs[0].recordCount, 2u);

  EXPECT_EQ(segs[1].nullBitset, mixed);
  EXPECT_EQ(segs[1].recordCount, 3u);

  EXPECT_EQ(segs[2].nullBitset, noNull);
  EXPECT_EQ(segs[2].recordCount, 4u);

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
    for (int i = 0; i < 5; ++i)
      meta.onRecordAppended(allNull);
    meta.onRecordAppended(mixed);
    meta.onRecordAppended(noNull);
    // destructor flushes currentEntry_ to disk
  }

  // Reload from disk and verify the full index is intact
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    EXPECT_EQ(meta.totalRecords(), 7u);

    size_t total = 0;
    for (const auto &e : meta.segments())
      total += e.recordCount;
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
  std::vector<bool> pat = {false, true, false};  // only field 1 is null

  for (int i = 0; i < 8; ++i)
    meta.onRecordAppended(pat);

  // All 8 records share the same pattern in one RLE accumulator
  for (int i = 0; i < 8; ++i) {
    EXPECT_FALSE(meta.getNullBitset(i)[0]) << "rec " << i << " field 0";
    EXPECT_TRUE(meta.getNullBitset(i)[1]) << "rec " << i << " field 1";
    EXPECT_FALSE(meta.getNullBitset(i)[2]) << "rec " << i << " field 2";
  }
}

// ── Integration tests: metaDataStream with payload nullBitset ────────

TEST_F(MetaTestFixture, integration_metadata_with_payload_nulls) {
  rdb::Descriptor descriptor;
  descriptor.append({{"val1", 4, 0, rdb::INTEGER}, {"val2", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);

  // Simulate payload null handling
  std::vector<bool> fieldNullsA = {true, false};   // val1 is null
  std::vector<bool> fieldNullsB = {false, true};   // val2 is null
  std::vector<bool> fieldNullsC = {false, false};  // no nulls

  // Record stream with different null patterns
  meta.onRecordAppended(fieldNullsA);
  meta.onRecordAppended(fieldNullsA);
  meta.onRecordAppended(fieldNullsB);
  meta.onRecordAppended(fieldNullsC);
  meta.onRecordAppended(fieldNullsC);

  EXPECT_EQ(meta.totalRecords(), 5u);

  // Verify each record's null pattern
  EXPECT_TRUE(meta.getNullBitset(0)[0]);   // rec 0, val1
  EXPECT_FALSE(meta.getNullBitset(0)[1]);  // rec 0, val2

  EXPECT_TRUE(meta.getNullBitset(1)[0]);   // rec 1, val1
  EXPECT_FALSE(meta.getNullBitset(1)[1]);  // rec 1, val2

  EXPECT_FALSE(meta.getNullBitset(2)[0]);  // rec 2, val1
  EXPECT_TRUE(meta.getNullBitset(2)[1]);   // rec 2, val2

  EXPECT_FALSE(meta.getNullBitset(3)[0]);  // rec 3, val1
  EXPECT_FALSE(meta.getNullBitset(3)[1]);  // rec 3, val2

  EXPECT_FALSE(meta.getNullBitset(4)[0]);  // rec 4, val1
  EXPECT_FALSE(meta.getNullBitset(4)[1]);  // rec 4, val2
}

TEST_F(MetaTestFixture, integration_storage_operations_with_meta) {
  rdb::Descriptor descriptor;
  descriptor.append({{"id", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);

  // Simulate storage operations - test with records committed to disk
  std::vector<bool> patA = {false};
  std::vector<bool> patB = {true};

  // Write pattern A (forced to disk), then pattern B (stays in memory)
  meta.onRecordAppended(patA);  // rec 0
  meta.onRecordAppended(patA);  // rec 1
  meta.onRecordAppended(patA);  // rec 2
  meta.onRecordAppended(patA);  // rec 3
  meta.onRecordAppended(patB);  // rec 4 - forces patA to disk

  EXPECT_EQ(meta.totalRecords(), 5u);
  EXPECT_EQ(meta.segments().size(), 2u);
  EXPECT_EQ(meta.segments().back().recordCount, 1u);

  // Test modify on committed entry
  meta.onRecordModified(0, patB);
  EXPECT_EQ(meta.getNullBitset(0), patB);
  EXPECT_EQ(meta.getNullBitset(4), patB);
}

TEST_F(MetaTestFixture, integration_gap_markers_with_operations) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> normal = {false};
  std::vector<bool> error  = {true};

  // Build sequence: normal data -> gap -> recovery -> gap -> normal data
  meta.onRecordAppended(normal);  // rec 0
  meta.onRecordAppended(normal);  // rec 1
  meta.onTransmissionGap();       // gap before rec 2
  meta.onRecordAppended(error);   // rec 2 - error state
  meta.onRecordAppended(error);   // rec 3 - error state
  meta.onTransmissionGap();       // gap before rec 4
  meta.onRecordAppended(normal);  // rec 4 - recovered
  meta.onRecordAppended(normal);  // rec 5 - recovered

  EXPECT_EQ(meta.totalRecords(), 6u);

  // Check gap positions
  EXPECT_FALSE(meta.isGapBefore(0));
  EXPECT_FALSE(meta.isGapBefore(1));
  EXPECT_TRUE(meta.isGapBefore(2));  // gap before error sequence
  EXPECT_FALSE(meta.isGapBefore(3));
  EXPECT_TRUE(meta.isGapBefore(4));  // gap before recovery

  {
    auto segs    = meta.segments();
    size_t gapCnt = std::count_if(segs.begin(), segs.end(), [](const auto &e) { return e.isGap; });
    EXPECT_EQ(gapCnt, 2u);
  }

  // Modify records in error state
  meta.onRecordModified(2, normal);
  meta.onRecordModified(3, normal);

  // Verify modification
  EXPECT_EQ(meta.getNullBitset(2), normal);
  EXPECT_EQ(meta.getNullBitset(3), normal);
}

TEST_F(MetaTestFixture, integration_reset_after_complex_operations) {
  rdb::Descriptor descriptor;
  descriptor.append({{"data", 8, 0, rdb::DOUBLE}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false};

  // Build complex index
  for (int i = 0; i < 50; ++i)
    meta.onRecordAppended(pat);
  meta.onTransmissionGap();
  for (int i = 0; i < 50; ++i)
    meta.onRecordAppended(pat);

  EXPECT_EQ(meta.totalRecords(), 100u);
  {
    auto e1 = meta.segments();
    EXPECT_EQ(std::count_if(e1.begin(), e1.end(), [](const auto &e) { return e.isGap; }), 1u);
  }

  // Reset and verify
  meta.reset();

  EXPECT_TRUE(meta.isEmpty());
  EXPECT_EQ(meta.totalRecords(), 0u);
  {
    auto e2 = meta.segments();
    EXPECT_EQ(std::count_if(e2.begin(), e2.end(), [](const auto &e) { return e.isGap; }), 0u);
  }

  // Should be reusable after reset
  meta.onRecordAppended(pat);
  EXPECT_EQ(meta.totalRecords(), 1u);
}

// ── Tests for new functionality: isEmpty, reset, purge, gap queries via entries() ───

TEST_F(MetaTestFixture, test_isEmpty_initial_state) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  EXPECT_TRUE(meta.isEmpty());
  EXPECT_EQ(meta.totalRecords(), 0u);
}

TEST_F(MetaTestFixture, test_isEmpty_after_append) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {true};

  meta.onRecordAppended(pat);
  EXPECT_FALSE(meta.isEmpty());
  EXPECT_EQ(meta.totalRecords(), 1u);
}

TEST_F(MetaTestFixture, test_reset_clears_all_data) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    std::vector<bool> pat = {true};

    // Add 10 records
    for (int i = 0; i < 10; ++i)
      meta.onRecordAppended(pat);

    EXPECT_EQ(meta.totalRecords(), 10u);
    EXPECT_FALSE(meta.isEmpty());

    // Reset
    meta.reset();

    EXPECT_EQ(meta.totalRecords(), 0u);
    EXPECT_TRUE(meta.isEmpty());
  }

  // Verify file is empty after reload
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    EXPECT_TRUE(meta.isEmpty());
    EXPECT_EQ(meta.totalRecords(), 0u);
  }
}

TEST_F(MetaTestFixture, test_reset_preserves_metadata) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);

  std::vector<bool> pat = {true};
  meta.onRecordAppended(pat);

  // Reset
  meta.reset();

  // Reset preserves metadata stream usability
  EXPECT_EQ(meta.totalRecords(), 0u);
}

TEST_F(MetaTestFixture, test_transmission_gaps_retrieval) {
  rdb::Descriptor descriptor;
  descriptor.append({{"v", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false};

  meta.onRecordAppended(pat);  // rec 0
  meta.onRecordAppended(pat);  // rec 1
  meta.onTransmissionGap();    // gap before record 2
  meta.onRecordAppended(pat);  // rec 2
  meta.onRecordAppended(pat);  // rec 3
  meta.onTransmissionGap();    // gap before record 4
  meta.onRecordAppended(pat);  // rec 4

  std::vector<size_t> gapPositions;
  size_t cum = 0;
  for (const auto &e : meta.segments()) {
    if (e.isGap)
      gapPositions.push_back(cum);
    else
      cum += e.recordCount;
  }
  EXPECT_EQ(gapPositions.size(), 2u);
  EXPECT_EQ(gapPositions[0], 2u);
  EXPECT_EQ(gapPositions[1], 4u);
}

TEST_F(MetaTestFixture, test_transmission_gaps_empty) {
  rdb::Descriptor descriptor;
  descriptor.append({{"v", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> pat = {false};

  meta.onRecordAppended(pat);
  meta.onRecordAppended(pat);

  {
    auto e = meta.segments();
    EXPECT_EQ(std::count_if(e.begin(), e.end(), [](const auto &x) { return x.isGap; }), 0u);
  }
}

TEST_F(MetaTestFixture, test_transmission_gaps_persistence) {
  rdb::Descriptor descriptor;
  descriptor.append({{"v", 4, 0, rdb::INTEGER}});

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    std::vector<bool> pat = {false};

    meta.onRecordAppended(pat);
    meta.onTransmissionGap();
    meta.onRecordAppended(pat);
    meta.onTransmissionGap();

    {
      auto e = meta.segments();
      EXPECT_EQ(std::count_if(e.begin(), e.end(), [](const auto &x) { return x.isGap; }), 2u);
    }
  }

  // Reload and verify gaps are persisted
  {
    rdb::metaDataStream meta(descriptor, metaFile);
    auto e = meta.segments();
    EXPECT_EQ(std::count_if(e.begin(), e.end(), [](const auto &x) { return x.isGap; }), 2u);
  }
}

// ── Lazy overwrite (tailDirty_) tests ────────────────────────────────

// When the same RLE pattern resumes after a flush, the disk entry count
// must reflect the merged total — not be split into two entries.
TEST_F(MetaTestFixture, test_lazy_overwrite_merged_count) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {true};

  meta.onRecordAppended(patA);  // rec 0
  meta.onRecordAppended(patA);  // rec 1
  meta.flushCurrentEntry();     // disk: [A,2]
  meta.onRecordAppended(patA);  // rec 2 — tailDirty_=true, currentEntry_={A,3}
  meta.onRecordAppended(patA);  // rec 3 — currentEntry_={A,4}
  meta.flushCurrentEntry();     // overwrite disk: [A,4]

  auto committed = meta.segments();
  ASSERT_EQ(committed.size(), 1u);
  EXPECT_EQ(committed[0].recordCount, 4u);
  EXPECT_EQ(committed[0].nullBitset, patA);
  EXPECT_EQ(meta.totalRecords(), 4u);
}

// File size must not shrink when the lazy overwrite path is taken.
// The old truncate-based approach would shrink the file to header-only size;
// seek-overwrite keeps it stable at header + one entry.
TEST_F(MetaTestFixture, test_lazy_overwrite_file_size_stable) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  // entrySize for 1 field: uint8_t(1) + size_t(8) + size_t(8) + ceil(1/8)(1) = 18
  // kHeaderSize = sizeof(int64_t) = 8
  constexpr std::uintmax_t kExpectedSize = 8u + 18u;

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {true};

  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.flushCurrentEntry();  // disk: [A,2]

  EXPECT_EQ(std::filesystem::file_size(metaFile), kExpectedSize);

  meta.onRecordAppended(patA);  // triggers tailDirty_ — file must NOT shrink

  EXPECT_EQ(std::filesystem::file_size(metaFile), kExpectedSize);

  meta.flushCurrentEntry();  // overwrite — still same size

  EXPECT_EQ(std::filesystem::file_size(metaFile), kExpectedSize);
}

// Merged count must survive a full serialize → reload round-trip.
TEST_F(MetaTestFixture, test_lazy_overwrite_persistence) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});
  std::vector<bool> patA = {false};

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    meta.onRecordAppended(patA);  // rec 0
    meta.onRecordAppended(patA);  // rec 1
    meta.flushCurrentEntry();     // disk: [A,2]
    meta.onRecordAppended(patA);  // rec 2 — tailDirty_=true
    meta.onRecordAppended(patA);  // rec 3
    // destructor: overwrite disk → [A,4]
  }

  rdb::metaDataStream meta(descriptor, metaFile);
  EXPECT_EQ(meta.totalRecords(), 4u);
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(meta.getNullBitset(i), patA) << "rec " << i;
}

// A transmission gap must trigger the overwrite of the dirty tail entry
// before the gap marker is appended.
TEST_F(MetaTestFixture, test_lazy_overwrite_gap_flushes_dirty_tail) {
  rdb::Descriptor descriptor;
  descriptor.append({{"v", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {false};

  meta.onRecordAppended(patA);  // rec 0
  meta.onRecordAppended(patA);  // rec 1
  meta.onRecordAppended(patA);  // rec 2
  meta.flushCurrentEntry();     // disk: [A,3]
  meta.onRecordAppended(patA);  // rec 3 — tailDirty_=true, currentEntry_={A,4}
  meta.onTransmissionGap();     // must overwrite [A,3]→[A,4] then append gap

  auto committed = meta.segments();
  ASSERT_EQ(committed.size(), 2u);
  EXPECT_FALSE(committed[0].isGap);
  EXPECT_EQ(committed[0].recordCount, 4u);
  EXPECT_TRUE(committed[1].isGap);
  EXPECT_TRUE(meta.isGapBefore(4));
  EXPECT_EQ(meta.totalRecords(), 4u);
}

// Multiple consecutive flush → re-merge cycles must each produce
// the correct merged count without accumulating stale entries.
TEST_F(MetaTestFixture, test_lazy_overwrite_multiple_cycles) {
  rdb::Descriptor descriptor;
  descriptor.append({{"z", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {false};
  std::vector<bool> patB = {true};

  // Cycle 1: flush A×3, re-merge A×2 → total A×5
  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.flushCurrentEntry();     // disk: [A,3]
  meta.onRecordAppended(patA);  // tailDirty_=true, {A,4}
  meta.onRecordAppended(patA);  // {A,5}

  // Cycle 2: switch to B — triggers overwrite [A,3]→[A,5], then B accumulates
  meta.onRecordAppended(patB);  // flush A×5 via overwrite, currentEntry_={B,1}
  meta.flushCurrentEntry();     // disk: [A,5][B,1]
  meta.onRecordAppended(patB);  // tailDirty_=true, {B,2}
  meta.onRecordAppended(patB);  // {B,3}
  meta.flushCurrentEntry();     // overwrite [B,1]→[B,3]

  auto committed = meta.segments();
  ASSERT_EQ(committed.size(), 2u);
  EXPECT_EQ(committed[0].nullBitset, patA);
  EXPECT_EQ(committed[0].recordCount, 5u);
  EXPECT_EQ(committed[1].nullBitset, patB);
  EXPECT_EQ(committed[1].recordCount, 3u);
  EXPECT_EQ(meta.totalRecords(), 8u);
}
