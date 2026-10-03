#include "rdb/storageShadow.hpp"

#include <fmt/format.h>

namespace rdb {

storageShadow::storageShadow(const Descriptor &descriptor, const std::string &metaFilePath)
    : metaData(descriptor, metaFilePath),
      shadow_(descriptor, metaFilePath) {
  shadow_.load();
}

Result<> storageShadow::onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitset) {
  if (recordIndex >= totalRecords())
    return fail(Errc::Logic, fmt::format("recordIndex {} out of range in storageShadow::onRecordModified (records: {})",
                                         recordIndex, totalRecords()));
  shadow_.appendOverride(recordIndex, nullBitset);
  return {};
}

std::vector<bool> storageShadow::getNullBitset(size_t recordIndex) const {
  if (auto shadowBitset = shadow_.lookup(recordIndex)) return *shadowBitset;
  return metaData::getNullBitset(recordIndex);
}

void storageShadow::reset() {
  metaData::reset();
  shadow_.discard();
}

Result<> storageShadow::mergeShadow() {
  for (const auto &ov : shadow_.overrides())
    RDB_TRY(metaData::onRecordModified(ov.recordIndex, ov.nullBitset));
  shadow_.discard();
  return {};
}

void storageShadow::discardShadow() { shadow_.discard(); }

std::string storageShadow::metaShadowFilePath(std::string_view metaIndexFile) {
  return metaShadow::shadowFilePathFor(metaIndexFile);
}

}  // namespace rdb
