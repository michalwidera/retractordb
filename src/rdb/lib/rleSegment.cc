#include "rdb/rleSegment.hpp"

#include <ranges>

namespace rdb {

std::vector<IndexRecord> splitSegment(const IndexRecord &seg, size_t offset, const std::vector<bool> &newBitset) {
  std::vector<IndexRecord> result;
  if (offset > 0) result.emplace_back(IndexRecord{.nullBitset = seg.nullBitset, .recordCount = offset, .isGap = false});
  result.emplace_back(IndexRecord{.nullBitset = newBitset, .recordCount = 1, .isGap = false});
  if (offset + 1 < seg.recordCount)
    result.emplace_back(IndexRecord{.nullBitset = seg.nullBitset, .recordCount = seg.recordCount - offset - 1, .isGap = false});
  return result;
}

size_t sumNonGapRecords(const std::vector<IndexRecord> &entries) {
  return std::ranges::fold_left(entries, 0UZ, [](size_t acc, const auto &e) { return acc + (e.isGap ? 0UZ : e.recordCount); });
}

}  // namespace rdb
