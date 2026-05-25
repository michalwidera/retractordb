#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "descriptor.hpp"

namespace rdb {

/// @brief Klasa utrzymująca trwały indeks informacji o wartościach null dla rekordów przechowywanych w storage.
///
/// Obiekt klasy metaDataStream powinien:
/// - przechowywać dla każdego rekordu wzorzec wartości null w postaci bitsetu zgodnego z Descriptor,
/// - umożliwiać dopisywanie informacji o nowym rekordzie oraz aktualizację informacji dla rekordu już istniejącego,
/// - kompresować kolejne rekordy o tym samym wzorcu null za pomocą prostego RLE,
/// - utrzymywać ostatni segment RLE w pamięci i zapisywać go do pliku przy zmianie wzorca, oznaczeniu gap lub zamknięciu obiektu,
/// - utrzymywać wszystkie segmenty RLE w pliku oprócz ostatniego, który jest w pamięci, aby umożliwić szybkie aktualizacje bez konieczności odczytu całego indeksu z pliku,
/// - zapisywać i odtwarzać indeks z pliku tak, aby mógł być użyty po ponownym uruchomieniu programu,
/// - umożliwiać odczyt wzorca null dla dowolnego logicznego rekordu zarejestrowanego w indeksie,
/// - przechowywać informację o przerwach w transmisji danych jako osobne wpisy gap z licznikiem długości przerwy i wzorcem wszystkich pól ustawionych na null,
/// - nie przechowywać znacznika czasu dla każdego rekordu; czas utworzenia indeksu jest zapisywany w nagłówku pliku,
/// - zarządzać własnymi zasobami w sposób bezpieczny i bez wycieków pamięci.
/// - przyjąć od obiektu storage informację o przerwie w transmisji danych (wraz z jej długością w jednostkach próbkowania) i zapisać ją jako wpis gap w indeksie,
/// - przyjąć od obiektu storage informację o rotacji pliku danych i zresetować indeks do stanu początkowego (bez wpisu gap),
/// - jeśli plik danych nie zrotował, przyjąć od obiektu storage informację o brakujących danych i dopisać odpowiedni wpis gap przed pierwszym nowym rekordem.
/// - umożliwić rotację indeksu podobnie jak obiekty klasy storage, tworząc kopię obecnego pliku indeksu z rozszerzeniem .old/percounter i tworząc nowy, czysty plik indeksu.
/// - gwarantować, że plik indeksu nigdy nie zawiera przestarzałych (nadpisanych logicznie) wpisów: jeśli bieżący segment RLE został wciągnięty do pamięci w celu rozszerzenia (mechanizm lazy overwrite), każda operacja dopisująca nowe wpisy do pliku musi najpierw zastąpić ten przestarzały wpis zamiast dopisywać za nim.
/// @note Indeks przechowuje metadane o wartościach null niezależnie od binarnej zawartości rekordów w storage.
/// @note Wpisy gap są markerami przerw transmisji danych między rekordami i nie są wliczane do numeracji logicznych rekordów zwracanej przez totalRecords().
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
  explicit metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath);

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

  /// @brief All RLE segments: committed (on-disk) entries followed by the pending
  ///        in-memory entry if it is non-empty. Use this for inspection and testing.
  std::vector<IndexRecord> segments() const;

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

  // ── Lifecycle interface ────────────────────────────────────────────

  /// @brief Check whether the index is empty (no records registered).
  /// @return true if totalRecords() == 0
  bool isEmpty() const;

  /// @brief Rotate the meta index file: rename current to .old<percounter>, reset to initial state.
  ///
  /// Called by storage when it detects at startup that the data file was rotated
  /// (empty data file but non-empty meta index). Renames the current meta file to
  /// `<metaFilePath>.old<percounter>` and creates a fresh, empty meta file.
  /// If percounter < 0, the rename is skipped and the index is simply reset.
  /// @param percounter rotation counter (same suffix used for data file rotation)
  void rotate(int percounter);

  /// @brief Clear all index data and reset to initial state.
  ///
  /// Removes all committed entries and resets the pending entry.
  /// Does not modify the creation time.
  /// The meta file is rewritten with only the header.
  void reset();

  /// @brief Flush the pending RLE entry to disk immediately.
  ///
  /// Normally the pending entry is committed lazily (on pattern change,
  /// gap, or destructor). Call this after every write() to guarantee that
  /// null metadata survives a crash or a second storage reader opening
  /// the same file.
  void flushCurrentEntry();

 private:
  /// @brief State of the lazy-overwrite optimisation for the last on-disk RLE entry.
  ///
  /// Invariant:
  ///   dirty == false, committedCount == 0  →  no tracked last entry (initial / after reset)
  ///   dirty == false, committedCount  > 0  →  on-disk tail is valid; may be re-absorbed on same-pattern append
  ///   dirty == true                        →  on-disk tail is stale; must overwrite before next append
  struct DiskTailState {
    size_t committedCount{0};  ///< recordCount of the last entry written to disk (0 = none)
    bool dirty{false};         ///< on-disk tail entry is stale; must overwrite, not append

    void reset() noexcept {
      committedCount = 0;
      dirty          = false;
    }
    void markWritten(size_t n) noexcept {
      committedCount = n;
      dirty          = false;
    }
    void markDirty() noexcept {
      dirty          = true;
      committedCount = 0;
    }
    void clearPending() noexcept { committedCount = 0; }
    bool shouldOverwrite() const noexcept { return dirty; }
    bool hasPending() const noexcept { return committedCount > 0; }
  };

  void createNullBitsetTemplate();
  void loadIndex();                                   ///< read header and restore currentEntry_ from file
  void saveHeader();                                  ///< write file header (creation time) without entries
  void appendEntry(const IndexRecord &entry);         ///< append a single entry to end of file
  void overwriteLastEntry(const IndexRecord &entry);  ///< overwrite the last committed entry in-place (for lazy RLE retract)
  void rewriteFile(const std::vector<IndexRecord> &entries);  ///< rewrite full file (header + entries)
  std::vector<IndexRecord> readCommittedEntries() const;      ///< read all committed entries from file

  /// @brief Locate the RLE segment and offset within it for a given global record index.
  /// @return {segment index in committed entries (nullopt = currentEntry_), offset within segment}
  std::pair<std::optional<size_t>, size_t> locateRecord(size_t recordIndex) const;

  std::string metaFilePath_{};                          ///< file path for saving/loading the meta index
  std::shared_ptr<Descriptor> descriptorRef_;           ///< descriptor of the indexed data stream
  std::chrono::system_clock::time_point creationTime_;  ///< index creation timestamp
  const size_t entrySize_;                         ///< byte size of one serialized IndexRecord; computed once at construction
  size_t committedRecordCount_{0};                 ///< cached total records in committed entries on disk
  DiskTailState tail_{};                           ///< lazy-overwrite state for the last on-disk entry
  IndexRecord currentEntry_;                       ///< accumulator for the pending (not yet committed) RLE run
  mutable std::vector<IndexRecord> entriesCache_;  ///< in-memory copy of committed on-disk entries
  mutable bool cacheValid_{false};                 ///< true when entriesCache_ matches the file; cleared on every write
};  // class metaDataStream

}  // namespace rdb
