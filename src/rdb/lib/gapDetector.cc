#include "rdb/gapDetector.hpp"

#include <utility>

namespace rdb {

void GapDetector::configure(int nullFillCount) {
  nullFillCount_ = nullFillCount;
  configured_    = true;
}

bool GapDetector::absorb(bool isAllNull) {
  if (isAllNull) {
    consecutiveNullCount_++;
    if (std::cmp_greater(consecutiveNullCount_, nullFillCount_)) {
      activeGapDuration_++;
      return true;
    }
  } else {
    consecutiveNullCount_ = 0;
  }
  return false;
}

size_t GapDetector::takePendingGap() {
  const size_t gap   = activeGapDuration_;
  activeGapDuration_ = 0;
  return gap;
}

void GapDetector::resetCounters() {
  consecutiveNullCount_ = 0;
  activeGapDuration_    = 0;
}

}  // namespace rdb
