#include "rdb/metaShadow.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <span>
#include <stdexcept>

#include "rdb/bitsetCodec.hpp"

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
    if (ptr + sizeof(T) > end) throw std::runtime_error("Buffer underrun while deserializing ShadowOverride");
    std::memcpy(&out, ptr, sizeof(T));
    ptr += sizeof(T);
  };

  ShadowOverride ov;
  uint8_t flag = 0;
  read(flag);
  read(ov.recordIndex);

  size_t bitsetSize = 0;
  read(bitsetSize);
  const size_t byteCount = packedByteCount(bitsetSize);
  if (ptr + byteCount > end) throw std::runtime_error("Buffer underrun in ShadowOverride bitset data");

  ov.nullBitset = unpackBits(std::span<const std::byte>(ptr, byteCount), bitsetSize);

  return ov;
}

// ── Construction ──────────────────────────────────────────────────────

std::string metaShadow::shadowFilePathFor(std::string_view metaFilePath) {
  return metaFilePath.empty() ? std::string{} : std::string(metaFilePath) + ".shadow";
}

metaShadow::metaShadow(const Descriptor &descriptor, std::string metaFilePath)
    : shadowFilePath_(shadowFilePathFor(metaFilePath)),
      entrySize_(sizeof(uint8_t) + (2 * sizeof(size_t)) + packedByteCount(descriptor.size())) {}

// ── Persistence ───────────────────────────────────────────────────────

void metaShadow::load() {
  overrides_.clear();
  if (shadowFilePath_.empty()) return;

  std::ifstream in(shadowFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0) return;

  const auto payloadSize = static_cast<size_t>(fileSize);
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("metaShadow: unexpected shadow alignment (size={}, entrySize={})", payloadSize, entrySize_);

  in.seekg(0, std::ios::beg);
  std::vector<std::byte> fileData(payloadSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(payloadSize));

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
  std::ofstream out(shadowFilePath_, std::ios::binary | std::ios::app);
  if (!out.is_open()) return;
  auto buf = ov.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
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

}  // namespace rdb
