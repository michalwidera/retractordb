#include "cmdMeta.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <span>
#include <utility>
#include <vector>

#include "rdb/metaData.hpp"

namespace {
constexpr size_t kBitsPerByte = 8;
}

static std::string resolveMetaPath(const CommandContext &ctx) {
  return (ctx.storageParam.empty() ? std::filesystem::path(ctx.file) : std::filesystem::path(ctx.storageParam) / ctx.file)
             .string() +
         ".meta";
}

std::pair<std::string, std::vector<std::string>> MetaCmd::usage() const {
  return {"meta", {"show meta index (null patterns) for open db"}};
}

bool MetaCmd::execute(CommandContext &ctx) {
  const auto path = resolveMetaPath(ctx);
  if (!std::filesystem::exists(path)) {
    std::print("{}meta file not found: {}\n{}", ctx.colors.RED, path, ctx.colors.RESET);
    return false;
  }
  rdb::metaData metaView(ctx.dacc->descriptor, path);
  const auto segs      = metaView.segments();
  const size_t nFields = ctx.dacc->descriptor.size();

  std::print("{}meta: {}\n", ctx.colors.BLUE, path);
  std::cout << "interval: " << ctx.dacc->getSamplingInterval() << "  ";  // boost::rational — brak std::formatter
  std::print("total records: {}\n{}", metaView.totalRecords(), ctx.colors.RESET);

  size_t segIdx = 0;
  for (const auto &entry : segs) {
    std::print("{}[{}] ", ctx.colors.YELLOW, segIdx++);
    if (entry.isGap) {
      std::print("gap (duration:{})\n", entry.recordCount);
    } else {
      std::print("[");
      for (size_t fi = 0; fi < nFields; ++fi) {
        if (fi > 0) std::print(" ");
        std::print("{}:", ctx.dacc->descriptor[fi].rname);
        std::print("{}", (fi < entry.nullBitset.size() && entry.nullBitset[fi] ? "null" : "ok"));
      }
      std::print("] runs:{}\n", entry.recordCount);
    }
    std::print("{}", ctx.colors.RESET);
  }
  std::print("{}{} segment(s)\n{}", ctx.colors.BLUE, segs.size(), ctx.colors.RESET);
  return false;
}

std::pair<std::string, std::vector<std::string>> MetaRawCmd::usage() const {
  return {"metaraw", {"show internal meta file structure"}};
}

bool MetaRawCmd::execute(CommandContext &ctx) {
  const auto path = resolveMetaPath(ctx);
  if (!std::filesystem::exists(path)) {
    std::print("{}meta file not found: {}\n{}", ctx.colors.RED, path, ctx.colors.RESET);
    return false;
  }

  constexpr size_t headerSize = sizeof(int64_t);
  const size_t bitsetBytes    = (ctx.dacc->descriptor.size() + (kBitsPerByte - 1)) / kBitsPerByte;
  const size_t entrySize      = sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + bitsetBytes;

  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    std::print("{}cannot open meta file: {}\n{}", ctx.colors.RED, path, ctx.colors.RESET);
    return false;
  }

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  size_t effectiveHeaderSize = headerSize;
  if (fileSize >= 0) {
    const auto fs = static_cast<size_t>(fileSize);
    if (fs < headerSize || (fs - headerSize) % entrySize != 0)
      std::print("{}warning: unexpected meta payload alignment\n{}", ctx.colors.YELLOW, ctx.colors.RESET);
  }

  int64_t creationTimeNs = 0;
  in.read(reinterpret_cast<char *>(&creationTimeNs), sizeof(creationTimeNs));

  std::print("{}meta raw: {}\n{}", ctx.colors.BLUE, path, ctx.colors.RESET);
  std::print("header size: {}\n", effectiveHeaderSize);
  std::print("entry size: {}\n", entrySize);
  std::cout << "sampling interval: " << ctx.dacc->getSamplingInterval() << "\n";  // boost::rational — brak std::formatter

  size_t entryIdx = 0;
  while (true) {
    std::vector<std::byte> raw(entrySize);
    in.read(reinterpret_cast<char *>(raw.data()), static_cast<std::streamsize>(entrySize));
    const auto bytesRead = in.gcount();
    if (bytesRead == 0) break;
    if (std::cmp_not_equal(bytesRead, entrySize)) {
      std::print("{}truncated entry at index: {}\n{}", ctx.colors.RED, entryIdx, ctx.colors.RESET);
      break;
    }
    auto rec = rdb::metaData::IndexRecord::deserialize(std::span<const std::byte>(raw));
    std::print("entry[{}] ", entryIdx);
    std::print("count:{} ", rec.recordCount);
    std::print("gap:{} ", (rec.isGap ? 1 : 0));
    std::print("bitsetSize:{} ", rec.nullBitset.size());
    std::print("bitsetHex:");
    for (size_t bi = 0; bi < bitsetBytes; ++bi) {
      uint8_t byteVal = 0;
      for (size_t bit = 0; bit < kBitsPerByte; ++bit) {
        const size_t fieldIdx = (bi * kBitsPerByte) + bit;
        if (fieldIdx < rec.nullBitset.size() && rec.nullBitset[fieldIdx]) byteVal |= static_cast<uint8_t>(1U << bit);
      }
      std::print("{:02x}", static_cast<unsigned>(byteVal));
    }
    std::print("\n");
    ++entryIdx;
  }
  std::print("entries: {}\n", entryIdx);
  return false;
}
