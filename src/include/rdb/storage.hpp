#pragma once

#include <memory>  // std::unique_ptr
#include <string>

#include <boost/rational.hpp>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "metaData.hpp"
#include "payload.hpp"
#include "sourceBuffer.hpp"
#include "storagePaths.hpp"

namespace rdb {

/// @brief Warstwa koordynująca Descriptor, payload i FileInterface dla odczytu oraz zapisu rekordów.
///
/// Obiekt klasy storage powinien:
/// - utrzymywać Descriptor oraz powiązane obiekty payload używane do odczytu, zapisu i buforowania danych,
/// - delegować wyliczanie i spójność ścieżek plików magazynu (deskryptor .desc, plik danych, indeks .meta)
///   do obiektu StoragePaths (storagePaths.hpp), łącznie z relokacją wg pola REF deskryptora
///   oraz kasowaniem kompletu plików magazynów dysponowalnych,
/// - tworzyć odpowiednią implementację FileInterface na podstawie konfiguracji i typu magazynu zakodowanego w Descriptor,
///   delegując samo odwzorowanie typu magazynu na konkretną klasę akcesora oraz dobór wariantu indeksu metadanych
///   do fabryki (accessorFactory.hpp) — storage zna wyłącznie abstrakcyjny FileInterface,
/// - zarządzać plikiem deskryptora oraz inicjalizacją właściwego magazynu danych; odczyt, zapis
///   i weryfikację zgodności pliku .desc delegować do funkcji descriptorIO (descriptorIO.hpp),
/// - udostępniać odczyt i zapis rekordów przez logiczne indeksy rekordów, mapowane wewnętrznie na pozycje właściwe dla accessor_,
/// - delegować całość metadanych null i gap do wstrzykniętego obiektu metaData, którego wariant jest wybierany
///   przy attachStorage(): storageShadow gdy accessor utrzymuje plik cienia danych (FileInterface::hasShadow()),
///   bazowy metaData dla pozostałych zapisywalnych, wariant inertny (pusta ścieżka pliku) dla źródeł
///   deklarowanych — dzięki temu write()/read() nie zawierają rozgałęzień zależnych od wariantu magazynu,
/// - traktować posiadanie pliku cienia danych i posiadanie metaindeksu jako niezależne własności magazynu:
///   plik cienia zachowuje oryginalną, zarejestrowaną zawartość danych (aktualizacje trafiają do cienia aż do
///   merge), a metaindeks pozwala uwzględnić wartości null oraz przerwy w transmisji,
/// - dla źródeł deklarowanych tylko do odczytu obsługiwać odczyt bieżącego rekordu oraz opcjonalny bufor historii,
///   delegując komorę bieżącego rekordu, fizyczny odczyt ze źródła i bufor historii do obiektu SourceBuffer
///   (sourceBuffer.hpp); storage zachowuje decyzje o wartościach zastępczych i stan bufferState,
/// - wspierać czyszczenie magazynu i odpowiadającego mu indeksu metadanych przez purge() lub reset(),
/// - umożliwiać konfigurację interwału próbkowania oraz liczby rekordów nullfill poprzedzających oznaczenie gap;
///   samą maszynę detekcji gap (nullfill/absorpcja/flush) realizuje metaData,
/// - przy starcie wyznaczać przerwę w transmisji danych (wynikającą z niedziałania systemu) jako liczbę interwałów próbkowania, które upłynęły od ostatniego zapisu pliku danych do momentu startu, i przekazywać ją do obiektu indeksu metadanych w celu zarejestrowania jako wpis gap,
/// - przy starcie wykrywać zaszłą rotację pliku danych (świeży/pusty magazyn przy niepustym indeksie metadanych) i informować obiekt indeksu, aby ten zrotował indeks tak jak storage (.old<percounter> dla percounter >= 0) i przygotował nowy ze stanem początkowym.
///
/// @note Klasa sama nie implementuje fizycznego formatu magazynu; deleguje operacje I/O do accessor_.
/// @note Semantyka nullfill i gap dotyczy tylko ścieżki zapisu z włączonym mechanizmem gap detection (configureGapDetection()).

class storage {
  std::unique_ptr<FileInterface> accessor_;
  std::unique_ptr<rdb::payload> storagePayload_;
  SourceBuffer buffer_;  ///< komora bieżącego rekordu i historia dla źródeł deklarowanych

  bool isDisposable_   = false;  // if true - storage and descriptor will be deleted after use
  bool isOneShot_      = false;  // if false - storage will be looped when end is reached
  bool isHold_         = false;  // if true - no processing until first query appear
  size_t recordsCount_ = 0;
  StoragePaths paths_;
  std::string storageType_;
  int percounter_ = -1;

  boost::rational<int> rInterval_{1};  ///< sampling interval for time calculations

  void detectStartupState();  ///< detect rotation or startup gap after meta index is ready
  void attachStorage();

  void abortIfStorageNotPrepared();
  void initializeAccessor();

  std::unique_ptr<rdb::metaData> metaData_;

 public:
  storage() = delete;
  explicit storage(std::string_view qryID,                    //
                   std::string_view fileName,                 //
                   std::string_view storageParam,             //
                   std::string_view storageType = "DEFAULT",  //
                   bool oneShot                 = false,      //
                   bool isHold                  = false,      //
                   int percounter               = -1          //
  );
  virtual ~storage();

  Descriptor descriptor;

  sourceState bufferState{sourceState::empty};  // ? test lock

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  bool write(size_t recordIndex = std::numeric_limits<size_t>::max());
  bool revRead(size_t recordIndexFromBack, uint8_t *destination = nullptr);
  bool read(size_t recordIndexFromFront, uint8_t *destination = nullptr);
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
  [[nodiscard]] bool hasGapBefore(size_t recordIndex) const;

  /// @brief Check if the meta index is empty (no records tracked).
  /// @return true if neither storage nor meta index contain any records
  [[nodiscard]] bool isMetaIndexEmpty() const;

  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setDisposable(bool value);
  void releaseOnHold();
  [[nodiscard]] size_t getRecordsCount() const;
  bool descriptorFileExist();

  /// @brief Rekord historii źródła deklarowanego (0 = najnowszy) bez mutacji bieżącego payloadu.
  [[nodiscard]] const payload &history(size_t recordIndexFromBack) const { return buffer_.history(recordIndexFromBack); }

  /// @brief Liczba rekordów dostępnych w historii bufora źródła deklarowanego.
  [[nodiscard]] size_t historySize() const { return buffer_.size(); }

  [[nodiscard]] bool isDeclared() const;

  void setCapacity(int capacity);
  void cleanPayload(uint8_t *destination = nullptr);

  /// @brief Configure sampling interval and nullfill length for gap detection.
  ///
  /// When set, write() will automatically detect transmission gaps based on
  /// consecutive all-null records from the source. Before a gap marker is registered,
  /// nullFillCount null/fallback records are written to storage and the meta index,
  /// satisfying R17.
  /// @param rInterval     sampling interval (e.g. 1/10 = 100ms)
  /// @param nullFillCount number of nullfill records to write before marking a gap (default: 2)
  void configureGapDetection(boost::rational<int> rInterval, int nullFillCount = 2);

  /// @brief Return the current sampling interval configured in storage.
  [[nodiscard]] boost::rational<int> getSamplingInterval() const;

  // technical function - for unit tests
  void resetForUnitTest();
};
}  // namespace rdb
