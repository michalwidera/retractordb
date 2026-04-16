#pragma once

#include <memory>  // std::unique_ptr
#include <string>

#include <boost/circular_buffer.hpp>
#include <boost/rational.hpp>

#include "descriptor.hpp"
#include "faccbindev.hpp"
#include "faccfs.hpp"
#include "faccmemory.hpp"
#include "faccposix.hpp"
#include "faccposixshd.hpp"
#include "facctxtsrc.hpp"
#include "fagrp.hpp"
#include "metaDataStream.hpp"
#include "payload.hpp"

namespace rdb {
enum class sourceState { empty, flux, armed };

/// @brief Warstwa koordynująca Descriptor, payload i FileInterface dla odczytu oraz zapisu rekordów.
///
/// Obiekt klasy storage powinien:
/// - utrzymywać Descriptor oraz powiązane obiekty payload używane do odczytu, zapisu i buforowania danych,
/// - tworzyć odpowiednią implementację FileInterface na podstawie konfiguracji i typu magazynu zakodowanego w Descriptor,
/// - zarządzać plikiem deskryptora oraz inicjalizacją właściwego magazynu danych,
/// - udostępniać odczyt i zapis rekordów przez logiczne indeksy rekordów, mapowane wewnętrznie na pozycje właściwe dla accessor_,
/// - dla magazynów zapisywalnych utrzymywać spójność między zapisanymi rekordami a metadanymi null przechowywanymi w metaDataStream,
/// - dla źródeł deklarowanych tylko do odczytu obsługiwać odczyt bieżącego rekordu oraz opcjonalny bufor historii w circularBuffer_,
/// - wspierać czyszczenie magazynu i odpowiadającego mu indeksu metadanych przez purge() lub reset(),
/// - opcjonalnie wykrywać przerwy w transmisji danych na podstawie kolejnych rekordów oznaczonych jako all-null,
/// - przed oznaczeniem gap zapisywać początkowe rekordy null jako nullfill, a dopiero dalsze braki danych agregować do znacznika gap w metaDataStream,
/// - przechowywać długość gap jako liczbę interwałów próbkowania i udostępniać informację, czy gap występuje przed danym rekordem,
/// - umożliwiać konfigurację interwału próbkowania oraz liczby rekordów nullfill poprzedzających oznaczenie gap.
///
/// @note Klasa sama nie implementuje fizycznego formatu magazynu; deleguje operacje I/O do accessor_.
/// @note Semantyka nullfill i gap dotyczy tylko ścieżki zapisu z włączonym mechanizmem gap detection oraz obecnym metaDataStream.

class storage {
  std::unique_ptr<FileInterface> accessor_;
  std::unique_ptr<rdb::payload> storagePayload_;
  std::unique_ptr<rdb::payload> chamber_;

  bool isDisposable_          = false;  // if true - storage and descriptor will be deleted after use
  bool isOneShot_             = false;  // if false - storage will be looped when end is reached
  bool isHold_                = false;  // if true - no processing until first query appear
  size_t recordsCount_        = 0;
  std::string descriptorFile_ = "";
  std::string storageFile_    = "";
  std::string metaIndexFile_  = "";  // file path for saving/loading the meta index
  std::string storageType_    = "";
  int percounter_             = -1;

  boost::rational<int> rInterval_{1};   ///< sampling interval for time calculations
  int nullFillCount_{2};                ///< number of nullfill records written before a gap is marked (R17)
  size_t consecutiveNullCount_{0};      ///< consecutive all-null records received from source
  size_t activeGapDuration_{0};         ///< accumulated gap duration not yet flushed to meta index
  bool gapDetectionConfigured_{false};  ///< true only when configureGapDetection() was explicitly called

  void flushPendingGap();  ///< flush accumulated gap duration to meta index
  void moveRef();
  void attachStorage();

  boost::circular_buffer<rdb::payload> circularBuffer_{0};

  void abortIfStorageNotPrepared();
  void initializeAccessor();

  std::unique_ptr<rdb::metaDataStream> metaDataStream_;

 public:
  storage() = delete;
  explicit storage(const std::string_view qryID,                    //
                   const std::string_view fileName,                 //
                   const std::string_view storageParam,             //
                   const std::string_view storageType = "DEFAULT",  //
                   bool oneShot                       = false,      //
                   bool isHold                        = false,      //
                   int percounter                     = -1          //
  );
  virtual ~storage();

  Descriptor descriptor;

  sourceState bufferState{sourceState::empty};  // ? test lock

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());
  bool revRead(const size_t recordIndexFromBack, uint8_t *destination = nullptr);
  bool read(const size_t recordIndexFromFront, uint8_t *destination = nullptr);
  void fire();
  void purge();

  /// @brief Mark a transmission gap at the current position.
  ///
  /// Indicates a discontinuity in the data stream. This is recorded in the meta index
  /// for later detection and handling. The gap is inserted as a marker entry.
  /// @note The gap is marked in the meta index; actual data storage is unaffected.
  void markTransmissionGap(size_t gapDuration = 1);

  /// @brief Check if there is a transmission gap before the given record.
  /// @param recordIndex global position of the record
  /// @return true if a gap marker exists immediately before this record
  bool hasGapBefore(size_t recordIndex) const;

  /// @brief Check if the meta index is empty (no records tracked).
  /// @return true if neither storage nor meta index contain any records
  bool isMetaIndexEmpty() const;

  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setDisposable(bool value);
  void releaseOnHold();
  size_t getRecordsCount();
  bool descriptorFileExist();

  bool isDeclared();

  void setCapacity(const int capacity);
  void cleanPayload(uint8_t *destination = nullptr);

  /// @brief Configure sampling interval and nullfill length for gap detection.
  ///
  /// When set, write() will automatically detect transmission gaps based on
  /// consecutive all-null records from the source. Before a gap marker is registered,
  /// nullFillCount null/fallback records are written to storage and the meta index,
  /// satisfying R17. The rInterval is forwarded to metaDataStream for timestamp calculations.
  /// @param rInterval     sampling interval (e.g. 1/10 = 100ms)
  /// @param nullFillCount number of nullfill records to write before marking a gap (default: 2)
  void configureGapDetection(boost::rational<int> rInterval, int nullFillCount = 2);

  // technical function - for unit tests
  void reset();
};
}  // namespace rdb
