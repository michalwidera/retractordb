#include "rdb/metaIndexStore.hpp"

#include <fcntl.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "rdb/storageFile.hpp"

namespace rdb {

namespace {

// Naglowek pliku `.meta`: 8 bajtow ZAREZERWOWANYCH, zapisywanych jako zero.
// Do 2026-09-02 nioslo to czas utworzenia; pole wycofane, bo nikt go nie odczytywal.
// Bajty zostaja: kHeaderSize wchodzi we wszystkie offsety wpisow, stare pliki maja
// pozostac czytelne, a oracle bramek badawczych adresuja wpisy od stalego offsetu 8.
constexpr size_t kHeaderSize      = sizeof(int64_t);
constexpr int64_t kReservedHeader = 0;

// Pliki `.meta` otwiera StorageFd, czyli bez podazania za dowiazaniem (#374). Bledy zapisu sa
// ignorowane jak wczesniej przy std::ofstream - indeks jest wtorny wobec pliku danych.
std::span<const std::byte> headerBytes() { return std::as_bytes(std::span{&kReservedHeader, 1}); }

}  // namespace

MetaIndexStore::MetaIndexStore(std::string metaFilePath, size_t entrySize)
    : metaFilePath_(std::move(metaFilePath)),
      entrySize_(entrySize) {}

bool MetaIndexStore::fileExists() const { return !metaFilePath_.empty() && std::filesystem::exists(metaFilePath_); }

void MetaIndexStore::saveHeader() {
  if (metaFilePath_.empty()) return;
  const StorageFd out(metaFilePath_, O_WRONLY | O_CREAT | O_TRUNC);
  if (!out.isOpen()) return;  // plik nietkniety -- cache pozostaje aktualny
  (void)out.write(headerBytes());
  // write-through: po truncate plik zawiera tylko naglowek -> zero wpisow
  entriesCache_.clear();
  cacheValid_ = true;
}

const std::vector<IndexRecord> &MetaIndexStore::readAll() const {
  if (cacheValid_) return entriesCache_;

  entriesCache_.clear();
  if (metaFilePath_.empty()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  const StorageFd in(metaFilePath_, O_RDONLY);
  if (!in.isOpen()) {
    cacheValid_ = true;
    return entriesCache_;
  }

  const off_t fileSize = in.size();
  if (fileSize <= 0 || std::cmp_less_equal(fileSize, kHeaderSize)) {
    cacheValid_ = true;
    return entriesCache_;
  }

  const auto payloadSize = static_cast<size_t>(fileSize) - kHeaderSize;
  if (payloadSize % entrySize_ != 0)
    SPDLOG_WARN("MetaIndexStore: unexpected payload alignment (payloadSize={}, entrySize={})", payloadSize, entrySize_);

  std::vector<std::byte> fileData(payloadSize);
  (void)in.readAt(fileData, static_cast<off_t>(kHeaderSize));

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
  const StorageFd out(metaFilePath_, O_WRONLY | O_CREAT | O_APPEND);
  if (!out.isOpen()) return;  // plik nietkniety -- cache pozostaje aktualny
  (void)out.write(entry.serialize());
  if (cacheValid_) entriesCache_.push_back(entry);  // write-through
}

void MetaIndexStore::overwriteLast(const IndexRecord &entry) {
  if (metaFilePath_.empty()) return;
  const StorageFd f(metaFilePath_, O_RDWR);
  if (!f.isOpen()) return;
  const off_t fileSize = f.size();
  if (std::cmp_less(fileSize, kHeaderSize + entrySize_)) return;
  (void)f.writeAt(entry.serialize(), fileSize - static_cast<off_t>(entrySize_));
  if (cacheValid_ && !entriesCache_.empty())
    entriesCache_.back() = entry;  // write-through
  else
    cacheValid_ = false;  // cache nie odzwierciedla ostatniego wpisu -- odbuduj przy odczycie
}

void MetaIndexStore::rewrite(const std::vector<IndexRecord> &entries) {
  if (metaFilePath_.empty()) return;
  const std::string tmpPath = std::format("{}.tmp", metaFilePath_);
  {
    const StorageFd out(tmpPath, O_WRONLY | O_CREAT | O_TRUNC);
    if (!out.isOpen()) return;  // plik nietkniety -- cache pozostaje aktualny
    std::vector<std::byte> content;
    content.reserve(kHeaderSize + (entries.size() * entrySize_));
    std::ranges::copy(headerBytes(), std::back_inserter(content));
    for (const auto &rec : entries)
      std::ranges::copy(rec.serialize(), std::back_inserter(content));
    (void)out.write(content);
  }
  std::filesystem::rename(tmpPath, metaFilePath_);
  // write-through; guard na wypadek, gdyby caller podal sam cache (self-assign jest
  // bezpieczny dla std::vector, ale jawny warunek dokumentuje intencje)
  if (&entries != &entriesCache_) entriesCache_ = entries;
  cacheValid_ = true;
}

}  // namespace rdb
