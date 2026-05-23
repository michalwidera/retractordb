#include "cmdMeta.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

#include "rdb/metaDataStream.hpp"

static std::string resolveMetaPath(const CommandContext& ctx) {
  return (ctx.storageParam.empty() ? std::filesystem::path(ctx.file)
                                   : std::filesystem::path(ctx.storageParam) / ctx.file)
             .string() +
         ".meta";
}

std::pair<std::string, std::vector<std::string>> MetaCmd::usage() const {
  return {"meta", {"show meta index (null patterns) for open db"}};
}

bool MetaCmd::execute(CommandContext& ctx) {
  const auto path = resolveMetaPath(ctx);
  if (!std::filesystem::exists(path)) {
    std::cout << ctx.colors.RED << "meta file not found: " << path << "\n" << ctx.colors.RESET;
    return false;
  }
  rdb::metaDataStream metaView(ctx.dacc->descriptor, path);
  const auto segs      = metaView.segments();
  const size_t nFields = ctx.dacc->descriptor.size();

  std::cout << ctx.colors.BLUE << "meta: " << path << "\n";
  std::cout << "interval: " << ctx.dacc->getSamplingInterval() << "  ";
  std::cout << "total records: " << metaView.totalRecords() << "\n" << ctx.colors.RESET;

  size_t segIdx = 0;
  for (const auto& entry : segs) {
    std::cout << ctx.colors.YELLOW << "[" << segIdx++ << "] ";
    if (entry.isGap) {
      std::cout << "gap (duration:" << entry.recordCount << ")\n";
    } else {
      std::cout << "[";
      for (size_t fi = 0; fi < nFields; ++fi) {
        if (fi > 0) std::cout << " ";
        std::cout << ctx.dacc->descriptor[fi].rname << ":";
        std::cout << (fi < entry.nullBitset.size() && entry.nullBitset[fi] ? "null" : "ok");
      }
      std::cout << "] runs:" << entry.recordCount << "\n";
    }
    std::cout << ctx.colors.RESET;
  }
  std::cout << ctx.colors.BLUE << segs.size() << " segment(s)\n" << ctx.colors.RESET;
  return false;
}

std::pair<std::string, std::vector<std::string>> MetaRawCmd::usage() const {
  return {"metaraw", {"show internal meta file structure"}};
}

bool MetaRawCmd::execute(CommandContext& ctx) {
  const auto path = resolveMetaPath(ctx);
  if (!std::filesystem::exists(path)) {
    std::cout << ctx.colors.RED << "meta file not found: " << path << "\n" << ctx.colors.RESET;
    return false;
  }

  constexpr size_t headerSize = sizeof(int64_t);
  const size_t bitsetBytes    = (ctx.dacc->descriptor.size() + 7) / 8;
  const size_t entrySize      = sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + bitsetBytes;

  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    std::cout << ctx.colors.RED << "cannot open meta file: " << path << "\n" << ctx.colors.RESET;
    return false;
  }

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  size_t effectiveHeaderSize = headerSize;
  if (fileSize >= 0) {
    const auto fs = static_cast<size_t>(fileSize);
    if (!(fs >= headerSize && (fs - headerSize) % entrySize == 0))
      std::cout << ctx.colors.YELLOW << "warning: unexpected meta payload alignment\n" << ctx.colors.RESET;
  }

  int64_t creationTimeNs = 0;
  in.read(reinterpret_cast<char*>(&creationTimeNs), sizeof(creationTimeNs));

  std::cout << ctx.colors.BLUE << "meta raw: " << path << "\n" << ctx.colors.RESET;
  std::cout << "header size: " << effectiveHeaderSize << "\n";
  std::cout << "entry size: " << entrySize << "\n";
  std::cout << "sampling interval: " << ctx.dacc->getSamplingInterval() << "\n";

  size_t entryIdx = 0;
  while (true) {
    std::vector<std::byte> raw(entrySize);
    in.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(entrySize));
    const auto bytesRead = in.gcount();
    if (bytesRead == 0) break;
    if (static_cast<size_t>(bytesRead) != entrySize) {
      std::cout << ctx.colors.RED << "truncated entry at index: " << entryIdx << "\n" << ctx.colors.RESET;
      break;
    }
    auto rec = rdb::metaDataStream::IndexRecord::deserialize(std::span<const std::byte>(raw));
    std::cout << "entry[" << entryIdx << "] ";
    std::cout << "count:" << rec.recordCount << " ";
    std::cout << "gap:" << (rec.isGap ? 1 : 0) << " ";
    std::cout << "bitsetSize:" << rec.nullBitset.size() << " ";
    std::cout << "bitsetHex:";
    for (size_t bi = 0; bi < bitsetBytes; ++bi) {
      uint8_t byteVal = 0;
      for (size_t bit = 0; bit < 8; ++bit) {
        const size_t fieldIdx = bi * 8 + bit;
        if (fieldIdx < rec.nullBitset.size() && rec.nullBitset[fieldIdx]) byteVal |= static_cast<uint8_t>(1u << bit);
      }
      std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byteVal) << std::dec;
    }
    std::cout << "\n";
    ++entryIdx;
  }
  std::cout << "entries: " << entryIdx << "\n";
  return false;
}
