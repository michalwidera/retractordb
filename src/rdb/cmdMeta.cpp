#include "cmdMeta.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <vector>

#include "rdb/metaDataStream.hpp"

void executeMeta(rdb::storage& dacc, const std::string& file, const std::string& storageParam, const Colors& colors) {
  const std::string metaFilePath =
      (storageParam.empty() ? std::filesystem::path(file) : std::filesystem::path(storageParam) / file).string() +
      ".meta";
  if (!std::filesystem::exists(metaFilePath)) {
    std::cout << colors.RED << "meta file not found: " << metaFilePath << "\n" << colors.RESET;
    return;
  }
  rdb::metaDataStream metaView(dacc.descriptor, metaFilePath);
  const auto segs      = metaView.segments();
  const size_t nFields = dacc.descriptor.size();

  std::cout << colors.BLUE << "meta: " << metaFilePath << "\n";
  std::cout << "interval: " << dacc.getSamplingInterval() << "  ";
  std::cout << "total records: " << metaView.totalRecords() << "\n" << colors.RESET;

  size_t segIdx = 0;
  for (const auto& entry : segs) {
    std::cout << colors.YELLOW << "[" << segIdx++ << "] ";
    if (entry.isGap) {
      std::cout << "gap (duration:" << entry.recordCount << ")\n";
    } else {
      std::cout << "[";
      for (size_t fi = 0; fi < nFields; ++fi) {
        if (fi > 0) std::cout << " ";
        std::cout << dacc.descriptor[fi].rname << ":";
        std::cout << (fi < entry.nullBitset.size() && entry.nullBitset[fi] ? "null" : "ok");
      }
      std::cout << "] runs:" << entry.recordCount << "\n";
    }
    std::cout << colors.RESET;
  }
  std::cout << colors.BLUE << segs.size() << " segment(s)\n" << colors.RESET;
}

void executeMetaRaw(rdb::storage& dacc, const std::string& file, const std::string& storageParam,
                    const Colors& colors) {
  const std::string metaFilePath =
      (storageParam.empty() ? std::filesystem::path(file) : std::filesystem::path(storageParam) / file).string() +
      ".meta";
  if (!std::filesystem::exists(metaFilePath)) {
    std::cout << colors.RED << "meta file not found: " << metaFilePath << "\n" << colors.RESET;
    return;
  }

  constexpr size_t headerSize = sizeof(int64_t);
  const size_t bitsetBytes    = (dacc.descriptor.size() + 7) / 8;
  const size_t entrySize      = sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + bitsetBytes;

  std::ifstream in(metaFilePath, std::ios::binary);
  if (!in.is_open()) {
    std::cout << colors.RED << "cannot open meta file: " << metaFilePath << "\n" << colors.RESET;
    return;
  }

  in.seekg(0, std::ios::end);
  const auto fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  size_t effectiveHeaderSize = headerSize;
  if (fileSize >= 0) {
    const auto fs = static_cast<size_t>(fileSize);
    if (!(fs >= headerSize && (fs - headerSize) % entrySize == 0)) {
      std::cout << colors.YELLOW << "warning: unexpected meta payload alignment\n" << colors.RESET;
    }
  }

  int64_t creationTimeNs = 0;
  in.read(reinterpret_cast<char*>(&creationTimeNs), sizeof(creationTimeNs));

  std::cout << colors.BLUE << "meta raw: " << metaFilePath << "\n" << colors.RESET;
  std::cout << "header size: " << effectiveHeaderSize << "\n";
  std::cout << "entry size: " << entrySize << "\n";
  std::cout << "sampling interval: " << dacc.getSamplingInterval() << "\n";

  size_t entryIdx = 0;
  while (true) {
    std::vector<std::byte> raw(entrySize);
    in.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(entrySize));
    const auto bytesRead = in.gcount();
    if (bytesRead == 0) break;
    if (static_cast<size_t>(bytesRead) != entrySize) {
      std::cout << colors.RED << "truncated entry at index: " << entryIdx << "\n" << colors.RESET;
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
}
