#include "rdb/storageShadow.hpp"

#include <stdexcept>
#include <utility>

namespace rdb {

storageShadow::storageShadow(const Descriptor &descriptor, std::string metaFilePath)
    : metaData(descriptor, metaFilePath),
      shadow_(descriptor, std::move(metaFilePath)) {
  shadow_.load();
}

void storageShadow::onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitset) {
  if (recordIndex >= totalRecords()) throw std::out_of_range("recordIndex out of range in storageShadow::onRecordModified");
  shadow_.appendOverride(recordIndex, nullBitset);
}

std::vector<bool> storageShadow::getNullBitset(size_t recordIndex) const {
  if (auto shadowBitset = shadow_.lookup(recordIndex)) return *shadowBitset;
  return metaData::getNullBitset(recordIndex);
}

void storageShadow::reset() {
  metaData::reset();
  shadow_.discard();
}

void storageShadow::mergeShadow() {
  for (const auto &ov : shadow_.overrides())
    metaData::onRecordModified(ov.recordIndex, ov.nullBitset);
  shadow_.discard();
}

void storageShadow::discardShadow() { shadow_.discard(); }

std::string storageShadow::metaShadowFilePath(std::string_view metaIndexFile) {
  return metaShadow::shadowFilePathFor(metaIndexFile);
}

}  // namespace rdb
