#include "rdb/storage.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

#include <fmt/format.h>
#include <cstring>  //std::memset
#include <filesystem>
#include <ranges>

#include "rdb/accessorFactory.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/probe.hpp"  // sonda K6: objętość materializacji

namespace rdb {

bool storage::isMemoryBackedStorage() const {
  // To samo źródło prawdy, którym posługuje się makeAccessor() przy wyborze
  // implementacji FileInterface (accessorFactory.cc): storageType_ pochodzi
  // z pola TYPE deskryptora. Dzięki temu podział trwałe/pamięciowe nie może
  // rozjechać się z faktycznie użytym magazynem.
  return storageType_ == "MEMORY";
}

storage::storage(StoragePaths paths,                  //
                 const std::string_view storageType,  //
                 bool oneShot,                        //
                 bool isHold,                         //
                 int percounter,                      //
                 MemoryStore *memory)
    : isOneShot_(oneShot),
      isHold_(isHold),
      paths_(std::move(paths)),
      storageType_(storageType),
      percounter_(percounter),
      memory_(memory) {}

Result<std::unique_ptr<storage>> storage::create(const std::string_view qryID,         //
                                                 const std::string_view fileName,      //
                                                 const std::string_view storageParam,  //
                                                 const std::string_view storageType,   //
                                                 bool oneShot,                         //
                                                 bool isHold,                          //
                                                 int percounter,                       //
                                                 MemoryStore *memory) {
  // Konfiguracja sprawdzana PRZED zbudowaniem obiektu: magazyn, ktory nie powstal, nie ma
  // destruktora, wiec plikow disposable nikt nie kasuje na podstawie zlej sciezki.
  RDB_TRY_ASSIGN(StoragePaths paths, StoragePaths::make(qryID, fileName, storageParam));
  return std::unique_ptr<storage>(new storage(std::move(paths), storageType, oneShot, isHold, percounter, memory));
}

Result<> storage::attachDescriptor(const Descriptor *descriptorParam) {
  const bool descriptorExisted = descriptorFileExist();
  if (descriptorExisted) {
    if (std::string error = tryLoadDescriptorFile(paths_.descriptorFile(), descriptor); !error.empty())
      return fail(Errc::CorruptDescriptor, std::move(error));
    if (descriptorParam != nullptr) RDB_TRY(verifyDescriptorMatch(*descriptorParam, descriptor, paths_.descriptorFile()));
  } else {
    if (descriptorParam == nullptr) {
      // Blad wolajacego, nie silnika: nie ma pliku .desc i nie podano deskryptora, wiec
      // nie ma z czego zbudowac magazynu. Osadzajacy proces chce to zobaczyc i zapytac.
      return fail(Errc::Config, "storage: no descriptor file and no descriptor provided: " + paths_.descriptorFile());
    }
    // Deskryptor zerowej szerokosci odrzucany NA GRANICY: kazdy akcesor dzieli przez rozmiar
    // rekordu, wiec dalej bylby to zlamany niezmiennik, a tutaj jest zwyklym zlym wejsciem.
    // Plik .desc tej kontroli nie potrzebuje - tryLoadDescriptorFile odrzuca pusty deskryptor.
    if (descriptorParam->getSizeInBytes() == 0)
      return fail(Errc::Config, "storage: descriptor has zero record size: " + paths_.descriptorFile());
    descriptor = *descriptorParam;
    RDB_TRY(saveDescriptorFile(paths_.descriptorFile(), descriptor));
  }

  // Sprzatanie po odmowie: .desc zapisany przed chwila przez TEN magazyn nie moze zostac, bo
  // nastepna proba z innym planem przeczytalaby go jako istniejacy schemat.
  auto discardFreshDescriptor = [&](Error error) -> Result<> {
    if (!descriptorExisted) {
      std::error_code ec;
      std::filesystem::remove(paths_.descriptorFile(), ec);
    }
    return fail(std::move(error));
  };

  if (auto relocated = paths_.relocateFromRef(descriptor); !relocated)
    return discardFreshDescriptor(std::move(relocated).error());
  storagePayload_ = std::make_unique<rdb::payload>(descriptor);
  buffer_.attach(descriptor);

  if (auto attached = attachStorage(); !attached) return discardFreshDescriptor(std::move(attached).error());
  return {};
}

Result<> storage::attachStorage() {
  // Niezmiennik, nie kontrola wejscia: relocateFromRef() tuz wyzej odmawia juz przy pustej
  // sciezce (Errc::Config), a attachStorage() jest prywatne i wola je tylko attachDescriptor().
  RDB_ASSERT(!paths_.storageFile().empty(), "storage: storage file path is empty - attachDescriptor() not called");

  auto it1 = std::ranges::find_if(descriptor,  //
                                  [](const auto &item) { return item.rtype == rdb::TYPE; });

  if (it1 != descriptor.end()) {
    storageType_ = (*it1).rname;
  }

  RDB_TRY(initializeAccessor());
  if (!accessor_->initializationError().empty()) return fail(Errc::IO, accessor_->initializationError());

  // Wstrzyknięcie wariantu metadanych - dobór wariantu (inertny/cień indeksu/bazowy) realizuje fabryka.
  metaData_ = makeMetaIndex(isDeclared(), accessor_->hasShadow(), descriptor, paths_.metaIndexFile());

  if (isDeclared()) return {};

  RDB_TRY_ASSIGN(recordsCount_, accessor_->count());
  detectStartupState();
  return {};
}

storage::~storage() {
  // Jedyny strażnik null: storage mógł zostać zniszczony przed attachDescriptor().
  // Pending gap musi trafić na dysk PRZED ewentualnym kasowaniem plików disposable.
  if (metaData_) metaData_->flushPendingGap();
  if (isDisposable_) {
    // Odłączenie od pliku PRZED usunięciem: bez tego destruktor metaData_ (wywołany automatycznie
    // po zakończeniu tego ciała) odtworzyłby właśnie skasowany plik .meta przez flushCurrentEntry().
    if (metaData_) metaData_->abandonFile();
    paths_.removeAllFiles();
  }
}

bool storage::isDeclared() const { return isDeclaredType(storageType_); }

Result<> storage::initializeAccessor() {
  RDB_TRY_ASSIGN(accessor_, makeAccessor(storageType_, paths_.storageFile(), descriptor, isOneShot_, percounter_, memory_));
  return {};
}

Result<> storage::resetForUnitTest() {
  if (paths_.storageFile().empty())
    return fail(Errc::Logic, "storage: storage file path is empty - storage not properly configured");

  if (!accessor_) return {};  // no accessor initialized - no need to reset.

  std::error_code ec;
  auto resourceAlreadyExist = std::filesystem::exists(paths_.storageFile(), ec);
  if (resourceAlreadyExist)
    if (!isDeclared()) remove(paths_.storageFile().c_str());

  RDB_TRY(initializeAccessor());
  if (!accessor_->initializationError().empty())
    return fail(Errc::IO, fmt::format("storage::resetForUnitTest: {}", accessor_->initializationError()));

  // Zrodlo deklarowane jest tylko do odczytu (purge zwraca ENOTSUP) - wystarcza mu ponowne otwarcie wyzej.
  if (!isDeclared()) {
    if (const auto result = accessor_->write(nullptr, 0); result != 0) {
      return fail(Errc::IO, fmt::format("storage::resetForUnitTest: purge of '{}' failed (result={}: {})", paths_.storageFile(),
                                        result, strerror(static_cast<int>(result))));
    }
  }
  recordsCount_ = 0;

  if (metaData_) (*metaData_).reset();

  RDB_TRY_ASSIGN(const size_t onMedium, accessor_->count());
  if (recordsCount_ != onMedium) {
    return fail(Errc::Logic, fmt::format("storage: internal record count mismatch: recordsCount_={} count()={} in {}",
                                         recordsCount_, onMedium, paths_.storageFile()));
  }
  return {};
}

void storage::cleanPayload(uint8_t *destination) {
  destination = (destination == nullptr)              //
                    ? storagePayload_->span().data()  //
                    : destination;
  auto size   = descriptor.getSizeInBytes();
  std::memset(destination, 0, size);
}

std::unique_ptr<rdb::payload>::pointer storage::getPayload() {
  RDB_ASSERT(storagePayload_ != nullptr, "storage::getPayload: payload not attached");
  return storagePayload_.get();
}

bool storage::descriptorFileExist() const {
  // Blad stat() (np. brak prawa do katalogu) znaczy tu "pliku nie widac" - attachDescriptor
  // sprobuje go wtedy zapisac i to zapis zglosi prawdziwy powod jako Errc::IO.
  std::error_code ec;
  return std::filesystem::exists(paths_.descriptorFile(), ec);
}

void storage::setDisposable(bool value) { isDisposable_ = value; }

void storage::releaseOnHold() { isHold_ = false; }

size_t storage::getRecordsCount() const { return recordsCount_; }

/// Cztery niezmienniki, ktorych zlamanie znaczy uzycie magazynu przed attachDescriptor().
///
/// Errc::Logic, a nie Config: zadnego z nich nie da sie wywolac poprawna sekwencja wywolan,
/// wiec nie sa czescia umowy z wolajacym - sa czescia umowy magazynu z samym soba. Zwracane,
/// a nie asercja, bo wszyscy wolajacy (read, write, purge) i tak zwracaja Result - host
/// dostaje InternalError zamiast zakonczonego procesu.
Result<> storage::requirePrepared() const {
  if (descriptor.empty()) return fail(Errc::Logic, "storage: descriptor is empty - storage not initialized");
  if (!accessor_) return fail(Errc::Logic, "storage: data file not opened - accessor not initialized");
  if (!storagePayload_) return fail(Errc::Logic, "storage: payload not attached");
  if (!metaData_) return fail(Errc::Logic, "storage: meta index not attached - attachDescriptor() not called");
  return {};
}

void storage::fire() {
  buffer_.fire(*storagePayload_);
  recordsCount_++;
}

Result<> storage::purge() {
  RDB_TRY(requirePrepared());

  // Nieudany purge zostawia dane na nosniku; bez zatrzymania recordsCount_ = 0 rozjechalby sie z count().
  if (const auto result = accessor_->write(nullptr, 0); result != 0) {
    return fail(Errc::IO, fmt::format("storage::purge: purge of '{}' failed (result={}: {})", paths_.storageFile(), result,
                                      strerror(static_cast<int>(result))));
  }
  recordsCount_ = 0;

  (*metaData_).reset();  // czyści indeks oraz liczniki maszyny gap
  return {};
}

void storage::markTransmissionGap(size_t gapDuration) { metaData_->onTransmissionGap(gapDuration); }

bool storage::hasGapBefore(size_t recordIndex) const { return metaData_->isGapBefore(recordIndex); }

bool storage::isMetaIndexEmpty() const {
  // Źródła deklarowane mają inertny indeks - o pustości decyduje licznik rekordów storage.
  if (isDeclared()) return recordsCount_ == 0;
  return metaData_->isEmpty();
}

Result<rdb::ReadStatus> storage::read(const size_t recordIndexFromFront, uint8_t *destination) {
  // Config, nie Logic: zrodla deklarowane czyta sie przez revRead(), a proba czytania ich
  // wprost jest bledem WOLAJACEGO, nie zlamanym niezmiennikiem silnika. To samo rozroznienie
  // widzi uzytkownik Pythona - straz w module.cpp zglasza tu StorageError.
  if (isDeclared()) return fail(Errc::Config, "storage::read: cannot read directly from a declared (DEVICE/TEXTSOURCE) storage");
  RDB_TRY(requirePrepared());

  if (destination == nullptr) {
    destination = storagePayload_->span().data();
  }

  if (destination == nullptr) return fail(Errc::Logic, "storage::read: destination pointer is null (payload span is empty)");
  auto size      = descriptor.getSizeInBytes();
  ssize_t result = 0;

  // Asercja spójności TYLKO w Debug. accessor_->count() nie jest odczytem pola: dla magazynu
  // DEFAULT (groupFile) to jeden stat() na KAŻDY żywy segment retencji, a read() stoi w pętli
  // okna - streamInstance woła revRead() raz na element (FIR mwi_long = 180 elementów), więc
  // przy interwale 1/360 s samo to okno wnosi kilkadziesiąt tysięcy wejść do jądra na sekundę.
  // Pod SCHED_FIFO każde z nich obciąża budżet slotu, a ten budżet jest wielkością mierzoną -
  // asercja płatna per rekord zmienia więc wynik pomiaru, dla którego silnik istnieje.
  //
  // W Release nie zostaje ślepa plama: jeśli plik został skrócony poniżej czytanej pozycji,
  // poniższe accessor_->read() zwraca błąd (krótki pread) i wraca Errc::IO z nazwą pliku
  // oraz pozycją. Tracimy wcześniejsze ostrzeżenie, nie samo wykrycie rozjazdu.
#ifndef NDEBUG
  {
    RDB_TRY_ASSIGN(const size_t onMedium, accessor_->count());
    if (recordsCount_ != onMedium) {
      return fail(Errc::Logic, fmt::format("storage::read: internal record count mismatch: recordsCount_={} count()={} in {}",
                                           recordsCount_, onMedium, paths_.storageFile()));
    }
  }
#endif

  if (isHold_) {
    // HOLD to stan LEGALNY, nie brak rekordu: strumien celowo wydaje wartosc zatrzymana, wiec
    // status jest Ok, a bitset zostaje taki, jaki byl przy ostatnim odczycie.
    std::memset(destination, 0, size);
    return ReadStatus::Ok;
  }

  if (recordsCount_ > 0 && recordIndexFromFront < recordsCount_) {
    result = accessor_->read(destination, recordIndexFromFront * size);
    if (result != 0) {
      return fail(Errc::IO, fmt::format("storage::read: read from '{}' at pos {} failed (result={}: {})", accessor_->name(),
                                        recordIndexFromFront, result, strerror(static_cast<int>(result))));
    }
    storagePayload_->setNullBitset(metaData_->nullBitsetFor(recordIndexFromFront));
  } else {
    // Rekordu NIE MA. Pamiec zerujemy, bo wolajacy moze na nia patrzec, ale bitset mowi all-null:
    // wartosc nieokreslona, nie zero. Poprzednio stalo tu `false` na kazdym polu, czyli jawne
    // "to nie jest NULL" - przez co semantyka pochlaniania NULL-i sie NIE wlaczala i reduktor
    // skladal to zero do MIN/MAX/SUM/AVG. Konwencja jest ta sama, ktora dataModel::fetchForward
    // stosuje dla rekordu poza zgromadzona historia.
    std::memset(destination, 0, size);
    storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), true));
    SPDLOG_ERROR("read fake {} - non existing data from pos:{} rec-count:{}", accessor_->name(), recordIndexFromFront,
                 recordsCount_);
    return ReadStatus::NoSuchRecord;
  }
  return ReadStatus::Ok;
}

Result<rdb::ReadStatus> storage::revRead(const size_t recordIndexFromBack, uint8_t *destination) {
  if (isHold_) {
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
                      : destination;

    if (destination == nullptr) return fail(Errc::Logic, "storage::revRead: destination pointer is null in hold path");
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    bufferState = sourceState::armed;  // fake armed on hold position
    return ReadStatus::Ok;
  }

  if (!isDeclared()) {
    // Spójność recordsCount_ vs accessor_->count() weryfikuje read(), i to tylko w Debug -
    // dla magazynów plikowych count() to syscall (stat), więc nie powtarzamy go tutaj ani nie
    // zostawiamy w Release (uzasadnienie przy asercji w read()).
    const auto recordPositionFromBack = recordsCount_ - recordIndexFromBack - 1;
    return read(recordPositionFromBack, destination);
  }

  // Zrodla deklarowane licza odczyty w pamieci (count() bez syscalli), wiec porownanie zostaje
  // takze w Release - jako ostrzezenie, jak dotad.
  if (const auto counted = accessor_->count(); counted && recordsCount_ != *counted)
    SPDLOG_ERROR("revRead {}: recordsCount:{} ->count():{}", paths_.storageFile(), recordsCount_, *counted);

  // For all _DECLARED_ data sources buffer capacity at least _MUST_ be 1
  // In order to maintain the consistency of declared data sources,
  // it is necessary to maintain a buffer of at least 1

  if (buffer_.capacity() == 0)
    return fail(Errc::Logic, "storage::revRead: circular buffer capacity is zero for a declared source");

  if (recordIndexFromBack == 0 && bufferState == sourceState::flux) {
    RDB_TRY(buffer_.readCurrent(*accessor_, *storagePayload_));
    bufferState = sourceState::armed;
    return ReadStatus::Ok;
  }
  // recordIndexFromBack is size_t (unsigned), always >= 0

  // Read data from Circular Buffer instead of data source
  // - only for declared data sources
  // - only for data sources that have buffer declared
  // - only for recordIndex > 0 if sourceState::flux
  // - also for recordIndex == 0

  if (recordIndexFromBack >= buffer_.capacity()) {
    return fail(Errc::Logic, fmt::format("storage::revRead: recordIndexFromBack {} >= circularBuffer_.capacity() {} in '{}'",
                                         recordIndexFromBack, buffer_.capacity(), accessor_->name()));
  }

  // in case of accessing buffer that has no data yet - zeros are returned

  if (recordIndexFromBack >= buffer_.size()) {
    destination = (destination == nullptr)              //
                      ? storagePayload_->span().data()  //
                      : destination;

    if (destination == nullptr)
      return fail(Errc::Logic, "storage::revRead: destination pointer is null in buffer fallback path");
    auto size = descriptor.getSizeInBytes();
    std::memset(destination, 0, size);
    // Ten sam brak rekordu co w read(), tylko dla zrodla DEKLAROWANEGO: bufor historii nie siega
    // tak gleboko. Poprzednio bitset zostawal tu NIETKNIETY, wiec rekord dziedziczyl znaczniki po
    // poprzednim odczycie - jeszcze gorzej niz zera oznaczone jako nie-null, bo wynik zalezal od
    // tego, co akurat lezalo w payloadzie.
    storagePayload_->setNullBitset(std::vector<bool>(descriptor.size(), true));
    SPDLOG_ERROR("read buffer fn {} - non existing data from [pos:{} cap:{} size:{}]", accessor_->name(), recordIndexFromBack,
                 buffer_.capacity(), buffer_.size());
    return ReadStatus::NoSuchRecord;
  }

  // Note: the previous if-block handles the case where recordIndexFromBack >= buffer_.size()
  // so here recordIndexFromBack < buffer_.size() is guaranteed

  *(storagePayload_) = buffer_.history(recordIndexFromBack);
  return ReadStatus::Ok;
}

void storage::setCapacity(const int capacity) {
  if (isDeclared()) buffer_.setCapacity(capacity);
}

Result<> storage::write(const size_t recordIndex) {
  RDB_TRY(requirePrepared());
  const auto nullInfo = storagePayload_->getNullBitset();

  // Maszyna detekcji gap żyje w metaData: rekord all-null poza fazą nullfill jest pochłaniany
  // (nie trafia do fizycznego magazynu), a jego brak zostanie oznaczony wpisem gap.
  if (recordIndex >= recordsCount_ && metaData_->absorbAppend(nullInfo)) return {};

  // Asercja spójności TYLKO w Debug, z tego samego powodu co w read(): accessor_->count() to
  // syscall (stat() na każdy segment), a write() wykonuje się raz na strumień na takt, wewnątrz
  // tego samego mierzonego budżetu slotu. Różnica wobec read() jest wyłącznie w krotności.
#ifndef NDEBUG
  {
    RDB_TRY_ASSIGN(const size_t onMedium, accessor_->count());
    if (recordsCount_ != onMedium) {
      return fail(Errc::Logic, fmt::format("storage::write: internal record count mismatch: recordsCount_={} count()={} in {}",
                                           recordsCount_, onMedium, paths_.storageFile()));
    }
  }
#endif

  ssize_t result = 0;
  if (recordIndex >= recordsCount_) {
    result = accessor_->write(storagePayload_->span().data());  // <- Call to append Function
    if (result != 0) {
      return fail(Errc::IO, fmt::format("storage::write: append to '{}' failed (result={}: {})", paths_.storageFile(), result,
                                        strerror(static_cast<int>(result))));
    }
    recordsCount_++;
    // `if constexpr` obejmuje całe wywołanie, nie tylko treść sondy: przy wyłączonej
    // sondzie z gorącej ścieżki zapisu musi zniknąć także wyliczenie argumentów
    // (isMemoryBackedStorage() porównuje napisy).
    if constexpr (rdb_probe_materialize) {
      probe::onMaterializedAppend(isMemoryBackedStorage(), descriptor.getSizeInBytes());
      // Kanoniczna szerokość liczona raz na magazyn: iteracja po polach w każdym zapisie
      // obciążałaby budżet slotu mierzony sondą E1.
      if (canonicalRecordBytes_ == 0) canonicalRecordBytes_ = probe::canonicalRecordBytes(descriptor);
      probe::onLogicalWrite(isSubstrate_, true, canonicalRecordBytes_);
    }

    metaData_->onRecordAppended(nullInfo);
    metaData_->flushCurrentEntry();
  } else {
    result = accessor_->write(storagePayload_->span().data(), recordIndex * descriptor.getSizeInBytes());
    if (result != 0) {
      return fail(Errc::IO, fmt::format("storage::write: overwrite to '{}' at index {} failed (result={}: {})",
                                        paths_.storageFile(), recordIndex, result, strerror(static_cast<int>(result))));
    }
    // Nadpisanie nie zwiększa objętości magazynu, więc nie wchodzi do `bytes`. Do metryki
    // K23 wchodzi, bo tam jednostką jest zapis rekordu, nie przyrost objętości - inaczej
    // substrat na buforze kołowym raportowałby zero.
    if constexpr (rdb_probe_materialize) {
      probe::onMaterializedOverwrite(isMemoryBackedStorage());
      if (canonicalRecordBytes_ == 0) canonicalRecordBytes_ = probe::canonicalRecordBytes(descriptor);
      probe::onLogicalWrite(isSubstrate_, false, canonicalRecordBytes_);
    }

    RDB_TRY(metaData_->onRecordModified(recordIndex, nullInfo));  // polimorficznie: cień indeksu albo główny indeks
  }
  return {};
}

void storage::configureGapDetection(boost::rational<int> rInterval, int nullFillCount) {
  rInterval_ = rInterval;
  if (metaData_) metaData_->configureGapDetection(nullFillCount);

  detectStartupState();
}

void storage::detectStartupState() {
  if (!metaData_ || !metaData_->gapDetectionEnabled()) return;

  // Detect rotation: data file is fresh/empty but meta index has records from previous run
  if (recordsCount_ == 0 && !metaData_->isEmpty()) {
    metaData_->rotate(percounter_);
    return;
  }

  // Fresh start with no previous records - nothing to gap from
  if (metaData_->isEmpty()) return;

  // Existing data: compute gap since data file was last written
  std::error_code ec;
  if (!std::filesystem::exists(paths_.storageFile(), ec) || rInterval_.numerator() <= 0) return;

  auto lastWriteFT = std::filesystem::last_write_time(paths_.storageFile(), ec);
  if (ec) return;

  auto lastWriteSC     = std::chrono::file_clock::to_sys(lastWriteFT);
  auto now             = std::chrono::system_clock::now();
  const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastWriteSC).count();
  if (elapsedNs <= 0) return;

  const auto intervalNs =
      static_cast<int64_t>(static_cast<double>(rInterval_.numerator()) / rInterval_.denominator() * 1'000'000'000.0);
  if (intervalNs <= 0) return;

  const auto gapDuration = static_cast<size_t>(elapsedNs / intervalNs);
  if (gapDuration > 0) metaData_->onTransmissionGap(gapDuration);
}

boost::rational<int> storage::getSamplingInterval() const { return rInterval_; }

}  // namespace rdb
