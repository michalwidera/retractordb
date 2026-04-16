#pragma once

#include "descriptor.hpp"

#include <boost/rational.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace rdb {

/// @brief Klasa utrzymująca trwały indeks informacji o wartościach null dla rekordów przechowywanych w storage.
///
/// Obiekt klasy metaDataStream powinien:
/// - przechowywać dla każdego rekordu wzorzec wartości null w postaci bitsetu zgodnego z Descriptor,
/// - umożliwiać dopisywanie informacji o nowym rekordzie oraz aktualizację informacji dla rekordu już istniejącego,
/// - kompresować kolejne rekordy o tym samym wzorcu null za pomocą prostego RLE,
/// - utrzymywać ostatni segment RLE w pamięci i zapisywać go do pliku przy zmianie wzorca, oznaczeniu gap lub zamknięciu obiektu,
/// - zapisywać i odtwarzać indeks z pliku tak, aby mógł być użyty po ponownym uruchomieniu programu,
/// - umożliwiać odczyt wzorca null dla dowolnego logicznego rekordu zarejestrowanego w indeksie,
/// - przechowywać informację o przerwach w transmisji danych jako osobne wpisy gap z licznikiem długości przerwy i wzorcem wszystkich pól ustawionych na null,
/// - nie przechowywać znacznika czasu dla każdego rekordu; czas utworzenia indeksu i interwał próbkowania są zapisywane w nagłówku pliku,
/// - zarządzać własnymi zasobami w sposób bezpieczny i bez wycieków pamięci.
///
/// @note Indeks przechowuje metadane o wartościach null niezależnie od binarnej zawartości rekordów w storage.
/// @note Wpisy gap są markerami przerw między rekordami i nie zwiększają numeracji logicznych rekordów zwracanej przez totalRecords().
/// @note Interfejs klasy jest ograniczony do operacji potrzebnych do dopisywania, modyfikacji, odczytu i trwałego utrzymania indeksu null.

class metaDataStream {
 public:
  // ── Nested types ───────────────────────────────────────────────────

  /// @brief Single entry in the meta index – a null bit-set pattern and count
  ///        of consecutive records sharing that pattern.
  struct IndexRecord {
    std::vector<bool> nullBitset;                                     ///< one bit per descriptor field (true = null)
    size_t recordCount{0};                                            ///< number of consecutive records with this pattern
    bool isGap{false};                                                ///< true if this entry represents a transmission gap
    std::vector<std::byte> serialize() const;                         ///< serialize the entry to a vector of bytes
    static IndexRecord deserialize(std::span<const std::byte> data);  ///< deserialize an entry from a vector of bytes
  };

  // ── Construction / destruction ──────────────────────────────────────

  /// @brief Construct meta index for the given descriptor.
  ///
  /// If the meta file already exists, its contents are deserialized
  /// into the in-memory index (loadIndex).
  /// @param descriptor  descriptor of the indexed data stream
  /// @param metaFilePath path of the file to save/load the meta index
  /// @param rInterval   sampling interval for time calculations (default: 1 second)
  explicit metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath,
                          boost::rational<int> rInterval = boost::rational<int>(1));

  /// @brief Destructor – flushes the pending entry to file.
  ~metaDataStream();

  // ── Core update interface ──────────────────────────────────────────

  /// @brief Notify the meta index that the record at @p recordIndex has been modified.
  ///
  /// If the new null bit-set differs from the original, the RLE segment
  /// containing the record is split accordingly.
  /// @param recordIndex position of the modified record in the stream
  /// @param nullBitset  new null bit-set pattern of the modified record
  /// @throws std::out_of_range if recordIndex >= totalRecords()
  void onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitset);

  /// @brief Notify the meta index that a record has been appended.
  ///
  /// If the null bit-set of the new record matches the pattern of the
  /// last entry the counter is incremented; otherwise the current entry
  /// is committed and a new entry is started with count = 1.
  /// @param nullBitset bit pattern indicating which fields are null
  void onRecordAppended(const std::vector<bool> &nullBitset);

  // ── Query interface ────────────────────────────────────────────────

  /// @brief Retrieve the null bit-set for the record at @p recordIndex.
  /// @param recordIndex global position of the record in the stream
  /// @return null bit-set (one bit per descriptor field, true = null)
  /// @throws std::out_of_range if recordIndex >= totalRecords()
  std::vector<bool> getNullBitset(size_t recordIndex) const;

  /// @brief Total number of records tracked by the meta index
  ///        (sum of recordCount across all committed entries + pending entry).
  size_t totalRecords() const;

  /// @brief Read-only access to all committed RLE entries (loaded from file).
  std::vector<IndexRecord> entries() const;

  /// @brief Read-only access to the pending (not yet committed) RLE entry.
  /// Returns the in-memory accumulator; recordCount == 0 if nothing is pending.
  const IndexRecord &pendingEntry() const;

  // ── Transmission-gap interface ─────────────────────────────────────

  /// @brief Mark a transmission gap at the current position.
  ///
  /// Commits the pending entry and inserts a gap marker with all-null
  /// bitset and recordCount equal to the gap duration (in rInterval units)
  /// so that downstream consumers can detect a discontinuity.
  /// The gap entry itself does not consume a record index; it serves as
  /// a marker between records.
  /// @param gapDuration number of missed sampling intervals (default: 1)
  void onTransmissionGap(size_t gapDuration = 1);

  /// @brief Check whether a gap marker exists right before @p recordIndex.
  /// @param recordIndex global position of the record to check
  /// @return true if a gap marker is positioned immediately before this record
  /// @note For recordIndex == 0, returns false (no gap before first record)
  bool isGapBefore(size_t recordIndex) const;

  // ── Time / configuration interface ────────────────────────────────

  /// @brief Get the configured sampling interval.
  /// @return rInterval value used for time calculations
  boost::rational<int> getSamplingInterval() const;

  // ── Lifecycle interface ────────────────────────────────────────────

  /// @brief Check whether the index is empty (no records registered).
  /// @return true if totalRecords() == 0
  bool isEmpty() const;

  /// @brief Clear all index data and reset to initial state.
  ///
  /// Removes all committed entries and resets the pending entry.
  /// Does not modify the creation time or sampling interval.
  /// The meta file is rewritten with only the header.
  void reset();

 private:
  void createNullBitsetTemplate();
  void loadIndex();                                           ///< read header and restore currentEntry_ from file
  void saveHeader();                                          ///< write file header (creation time, rInterval) without entries
  void appendEntry(const IndexRecord &entry);                 ///< append a single entry to end of file
  void rewriteFile(const std::vector<IndexRecord> &entries);  ///< rewrite full file (header + entries)
  void flushCurrentEntry();                                   ///< commit currentEntry_ to file if recordCount > 0
  std::vector<IndexRecord> readCommittedEntries() const;      ///< read all committed entries from file

  /// @brief Locate the RLE segment and offset within it for a given global record index.
  /// @return pair(segment index in committed entries, offset within that segment)
  ///         If segment index == committed entry count, the record is in currentEntry_.
  std::pair<size_t, size_t> locateRecord(size_t recordIndex) const;

  size_t entrySize() const;  ///< byte size of one serialized IndexRecord

  std::string metaFilePath_{};                          ///< file path for saving/loading the meta index
  std::shared_ptr<Descriptor> descriptorRef_;           ///< descriptor of the indexed data stream
  boost::rational<int> rInterval_{1};                   ///< stream sampling interval for time calculations
  std::chrono::system_clock::time_point creationTime_;  ///< index creation timestamp
  size_t committedRecordCount_{0};                      ///< cached total records in committed entries on disk
  IndexRecord currentEntry_;                            ///< accumulator for the pending (not yet committed) RLE run
};  // class metaDataStream

}  // namespace rdb
