#pragma once

#include "descriptor.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
namespace rdb {

/// @brief Meta index for an indexed data stream.
///
/// Tracks null-field information across records in a data stream using
/// run-length encoded null bit-set entries.  Each entry holds a bit pattern
/// (one bit per descriptor field, 1 = null) together with a count of
/// consecutive records that share the same pattern.
///
/// Design notes (system design):
///
/// - The meta index shall be updated (grow) whenever the indexed data stream changes (grows).
/// - The meta index shall count the number of records in the indexed data stream that share the same null bit-set values.
/// - The meta index shall store records in the form of bit fields indicating which fields are nulls in the indexed data stream.
/// - The meta index shall contain, in each bit field, information about which fields are nulls.
/// - The meta index shall contain a counter alongside each bit field indicating the number of records with the same null bit-set
/// values.
/// - The meta index shall create a new entry when the null bit-set value changes and restart counting from one for that entry.
/// - The meta index shall, when there are no records containing null values, contain only a single entry counting all records in
/// the indexed stream.
/// - The meta index shall be a data structure that grows with the number of records in the indexed data stream.
/// - The meta index shall be stored on disk together with the indexed data stream.
/// - The meta index shall update itself automatically when the indexed data stream changes, without manual intervention.
/// - The meta index shall be rebuilt when the indexed data stream changes due to record addition, modification, or deletion.
/// - The meta index shall grow with the number of records in the indexed data stream.
/// - The meta index shall provide information about records containing null values in the indexed data stream.
/// - The meta index shall save itself to disk when work with the indexed data stream is finished.
/// - The meta index shall be loaded from disk when the program is restarted.
/// - The meta index shall write an Entry to disk (counting the number of records in the indexed data stream) when the null
/// bit-set value changes.
/// - The meta index shall not update the Entry on disk when a record with the same null bit-set values is appended.

class metaDataStream {
  void createNullBitsetTemplate();
  std::vector<bool> nullBitset_;  ///< one bit per descriptor field (true = null)
  std::string metaFilePath_{};    ///< file path for saving/loading the meta index
  std::fstream indexFile_;  ///< file stream for reading/writing the meta index

 public:
  /// @brief Single entry in the meta index – a null bit-set pattern and count
  ///        of consecutive records sharing that pattern.
  struct IndexRecord {
    std::vector<bool> nullBitset;  ///< one bit per descriptor field (true = null)
    size_t recordCount{0};         ///< number of consecutive records with this pattern
  };

  // ── Construction / destruction ──────────────────────────────────────

  /// @brief Construct meta index for the given descriptor.
  /// @param descriptor descriptor of the indexed data stream
  /// @param metaFilePath path of the file to save/load the meta index
  explicit metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath);

  /// @brief Destructor - closes artifacts.
  virtual ~metaDataStream();

  // ── Core update interface ──────────────────────────────────────────

  /// @brief Notify the meta index that the record at @p recordIndex has
  ///        been modified.
  /// @param recordIndex  position of the modified record in the stream
  /// @param nullBitset   new null bit-set pattern of the modified record
  void onRecordModified(size_t recordIndex, std::vector<bool> &nullBitset);

  /// @brief Notify the meta index that a record has been appended.
  ///
  /// If the null bit-set of the new record matches the pattern of the
  /// last entry the counter is incremented; otherwise a new entry is
  /// created and counting restarts from one.
  /// @param nullBitset bit pattern indicating which fields are null
  void onRecordAppended(std::vector<bool> &nullBitset);

#ifdef RDB_WORK_IN_PROGRESS
  /// @brief Notify the meta index that the record at @p recordIndex has
  ///        been deleted.
  /// @param recordIndex position of the deleted record in the stream
  void onRecordDeleted(size_t recordIndex);

  // ── Rebuild ────────────────────────────────────────────────────────

  /// @brief Fully rebuild the meta index by scanning all records in the
  ///        indexed data stream.
  /// @param allNullBitsets ordered sequence of null bit-set patterns,
  ///        one per record in the stream
  void rebuild(const std::vector<std::vector<bool>> &allNullBitsets);

  /// @brief Clear the meta index (remove all entries).
  void clear();

  // ── Persistence ────────────────────────────────────────────────────

  /// @brief Save the meta index to disk.
  /// @param filePath path of the file to write
  /// @return true on success
  bool save(const std::string &filePath) const;

  /// @brief Load the meta index from disk.
  /// @param filePath path of the file to read
  /// @return true on success
  bool load(const std::string &filePath);

  // ── Query interface ────────────────────────────────────────────────

  /// @brief Return the current list of entries.
  const std::vector<IndexRecord> &entries() const;

  /// @brief Return the total number of indexed records (sum of all
  ///        entry counters).
  size_t totalRecords() const;

  /// @brief Return the number of entries in the meta index.
  size_t size() const;

  /// @brief Check whether any record in the stream has a null value in
  ///        the field at @p fieldPosition.
  /// @param fieldPosition zero-based field index in the descriptor
  /// @return true if at least one entry with a non-zero counter has the
  ///         corresponding bit set
  bool hasNullInField(size_t fieldPosition) const;

  /// @brief Return the total number of records that have a null value
  ///        in the field at @p fieldPosition.
  /// @param fieldPosition zero-based field index in the descriptor
  size_t countNullsInField(size_t fieldPosition) const;

  /// @brief Check whether any record contains at least one null field.
  /// @return true if there is at least one entry whose bit-set has any
  ///         bit set and whose counter is greater than zero
  bool hasAnyNull() const;

  /// @brief Return the entry at the given index.
  /// @param index zero-based index into the entries vector
  const IndexRecord &operator[](size_t index) const;

  // ── Data members ───────────────────────────────────────────────────
#endif
  /// @brief Descriptor of the indexed data stream.
  std::shared_ptr<Descriptor> descriptorRef_;
};  // class metaDataStream

}  // namespace rdb
