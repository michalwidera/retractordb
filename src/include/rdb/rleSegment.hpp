#pragma once

#include <cstddef>
#include <vector>

#include "indexRecord.hpp"

namespace rdb {

/// @brief Split an RLE segment at @p offset, replacing that one record with @p newBitset.
///
/// Used when a single record inside a multi-record RLE segment gets a corrected null pattern.
/// @return 1 to 3 parts, in order: prefix (if offset > 0), the replaced record, suffix (if any).
std::vector<IndexRecord> splitSegment(const IndexRecord &seg, size_t offset, const std::vector<bool> &newBitset);

/// @brief Sum recordCount across all non-gap entries (gap markers don't consume record indices).
size_t sumNonGapRecords(const std::vector<IndexRecord> &entries);

}  // namespace rdb
