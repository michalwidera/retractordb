#include "rdb/metaIndexStore.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <utility>

namespace rdb {

namespace {

constexpr size_t kHeaderSize = sizeof(int64_t);

void writeHeader(std::ostream &out, std::chrono::system_clock::time_point t) {
  int64_t ns       = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
  const auto bytes = std::as_bytes(std::span{&ns, 1});
  out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes()));
}

void writeEntry(std::ostream &out, const IndexRecord &entry) {
  auto buf = entry.serialize();
  out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

}  // namespace

MetaIndexStore::MetaIndexStore(std::string metaFilePath, size_t entrySize)
    : metaFilePath_(std::move(metaFilePath)),
      entrySize_(entrySize),
      creationTime_(std::chrono::system_clock::now()) {
  loadHeaderTimestamp();
}

bool MetaIndexStore::fileExists() const { return !metaFilePath_.empty() && std::filesystem::exists(metaFilePath_); }

void MetaIndexStore::loadHeaderTimestamp() {
  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) return;

  in.seekg(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0 || std::cmp_less(fileSize, kHeaderSize)) return;
  in.seekg(0, std::ios::beg);

  std::array<std::byte, sizeof(int64_t)> nsBuf{};
  in.read(reinterpret_cast<char *>(nsBuf.data()), static_cast<std::streamsize>(nsBuf.size()));
  in.close();
  int64_t creationTimeNs{};
  std::memcpy(&creationTimeNs, nsBuf.data(), sizeof(creationTimeNs));

  creationTime_ = std::chrono::system_clock::time_point(std::chrono::nanoseconds(creationTimeNs));
}

void MetaIndexStore::saveHeader() {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::trunc);
  if (out.is_open()) writeHeader(out, creationTime_);
  cacheValid_ = false;
}

std::vector<IndexRecord> MetaIndexStore::readAll() const {
  if (cacheValid_) return entriesCache_;

  entriesCache_.clear();
  if (metaFilePath_.empty()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  std::ifstream in(metaFilePath_, std::ios::binary);
  if (!in.is_open()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  in.seekg(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(in.tellg());
  if (fileSize <= 0 || std::cmp_less_equal(fileSize, kHeaderSize)) {
    cacheValid_ = true;
    return entriesCache_;
  }

  const auto payloadSize = static_cast<size_t>(fileSize - static_cast<std::streamoff>(kHeaderSize));
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("MetaIndexStore: unexpected payload alignment (payloadSize={}, entrySize={})", payloadSize, entrySize_);

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);
  std::vector<std::byte> fileData(payloadSize);
  in.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(payloadSize));

  std::span<const std::byte> remaining(fileData);
  while (remaining.size() >= entrySize_) {
    entriesCache_.push_back(IndexRecord::deserialize(remaining.subspan(0, entrySize_)));
    remaining = remaining.subspan(entrySize_);
  }

  cacheValid_ = true;
  return entriesCache_;
}

void MetaIndexStore::appendEntry(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::ofstream out(metaFilePath_, std::ios::binary | std::ios::app);
  if (out.is_open()) writeEntry(out, entry);
  cacheValid_ = false;
}

void MetaIndexStore::overwriteLast(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  std::fstream f(metaFilePath_, std::ios::binary | std::ios::in | std::ios::out);
  if (!f.is_open()) return;
  f.seekp(0, std::ios::end);
  // Compare stream positions as streamoff; cmp_less with streampos is ill-formed.
  const auto fileSize = static_cast<std::streamoff>(f.tellp());
  if (std::cmp_less(fileSize, kHeaderSize + entrySize_)) return;
  f.seekp(-static_cast<std::streamoff>(entrySize_), std::ios::end);
  auto buf = entry.serialize();
  f.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  cacheValid_ = false;
}

void MetaIndexStore::rewrite(const std::vector<IndexRecord> &entries) {
  if (metaFilePath_.empty()) return;
  const std::string tmpPath = std::format("{}.tmp", metaFilePath_);
  std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) return;
  writeHeader(out, creationTime_);
  for (const auto &rec : entries)
    writeEntry(out, rec);
  out.close();
  std::filesystem::rename(tmpPath, metaFilePath_);
  cacheValid_ = false;
}

}  // namespace rdb
