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

/// @brief Klasa opisująca strumień indeksujący dane napływające w klasie storage.
///
/// obiekt klasy metaDataStream powinien:
/// - tworzyć dane wraz z napływem danych w storage.
/// - być odpowiedzialny za przechowanie informacji o tym, które pola w rekordach są nullami.
/// - udostępniać informację o wartościach nulli dla każdego zarejestrowanego rekordu w storage.
/// - umożliwiać aktualizację informacji o nullach dla istniejących rekordów.
/// - na bieżąco zapisywać dane do pliku, aby indeks był trwały i mógł być odczytany po ponownym uruchomieniu programu.
/// - przechowywać wszystkie dane w pliku oprócz ostatniego wpisu, który jest buforowany w pamięci i zapisywany do pliku dopiero przy pojawieniu się nowego wzoru nulli lub przy zamknięciu systemu.
/// - umożliwiać jedynie dodawnie i modyfikowanie wartości, ale nie usuwanie, ponieważ usuwanie rekordów w storage jest niedozwolone. (wyjątek stanowi czyszczenie - tzw. purge).
/// - być odpowiedzialny za zarządzanie pamięcią, aby uniknąć wycieków pamięci i zapewnić efektywne wykorzystanie zasobów.
/// - zapewniać informacje o przerwach w transmisji danych.
/// - powinien być w stanie obsłużyć duże ilości danych.
/// - zapewnić serializacji i deseralizacji danych przy urchomieniu i zamknięciu systemu.
/// - nie zapisywać natychmiast danych na dysku w przypadku pojawienia się danych o tym samym wzorze nulli co poprzedni rekord, ale powinien zliczać takie rekordy i zapisywać je jako jeden wpis z licznikiem (RLE).
/// - nie przechowywać znacznika czasu wewnątrz struktury indeksu.
/// - zapis pierwszego rekordu do pliku indeksu nie jest wymagany natychmiast; wymagane jest utworzenie i synchronizacja indeksu przy zamknięciu systemu, a jeśli plik .meta pozostaje po zamknięciu i zarejestrowano dane (nawet bez null), liczba rekordów w indeksie powinna być niezerowa.
///
/// @note Klasa metaDataStream jest kluczowym elementem systemu, który umożliwia efektywne zarządzanie i indeksowanie danych napływających do storage, zapewniając jednocześnie trwałość i integralność danych.
/// @note Implementacja tej klasy powinna być zoptymalizowana pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych w storage.
/// @note W przypadku dużych ilości danych, implementacja powinna uwzględniać mechanizmy buforowania i zarządzania pamięcią, aby zapewnić płynne działanie systemu.
/// @note Klasa metaDataStream powinna być projektowana z myślą o łatwej integracji z innymi komponentami systemu, takimi jak storage, aby zapewnić spójność i efektywność całego systemu.
/// @note W przypadku przerw w transmisji danych, klasa metaDataStream powinna być w stanie wykryć i odpowiednio zareagować na takie sytuacje, np. poprzez zapisywanie stanu indeksu przed przerwą i przywracanie go po wznowieniu transmisji.

class metaDataStream {
 public:
  // ── Nested types ───────────────────────────────────────────────────

  /// @brief Single entry in the meta index – a null bit-set pattern and count
  ///        of consecutive records sharing that pattern.
  struct IndexRecord {
    std::vector<bool> nullBitset;                                     ///< one bit per descriptor field (true = null)
    size_t recordCount{0};                                            ///< number of consecutive records with this pattern
    bool isGap{false};                                                ///< true = this entry marks a transmission gap
    std::vector<std::byte> serialize() const;                         ///< serialize the entry to a vector of bytes
    static IndexRecord deserialize(std::span<const std::byte> data);  ///< deserialize an entry from a vector of bytes
  };

  /// @brief Represents a transmission gap in the data stream.
  struct TransmissionGap {
    size_t recordIndex{0};                           ///< global position where the gap starts
    std::chrono::system_clock::time_point timestamp; ///< calculated timestamp of the gap
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

  /// @brief Purge (remove) all records up to and including @p upToRecordIndex.
  ///
  /// This is the only operation that removes records from the meta index.
  /// Records are removed in the context of storage purge operations.
  /// The meta index is rewritten to remove the purged entries.
  /// @param upToRecordIndex global position of the last record to remove (inclusive)
  /// @throws std::out_of_range if upToRecordIndex >= totalRecords()
  /// @note This operation rebuilds the file, renumbering all remaining record indices.
  void purge(size_t upToRecordIndex);

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
  /// Commits the pending entry and inserts a zero-length gap marker
  /// so that downstream consumers can detect a discontinuity.
  /// The gap entry itself does not consume a record index; it serves as
  /// a marker between records.
  void onTransmissionGap();

  /// @brief Check whether a gap marker exists right before @p recordIndex.
  /// @param recordIndex global position of the record to check
  /// @return true if a gap marker is positioned immediately before this record
  /// @note For recordIndex == 0, returns false (no gap before first record)
  bool isGapBefore(size_t recordIndex) const;

  /// @brief Retrieve all transmission gaps recorded in the index.
  /// @return vector of TransmissionGap structures with their positions and timestamps
  /// @note Gaps are represented as markers between records; gaps themselves don't have indices
  std::vector<TransmissionGap> getTransmissionGaps() const;

  // ── Time / configuration interface ────────────────────────────────

  /// @brief Get the creation time of the index.
  /// @return timestamp when index was created
  std::chrono::system_clock::time_point getCreationTime() const;

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
  /// @note This is different from purge() which maintains relative indices.
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

  std::chrono::system_clock::time_point calculateRecordTimestamp(size_t recordIndex) const;
  bool isFieldNull(size_t recordIndex, size_t fieldIndex) const;
  const Descriptor &descriptor() const;
  size_t entrySize() const;  ///< byte size of one serialized IndexRecord

  std::string metaFilePath_{};                          ///< file path for saving/loading the meta index
  std::shared_ptr<Descriptor> descriptorRef_;           ///< descriptor of the indexed data stream
  boost::rational<int> rInterval_{1};                   ///< stream sampling interval for time calculations
  std::chrono::system_clock::time_point creationTime_;  ///< index creation timestamp
  size_t committedRecordCount_{0};                      ///< cached total records in committed entries on disk
  IndexRecord currentEntry_;                            ///< accumulator for the pending (not yet committed) RLE run
};  // class metaDataStream

}  // namespace rdb
