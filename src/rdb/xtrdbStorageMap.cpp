#include "xtrdbStorageMap.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/metaDataStream.hpp"

// Prezentacja mapy pliku (przykład)
//
// ┌──────────────────────────────────────────────────────────────┐
// │  Storage map: test_file                                      │
// ├──────────────────────────────────────────────────────────────┤
// │ [shadow]   │ [binary data] │ [meta index]                    │
// ├────────────┼───────────────┼─────────────────────────────────┤
// │ s0 1u      │ s0 0-3        │ [====] 4 records, no nulls      │
// │ 1 updates  │ s1 3-4        │                                 │
// ├────────────┴───────────────┴─────────────────────────────────┤
// │  DESCRIPTOR  test_file.desc                             28 B │
// │  BYTE dane                                               1 B │
// │  Record size:                                            1 B │
// ├──────────────────────────────────────────────────────────────┤
// │  DATA TOTAL  rec=4 src=0 seg=4                           4 B │
// │  Records: 4                                                  │
// │  Source: test_file   Segments: test_file_segment_*           │
// │  Segmented data (RETENTION): 2                               │
// │  Policy: segments=2 capacity=3                               │
// │  Retention cap records: 6                                    │
// │  Retention cap bytes: 6                                      │
// │  Total records: 4                                            │
// │    current=0  segments=4                                     │
// │  Total bytes: 4                                              │
// │    current=0  segments=4                                     │
// │    [0] test_file_segment_0 rec:3 range:0-3                   │
// │    [1] test_file_segment_1 rec:1 range:3-4                   │
// ├──────────────────────────────────────────────────────────────┤
// │  META        test_file.meta                             26 B │
// │  Segments: 1   Records: 4                                    │
// │  [===========================4============================]  │
// │  Legend: [====] data  [----] partial null                    │
// │          [~~~~] nullfill  [XXXX] gap                         │
// ├──────────────────────────────────────────────────────────────┤
// │  SHADOW      test_file.shadow                            9 B │
// │  Updates: 1                                                  │
// ├──────────────────────────────────────────────────────────────┤
// │  ROTATED FILES                                               │
// │  [2] test_file.old2                                      4 B │
// │      test_file.meta.old2                                26 B │
// │  [1] test_file.old1                                      4 B │
// └──────────────────────────────────────────────────────────────┘

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

struct RotatedFileInfo {
  int counter{};
  std::string ext;  // middle part between stem and .old<N>, e.g. "" / ".meta" / ".shadow"
  std::string path;
  uintmax_t bytes{};
};

struct DataStats {
  bool segmentedData{};
  bool hasData{};
  bool hasMeta{};
  bool hasShadow{};

  uintmax_t descSize{};
  uintmax_t currentDataSize{};
  uintmax_t segmentDataSize{};
  uintmax_t dataSize{};
  uintmax_t metaSize{};
  uintmax_t shadowSize{};

  size_t currentDataRecords{};
  size_t segmentDataRecords{};
  size_t dataRecords{};
  size_t shadowUpdates{};

  bool hasBoundedRetention{};
  size_t retentionCapacityRecords{};
  uintmax_t retentionCapacityBytes{};
};

static std::string metaLabel(const Segment &seg, const std::vector<size_t> &dataIdx);

static std::vector<RotatedFileInfo> readRotatedFiles(const std::string &baseName) {
  const fs::path basePath(baseName);
  const fs::path dirPath               = basePath.has_parent_path() ? basePath.parent_path() : fs::path(".");
  const std::string stem               = basePath.filename().string();
  constexpr std::string_view oldMarker = ".old";

  std::vector<RotatedFileInfo> out;
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) return out;

  for (const auto &entry : fs::directory_iterator(dirPath)) {
    if (!entry.is_regular_file()) continue;
    const std::string fname = entry.path().filename().string();
    if (!fname.starts_with(stem)) continue;
    const auto oldPos = fname.find(oldMarker, stem.size());
    if (oldPos == std::string::npos) continue;
    const std::string suffix = fname.substr(oldPos + oldMarker.size());
    if (suffix.empty()) continue;
    if (!std::all_of(suffix.begin(), suffix.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) continue;
    RotatedFileInfo info;
    info.counter = std::stoi(suffix);
    info.ext     = fname.substr(stem.size(), oldPos - stem.size());
    info.path    = entry.path().string();
    info.bytes   = fs::file_size(entry.path());
    out.push_back(std::move(info));
  }

  std::ranges::sort(out,
                    [](const auto &a, const auto &b) { return a.counter != b.counter ? a.counter > b.counter : a.ext < b.ext; });
  return out;
}

static size_t recordCount(uintmax_t bytes, size_t recordSize) {
  return (recordSize > 0) ? static_cast<size_t>(bytes / recordSize) : 0;
}

static std::vector<DataSegmentInfo> readDataSegments(const std::string &baseName, const rdb::retention_t &retention,
                                                     size_t recordSize) {
  if (retention.noRetention()) return {};

  const fs::path basePath(baseName);
  const fs::path dirPath   = basePath.has_parent_path() ? basePath.parent_path() : fs::path(".");
  const std::string stem   = basePath.filename().string();
  const std::string prefix = stem + "_segment_";

  std::vector<DataSegmentInfo> out;
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) return out;

  for (const auto &entry : fs::directory_iterator(dirPath)) {
    if (!entry.is_regular_file()) continue;

    const std::string fileName = entry.path().filename().string();
    if (!fileName.starts_with(prefix)) continue;

    const std::string suffix = fileName.substr(prefix.size());
    if (suffix.empty()) continue;
    if (!std::all_of(suffix.begin(), suffix.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) continue;

    DataSegmentInfo info;
    info.index                = static_cast<size_t>(std::stoull(suffix));
    info.path                 = entry.path().string();
    info.bytes                = fs::file_size(entry.path());
    info.records              = (recordSize > 0) ? static_cast<size_t>(info.bytes / recordSize) : 0;
    info.begin                = retention.capacity > 0 ? info.index * retention.capacity : 0;
    info.end                  = info.begin + info.records;
    const fs::path shadowPath = entry.path().string() + ".shadow";
    if (fs::exists(shadowPath)) {
      info.shadowBytes   = fs::file_size(shadowPath);
      info.shadowUpdates = (recordSize > 0) ? static_cast<size_t>(info.shadowBytes / (sizeof(size_t) + recordSize)) : 0;
    }
    out.push_back(std::move(info));
  }

  std::ranges::sort(out, [](const auto &a, const auto &b) { return a.index < b.index; });
  return out;
}

static std::string segmentLabel(size_t index) { return "s" + std::to_string(index); }

static DataStats collectDataStats(const std::string &descFile, const std::string &dataFile, const std::string &metaFile,
                                  const std::string &shadowFile, const std::vector<DataSegmentInfo> &dataSegments,
                                  const rdb::retention_t &retention, size_t recordSize) {
  DataStats stats;
  stats.segmentedData = !retention.noRetention();

  const bool hasDataFile    = fs::exists(dataFile);
  const bool hasSegmentData = !dataSegments.empty();
  const bool hasRootShadow  = fs::exists(shadowFile);

  stats.hasData = stats.segmentedData ? (hasSegmentData || hasDataFile) : hasDataFile;
  stats.hasMeta = fs::exists(metaFile);

  stats.descSize        = fs::file_size(descFile);
  stats.currentDataSize = hasDataFile ? fs::file_size(dataFile) : 0;

  uintmax_t segmentShadowSize = 0;
  size_t segmentShadowUpdates = 0;
  for (const auto &seg : dataSegments) {
    stats.segmentDataSize += seg.bytes;
    stats.segmentDataRecords += seg.records;
    segmentShadowSize += seg.shadowBytes;
    segmentShadowUpdates += seg.shadowUpdates;
  }

  stats.dataSize           = stats.segmentDataSize + stats.currentDataSize;
  stats.currentDataRecords = recordCount(stats.currentDataSize, recordSize);
  stats.dataRecords        = stats.segmentDataRecords + stats.currentDataRecords;

  if (!stats.segmentedData) {
    stats.dataSize    = hasDataFile ? stats.currentDataSize : 0;
    stats.dataRecords = hasDataFile ? stats.currentDataRecords : 0;
  }

  stats.metaSize = stats.hasMeta ? fs::file_size(metaFile) : 0;

  if (stats.segmentedData) {
    stats.shadowSize    = segmentShadowSize + (hasRootShadow ? fs::file_size(shadowFile) : 0);
    stats.shadowUpdates = segmentShadowUpdates;
    if (hasRootShadow) stats.shadowUpdates += recordCount(fs::file_size(shadowFile), sizeof(size_t) + recordSize);
    stats.hasShadow = hasRootShadow || segmentShadowSize > 0;
  } else {
    stats.shadowSize    = hasRootShadow ? fs::file_size(shadowFile) : 0;
    stats.shadowUpdates = hasRootShadow ? recordCount(stats.shadowSize, sizeof(size_t) + recordSize) : 0;
    stats.hasShadow     = hasRootShadow;
  }

  stats.hasBoundedRetention = stats.segmentedData && retention.segments > 0 && retention.capacity > 0;
  if (stats.hasBoundedRetention) {
    stats.retentionCapacityRecords = retention.segments * retention.capacity;
    stats.retentionCapacityBytes   = static_cast<uintmax_t>(stats.retentionCapacityRecords) * recordSize;
  }

  return stats;
}

static void buildMainMapColumns(const std::vector<DataSegmentInfo> &dataSegments, bool segmentedData, bool hasMeta, bool hasData,
                                size_t dataRecords, const std::vector<Segment> &segs, const std::vector<size_t> &dataIdx,
                                std::vector<std::string> &shadowCol, std::vector<std::string> &dataCol,
                                std::vector<std::string> &metaCol) {
  if (segmentedData) {
    for (const auto &ds : dataSegments) {
      const std::string segLbl = segmentLabel(ds.index);
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
      return;
    }

    if (metaCol.empty()) {
      shadowCol.emplace_back("");
      dataCol.emplace_back("");
      metaCol.emplace_back(hasMeta ? "meta file is empty" : "meta missing");
    }
    return;
  }

  if (hasMeta && !segs.empty()) {
    size_t dataPos = 0;
    for (const auto &seg : segs) {
      shadowCol.emplace_back("");
      if (seg.isGap) {
        dataCol.emplace_back("");
      } else {
        const size_t begin = dataPos;
        const size_t end   = dataPos + seg.recordCount;
        dataPos            = end;
        dataCol.push_back(std::to_string(begin) + "-" + std::to_string(end));
      }
      metaCol.push_back(metaLabel(seg, dataIdx));
    }
    return;
  }

  shadowCol.emplace_back("");
  dataCol.push_back((hasData && dataRecords > 0) ? ("0-" + std::to_string(dataRecords)) : "");
  metaCol.emplace_back(hasMeta ? "meta file is empty" : "meta missing");
}

static std::vector<Segment> readMetaFile(const std::string &path, size_t fieldCount) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) return {};

  in.seekg(0, std::ios::end);
  const auto fileSize          = static_cast<size_t>(in.tellg());
  constexpr size_t kHeaderSize = sizeof(int64_t);
  if (fileSize <= kHeaderSize) return {};

  const size_t entrySize   = sizeof(uint8_t) + sizeof(size_t) + sizeof(size_t) + (fieldCount + 7) / 8;
  const size_t payloadSize = fileSize - kHeaderSize;

  in.seekg(static_cast<std::streamoff>(kHeaderSize), std::ios::beg);
  std::vector<std::byte> buf(payloadSize);
  in.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(payloadSize));

  std::vector<Segment> result;
  std::span<const std::byte> rem(buf);
  while (rem.size() >= entrySize) {
    result.push_back(Segment::deserialize(rem.subspan(0, entrySize)));
    rem = rem.subspan(entrySize);
  }
  return result;
}

static char nullChar(const Segment &seg, const std::vector<size_t> &dataIdx) {
  if (dataIdx.empty()) return '=';
  bool anyNull = false;
  bool allNull = true;
  for (size_t i : dataIdx) {
    const bool n = (i < seg.nullBitset.size()) && seg.nullBitset[i];
    if (n)
      anyNull = true;
    else
      allNull = false;
  }
  if (!anyNull) return '=';
  if (allNull) return '~';
  return '-';
}

static std::string makeMetaBar(const std::vector<Segment> &segs, const std::vector<size_t> &dataIdx, int barWidth) {
  size_t total = 0;
  for (const auto &s : segs)
    total += s.recordCount;
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
    const auto &seg         = segs[i];
    const int inner         = widths[i] - 2;
    const char fill         = seg.isGap ? 'X' : nullChar(seg, dataIdx);
    const std::string label = seg.isGap ? ("gap:" + std::to_string(seg.recordCount)) : std::to_string(seg.recordCount);

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

static std::string fit(const std::string &s, int w) {
  if (w <= 0) return "";
  if (static_cast<int>(s.size()) <= w) return s + std::string(w - static_cast<int>(s.size()), ' ');
  if (w <= 3) return s.substr(0, w);
  return s.substr(0, w - 3) + "...";
}

static std::string repeatGlyph(const std::string &glyph, int count) {
  std::string out;
  for (int i = 0; i < count; ++i)
    out += glyph;
  return out;
}

class TablePrinter {
 public:
  explicit TablePrinter(int width) : width_(width), inner_(width - 4) {}

  [[nodiscard]] int inner() const { return inner_; }
  [[nodiscard]] int metaBarWidth() const { return inner_ - 2; }

  void hline(std::string_view l, std::string_view m, std::string_view r) const {
    std::cout << l << repeatGlyph(std::string(m), width_ - 2) << r << "\n";
  }

  void row(const std::string &s) const { std::cout << "│ " << std::left << std::setw(inner_) << s.substr(0, inner_) << " │\n"; }

  void valueRow(const std::string &left, const std::string &right) const {
    const int leftMax         = inner_ - static_cast<int>(right.size()) - 1;
    const std::string leftFit = fit(left, std::max(0, leftMax));
    const int gap             = std::max(1, inner_ - static_cast<int>(leftFit.size()) - static_cast<int>(right.size()));
    std::cout << "│ " << leftFit << std::string(gap, ' ') << right << " │\n";
  }

  void sizeRow(const std::string &tag, const std::string &name, uintmax_t bytes) const {
    valueRow(tag + name, std::to_string(bytes) + " B");
  }

  void mapHLine(std::string_view l, std::string_view j1, std::string_view j2, std::string_view r, int c1, int c2, int c3) const {
    std::cout << l << repeatGlyph("─", c1 + 2) << j1 << repeatGlyph("─", c2 + 2) << j2 << repeatGlyph("─", c3 + 2) << r << "\n";
  }

  void mapRow(const std::string &s1, const std::string &s2, const std::string &s3, int c1, int c2, int c3) const {
    std::cout << "│ " << fit(s1, c1) << " │ " << fit(s2, c2) << " │ " << fit(s3, c3) << " │\n";
  }

 private:
  int width_;
  int inner_;
};

static std::string metaLabel(const Segment &seg, const std::vector<size_t> &dataIdx) {
  if (seg.isGap) return "[XXXX] " + std::to_string(seg.recordCount) + " records, gap";

  const char fill   = nullChar(seg, dataIdx);
  std::string state = "no nulls";
  if (fill == '-') state = "some nulls";
  if (fill == '~') state = "all nulls";

  return "[" + std::string(4, fill) + "] " + std::to_string(seg.recordCount) + " records, " + state;
}

void showStorageMap(const std::string &baseName) {
  const std::string descFile   = baseName + ".desc";
  const std::string &dataFile  = baseName;
  const std::string metaFile   = baseName + ".meta";
  const std::string shadowFile = baseName + ".shadow";
  const auto rotatedFiles      = readRotatedFiles(baseName);

  if (!fs::exists(descFile)) {
    std::cerr << "Descriptor not found: " << descFile << "\n";
    return;
  }

  rdb::Descriptor desc;
  {
    std::ifstream df(descFile);
    df >> desc;
  }

  const size_t recordSize = desc.getSizeInBytes();
  const size_t fieldCount = desc.size();
  const auto retention    = desc.retention();

  // indices of data fields (non-config, non-null) in the descriptor vector
  std::vector<size_t> dataIdx;
  for (size_t i = 0; i < desc.size(); ++i)
    if (desc[i].rtype < rdb::TYPE && desc[i].rtype != rdb::NULLTYPE) dataIdx.push_back(i);

  const auto dataSegments = readDataSegments(baseName, retention, recordSize);
  const DataStats stats   = collectDataStats(descFile, dataFile, metaFile, shadowFile, dataSegments, retention, recordSize);

  auto segs          = stats.hasMeta ? readMetaFile(metaFile, fieldCount) : std::vector<Segment>{};
  size_t metaRecords = 0;
  for (const auto &s : segs)
    if (!s.isGap) metaRecords += s.recordCount;

  constexpr int W  = 64;
  constexpr int c1 = 10;
  constexpr int c2 = 13;
  constexpr int c3 = 31;

  const TablePrinter table(W);

  auto printField = [&](const rdb::rField &f) {
    std::string left = "  " + std::string(rdb::GetStringdescFld(f.rtype)) + "  " + f.rname;
    if (f.rarray > 1) left += "[" + std::to_string(f.rarray) + "]";
    table.valueRow(left, std::to_string(desc.fieldSize(f)) + " B");
  };

  table.hline("┌", "─", "┐");
  { table.row("  Storage map: " + baseName); }
  table.hline("├", "─", "┤");

  // Primary three-column map, similar to the visual example in the header comment.
  table.mapRow("[shadow]", "[binary data]", "[meta index]", c1, c2, c3);
  table.mapHLine("├", "┼", "┼", "┤", c1, c2, c3);

  std::vector<std::string> shadowCol;
  std::vector<std::string> dataCol;
  std::vector<std::string> metaCol;

  buildMainMapColumns(dataSegments, stats.segmentedData, stats.hasMeta, stats.hasData, stats.dataRecords, segs, dataIdx,
                      shadowCol, dataCol, metaCol);

  if (stats.hasShadow && stats.shadowUpdates > 0) {
    if (shadowCol.size() == 1) {
      shadowCol[0] = std::to_string(stats.shadowUpdates) + " updates";
    } else {
      const size_t top    = stats.shadowUpdates / 2;
      const size_t bottom = stats.shadowUpdates - top;
      if (top > 0) shadowCol.front() = std::to_string(top) + " updates";
      if (bottom > 0) shadowCol.back() = std::to_string(bottom) + " updates";
    }
  }

  for (size_t i = 0; i < metaCol.size(); ++i)
    table.mapRow(shadowCol[i], dataCol[i], metaCol[i], c1, c2, c3);

  table.mapHLine("├", "┴", "┴", "┤", c1, c2, c3);

  // ── DESCRIPTOR ──────────────────────────────────────────────────────
  table.sizeRow("  DESCRIPTOR  ", descFile, stats.descSize);
  for (const auto &f : desc.dataFields())
    printField(f);
  table.valueRow("  Record size:", std::to_string(recordSize) + " B");
  table.hline("├", "─", "┤");

  // ── DATA ────────────────────────────────────────────────────────────
  if (stats.segmentedData) {
    table.sizeRow("  DATA TOTAL  ",
                  "rec=" + std::to_string(stats.dataRecords) + " src=" + std::to_string(stats.currentDataRecords) +
                      " seg=" + std::to_string(stats.segmentDataRecords),
                  stats.dataSize);
  } else {
    table.sizeRow("  DATA        ", stats.hasData ? dataFile : dataFile + " (missing)", stats.dataSize);
  }
  table.row("  Records: " + std::to_string(stats.dataRecords));
  if (stats.segmentedData) {
    table.row("  Source: " + dataFile + "   Segments: " + dataFile + "_segment_*");
    table.row("  Segmented data (RETENTION): " + std::to_string(dataSegments.size()));
    table.row("  Policy: segments=" + std::to_string(retention.segments) + " capacity=" + std::to_string(retention.capacity));
    if (stats.hasBoundedRetention) {
      table.row("  Retention cap records: " + std::to_string(stats.retentionCapacityRecords));
      table.row("  Retention cap bytes: " + std::to_string(stats.retentionCapacityBytes));
    } else {
      table.row("  Retention cap: unbounded");
    }
    table.row("  Total records: " + std::to_string(stats.dataRecords));
    table.row("    current=" + std::to_string(stats.currentDataRecords) +
              "  segments=" + std::to_string(stats.segmentDataRecords));
    table.row("  Total bytes: " + std::to_string(stats.dataSize));
    table.row("    current=" + std::to_string(stats.currentDataSize) + "  segments=" + std::to_string(stats.segmentDataSize));
    for (const auto &seg : dataSegments) {
      const fs::path segPath(seg.path);
      table.row("    [" + std::to_string(seg.index) + "] " + segPath.filename().string() +
                " rec:" + std::to_string(seg.records) + " range:" + std::to_string(seg.begin) + "-" + std::to_string(seg.end));
    }
  }
  table.hline("├", "─", "┤");

  // ── META ────────────────────────────────────────────────────────────
  table.sizeRow("  META        ", stats.hasMeta ? metaFile : metaFile + " (missing)", stats.metaSize);
  if (stats.hasMeta) {
    table.row("  Segments: " + std::to_string(segs.size()) + "   Records: " + std::to_string(metaRecords));
    if (!segs.empty()) {
      const std::string bar = makeMetaBar(segs, dataIdx, table.metaBarWidth());
      table.row("  " + bar);
      table.row("  Legend: [====] data  [----] partial null");
      table.row("          [~~~~] nullfill  [XXXX] gap");
    }
  }
  table.hline("├", "─", "┤");

  // ── SHADOW ──────────────────────────────────────────────────────────
  table.sizeRow("  SHADOW      ", stats.hasShadow ? shadowFile : shadowFile + " (missing)", stats.shadowSize);
  if (stats.hasShadow) table.row("  Updates: " + std::to_string(stats.shadowUpdates));

  // ── ROTATED FILES ────────────────────────────────────────────────────
  if (!rotatedFiles.empty()) {
    table.hline("├", "─", "┤");
    table.row("  ROTATED FILES");
    int lastCounter = -1;
    for (const auto &rf : rotatedFiles) {
      const fs::path rp(rf.path);
      const std::string label =
          (rf.counter != lastCounter ? "[" + std::to_string(rf.counter) + "] " : "    ") + rp.filename().string();
      lastCounter = rf.counter;
      table.valueRow("  " + label, std::to_string(rf.bytes) + " B");
    }
  }
  table.hline("└", "─", "┘");
}
