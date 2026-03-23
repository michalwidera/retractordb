#pragma once

#include "descriptor.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <span>

namespace rdb {

/// @brief Klasa opisująca strumień indeksujący dane napływające w klasie storageAccessor.
///
/// obiekt klasy metaDataStream powinien:
/// - tworzyć dane wraz z napływem danych w storageAccessor.
/// - być odpowiedzialny za przechowanie informacji o tym, które pola w rekordach są nullami.
/// - udostępniać informację o wartościach nulli dla każdego zarejestrowanego rekordu w storageAccessor.
/// - umożliwiać aktualizację informacji o nullach dla istniejących rekordów.
/// - na bieżąco zapisywać dane do pliku, aby indeks był trwały i mógł być odczytany po ponownym uruchomieniu programu.
/// - umożliwiać jedynie dodawnie i modyfikowanie wartości, ale nie usuwanie, ponieważ usuwanie rekordów w storageAccessor jest niedozwolone.
/// - być odpowiedzialny za zarządzanie pamięcią, aby uniknąć wycieków pamięci i zapewnić efektywne wykorzystanie zasobów.
/// - zapewniać informacje o przerwach w transmisji danych.
/// - powinien dostarczyć informacji o rzeczywistym czasie rejestracji danych.
/// - powinien być w stanie obsłużyć duże ilości danych.
/// - zapewnić serializacji i deseralizacji danych przy urchomieniu i zamknięciu systemu.
///
/// @note Klasa metaDataStream jest kluczowym elementem systemu, który umożliwia efektywne zarządzanie i indeksowanie danych napływających do storageAccessor, zapewniając jednocześnie trwałość i integralność danych.
/// @note Implementacja tej klasy powinna być zoptymalizowana pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych w storageAccessor.
/// @note W przypadku dużych ilości danych, implementacja powinna uwzględniać mechanizmy buforowania i zarządzania pamięcią, aby zapewnić płynne działanie systemu.
/// @note Klasa metaDataStream powinna być projektowana z myślą o łatwej integracji z innymi komponentami systemu, takimi jak storageAccessor, aby zapewnić spójność i efektywność całego systemu.
/// @note W przypadku przerw w transmisji danych, klasa metaDataStream powinna być w stanie wykryć i odpowiednio zareagować na takie sytuacje, np. poprzez zapisywanie stanu indeksu przed przerwą i przywracanie go po wznowieniu transmisji.

class metaDataStream {
 public:
  /// @brief Single entry in the meta index – a null bit-set pattern and count
  ///        of consecutive records sharing that pattern.
  struct IndexRecord {
    std::vector<bool> nullBitset;              ///< one bit per descriptor field (true = null)
    size_t recordCount{0};                     ///< number of consecutive records with this pattern
    int64_t timestamp{0};                      ///< epoch time (ms) of the first record in this run
    bool isGap{false};                         ///< true = this entry marks a transmission gap
    std::vector<std::byte> serialize() const;  ///< serialize the entry to a vector of bytes
    static IndexRecord deserialize(std::span<const std::byte> data);  ///< deserialize an entry from a vector of bytes
  };

 private:
  void createNullBitsetTemplate();
  void loadIndex();               ///< deserialize all committed entries from file into index_
  void saveIndex();               ///< serialize all entries (index_ + currentEntry_) to file
  void flushCurrentEntry();       ///< commit currentEntry_ to index_ if recordCount > 0

  /// @brief Locate the RLE segment and offset within it for a given global record index.
  /// @return pair(segment index in index_, offset within that segment)
  std::pair<size_t, size_t> locateRecord(size_t recordIndex) const;

  std::string metaFilePath_{};    ///< file path for saving/loading the meta index
  std::fstream indexFile_;        ///< file stream for reading/writing the meta index
  std::vector<IndexRecord> index_;   ///< all committed RLE segments (in memory)
  std::shared_ptr<Descriptor> descriptorRef_;  ///< descriptor of the indexed data stream

 public:
  IndexRecord currentEntry_;  ///< accumulator for the current (not yet committed) RLE run

  // ── Construction / destruction ──────────────────────────────────────

  /// @brief Construct meta index for the given descriptor.
  ///
  /// If the meta file already exists, its contents are deserialized
  /// into the in-memory index (loadIndex).
  /// @param descriptor descriptor of the indexed data stream
  /// @param metaFilePath path of the file to save/load the meta index
  explicit metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath);

  /// @brief Destructor – flushes the current entry and saves the full
  ///        index to file.
  virtual ~metaDataStream();

  // ── Core update interface ──────────────────────────────────────────

  /// @brief Notify the meta index that the record at @p recordIndex has
  ///        been modified.
  ///
  /// If the new null bit-set differs from the original, the RLE segment
  /// containing the record is split accordingly.
  /// @param recordIndex  position of the modified record in the stream
  /// @param nullBitset   new null bit-set pattern of the modified record
  void onRecordModified(size_t recordIndex, std::vector<bool> &nullBitset);

  /// @brief Notify the meta index that a record has been appended.
  ///
  /// If the null bit-set of the new record matches the pattern of the
  /// last entry the counter is incremented; otherwise the current entry
  /// is committed and a new entry is started with count = 1.
  /// @param nullBitset bit pattern indicating which fields are null
  void onRecordAppended(std::vector<bool> &nullBitset);

  // ── Query interface ────────────────────────────────────────────────

  /// @brief Retrieve the null bit-set for the record at @p recordIndex.
  /// @param recordIndex global position of the record in the stream
  /// @return null bit-set (one bit per descriptor field, true = null)
  /// @throws std::out_of_range if recordIndex >= totalRecords()
  std::vector<bool> getNullBitset(size_t recordIndex) const;

  /// @brief Check whether a single field is null in a given record.
  /// @param recordIndex global position of the record
  /// @param fieldIndex  position of the field in the descriptor
  /// @return true if the field is null
  bool isFieldNull(size_t recordIndex, size_t fieldIndex) const;

  /// @brief Total number of records tracked by the meta index
  ///        (sum of recordCount across all committed entries + currentEntry).
  size_t totalRecords() const;

  /// @brief Read-only access to all committed RLE entries.
  const std::vector<IndexRecord> &entries() const;

  // ── Transmission-gap interface ─────────────────────────────────────

  /// @brief Mark a transmission gap at the current position.
  ///
  /// Commits the current entry and inserts a zero-length gap marker
  /// so that downstream consumers can detect a discontinuity.
  void onTransmissionGap();

  /// @brief Check whether a gap marker exists right before @p recordIndex.
  bool isGapBefore(size_t recordIndex) const;

  // ── Timestamp interface ────────────────────────────────────────────

  /// @brief Return the registration timestamp (epoch ms) for the
  ///        record at @p recordIndex.
  int64_t getRecordTimestamp(size_t recordIndex) const;

  // ── Accessor ───────────────────────────────────────────────────────

  /// @brief Read-only access to the descriptor.
  const Descriptor &descriptor() const;
};  // class metaDataStream

}  // namespace rdb
