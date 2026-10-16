#pragma once

#include <cstdint>
#include <memory>  // std::unique_ptr
#include <string>
#include <utility>
#include <vector>

#include <boost/rational.hpp>

#include "descriptor.hpp"
#include "error.hpp"
#include "fainterface.hpp"
#include "memoryStore.hpp"
#include "metaData.hpp"
#include "payload.hpp"
#include "sourceBuffer.hpp"
#include "storagePaths.hpp"

namespace rdb {

class binaryDeviceRO;

/// @brief Wynik odczytu rekordu.
///
/// Prawdziwa awaria odczytu (blad wejscia-wyjscia, zrodlo deklarowane czytane wprost) wraca jako
/// blad Result; ReadStatus opisuje wylacznie odczyty UDANE. Historia: do 2026-09-23 read() i
/// revRead() zwracaly `bool` o JEDNEJ mozliwej wartosci: kazda prawdziwa
/// awaria konczyla sie FatalError i nie wracala, a jedyna awaria odwracalna - brak rekordu - wracala
/// jako `true` z pamiecia wyzerowana i jawnie oznaczona jako NIE-null. Trzy miejsca obslugi bledu
/// u wolajacych byly przez to martwe, a najgorzej konczylo sie to w reduktorze: zamiast pominac
/// brakujacy rekord, skladal do MIN/MAX/SUM/AVG sfalszowane zero.
///
/// Rekord nieistniejacy jest wartoscia NIEOKRESLONA, nie zerem - dlatego towarzyszy mu payload
/// all-null, tak samo jak w dataModel::fetchForward, ktory te konwencje stosuje od dawna. Dzieki
/// temu wlacza sie pochlanianie NULL-i i wolajacy, ktory statusu nie sprawdzi, dostaje wartosc
/// nieokreslona zamiast zera udajacego dane.
enum class ReadStatus : std::uint8_t {
  Ok,           ///< rekord odczytany ze zrodla, bufora albo wydany w stanie HOLD
  NoSuchRecord  ///< rekordu nie ma; payload jest all-null, a wolajacy ma go POMINAC
};

/// @brief Wynik zapisu rekordu.
///
/// Do 2026-10-03 write() zwracalo `bool` o JEDNEJ mozliwej wartosci: od 7054eb5f kazda awaria
/// akcesora konczy sie FatalError i nie wraca, wiec `result == 0` bylo zawsze prawda. xtrdb
/// odwracal ten bool i po kazdym udanym `write n` meldowal `error` (E-02, #269).
///
/// Jedyna awaria odwracalna to zapis do zrodla deklarowanego, ktore jest tylko do odczytu: akcesor
/// odmawia ENOTSUP, zanim cokolwiek zapisze, wiec licznik rekordow i indeks metadanych zostaja
/// nietkniete. Inna awaria wraca jako blad Result; silnik zatrzymuje przetwarzanie, aby czesciowy zapis
/// nie rozjechal recordsCount_ z count().
enum class WriteStatus : std::uint8_t {
  Ok,       ///< rekord dopisany, nadpisany albo pochloniety przez detekcje gap (rozliczony jako przerwa)
  ReadOnly  ///< magazyn jest zrodlem deklarowanym; nic nie zapisano
};

/// @brief Warstwa koordynująca Descriptor, payload i FileInterface dla odczytu oraz zapisu rekordów.
///
/// Obiekt klasy storage powinien:
/// - utrzymywać Descriptor oraz powiązane obiekty payload używane do odczytu, zapisu i buforowania danych,
///   Test: `xrdb.test_storage`.
/// - delegować wyliczanie i spójność ścieżek plików magazynu (deskryptor .desc, plik danych, indeks .meta)
///   do obiektu StoragePaths (storagePaths.hpp), łącznie z relokacją wg pola REF deskryptora
///   oraz kasowaniem kompletu plików magazynów dysponowalnych,
///   Test: `StoragePathsTest.storage_dir_prefixes_paths`, `StoragePathsTest.relocate_from_ref_moves_data_and_meta`, `StoragePathsTest.remove_all_files_deletes_whole_set`. Test: `xrdb.test_storage_disposal_removes_all_files`, `xrdb.storage_ref_outside_store_reads_but_is_not_removed`.
/// - tworzyć odpowiednią implementację FileInterface na podstawie konfiguracji i typu magazynu zakodowanego w Descriptor,
///   delegując samo odwzorowanie typu magazynu na konkretną klasę akcesora oraz dobór wariantu indeksu metadanych
///   do fabryki (accessorFactory.hpp) - storage zna wyłącznie abstrakcyjny FileInterface,
///   Test: `AccessorFactoryTest.maps_type_to_accessor_class`, `AccessorFactoryTest.maps_declared_sources`, `AccessorFactoryTest.meta_index_variant_selection`. Kontrola strukturalna: initializeAccessor i makeMetaIndex w storage.cc delegują wybór do fabryki.
/// - zarządzać plikiem deskryptora oraz inicjalizacją właściwego magazynu danych; odczyt, zapis
///   i weryfikację zgodności pliku .desc delegować do funkcji descriptorIO (descriptorIO.hpp),
///   Test: `DescriptorIOTest.save_and_load_round_trip`, `DescriptorIOTest.storage_returns_a_bad_descriptor_error`, `DescriptorIOTest.verify_match_accepts_equal_descriptors`, `DescriptorIOTest.verify_match_rejects_different_descriptors`.
/// - udostępniać odczyt i zapis rekordów przez logiczne indeksy rekordów, mapowane wewnętrznie na pozycje właściwe dla accessor_,
///   Test: `xrdb.test_storage`, `xrdb.storage_read_beyond_last_record_reports_no_such_record`.
/// - delegować całość metadanych null i gap do wstrzykniętego obiektu metaData, którego wariant jest wybierany
///   przy attachStorage(): storageShadow gdy accessor utrzymuje plik cienia danych (FileInterface::hasShadow()),
///   bazowy metaData dla pozostałych zapisywalnych, wariant inertny (pusta ścieżka pliku) dla źródeł
///   deklarowanych - dzięki temu write()/read() nie zawierają rozgałęzień zależnych od wariantu magazynu,
///   Test: `AccessorFactoryTest.meta_index_variant_selection`. Test: `StorageShadowFixture.polymorphic_updates_preserve_main_bytes_and_merge_in_arrival_order`. Test: `xrdb.declared_history_replays_values_and_nulls_without_another_source_read`.
/// - traktować posiadanie pliku cienia danych i posiadanie metaindeksu jako niezależne własności magazynu:
///   plik cienia zachowuje oryginalną, zarejestrowaną zawartość danych (aktualizacje trafiają do cienia aż do
///   merge), a metaindeks pozwala uwzględnić wartości null oraz przerwy w transmisji,
///   Test: `xrdb.storage_rotates_data_and_null_indexes_on_shutdown`. Test: `StorageShadowTest.hasShadow_raportowany_przez_accessor`.
/// - dla źródeł deklarowanych tylko do odczytu obsługiwać odczyt bieżącego rekordu oraz opcjonalny bufor historii,
///   delegując komorę bieżącego rekordu, fizyczny odczyt ze źródła i bufor historii do obiektu SourceBuffer
///   (sourceBuffer.hpp); storage zachowuje decyzje o wartościach zastępczych i stan bufferState,
///   Test: `xrdb.declared_history_replays_values_and_nulls_without_another_source_read`. Test: `SourceBufferTest.fire_pushes_chamber_into_history`.
/// - wspierać czyszczenie magazynu i odpowiadającego mu indeksu metadanych przez purge() lub reset(),
///   Test: `xrdb.storage_purge_resets_metadata_stream_state`.
/// - umożliwiać konfigurację interwału próbkowania oraz liczby rekordów nullfill poprzedzających oznaczenie gap;
///   samą maszynę detekcji gap (nullfill/absorpcja/flush) realizuje metaData,
///   Test: `xrdb.storage_setSamplingInterval_propagates_to_meta`, `xrdb.storage_auto_gap_detection_marks_gap_after_null_records`.
/// - przy starcie wyznaczać przerwę w transmisji danych (wynikającą z niedziałania systemu) jako liczbę interwałów próbkowania, które upłynęły od ostatniego zapisu pliku danych do momentu startu, i przekazywać ją do obiektu indeksu metadanych w celu zarejestrowania jako wpis gap,
///   Test: `xrdb.startup_gap_uses_the_data_file_age_and_survives_reopen`.
/// - przy zamknieciu rotowac indeks metadanych pod tym samym numerem .old<percounter> co dane,
///   a przy starcie pustego magazynu usuwac osierocony indeks pozostawiony przez starsza wersje.
///   Test: `xrdb.storage_rotates_data_and_null_indexes_on_shutdown`, `xrdb.empty_data_file_discards_orphaned_metadata_before_the_first_write`.
///
/// @note Klasa sama nie implementuje fizycznego formatu magazynu; deleguje operacje I/O do accessor_.
/// @note Semantyka nullfill i gap dotyczy tylko ścieżki zapisu z włączonym mechanizmem gap detection (configureGapDetection()).

class storage {
  std::unique_ptr<FileInterface> accessor_;
  std::unique_ptr<rdb::payload> storagePayload_;
  SourceBuffer buffer_;  ///< komora bieżącego rekordu i historia dla źródeł deklarowanych

  bool isDisposable_   = false;  // if true - storage and descriptor will be deleted after use
  bool refFromCaller_  = false;  // REF pochodzi od wolajacego (plan, schemat), nie wylacznie z wczytanego .desc
  bool isOneShot_      = false;  // if false - storage will be looped when end is reached
  bool isHold_         = false;  // if true - no processing until first query appear
  size_t recordsCount_ = 0;
  StoragePaths paths_;
  std::string storageType_;
  int percounter_ = -1;

  boost::rational<int> rInterval_{1};  ///< sampling interval for time calculations

  void detectStartupState();  ///< detect rotation or startup gap after meta index is ready
  [[nodiscard]] Result<> attachStorage();

  /// Cztery niezmienniki, ktorych zlamanie znaczy uzycie magazynu przed attachDescriptor().
  /// Szybka sciezka inline: wolajacy (read, revRead, write - kazdy rekord) placa jedna galaz, a
  /// Result<> sukcesu znika po wklejeniu. Tresc bledu buduje zimne notPrepared().
  [[nodiscard]] Result<> requirePrepared() const {
    if (!descriptor.empty() && accessor_ && storagePayload_ && metaData_) [[likely]]
      return {};
    return notPrepared();
  }
  [[nodiscard, gnu::cold]] Result<> notPrepared() const;
  [[nodiscard]] Result<> initializeAccessor();

  /// @brief Czy magazyn trzyma dane w pamięci, a nie na dysku (polityka TYPE deskryptora).
  ///
  /// Rozróżnienie potrzebne sondzie materializacji (K6): plan z `SUBSTRAT 'memory'`
  /// nie może raportować objętości, której nigdy nie zapisał na dysk.
  [[nodiscard]] bool isMemoryBackedStorage() const;

  /// Rola strumienia dla sondy logicznych zapisów (K23): substrat to materializowany
  /// podplan, strumień publiczny wchodzi do mianownika metryki. `storage` sam tego nie
  /// wie - `isSubstrat` należy do planu, więc rolę ustawia właściciel przy konstrukcji.
  bool isSubstrate_ = false;

  /// Kanoniczna szerokość rekordu, liczona raz i pamiętana: iteracja po polach przy każdym
  /// zapisie obciążałaby budżet slotu, który mierzy sonda E1. Zero = jeszcze nie liczona.
  ///
  /// Pamiętana tu, a nie w funkcji pomocniczej: metoda poza `if constexpr` miałaby ciało
  /// kompilowane w KAŻDYM buildzie i zostawiała w `storage.cc.o` odwołanie do
  /// `probe::canonicalRecordBytes` także przy wyłączonej sondzie. Sprawdzone `nm`.
  std::size_t canonicalRecordBytes_ = 0;

  std::unique_ptr<rdb::metaData> metaData_;

  /// Memory Store magazynu MEMORY, jesli wolajacy podal wlasny; nullptr znaczy instancje domyslna
  /// procesu. Przekazywany do makeAccessor przy budowie akcesora - patrz memoryStore.hpp.
  /// Dla pozostalych typow magazynu bez znaczenia.
  MemoryStore *memory_ = nullptr;

  storage(StoragePaths paths, std::string_view storageType, bool oneShot, bool isHold, int percounter, MemoryStore *memory);

 public:
  storage() = delete;

  /// Jedyna droga do magazynu. Zla konfiguracja (pusty identyfikator, katalog :STORAGE, ktorego
  /// nie ma) wraca jako Errc::Config, zanim obiekt powstanie - wiec destruktor magazynu
  /// disposable nie ma czego kasowac. Dawny konstruktor rzucal ConfigError z listy inicjalizacyjnej.
  [[nodiscard]] static Result<std::unique_ptr<storage>> create(std::string_view qryID,                    //
                                                               std::string_view fileName,                 //
                                                               std::string_view storageParam,             //
                                                               std::string_view storageType = "DEFAULT",  //
                                                               bool oneShot                 = false,      //
                                                               bool isHold                  = false,      //
                                                               int percounter               = -1,         //
                                                               MemoryStore *memory          = nullptr     //
  );
  storage(const storage &)            = delete;
  storage &operator=(const storage &) = delete;
  virtual ~storage();

  Descriptor descriptor;

  sourceState bufferState{sourceState::empty};  // ? test lock

  /// @return sukces albo powod odmowy: odczyt deskryptora (CorruptDescriptor tresc jak w
  ///         tryLoadDescriptorFile), niezgodnosc schematu albo brak deskryptora (Config), zapis .desc
  ///         lub otwarcie magazynu (IO).
  [[nodiscard]] Result<> attachDescriptor(const Descriptor *descriptor = nullptr);

  /// @brief Oznacz magazyn jako materializowany podplan (substrat) dla sondy K23.
  ///
  /// Bez tego sonda logicznych zapisów nie odróżni zapisu pośredniego od publicznego
  /// wyniku, a metryka pierwotna H9 wymaga obu wielkości rozdzielnie. Setter jest
  /// w nagłówku, żeby w buildzie bez sondy nie powstał osobny symbol w bibliotece.
  void markAsSubstrate(const bool value) { isSubstrate_ = value; }

  [[nodiscard]] Result<WriteStatus> write(size_t recordIndex = std::numeric_limits<size_t>::max());

  [[nodiscard]] Result<ReadStatus> revRead(size_t recordIndexFromBack, uint8_t *destination = nullptr);
  /// Errc::Config dla zrodla deklarowanego (czyta sie je przez revRead), Errc::IO dla nieudanego odczytu.
  [[nodiscard]] Result<ReadStatus> read(size_t recordIndexFromFront, uint8_t *destination = nullptr);
  void fire();
  [[nodiscard]] Result<> purge();

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

  /// Payload magazynu. Istnieje po kazdym udanym attachDescriptor(); wolanie wczesniej to blad w
  /// kodzie wolajacego (RDB_ASSERT), nie stan, ktory trzeba obslugiwac.
  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setDisposable(bool value);

  /// @brief Katalogi spoza katalogu magazynu, do których REF z wczytanego `.desc` może przenieść
  ///        zapisywalny plik danych (`storage.ref_dirs`, #278). Wołać przed attachDescriptor().
  void allowRefDirs(std::vector<std::string> dirs) { paths_.allowRefDirs(std::move(dirs)); }

  void releaseOnHold();
  [[nodiscard]] size_t getRecordsCount() const;
  [[nodiscard]] bool descriptorFileExist() const;

  /// @brief Rekord historii źródła deklarowanego (0 = najnowszy) bez mutacji bieżącego payloadu.
  [[nodiscard]] const payload &history(size_t recordIndexFromBack) const { return buffer_.history(recordIndexFromBack); }

  /// @brief Liczba rekordów dostępnych w historii bufora źródła deklarowanego.
  [[nodiscard]] size_t historySize() const { return buffer_.size(); }
  [[nodiscard]] size_t historyCapacity() const { return buffer_.capacity(); }

  [[nodiscard]] bool isDeclared() const;

  /// @brief Czy źródło deklarowane wyczerpało wejście (patrz FileInterface::exhausted()).
  ///
  /// Pytanie ma sens wyłącznie dla źródeł czytanych bez zawijania; magazyny zapisywalne
  /// i źródła zawijane zawsze odpowiadają false.
  [[nodiscard]] bool sourceExhausted() const { return accessor_ != nullptr && accessor_->exhausted(); }

  /// @brief Akcesor źródła DEVICE dla fazy DEVICE (rdb::awaitRecords, #347); nullptr dla każdego innego magazynu.
  [[nodiscard]] binaryDeviceRO *deviceSource() const;

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
  [[nodiscard]] Result<> resetForUnitTest();
};
}  // namespace rdb
