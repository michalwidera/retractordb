#include "rdb/storageShadow.hpp"

#include <filesystem>

#include <system_error>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace rdb {

storageShadow::storageShadow(const Descriptor &descriptor, const std::string &metaFilePath)
    : metaData(descriptor, metaFilePath),
      shadow_(descriptor, metaFilePath) {
  shadow_.load();
  if (storageShadow::initializationError()) abandonFile();
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

Result<> storageShadow::rotate(int percounter, const bool reopen) {
  // Przed reset() glownego indeksu, ktory odrzucilby niezarchiwizowany cien.
  if (!shadow_.rotate(percounter)) {
    // reset() odrzucilby cien, ktorego nie udalo sie zarchiwizowac.
    if (reopen) return fail(Errc::IO, "storageShadow::rotate: shadow rotation failed", EIO);
    // Cien pod aktywna nazwa zostaje razem z .meta: nastepny start skasuje oba przez reset().
    // Archiwum .meta obok niego dalo pusty indeks, do ktorego load() wczytalby cudze nadpisania.
    // Cien juz przeniesiony (zawiodlo tylko utrwalenie katalogu) nie ma czego trzymac - .meta
    // idzie do archiwum, inaczej reset() przy starcie skasowalby indeks zamknietej sesji.
    std::error_code ec;
    const bool shadowStayed = std::filesystem::exists(shadow_.filePath(), ec);
    if (ec) SPDLOG_ERROR("storageShadow::rotate: checking '{}' failed: {}", shadow_.filePath(), ec.message());
    if (ec || shadowStayed) {
      flushCurrentEntry();
      abandonFile();
      return {};
    }
  }
  return metaData::rotate(percounter, reopen);
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
