#pragma once

#include <chrono>
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
enum class storageState { noDescriptor, attachedDescriptor, openAndCreate };
enum class sourceState { empty, flux, armed };

/// @brief Klasa nadzorująca zapis danych na dysku.
///
/// obiekt klasy storage powinien:
/// - zarządzać zapisem danych na dysku, zapewniając trwałość i integralność danych.
/// - umożliwiać odczyt danych z dysku, zapewniając szybki dostęp do zapisanych danych.
/// - obsługiwać różne formaty danych, zgodnie z opisem w Descriptorze.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - obsługiwać różne strategie przechowywania danych, takie jak przechowywanie w pamięci lub na dysku w zależności od potrzeb i konfiguracji.
/// - zapewniać mechanizmy buforowania danych, aby zoptymalizować operacje zapisu i odczytu, zwłaszcza w przypadku dużych ilości danych.
/// - umożliwiać konfigurację parametrów przechowywania danych, takich jak rozmiar bufora, częstotliwość zapisu itp., aby dostosować działanie do różnych scenariuszy użytkowania.
/// - zarządzać plikiem indeksu metaDataStream, który przechowuje informacje o nullach dla każdego rekordu, zapewniając trwałość tych informacji i umożliwiając ich odczyt po ponownym uruchomieniu programu.
/// - zapewniać że każda dana zapisana do storage jest odpowiednio zarejestrowana w metaDataStream, aby utrzymać spójność między danymi a ich indeksami.
/// - rozpoznać jako przerwę (gap) sytuację gdy dane nie są dostarczne przez konfigurowany czas i oznaczyć to w metaDataStream, aby umożliwić późniejsze wykrycie i obsługę takich sytuacji.
/// - zapisać informacje o przerwach w transmisji danych (gaps) w metaDataStream, aby umożliwić późniejsze wykrycie i obsługę takich sytuacji.
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

  boost::rational<int> rInterval_{1};                          ///< sampling interval for time calculations
  int gapMultiplier_{3};                                       ///< gap threshold = gapMultiplier_ × rInterval_ without data
  std::chrono::steady_clock::time_point lastWriteTime_{};      ///< timestamp of last write()
  bool lastWriteTimeInitialized_{false};                       ///< true after first write

  void checkAndMarkAutoGap();  ///< auto-detect gap based on elapsed time since last write
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

  storageState dataFileStatus = storageState::noDescriptor;
  sourceState bufferState     = sourceState::empty;  // ? test lock

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
  void markTransmissionGap();

  /// @brief Check if there is a transmission gap before the given record.
  /// @param recordIndex global position of the record
  /// @return true if a gap marker exists immediately before this record
  bool hasGapBefore(size_t recordIndex) const;

  /// @brief Get all transmission gaps recorded in the stream.
  /// @return vector of TransmissionGap structures with their positions and timestamps
  std::vector<rdb::metaDataStream::TransmissionGap> getTransmissionGaps() const;

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

  /// @brief Configure sampling interval and gap detection threshold.
  ///
  /// When set, write() will automatically detect transmission gaps by comparing
  /// elapsed time since the last write against gapMultiplier × rInterval.
  /// The rInterval is also forwarded to metaDataStream for timestamp calculations.
  /// @param rInterval sampling interval (e.g. 1/10 = 100ms)
  /// @param gapMultiplier number of missed intervals before a gap is marked (default: 3)
  void setSamplingInterval(boost::rational<int> rInterval, int gapMultiplier = 3);

  // technical function - for unit tests
  void reset();
};
}  // namespace rdb
