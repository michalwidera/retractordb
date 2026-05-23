#include "xtrdbStorageMap.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

// Prezentacja mapy pliku (przykład)
//
// ┌────────────────────────────────────────────────────────────────┐
// │  Storage map: /path/to/file                                    │
// ├────────────────────────────────────────────────────────────────┤
// │ [shadow]   │ [binary data] │ [meta index]                      │
// ├────────────┼───────────────┼───────────────────────────────────┤
// │            │ 0-100         │ [====] 100 records, no nulls      │
// │            │ 100-150       │ [----] 60 records, some nulls     │
// │ 10 records │ 150-160       │                                   │
// │            │ 160-200       │ [~~~~] 40 records, all nulls      │
// │            │               │ [XXXX] 50 records, gap (missing)  │
// │            │ 200-210       │ [====] 110 records, no nulls      │
// │ 10 records │ 210-250       │                                   │
// └────────────┴───────────────┴───────────────────────────────────┘
// ┌────────────────────────────────────────────────────────────────┐
// │  DESCRIPTOR  file.desc (123 B)                                 │
// │  int id (4 B)                                                  │
// │  char name[20] (20 B)                                          │
// │  float value (4 B)                                             │  
// │  Record size: 28 B                                             │
// ├────────────────────────────────────────────────────────────────┤
// │  DATA        file (5000 B)                                     │
// │  Records: 250                                                  │
// ├────────────────────────────────────────────────────────────────┤
// │  META        file.meta (800 B)                                 │
// │  Segments: 3   Records: 178                                    │
// │  [====] 100 records, no nulls                                  │
// │  [----] 60 records, some nulls                                 │
// │  [~~~~] 18 records, all nulls                                  │
// ├────────────────────────────────────────────────────────────────┤
// │  SHADOW      file.shadow (280 B)                               │
// │  Updates: 20                                                   │  
// └────────────────────────────────────────────────────────────────┘

namespace fs = std::filesystem;

using Segment = rdb::metaDataStream::IndexRecord;

struct DataSegmentInfo {
  size_t index{};
  std::string path;
  uintmax_t bytes{};
  size_t records{};
  size_t begin{};
  size_t end{};
  uintmax_t shadowBytes{};
  size_t shadowUpdates{};
};

static std::vector<DataSegmentInfo> readDataSegments(const std::string& baseName,
                                                     const rdb::retention_t& retention,
                                                     size_t recordSize) {
  if (retention.noRetention()) return {};

  const fs::path basePath(baseName);
  const fs::path dirPath = basePath.has_parent_path() ? basePath.parent_path() : fs::path(".");
  const std::string stem = basePath.filename().string();
  const std::string prefix = stem + "_segment_";

  std::vector<DataSegmentInfo> out;
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) return out;

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    if (!entry.is_regular_file()) continue;

    const std::string fileName = entry.path().filename().string();
    if (!fileName.starts_with(prefix)) continue;

    const std::string suffix = fileName.substr(prefix.size());
    if (suffix.empty()) continue;
    if (!std::all_of(suffix.begin(), suffix.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) continue;

    DataSegmentInfo info;
    info.index = static_cast<size_t>(std::stoull(suffix));
    info.path = entry.path().string();
    info.bytes = fs::file_size(entry.path());
    info.records = (recordSize > 0) ? static_cast<size_t>(info.bytes / recordSize) : 0;
    info.begin = retention.capacity > 0 ? info.index * retention.capacity : 0;
    info.end = info.begin + info.records;
    const fs::path shadowPath = entry.path().string() + ".shadow";
    if (fs::exists(shadowPath)) {
      info.shadowBytes = fs::file_size(shadowPath);
      info.shadowUpdates = (recordSize > 0) ? static_cast<size_t>(info.shadowBytes / (sizeof(size_t) + recordSize)) : 0;
    }
    out.push_back(std::move(info));
  }

  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.index < b.index; });
  return out;
}

static std::string segmentLabel(size_t first, size_t last) {
  if (first == last) return "s" + std::to_string(first);
  return "s" + std::to_string(first) + "-" + std::to_string(last);
}

static std::vector<Segment> readMetaFile(const std::string& path, size_t fieldCount) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) return {};

  in.seekg(0, std::ios::end);
  const auto fileSize = static_cast<size_t>(in.tellg());
  constexpr size_t kHeaderSize = sizeof(int64_t);
  if (fileSize <= kHeaderSize) return {};

  const size_t entrySize   = sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + (fieldCount + 7) / 8;
  const size_t payloadSize = fileSize - kHeaderSize;

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);
  std::vector<std::byte> buf(payloadSize);
  in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(payloadSize));

  std::vector<Segment> result;
  std::span<const std::byte> rem(buf);
  while (rem.size() >= entrySize) {
    result.push_back(Segment::deserialize(rem.subspan(0, entrySize)));
    rem = rem.subspan(entrySize);
  }
  return result;
}

static char nullChar(const Segment& seg, const std::vector<size_t>& dataIdx) {
  if (dataIdx.empty()) return '=';
  bool anyNull = false, allNull = true;
  for (size_t i : dataIdx) {
    const bool n = (i < seg.nullBitset.size()) && seg.nullBitset[i];
    if (n) anyNull = true;
    else   allNull = false;
  }
  if (!anyNull) return '=';
  if (allNull)  return '~';
  return '-';
}

static std::string makeMetaBar(const std::vector<Segment>& segs,
                               const std::vector<size_t>& dataIdx,
                               int barWidth) {
  size_t total = 0;
  for (const auto& s : segs) total += s.recordCount;
  if (total == 0) return "";

  std::vector<int> widths(segs.size());
  int used = 0;
  for (size_t i = 0; i < segs.size(); ++i) {
    widths[i] = std::max(3, static_cast<int>(segs[i].recordCount * barWidth / total));
    used += widths[i];
  }
  // give leftover to the largest segment
  if (int leftover = barWidth - used; leftover > 0) {
    auto it = std::max_element(widths.begin(), widths.end());
    *it += leftover;
  }

  std::string bar;
  for (size_t i = 0; i < segs.size(); ++i) {
    const auto& seg = segs[i];
    const int inner = widths[i] - 2;
    const char fill = seg.isGap ? 'X' : nullChar(seg, dataIdx);
    const std::string label =
        seg.isGap ? ("gap:" + std::to_string(seg.recordCount)) : std::to_string(seg.recordCount);

    bar += '[';
    if (static_cast<int>(label.size()) >= inner) {
      bar += label.substr(0, inner);
    } else {
      const int padL = (inner - static_cast<int>(label.size())) / 2;
      const int padR = inner - static_cast<int>(label.size()) - padL;
      bar += std::string(padL, fill) + label + std::string(padR, fill);
    }
    bar += ']';
  }
  return bar;
}

static std::string fit(const std::string& s, int w) {
  if (w <= 0) return "";
  if (static_cast<int>(s.size()) <= w) return s + std::string(w - static_cast<int>(s.size()), ' ');
  if (w <= 3) return s.substr(0, w);
  return s.substr(0, w - 3) + "...";
}

static std::string repeatGlyph(const std::string& glyph, int count) {
  std::string out;
  for (int i = 0; i < count; ++i) out += glyph;
  return out;
}

static std::string metaLabel(const Segment& seg, const std::vector<size_t>& dataIdx) {
  if (seg.isGap) return "[XXXX] " + std::to_string(seg.recordCount) + " records, gap";

  const char fill = nullChar(seg, dataIdx);
  std::string state = "no nulls";
  if (fill == '-') state = "some nulls";
  if (fill == '~') state = "all nulls";

  return "[" + std::string(4, fill) + "] " + std::to_string(seg.recordCount) + " records, " + state;
}

static void mapHLine(const std::string& l,
                     const std::string& j1,
                     const std::string& j2,
                     const std::string& r,
                     int c1,
                     int c2,
                     int c3) {
  std::cout << l << repeatGlyph("─", c1 + 2) << j1 << repeatGlyph("─", c2 + 2) << j2
            << repeatGlyph("─", c3 + 2) << r << "\n";
}

static void mapRow(const std::string& s1,
                   const std::string& s2,
                   const std::string& s3,
                   int c1,
                   int c2,
                   int c3) {
  std::cout << "│ " << fit(s1, c1) << " │ " << fit(s2, c2) << " │ " << fit(s3, c3) << " │\n";
}

static void hline(const std::string& l, const std::string& m, const std::string& r, int W) {
  std::cout << l << repeatGlyph(m, W - 2) << r << "\n";
}

static void row(const std::string& s, int W) {
  const int inner = W - 4;
  std::cout << "│ " << std::left << std::setw(inner) << s.substr(0, inner) << " │\n";
}

void showStorageMap(const std::string& baseName) {
  const std::string descFile   = baseName + ".desc";
  const std::string dataFile   = baseName;
  const std::string metaFile   = baseName + ".meta";
  const std::string shadowFile = baseName + ".shadow";

  if (!fs::exists(descFile)) {
    std::cerr << "Descriptor not found: " << descFile << "\n";
    return;
  }

  rdb::Descriptor desc;
  {
    std::ifstream df(descFile);
    df >> desc;
  }

  const size_t recordSize  = desc.getSizeInBytes();
  const size_t fieldCount  = desc.size();
  const auto retention = desc.retention();
  const bool segmentedData = !retention.noRetention();

  // indices of data fields (non-config, non-null) in the descriptor vector
  std::vector<size_t> dataIdx;
  for (size_t i = 0; i < desc.size(); ++i)
    if (desc[i].rtype < rdb::TYPE && desc[i].rtype != rdb::NULLTYPE)
      dataIdx.push_back(i);

  const auto dataSegments = readDataSegments(baseName, retention, recordSize);
  const bool hasSegmentData = !dataSegments.empty();
  const bool hasDataFile = fs::exists(dataFile);
  const bool hasData   = segmentedData ? hasSegmentData : hasDataFile;
  const bool hasMeta   = fs::exists(metaFile);
  const bool hasRootShadow = fs::exists(shadowFile);

  uintmax_t totalSegmentShadowSize = 0;
  size_t totalSegmentShadowUpdates = 0;
  std::unordered_map<size_t, size_t> segmentShadowUpdates;
  for (const auto& ds : dataSegments) {
    totalSegmentShadowSize += ds.shadowBytes;
    totalSegmentShadowUpdates += ds.shadowUpdates;
    segmentShadowUpdates.emplace(ds.index, ds.shadowUpdates);
  }

  const bool hasShadow = segmentedData ? (hasRootShadow || totalSegmentShadowSize > 0) : hasRootShadow;

  const uintmax_t descSize   = fs::file_size(descFile);
  uintmax_t dataSize = 0;
  if (segmentedData) {
    for (const auto& seg : dataSegments) dataSize += seg.bytes;
  } else if (hasData) {
    dataSize = fs::file_size(dataFile);
  }
  const uintmax_t metaSize   = hasMeta   ? fs::file_size(metaFile)   : 0;
  uintmax_t shadowSize = 0;
  if (segmentedData) {
    shadowSize = totalSegmentShadowSize;
    if (hasRootShadow) shadowSize += fs::file_size(shadowFile);
  } else if (hasShadow) {
    shadowSize = fs::file_size(shadowFile);
  }

  size_t dataRecords = 0;
  if (segmentedData) {
    for (const auto& seg : dataSegments) dataRecords += seg.records;
  } else {
    dataRecords = (recordSize > 0 && hasData) ? static_cast<size_t>(dataSize / recordSize) : 0;
  }
  size_t shadowUpdates = 0;
  if (segmentedData) {
    shadowUpdates = totalSegmentShadowUpdates;
    if (hasRootShadow && recordSize > 0)
      shadowUpdates += static_cast<size_t>(fs::file_size(shadowFile) / (sizeof(size_t) + recordSize));
  } else if (hasShadow && recordSize > 0) {
    shadowUpdates = static_cast<size_t>(shadowSize / (sizeof(size_t) + recordSize));
  }

  auto segs = hasMeta ? readMetaFile(metaFile, fieldCount) : std::vector<Segment>{};
  size_t metaRecords = 0;
  for (const auto& s : segs) if (!s.isGap) metaRecords += s.recordCount;

  constexpr int W       = 64;
  constexpr int inner   = W - 4;  // content width between "| " and " |"
  constexpr int barWidth = inner - 2;

  auto fmtSize = [&](const std::string& tag, const std::string& name, uintmax_t bytes) {
    const std::string right = std::to_string(bytes) + " B";
    const int leftMax = inner - static_cast<int>(right.size()) - 1;
    const std::string left = fit(tag + name, std::max(0, leftMax));
    const int gap = std::max(1, inner - static_cast<int>(left.size()) - static_cast<int>(right.size()));
    std::cout << "│ " << left << std::string(gap, ' ') << right << " │\n";
  };

  auto fmtField = [&](const rdb::rField& f) {
    std::string left = "  " + std::string(rdb::GetStringdescFld(f.rtype)) + "  " + f.rname;
    if (f.rarray > 1) left += "[" + std::to_string(f.rarray) + "]";
    const std::string right = std::to_string(desc.fieldSize(f)) + " B";
    const int leftMax = inner - static_cast<int>(right.size()) - 1;
    left = fit(left, std::max(0, leftMax));
    int gap = inner - static_cast<int>(left.size()) - static_cast<int>(right.size());
    if (gap < 1) gap = 1;
    std::cout << "│ " << left << std::string(gap, ' ') << right << " │\n";
  };

  hline("┌", "─", "┐", W);
  {
    const std::string hdr = "  Storage map: " + baseName;
    std::cout << "│ " << std::left << std::setw(inner) << hdr << " │\n";
  }
  hline("├", "─", "┤", W);

  // Primary three-column map, similar to the visual example in the header comment.
  constexpr int c1 = 10;
  constexpr int c2 = 13;
  constexpr int c3 = 31;
  mapRow("[shadow]", "[binary data]", "[meta index]", c1, c2, c3);
  mapHLine("├", "┼", "┼", "┤", c1, c2, c3);

  std::vector<std::string> shadowCol;
  std::vector<std::string> dataCol;
  std::vector<std::string> metaCol;

  if (segmentedData) {
    // For RETENTION, always render binary-data segments sequentially: s0, s1, s2...
    for (const auto& ds : dataSegments) {
      const std::string segLbl = segmentLabel(ds.index, ds.index);
      shadowCol.push_back(ds.shadowUpdates > 0 ? segLbl + " " + std::to_string(ds.shadowUpdates) + "u" : segLbl);
      dataCol.push_back(segLbl + " " + std::to_string(ds.begin) + "-" + std::to_string(ds.end));
      metaCol.emplace_back("");
    }

    if (hasMeta && !segs.empty()) {
      for (size_t i = 0; i < segs.size(); ++i) {
        if (i < metaCol.size()) {
          metaCol[i] = metaLabel(segs[i], dataIdx);
        } else {
          shadowCol.emplace_back("");
          dataCol.emplace_back("");
          metaCol.push_back(metaLabel(segs[i], dataIdx));
        }
      }
    } else if (!hasMeta && metaCol.empty()) {
      shadowCol.emplace_back("");
      dataCol.emplace_back("");
      metaCol.emplace_back("meta missing");
    } else if (hasMeta && metaCol.empty()) {
      shadowCol.emplace_back("");
      dataCol.emplace_back("");
      metaCol.emplace_back("meta file is empty");
    }
  } else if (hasMeta && !segs.empty()) {
    size_t dataPos = 0;
    for (const auto& seg : segs) {
      shadowCol.emplace_back("");
      if (seg.isGap) {
        dataCol.emplace_back("");
      } else {
        const size_t begin = dataPos;
        const size_t end = dataPos + seg.recordCount;
        dataPos = end;
        dataCol.push_back(std::to_string(begin) + "-" + std::to_string(end));
      }
      metaCol.push_back(metaLabel(seg, dataIdx));
    }
  } else {
    shadowCol.emplace_back("");
    if (hasData && dataRecords > 0) {
      dataCol.push_back("0-" + std::to_string(dataRecords));
    } else {
      dataCol.emplace_back("");
    }
    if (hasMeta) {
      metaCol.emplace_back("meta file is empty");
    } else {
      metaCol.emplace_back("meta missing");
    }
  }

  if (hasShadow && shadowUpdates > 0) {
    if (shadowCol.size() == 1) {
      shadowCol[0] = std::to_string(shadowUpdates) + " updates";
    } else {
      const size_t top = shadowUpdates / 2;
      const size_t bottom = shadowUpdates - top;
      if (top > 0) shadowCol.front() = std::to_string(top) + " updates";
      if (bottom > 0) shadowCol.back() = std::to_string(bottom) + " updates";
    }
  }

  for (size_t i = 0; i < metaCol.size(); ++i)
    mapRow(shadowCol[i], dataCol[i], metaCol[i], c1, c2, c3);

  mapHLine("├", "┴", "┴", "┤", c1, c2, c3);

  // ── DESCRIPTOR ──────────────────────────────────────────────────────
  fmtSize("  DESCRIPTOR  ", descFile, descSize);
  for (const auto& f : desc.dataFields()) fmtField(f);
  {
    const std::string right = std::to_string(recordSize) + " B";
    const int leftMax = inner - static_cast<int>(right.size()) - 1;
    const std::string left = fit("  Record size:", std::max(0, leftMax));
    int gap = inner - static_cast<int>(left.size()) - static_cast<int>(right.size());
    std::cout << "│ " << left << std::string(std::max(1, gap), ' ') << right << " │\n";
  }
  hline("├", "─", "┤", W);

  // ── DATA ────────────────────────────────────────────────────────────
    fmtSize("  DATA        ",
      hasData ? (segmentedData ? dataFile + "_segment_*" : dataFile)
        : (segmentedData ? dataFile + "_segment_* (missing)" : dataFile + " (missing)"),
      dataSize);
  row("  Records: " + std::to_string(dataRecords), W);
    if (segmentedData) {
      row("  Segmented data (RETENTION): " + std::to_string(dataSegments.size()), W);
      row("  Policy: segments=" + std::to_string(retention.segments) +
        " capacity=" + std::to_string(retention.capacity),
    W);
      for (const auto& seg : dataSegments) {
        const fs::path segPath(seg.path);
        row("    [" + std::to_string(seg.index) + "] " + segPath.filename().string() +
          " rec:" + std::to_string(seg.records) +
          " range:" + std::to_string(seg.begin) + "-" + std::to_string(seg.end),
      W);
      }
    }
  hline("├", "─", "┤", W);

  // ── META ────────────────────────────────────────────────────────────
  fmtSize("  META        ", hasMeta ? metaFile : metaFile + " (missing)", metaSize);
  if (hasMeta) {
    row("  Segments: " + std::to_string(segs.size()) +
        "   Records: " + std::to_string(metaRecords), W);
    if (!segs.empty()) {
      const std::string bar = makeMetaBar(segs, dataIdx, barWidth);
      row("  " + bar, W);
      row("  Legend: [====] data  [----] partial null", W);
      row("          [~~~~] nullfill  [XXXX] gap", W);
    }
  }
  hline("├", "─", "┤", W);

  // ── SHADOW ──────────────────────────────────────────────────────────
  fmtSize("  SHADOW      ", hasShadow ? shadowFile : shadowFile + " (missing)", shadowSize);
  if (hasShadow)
    row("  Updates: " + std::to_string(shadowUpdates), W);
  hline("└", "─", "┘", W);
}
