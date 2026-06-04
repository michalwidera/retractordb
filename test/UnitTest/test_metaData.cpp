#include <gtest/gtest.h>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>

struct MetaTestFixture : public ::testing::Test {
  const std::string metaFile = "test_metaDataStream.meta";
  void TearDown() override { std::remove(metaFile.c_str()); }
};

// ── Low-level serialization ──────────────────────────────────────────

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
  EXPECT_EQ(deserialized.recordCount, 5U);
  EXPECT_EQ(deserialized.nullBitset, gap.nullBitset);
}

// ── onRecordModified: committed-on-disk path (rewriteFile) ──────────
//
// Scenario-6 in usage tests only hits the in-memory (pending) path.
// This test covers the rewriteFile code path triggered when the modified
// record has already been committed to disk by a pattern change.

TEST_F(MetaTestFixture, test_modify_committed_entry_on_disk) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> null_   = {true};
  std::vector<bool> noNull_ = {false};

  meta.onRecordAppended(null_);    // rec 0
  meta.onRecordAppended(null_);    // rec 1
  meta.onRecordAppended(null_);    // rec 2
  meta.onRecordAppended(noNull_);  // rec 3 — forces recs 0-2 to disk

  EXPECT_EQ(meta.segments().size(), 2U);

  meta.onRecordModified(1, noNull_);  // triggers rewriteFile

  EXPECT_EQ(meta.totalRecords(), 4U);
  EXPECT_EQ(meta.getNullBitset(0), null_);
  EXPECT_EQ(meta.getNullBitset(1), noNull_);
  EXPECT_EQ(meta.getNullBitset(2), null_);
  EXPECT_EQ(meta.getNullBitset(3), noNull_);

  // [null_,2] split → [null_,1][noNull_,1][null_,1] + pending [noNull_,1] = 4 segments
  EXPECT_EQ(meta.segments().size(), 4U);
}

// ── Stress ───────────────────────────────────────────────────────────

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
    EXPECT_EQ(meta.getNullBitset(0), patA);
    EXPECT_EQ(meta.getNullBitset(49), patA);
    EXPECT_EQ(meta.getNullBitset(50), patB);
    EXPECT_EQ(meta.getNullBitset(99), patB);
    EXPECT_EQ(meta.getNullBitset(N - 1), (((N - 1) % 100 < 50) ? patA : patB));
  }

  rdb::metaDataStream meta(descriptor, metaFile);
  EXPECT_EQ(meta.totalRecords(), N);
  std::vector<bool> patA = {true, false};
  EXPECT_EQ(meta.getNullBitset(0), patA);
}

// ── Reset persistence ────────────────────────────────────────────────
//
// Scenario-7 in usage tests verifies in-memory reset behaviour.
// This test adds the persistence dimension: after reset + destructor,
// a freshly loaded instance must see an empty stream.

TEST_F(MetaTestFixture, test_reset_clears_all_data) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  {
    rdb::metaDataStream meta(descriptor, metaFile);
    std::vector<bool> pat = {true};
    for (int i = 0; i < 10; ++i)
      meta.onRecordAppended(pat);
    EXPECT_EQ(meta.totalRecords(), 10U);
    meta.reset();
    EXPECT_EQ(meta.totalRecords(), 0U);
    EXPECT_TRUE(meta.isEmpty());
  }

  rdb::metaDataStream meta(descriptor, metaFile);
  EXPECT_TRUE(meta.isEmpty());
  EXPECT_EQ(meta.totalRecords(), 0U);
}

// ── Gap + modify interaction ─────────────────────────────────────────

TEST_F(MetaTestFixture, integration_gap_markers_with_operations) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> normal = {false};
  std::vector<bool> error  = {true};

  meta.onRecordAppended(normal);  // rec 0
  meta.onRecordAppended(normal);  // rec 1
  meta.onTransmissionGap();       // gap before rec 2
  meta.onRecordAppended(error);   // rec 2
  meta.onRecordAppended(error);   // rec 3
  meta.onTransmissionGap();       // gap before rec 4
  meta.onRecordAppended(normal);  // rec 4
  meta.onRecordAppended(normal);  // rec 5

  EXPECT_EQ(meta.totalRecords(), 6U);

  EXPECT_FALSE(meta.isGapBefore(0));
  EXPECT_FALSE(meta.isGapBefore(1));
  EXPECT_TRUE(meta.isGapBefore(2));
  EXPECT_FALSE(meta.isGapBefore(3));
  EXPECT_TRUE(meta.isGapBefore(4));

  {
    auto segs     = meta.segments();
    size_t gapCnt = std::count_if(segs.begin(), segs.end(), [](const auto &e) { return e.isGap; });
    EXPECT_EQ(gapCnt, 2U);
  }

  meta.onRecordModified(2, normal);
  meta.onRecordModified(3, normal);

  EXPECT_EQ(meta.getNullBitset(2), normal);
  EXPECT_EQ(meta.getNullBitset(3), normal);
}

// ── Gap persistence ──────────────────────────────────────────────────

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

    auto e = meta.segments();
    EXPECT_EQ(std::count_if(e.begin(), e.end(), [](const auto &x) { return x.isGap; }), 2U);
  }

  rdb::metaDataStream meta(descriptor, metaFile);
  auto e = meta.segments();
  EXPECT_EQ(std::count_if(e.begin(), e.end(), [](const auto &x) { return x.isGap; }), 2U);
}

// ── Lazy overwrite (tailDirty_) ──────────────────────────────────────

// File size must not change when the overwrite-in-place path is taken.
TEST_F(MetaTestFixture, test_lazy_overwrite_file_size_stable) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  // entrySize for 1 field: uint8_t(1) + size_t(8) + size_t(8) + ceil(1/8)(1) = 18
  // kHeaderSize = sizeof(int64_t) = 8
  constexpr std::uintmax_t kExpectedSize = 8U + 18U;

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {true};

  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.flushCurrentEntry();

  EXPECT_EQ(std::filesystem::file_size(metaFile), kExpectedSize);

  meta.onRecordAppended(patA);  // tailDirty_=true — file must NOT shrink

  EXPECT_EQ(std::filesystem::file_size(metaFile), kExpectedSize);

  meta.flushCurrentEntry();  // overwrite-in-place — still same size

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
  EXPECT_EQ(meta.totalRecords(), 4U);
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
  ASSERT_EQ(committed.size(), 2U);
  EXPECT_FALSE(committed[0].isGap);
  EXPECT_EQ(committed[0].recordCount, 4U);
  EXPECT_TRUE(committed[1].isGap);
  EXPECT_TRUE(meta.isGapBefore(4));
  EXPECT_EQ(meta.totalRecords(), 4U);
}

// ── metaDataStream::rotate() ────────────────────────────────────────────────

struct RotateFixture : public ::testing::Test {
  const std::string meta     = "rotate_test.meta";
  const std::string metaOld0 = meta + ".old0";
  const std::string metaOld1 = meta + ".old1";
  rdb::Descriptor descriptor;

  void SetUp() override { descriptor.append({{"v", 4, 0, rdb::INTEGER}}); }
  void TearDown() override {
    for (const auto &f : {meta, metaOld0, metaOld1})
      std::remove(f.c_str());
  }
};

// rotate() renames current file to .old<N> and clears all in-memory state.
TEST_F(RotateFixture, rotate_renames_file_and_clears_state) {
  {
    rdb::metaDataStream m(descriptor, meta);
    m.onRecordAppended({false});
    m.onRecordAppended({false});
    m.onRecordAppended({true});
  }

  rdb::metaDataStream m(descriptor, meta);
  ASSERT_EQ(m.totalRecords(), 3U);

  m.rotate(0);

  EXPECT_TRUE(m.isEmpty());
  EXPECT_EQ(m.totalRecords(), 0U);
  EXPECT_TRUE(std::filesystem::exists(metaOld0));
  EXPECT_TRUE(std::filesystem::exists(meta));
}

// rotate(-1) resets state without renaming the file.
TEST_F(RotateFixture, rotate_negative_percounter_resets_without_rename) {
  {
    rdb::metaDataStream m(descriptor, meta);
    m.onRecordAppended({false});
    m.onRecordAppended({true});
  }

  rdb::metaDataStream m(descriptor, meta);
  ASSERT_EQ(m.totalRecords(), 2U);

  m.rotate(-1);

  EXPECT_TRUE(m.isEmpty());
  EXPECT_FALSE(std::filesystem::exists(meta + ".old-1"));
}

// Renamed .old file contains the original records.
TEST_F(RotateFixture, rotate_preserves_data_in_renamed_file) {
  {
    rdb::metaDataStream m(descriptor, meta);
    m.onRecordAppended({false});
    m.onRecordAppended({true});
  }

  {
    rdb::metaDataStream m(descriptor, meta);
    m.rotate(1);
  }

  rdb::metaDataStream old(descriptor, metaOld1);
  EXPECT_EQ(old.totalRecords(), 2U);
  EXPECT_EQ(old.getNullBitset(0), (std::vector<bool>{false}));
  EXPECT_EQ(old.getNullBitset(1), (std::vector<bool>{true}));
}

// After rotation, new records can be appended and queried correctly.
TEST_F(RotateFixture, rotate_allows_records_after_rotation) {
  {
    rdb::metaDataStream m(descriptor, meta);
    for (int i = 0; i < 5; ++i)
      m.onRecordAppended({false});
  }

  rdb::metaDataStream m(descriptor, meta);
  m.rotate(0);

  m.onRecordAppended({false});
  m.onRecordAppended({true});
  EXPECT_EQ(m.totalRecords(), 2U);
  EXPECT_EQ(m.getNullBitset(1), (std::vector<bool>{true}));
}

// ── Multiple consecutive flush → re-merge cycles must each produce
// the correct merged count without accumulating stale entries.
TEST_F(MetaTestFixture, test_lazy_overwrite_multiple_cycles) {
  rdb::Descriptor descriptor;
  descriptor.append({{"z", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  std::vector<bool> patA = {false};
  std::vector<bool> patB = {true};

  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.onRecordAppended(patA);
  meta.flushCurrentEntry();     // disk: [A,3]
  meta.onRecordAppended(patA);  // tailDirty_=true, {A,4}
  meta.onRecordAppended(patA);  // {A,5}

  meta.onRecordAppended(patB);  // flush A×5 via overwrite, currentEntry_={B,1}
  meta.flushCurrentEntry();     // disk: [A,5][B,1]
  meta.onRecordAppended(patB);  // tailDirty_=true, {B,2}
  meta.onRecordAppended(patB);  // {B,3}
  meta.flushCurrentEntry();     // overwrite [B,1]→[B,3]

  auto committed = meta.segments();
  ASSERT_EQ(committed.size(), 2U);
  EXPECT_EQ(committed[0].nullBitset, patA);
  EXPECT_EQ(committed[0].recordCount, 5U);
  EXPECT_EQ(committed[1].nullBitset, patB);
  EXPECT_EQ(committed[1].recordCount, 3U);
  EXPECT_EQ(meta.totalRecords(), 8U);
}

// ── tailDirty_ + onRecordModified interaction ────────────────────────
//
// Bug: when tailDirty_=true, onRecordModified() on a non-last record in
// currentEntry_ used to appendEntry() after the stale on-disk entry
// instead of overwriting it. The stale entry remained on disk and shadowed
// the new split entries, causing getNullBitset() to return the wrong pattern.

TEST_F(MetaTestFixture, test_modify_non_last_in_current_entry_while_tail_dirty) {
  rdb::Descriptor descriptor;
  descriptor.append({{"x", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  const std::vector<bool> A = {false};
  const std::vector<bool> B = {true};

  // Set up tailDirty_: flush 3 records, then append same-pattern record (no flush).
  meta.onRecordAppended(A);  // rec 0
  meta.onRecordAppended(A);  // rec 1
  meta.onRecordAppended(A);  // rec 2
  meta.flushCurrentEntry();  // disk: [{A,3}], pendingCommittedCount_=3

  meta.onRecordAppended(A);  // rec 3 — tailDirty_=true, currentEntry_={A,4}, committed=0

  // Modify non-last record in currentEntry_ (rec 1, offset 1, two records follow).
  // The stale on-disk entry [{A,3}] must be overwritten with the prefix [{A,1}],
  // not left as-is with new entries appended after it.
  meta.onRecordModified(1, B);

  meta.flushCurrentEntry();  // flush suffix {A,2}

  ASSERT_EQ(meta.totalRecords(), 4U);
  EXPECT_EQ(meta.getNullBitset(0), A);  // prefix
  EXPECT_EQ(meta.getNullBitset(1), B);  // modified
  EXPECT_EQ(meta.getNullBitset(2), A);  // suffix
  EXPECT_EQ(meta.getNullBitset(3), A);  // suffix (cont.)

  // Disk must contain exactly 3 segments: [{A,1},{B,1},{A,2}].
  auto segs = meta.segments();
  ASSERT_EQ(segs.size(), 3U);
  EXPECT_EQ(segs[0].recordCount, 1U);
  EXPECT_EQ(segs[0].nullBitset, A);
  EXPECT_EQ(segs[1].recordCount, 1U);
  EXPECT_EQ(segs[1].nullBitset, B);
  EXPECT_EQ(segs[2].recordCount, 2U);
  EXPECT_EQ(segs[2].nullBitset, A);
}

// ── tailDirty_ + onRecordModified on a committed (on-disk) entry ──────
//
// Bug: when tailDirty_=true (last on-disk entry was re-absorbed into
// currentEntry_ via lazy overwrite) and onRecordModified() targets an
// EARLIER committed entry, applyModificationToMainIndex() used to rewrite
// the file from readCommittedEntries(), which still contained the stale
// tail. The stale entry was then persisted AND counted, while currentEntry_
// still represented the same records — double counting (totalRecords off
// by the stale count) and wrong patterns at the logical tail.

TEST_F(MetaTestFixture, test_modify_committed_entry_while_tail_dirty) {
  rdb::Descriptor descriptor;
  descriptor.append({{"f1", 4, 0, rdb::INTEGER}, {"f2", 4, 0, rdb::INTEGER}});

  rdb::metaDataStream meta(descriptor, metaFile);
  const std::vector<bool> A = {true, false};
  const std::vector<bool> B = {false, true};
  const std::vector<bool> C = {true, true};

  meta.onRecordAppended(A);  // rec 0
  meta.onRecordAppended(A);  // rec 1
  meta.onRecordAppended(A);  // rec 2
  meta.flushCurrentEntry();  // disk: [{A,3}]
  meta.onRecordAppended(B);  // rec 3
  meta.flushCurrentEntry();  // disk: [{A,3},{B,1}]
  meta.onRecordAppended(B);  // rec 4 — re-absorb {B,1}: tailDirty_=true, currentEntry_={B,2}

  ASSERT_EQ(meta.totalRecords(), 5U);

  // Modify an earlier committed record (rec 1) while the on-disk tail is stale.
  meta.onRecordModified(1, C);

  ASSERT_EQ(meta.totalRecords(), 5U);
  EXPECT_EQ(meta.getNullBitset(0), A);
  EXPECT_EQ(meta.getNullBitset(1), C);  // modified
  EXPECT_EQ(meta.getNullBitset(2), A);
  EXPECT_EQ(meta.getNullBitset(3), B);
  EXPECT_EQ(meta.getNullBitset(4), B);

  // [{A,1},{C,1},{A,1}] on disk + pending {B,2} = 4 segments, no stale {B,1}.
  auto segs = meta.segments();
  ASSERT_EQ(segs.size(), 4U);
  EXPECT_EQ(segs[3].recordCount, 2U);
  EXPECT_EQ(segs[3].nullBitset, B);
}
