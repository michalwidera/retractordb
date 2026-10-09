#include "rdb/storageShadow.hpp"

#include <filesystem>
#include <stdexcept>
#include <system_error>

#include <spdlog/spdlog.h>

namespace rdb {

storageShadow::storageShadow(const Descriptor &descriptor, const std::string &metaFilePath)
    : metaData(descriptor, metaFilePath),
      shadow_(descriptor, metaFilePath) {
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

void storageShadow::rotate(int percounter, const bool reopen) {
  // Przed reset() glownego indeksu, ktory odrzucilby niezarchiwizowany cien.
  if (!shadow_.rotate(percounter)) {
    // reset() odrzucilby cien, ktorego nie udalo sie zarchiwizowac.
    if (reopen) throw std::runtime_error("storageShadow::rotate: shadow rotation failed");
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
      return;
    }
  }
  metaData::rotate(percounter, reopen);
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
