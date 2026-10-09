#include "rdb/metaShadow.hpp"

#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "rdb/bitsetCodec.hpp"
#include "rdb/storageFile.hpp"
#include "rdb/storageRotation.hpp"

namespace rdb {

// ── ShadowOverride serialization ─────────────────────────────────────

std::vector<std::byte> metaShadow::ShadowOverride::serialize() const {
  const size_t bitsetSize = nullBitset.size();
  const size_t byteCount  = packedByteCount(bitsetSize);
  std::vector<std::byte> buf(sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + byteCount, std::byte{0});
  std::byte *ptr = buf.data();

  auto write = [&]<typename T>(const T &val) {
    std::memcpy(ptr, &val, sizeof(T));
    ptr += sizeof(T);
  };

  write(static_cast<uint8_t>(0));  // flag byte reserved, unused for overrides
  write(recordIndex);
  write(bitsetSize);
  packBits(nullBitset, std::span<std::byte>(ptr, byteCount));

  return buf;
}

metaShadow::ShadowOverride metaShadow::ShadowOverride::deserialize(std::span<const std::byte> data) {
  const std::byte *ptr = data.data();
  const std::byte *end = ptr + data.size();

  auto read = [&]<typename T>(T &out) {
    if (static_cast<size_t>(end - ptr) < sizeof(T))
      throw std::runtime_error("Buffer underrun while deserializing ShadowOverride");
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
  };

  ShadowOverride ov;
  uint8_t flag = 0;
  read(flag);
  read(ov.recordIndex);

  size_t bitsetSize = 0;
  read(bitsetSize);
  // Jak w IndexRecord::deserialize: dlugosc z pliku przed zaokragleniem (#421).
  const auto available = static_cast<size_t>(end - ptr);
  if (bitsetSize > available * kBitsPerByte)
    throw std::runtime_error(
        std::format("ShadowOverride bitset size {} exceeds remaining buffer ({} bytes)", bitsetSize, available));
  const size_t byteCount = packedByteCount(bitsetSize);

  ov.nullBitset = unpackBits(std::span<const std::byte>(ptr, byteCount), bitsetSize);

  return ov;
}

// ── Construction ──────────────────────────────────────────────────────

std::string metaShadow::shadowFilePathFor(std::string_view metaFilePath) {
  return metaFilePath.empty() ? std::string{} : std::string(metaFilePath) + ".shadow";
}

metaShadow::metaShadow(const Descriptor &descriptor, const std::string &metaFilePath)
    : shadowFilePath_(shadowFilePathFor(metaFilePath)),
      entrySize_(sizeof(uint8_t) + (2 * sizeof(size_t)) + packedByteCount(descriptor.size())) {}

// ── Persistence ───────────────────────────────────────────────────────

void metaShadow::load() {
  overrides_.clear();
  if (shadowFilePath_.empty()) return;

  // Bez podazania za dowiazaniem (#374), tak jak `.meta` obok.
  const StorageFd in(shadowFilePath_, O_RDONLY);
  if (!in.isOpen()) return;

  const off_t fileSize = in.size();
  if (fileSize <= 0) return;

  const auto payloadSize = static_cast<size_t>(fileSize);
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("metaShadow: unexpected shadow alignment (size={}, entrySize={})", payloadSize, entrySize_);

  std::vector<std::byte> fileData(payloadSize);
  (void)in.readAt(fileData, 0);

  std::span<const std::byte> remaining(fileData);
  while (remaining.size() >= entrySize_) {
    overrides_.push_back(ShadowOverride::deserialize(remaining.subspan(0, entrySize_)));
    remaining = remaining.subspan(entrySize_);
  }
}

void metaShadow::appendOverride(size_t recordIndex, const std::vector<bool> &nullBitset) {
  ShadowOverride ov{.recordIndex = recordIndex, .nullBitset = nullBitset};
  overrides_.push_back(ov);

  if (shadowFilePath_.empty()) return;
  const StorageFd out(shadowFilePath_, O_WRONLY | O_CREAT | O_APPEND);
  if (!out.isOpen()) return;
  (void)out.write(ov.serialize());
}

std::optional<std::vector<bool>> metaShadow::lookup(size_t recordIndex) const {
  for (const auto &shadowOverride : std::ranges::reverse_view(overrides_))
    if (shadowOverride.recordIndex == recordIndex)
      return shadowOverride.nullBitset;  // ostatni wpis dla pozycji jest obowiązujący
  return std::nullopt;
}

void metaShadow::discard() {
  overrides_.clear();
  if (shadowFilePath_.empty()) return;
  std::error_code ec;
  std::filesystem::remove(shadowFilePath_, ec);
}

bool metaShadow::rotate(int percounter) {
  if (percounter < 0 || shadowFilePath_.empty()) return true;
  std::error_code ec;
  const bool exists = std::filesystem::exists(shadowFilePath_, ec);
  if (ec) {
    SPDLOG_ERROR("metaShadow::rotate: checking '{}' failed: {}", shadowFilePath_, ec.message());
    return false;
  }
  if (!exists) return true;
  const std::string rotatedPath = std::format("{}.old{}", shadowFilePath_, percounter);
  if (!rotateStorageFile(shadowFilePath_, rotatedPath)) return false;
  overrides_.clear();
  return true;
}

}  // namespace rdb
