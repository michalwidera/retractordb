#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "indexRecord.hpp"

namespace rdb {

/// @brief Surowe I/O formatu pliku indeksu `.meta`: 8-bajtowy nagłówek ZAREZERWOWANY,
///        po którym następują wpisy IndexRecord o stałym rozmiarze.
///
/// Nagłówek do 2026-09-02 niósł czas utworzenia pliku. Pole zostało wycofane, bo żadna
/// ścieżka wykonania go nie odczytywała: było wczytywane z pliku wyłącznie po to, żeby
/// zapisać je z powrotem. Same bajty ZOSTAJĄ i są zapisywane jako zero — kHeaderSize
/// wchodzi we wszystkie offsety wpisów, stare pliki `.meta` mają pozostać czytelne,
/// a oracle bramek badawczych adresują wpisy od stałego offsetu 8 (h10 `decode_meta`,
/// h9 `META_OFFSET_GAP`/`META_OFFSET_NULLBITS`). Nie skracać nagłówka i nie
/// reinterpretować tych bajtów pod nowe znaczenie.
///
/// Obiekt klasy MetaIndexStore powinien:
/// - utrzymywać ścieżkę pliku indeksu oraz rozmiar pojedynczego zserializowanego wpisu,
/// - cache'ować odczytane wpisy (readAll()) i utrzymywać cache przyrostowo przy mutacjach
///   (write-through: append/overwrite/rewrite aktualizują cache zamiast go unieważniać;
///   plik jest ponownie czytany i deserializowany wyłącznie przy pierwszym dostępie),
/// - działać jako wariant inertny (bez żadnego I/O), gdy ścieżka pliku jest pusta — tak samo
///   jak dotychczasowy wariant metaData dla źródeł deklarowanych,
/// - udostępniać abandon() odłączające magazyn od pliku — dalsze operacje stają się no-opem;
///   używane przez metaData przed usunięciem pliku dysponowalnego magazynu, żeby destruktor
///   nie odtworzył go ponownie.
class MetaIndexStore {
 public:
  MetaIndexStore(std::string metaFilePath, size_t entrySize);

  [[nodiscard]] bool empty() const { return metaFilePath_.empty(); }
  [[nodiscard]] const std::string &path() const { return metaFilePath_; }
  [[nodiscard]] bool fileExists() const;

  /// @brief Truncate the file and write only the reserved header. No-op if empty().
  void saveHeader();

  /// @brief All committed entries, in file order. Empty if empty() or file absent/too short.
  /// Zwraca referencję do wewnętrznego cache (gorąca ścieżka: odczyt nullBitset per rekord)
  /// — ważną do następnej operacji mutującej; kto potrzebuje własnej kopii, przypisuje do
  /// zmiennej przez auto (kopia) zamiast const auto&.
  [[nodiscard]] const std::vector<IndexRecord> &readAll() const;

  /// @brief Append one entry to the end of the file. No-op if empty().
  void appendEntry(const IndexRecord &entry);

  /// @brief Overwrite the last on-disk entry in place. No-op if empty() or file too short.
  void overwriteLast(const IndexRecord &entry);

  /// @brief Rewrite the whole file: reserved header + given entries. No-op if empty().
  void rewrite(const std::vector<IndexRecord> &entries);

  /// @brief Detach from the file path — all subsequent operations become no-ops.
  void abandon() { metaFilePath_.clear(); }

 private:
  std::string metaFilePath_;
  size_t entrySize_;
  mutable std::vector<IndexRecord> entriesCache_;
  mutable bool cacheValid_{false};
};

}  // namespace rdb
