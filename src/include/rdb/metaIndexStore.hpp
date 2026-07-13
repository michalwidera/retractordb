#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include "indexRecord.hpp"

namespace rdb {

/// @brief Surowe I/O formatu pliku indeksu `.meta`: 8-bajtowy nagłówek (czas utworzenia)
///        po którym następują wpisy IndexRecord o stałym rozmiarze.
///
/// Obiekt klasy MetaIndexStore powinien:
/// - utrzymywać ścieżkę pliku indeksu oraz rozmiar pojedynczego zserializowanego wpisu,
/// - cache'ować odczytane wpisy (readAll()) aż do kolejnej mutacji pliku,
/// - działać jako wariant inertny (bez żadnego I/O), gdy ścieżka pliku jest pusta — tak samo
///   jak dotychczasowy wariant metaData dla źródeł deklarowanych,
/// - przy konstrukcji wczytać czas utworzenia z nagłówka istniejącego pliku, jeśli plik istnieje,
/// - udostępniać abandon() odłączające magazyn od pliku — dalsze operacje stają się no-opem;
///   używane przez metaData przed usunięciem pliku dysponowalnego magazynu, żeby destruktor
///   nie odtworzył go ponownie.
class MetaIndexStore {
 public:
  MetaIndexStore(std::string metaFilePath, size_t entrySize);

  [[nodiscard]] bool empty() const { return metaFilePath_.empty(); }
  [[nodiscard]] const std::string &path() const { return metaFilePath_; }
  [[nodiscard]] bool fileExists() const;

  void setCreationTime(std::chrono::system_clock::time_point t) { creationTime_ = t; }

  /// @brief Truncate the file and write only the header (creationTime_). No-op if empty().
  void saveHeader();

  /// @brief All committed entries, in file order. Empty if empty() or file absent/too short.
  [[nodiscard]] std::vector<IndexRecord> readAll() const;

  /// @brief Append one entry to the end of the file. No-op if empty().
  void appendEntry(const IndexRecord &entry);

  /// @brief Overwrite the last on-disk entry in place. No-op if empty() or file too short.
  void overwriteLast(const IndexRecord &entry);

  /// @brief Rewrite the whole file: header (creationTime_) + given entries. No-op if empty().
  void rewrite(const std::vector<IndexRecord> &entries);

  /// @brief Detach from the file path — all subsequent operations become no-ops.
  void abandon() { metaFilePath_.clear(); }

 private:
  void loadHeaderTimestamp();  ///< read creationTime_ from an existing file's header, if present

  std::string metaFilePath_;
  size_t entrySize_;
  std::chrono::system_clock::time_point creationTime_;
  mutable std::vector<IndexRecord> entriesCache_;
  mutable bool cacheValid_{false};
};

}  // namespace rdb
