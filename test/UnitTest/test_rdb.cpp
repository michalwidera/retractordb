#include <gtest/gtest.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>

#include "rdb/descriptor.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/fainterface.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

// Tests intentionally use raw arrays and direct optional access to exercise binary storage semantics.
// NOLINTBEGIN(modernize-avoid-c-arrays,bugprone-unchecked-optional-access)

// ctest -R '^ut-test_rdb' -V

const uint AREA_SIZE = 10;

// Helper: build a single-field Descriptor matching AREA_SIZE bytes
static rdb::Descriptor makeDesc(size_t size) { return {"f", static_cast<int>(size), 1, rdb::BYTE}; }

template <typename T, typename K>
bool test_1() {
  const int noPerCounter = -1;
  K binary1("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    binary1.write(xData);

    if (std::memcmp(xData, "test data", AREA_SIZE) != 0) return false;

    T yData[AREA_SIZE];
    binary1.read(yData, 0);

    if (std::memcmp(yData, "test data", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(binary1.name().c_str());
  return static_cast<bool>(statusRemove1 == 0);
}

template <typename T, typename K>
bool test_2() {
  const int noPerCounter = -1;
  K dataStore("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
  {
    T xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);

    dataStore.write(xData);
    dataStore.write(xData);  // Add one extra record

    if (std::memcmp(xData, "test data", AREA_SIZE) != 0) return false;

    T yData[AREA_SIZE];
    dataStore.read(yData, 0);

    if (std::memcmp(yData, "test data", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.name().c_str());
  return static_cast<bool>(statusRemove1 == 0);
}

template <typename T, typename K>
bool test_3() {
  const int noPerCounter = -1;
  K dataStore("testfile-fstream", makeDesc(AREA_SIZE), noPerCounter);
  {
    T xData[AREA_SIZE];

    std::memcpy(xData, "test aaaa", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test bbbb", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test cccc", AREA_SIZE);
    dataStore.write(xData);

    std::memcpy(xData, "test xxxx", AREA_SIZE);
    dataStore.write(xData, AREA_SIZE);  // <- Update

    std::memcpy(xData, "test dddd", AREA_SIZE);
    dataStore.write(xData);

    T yData[AREA_SIZE];

    dataStore.read(yData, 0);

    if (std::memcmp(yData, "test aaaa", AREA_SIZE) != 0) return false;

    dataStore.read(yData, AREA_SIZE);

    if (std::memcmp(yData, "test xxxx", AREA_SIZE) != 0) return false;
  }
  auto statusRemove1 = remove(dataStore.name().c_str());
  return static_cast<bool>(statusRemove1 == 0);
}

TEST(xrdb, test_storage) {
  // This structure is tricky
  // If not aligned - size is 15
  // If aligned - size is 16
  // Note that this should be packed and size should be 15

  union dataPayload {
    uint8_t ptr[15];
    struct __attribute__((packed)) {
      char Name[10];    // 10
      uint8_t Control;  // 1
      int TLen;         // 4
    };
  };

  static_assert(sizeof(dataPayload) == 15);

  auto dataDescriptor{rdb::Descriptor("Name", 1, 10, rdb::STRING) +  //
                      rdb::Descriptor("Control", 1, 1, rdb::BYTE) +  //
                      rdb::Descriptor("TLen", 4, 1, rdb::INTEGER)};

  // This assert will fail is structure is not packed.
  EXPECT_TRUE(dataDescriptor.getSizeInBytes() == sizeof(dataPayload));

  rdb::storage dAcc2("datafile-fstream2", "datafile-fstream2", "");

  EXPECT_EQ(dAcc2.attachDescriptor(&dataDescriptor), "");
  dAcc2.setDisposable(true);

  auto *pl = dAcc2.getPayload();

  pl->setItem(0, std::string("test data"));
  pl->setItem(1, static_cast<uint8_t>(0x22));
  pl->setItem(2, 0x66);

  // Verify fieldByteOffset("TLen") points to the correct location in raw memory
  {
    int tlenViaOffset;
    std::memcpy(&tlenViaOffset, pl->span().data() + dAcc2.descriptor.fieldByteOffset("TLen"), sizeof(int));
    EXPECT_EQ(std::any_cast<int>(pl->getItem(2).value()), tlenViaOffset);
  }

  static_cast<void>(dAcc2.write());
  static_cast<void>(dAcc2.write());
  static_cast<void>(dAcc2.write());

  pl->setItem(0, std::string("xxxx xxxx"));
  pl->setItem(1, static_cast<uint8_t>(0x33));
  pl->setItem(2, 0x67);

  static_cast<void>(dAcc2.write(1));

  static_cast<void>(dAcc2.revRead(dAcc2.getRecordsCount() - 1 - 1));

  EXPECT_EQ(std::any_cast<std::string>(pl->getItem(0).value()), "xxxx xxxx");
  EXPECT_EQ(std::any_cast<int>(pl->getItem(2).value()), 0x67);
  EXPECT_EQ(std::any_cast<uint8_t>(pl->getItem(1).value()), 0x33);
}

// ---------------------------------------------------------------------------
// Dysponowalny storage (setDisposable(true)) nie zostawia po sobie żadnych
// plików po destrukcji - ani danych, ani deskryptora, ani indeksu metadanych
// (.meta). Bez odłączenia metaData_ od pliku przed usunięciem
// (storage::~storage() woła metaData_->abandonFile()), automatyczny
// destruktor metaData_ (flushCurrentEntry()) odtworzyłby właśnie skasowany
// plik .meta, bo appendEntry() otwiera plik trybem ios::app.
// ---------------------------------------------------------------------------
TEST(xrdb, test_storage_disposal_removes_all_files) {
  const std::string qryId = "disposal-fstream";
  auto desc               = makeDesc(AREA_SIZE);

  {
    rdb::storage s(qryId, qryId, "");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.setDisposable(true);

    auto *pl = s.getPayload();
    uint8_t xData[AREA_SIZE];
    std::memcpy(xData, "test data", AREA_SIZE);
    std::memcpy(pl->span().data(), xData, AREA_SIZE);

    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
  }  // ~storage(): isDisposable_ => usuwa plik danych, .desc i .meta

  EXPECT_FALSE(std::filesystem::exists(qryId));
  EXPECT_FALSE(std::filesystem::exists(qryId + ".desc"));
  EXPECT_FALSE(std::filesystem::exists(qryId + ".meta"));
}

TEST(crdb, genericBinaryFile_byte) {
  auto result1 = test_1<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result1);
  auto result2 = test_2<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result2);
  auto result3 = test_3<uint8_t, rdb::genericBinaryFile>();
  EXPECT_TRUE(result3);
}

TEST(crdb, posixBinaryFile_byte) {
  auto result1 = test_1<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result1);
  auto result2 = test_2<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result2);
  auto result3 = test_3<uint8_t, rdb::posixBinaryFile>();
  EXPECT_TRUE(result3);
}

TEST(xrdb, storage_persists_null_flags_via_metadata_stream) {
  const std::string streamName = "ut-null-meta-stream";
  const std::string dataFile   = "ut-null-meta-stream.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.setDisposable(true);

    auto *pl = s.getPayload();

    pl->setItem(0, std::nullopt);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    pl->setItem(0, 77);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    ASSERT_EQ(s.read(0), rdb::ReadStatus::Ok);
    EXPECT_FALSE(pl->getItem(0).has_value());

    ASSERT_EQ(s.read(1), rdb::ReadStatus::Ok);
    ASSERT_TRUE(pl->getItem(0).has_value());
    EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 77);
  }

  std::filesystem::remove(metaFile);
}

// Rekord, ktorego nie ma, jest wartoscia NIEOKRESLONA - nie zerem.
//
// Do 2026-09-23 read() zwracalo `bool`, ktory nie mial jak tego powiedziec: kazda prawdziwa awaria
// konczyla sie FatalError, a brak rekordu wracal jako `true` z pamiecia wyzerowana i JAWNIE
// oznaczona jako nie-null. Przez to semantyka pochlaniania NULL-i sie nie wlaczala, a obsluga bledu
// u trzech wolajacych byla martwa. Ten test pilnuje obu polowek kontraktu naraz: statusu i bitsetu.
TEST(xrdb, storage_read_beyond_last_record_reports_no_such_record) {
  const std::string streamName = "ut-no-such-record";
  const std::string dataFile   = "ut-no-such-record.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER) + rdb::Descriptor("b", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.setDisposable(true);

    auto *pl = s.getPayload();
    pl->setItem(0, 11);
    pl->setItem(1, 22);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Rekord istniejacy: status Ok i wartosci nietkniete.
    ASSERT_EQ(s.read(0), rdb::ReadStatus::Ok);
    ASSERT_TRUE(pl->getItem(0).has_value());
    EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 11);

    // Rekord tuz za koncem magazynu: status NoSuchRecord, a KAZDE pole jest NULL - nie zerem.
    // Gdyby bitset mowil "nie-null", reduktor zlozylby to zero do MIN/MAX/SUM/AVG zamiast pominac,
    // a porownanie z zerem nie odroznilo by braku danych od danych rownych zeru.
    EXPECT_EQ(s.read(1), rdb::ReadStatus::NoSuchRecord);
    EXPECT_FALSE(pl->getItem(0).has_value());
    EXPECT_FALSE(pl->getItem(1).has_value());

    // Pozycja daleko za koncem - ten sam kontrakt, bez wzgledu na to, jak daleko.
    EXPECT_EQ(s.read(99), rdb::ReadStatus::NoSuchRecord);
    EXPECT_FALSE(pl->getItem(0).has_value());
  }

  std::filesystem::remove(metaFile);
}

// Zapis do zrodla deklarowanego jest odmowa, nie awaria (#269).
//
// Do 2026-10-03 write() zwracalo `bool` o jednej mozliwej wartosci, a akcesor zrodla tylko do
// odczytu odpowiadal ENOTSUP, co konczylo proces przez FatalError. Test pilnuje, ze odmowa wraca
// jako WriteStatus::ReadOnly, a licznik rekordow i plik zrodla zostaja nietkniete.
TEST(xrdb, storage_append_to_declared_source_reports_read_only) {
  const std::string streamName = "ut-readonly-source";
  const std::string dataFile   = "ut-readonly-source.txt";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string content    = "1 2\n";

  {
    std::ofstream(dataFile) << content;
    std::ofstream(descFile) << "{\tINTEGER a\n\tINTEGER b\n\tREF \"" << dataFile << "\"\n\tTYPE TEXTSOURCE\n}\n";
  }

  {
    rdb::storage s(streamName, streamName, ".");
    ASSERT_EQ(s.attachDescriptor(), "");
    ASSERT_TRUE(s.isDeclared());

    EXPECT_EQ(s.write(), rdb::WriteStatus::ReadOnly);
    EXPECT_EQ(s.getRecordsCount(), 0);
  }

  std::ifstream in(dataFile);
  std::stringstream after;
  after << in.rdbuf();
  EXPECT_EQ(after.str(), content);

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
}

// Magazyn pusty: pierwszy odczyt nie ma czego zwrocic. Ta sciezka jest zywa w silniku, bo
// revRead(0) nad strumieniem bez rekordow liczy `0 - 0 - 1` i wchodzi tu z pozycja SIZE_MAX.
TEST(xrdb, storage_read_from_empty_storage_reports_no_such_record) {
  const std::string streamName = "ut-empty-storage";
  const std::string dataFile   = "ut-empty-storage.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.setDisposable(true);

    ASSERT_EQ(s.getRecordsCount(), 0U);
    EXPECT_EQ(s.read(0), rdb::ReadStatus::NoSuchRecord);
    EXPECT_FALSE(s.getPayload()->getItem(0).has_value());

    EXPECT_EQ(s.revRead(0), rdb::ReadStatus::NoSuchRecord);
    EXPECT_FALSE(s.getPayload()->getItem(0).has_value());
  }

  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_updates_null_flags_on_record_modify) {
  const std::string streamName = "ut-null-meta-modify";
  const std::string dataFile   = "ut-null-meta-modify.bin";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.setDisposable(true);

    auto *pl = s.getPayload();

    pl->setItem(0, 123);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    pl->setItem(0, std::nullopt);
    ASSERT_EQ(s.write(0), rdb::WriteStatus::Ok);

    ASSERT_EQ(s.read(0), rdb::ReadStatus::Ok);
    EXPECT_FALSE(pl->getItem(0).has_value());
  }

  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_purge_resets_metadata_stream_state) {
  const std::string streamName = "ut-purge-meta-reset";
  const std::string dataFile   = "ut-purge-meta-reset.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  // Clean up any stale files from a previous failed run
  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
  std::filesystem::remove("./" + dataFile + ".shadow");

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    auto *pl = s.getPayload();

    pl->setItem(0, std::nullopt);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    ASSERT_EQ(s.getRecordsCount(), 1U);

    s.purge();
    ASSERT_EQ(s.getRecordsCount(), 0U);

    pl->setItem(0, 1234);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    ASSERT_EQ(s.getRecordsCount(), 1U);

    ASSERT_EQ(s.read(0), rdb::ReadStatus::Ok);
    ASSERT_TRUE(pl->getItem(0).has_value());
    EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 1234);
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
  std::filesystem::remove("./" + dataFile + ".shadow");
}

TEST(xrdb, storage_first_write_persists_first_meta_record) {
  const std::string streamName = "ut-meta-first-record";
  const std::string dataFile   = "ut-meta-first-record.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    auto *pl = s.getPayload();
    pl->setItem(0, std::nullopt);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    // Meta index is flushed to disk at destructor (lazy-write per spec):
    // "zapis pierwszego rekordu do pliku indeksu nie jest wymagany natychmiast"
  }

  // After storage is closed, meta file must contain at least one RLE entry
  ASSERT_TRUE(std::filesystem::exists(metaFile));
  constexpr uintmax_t headerSize = sizeof(int64_t);
  EXPECT_GT(std::filesystem::file_size(metaFile), headerSize);

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_detection_marks_gap_after_null_records) {
  const std::string streamName = "ut-auto-gap";
  const std::string dataFile   = "ut-auto-gap.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    // nullFillCount=2: first 2 consecutive all-null appends go to storage (nullfill phase)
    // gap phase starts when consecutiveNullCount_ > nullFillCount_
    s.configureGapDetection(boost::rational<int>(1, 100), 2);

    auto *pl = s.getPayload();
    const std::vector<bool> allNull(desc.size(), true);
    const std::vector<bool> nonNull(desc.size(), false);

    // Nullfill phase: 2 all-null records written to storage as records 0 and 1
    pl->setNullBitset(allNull);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Gap phase: 2 more all-null records NOT written to storage (activeGapDuration_ = 2)
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Non-null record: flushPendingGap(2) marks gap, then written as record 2
    pl->setNullBitset(nonNull);
    pl->setItem(0, 42);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    EXPECT_TRUE(s.hasGapBefore(2));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_gap_flushed_on_destructor) {
  const std::string streamName = "ut-auto-gap-dtor";
  const std::string dataFile   = "ut-auto-gap-dtor.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.configureGapDetection(boost::rational<int>(1, 100), 1);

    auto *pl = s.getPayload();
    const std::vector<bool> allNull(desc.size(), true);

    // Nullfill phase: 1 all-null record written to storage as record 0
    pl->setNullBitset(allNull);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Gap phase: 1 more all-null record NOT written (activeGapDuration_ = 1)
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Destructor fires here - flushPendingGap() must persist the gap to meta file
  }

  // Reopen and verify gap was saved
  {
    rdb::storage s2(streamName, dataFile, ".");
    EXPECT_EQ(s2.attachDescriptor(&desc), "");

    // Record 0 was written; gap marker should be before record 1 (which doesn't exist yet,
    // but isGapBefore is based on meta entries, not record count)
    EXPECT_TRUE(s2.hasGapBefore(1));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_not_triggered_on_fast_writes) {
  const std::string streamName = "ut-auto-gap-fast";
  const std::string dataFile   = "ut-auto-gap-fast.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    // rInterval=1 (1 second), nullFillCount=2
    // All writes happen within milliseconds so no gap should be detected
    s.configureGapDetection(boost::rational<int>(1), 2);

    auto *pl = s.getPayload();

    for (int i = 0; i < 5; ++i) {
      pl->setItem(0, i);
      ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    }

    EXPECT_FALSE(s.hasGapBefore(0));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_auto_gap_not_triggered_on_modify) {
  const std::string streamName = "ut-auto-gap-modify";
  const std::string dataFile   = "ut-auto-gap-modify.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    // rInterval=1/100 (10ms), nullFillCount=2
    s.configureGapDetection(boost::rational<int>(1, 100), 2);

    auto *pl = s.getPayload();

    pl->setItem(0, 10);
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);

    // Wait longer than threshold
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Modify existing record - auto-gap should NOT be triggered for modifications
    pl->setItem(0, 99);
    ASSERT_EQ(s.write(0), rdb::WriteStatus::Ok);

    EXPECT_FALSE(s.hasGapBefore(0));
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

TEST(xrdb, storage_setSamplingInterval_propagates_to_meta) {
  const std::string streamName = "ut-interval-prop";
  const std::string dataFile   = "ut-interval-prop.bin";
  const std::string descFile   = "./" + streamName + ".desc";
  const std::string metaFile   = "./" + dataFile + ".meta";

  auto desc = rdb::Descriptor("a", 4, 1, rdb::INTEGER);

  {
    rdb::storage s(streamName, dataFile, ".");
    EXPECT_EQ(s.attachDescriptor(&desc), "");

    s.configureGapDetection(boost::rational<int>(1, 10));

    auto *pl = s.getPayload();

    // Write 3 records and verify meta tracks them
    for (int i = 0; i < 3; ++i) {
      pl->setItem(0, i * 10);
      ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    }

    EXPECT_FALSE(s.isMetaIndexEmpty());
    EXPECT_EQ(s.getRecordsCount(), 3U);
  }

  std::filesystem::remove(dataFile);
  std::filesystem::remove(descFile);
  std::filesystem::remove(metaFile);
}

// Rotacja zamyka dane, indeks i oba cienie w tej samej sesji, bez detekcji gap.
// Korekta rekordu 0 sprawdza niepusty cien, a ponowny odczyt archiwum - jego NULL-e.
TEST(xrdb, storage_rotates_data_and_null_indexes_on_shutdown) {
  const auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);
  for (const std::string type : {"DIRECT", "DEFAULT", "POSIX", "POSIXSHD", "GENERIC"}) {
    SCOPED_TRACE(type);
    const std::filesystem::path root = "ut-rotation-" + type;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "view");
    const auto dataFile = root / "result";
    const auto metaFile = root / "result.meta";
    const bool shadow   = type == "DEFAULT" || type == "POSIXSHD";
    for (int session = 0; session < 2; ++session) {
      {
        rdb::storage s("result", "result", root.string(), type, false, false, session);
        ASSERT_EQ(s.attachDescriptor(&desc), "");
        EXPECT_EQ(s.getRecordsCount(), 0U);
        EXPECT_TRUE(s.isMetaIndexEmpty());
        auto *pl = s.getPayload();
        pl->setItem(0, session == 0 ? std::optional<std::any>{std::nullopt} : std::optional<std::any>{42});
        EXPECT_EQ(s.write(), rdb::WriteStatus::Ok);
        pl->setItem(0, 100 + session);
        EXPECT_EQ(s.write(), rdb::WriteStatus::Ok);
        pl->setItem(0, session == 0 ? std::optional<std::any>{77} : std::optional<std::any>{std::nullopt});
        EXPECT_EQ(s.write(0), rdb::WriteStatus::Ok);
        EXPECT_EQ(s.read(0), rdb::ReadStatus::Ok);
        EXPECT_EQ(pl->getNullBitset(), std::vector<bool>{session == 1});
        if (session == 0) EXPECT_EQ(std::any_cast<int>(pl->getItem(0).value()), 77);
      }

      const std::string suffix = ".old" + std::to_string(session);
      ASSERT_TRUE(std::filesystem::exists(dataFile.string() + suffix));
      ASSERT_TRUE(std::filesystem::exists(metaFile.string() + suffix));
      EXPECT_FALSE(std::filesystem::exists(dataFile));
      EXPECT_FALSE(std::filesystem::exists(metaFile));
      for (const auto &name : {"result.shadow", "result.meta.shadow"}) {
        EXPECT_FALSE(std::filesystem::exists(root / name));
        EXPECT_EQ(std::filesystem::exists(root / (name + suffix)), shadow);
      }

      // Archiwum odtwarzamy pod nazwami roboczymi, aby sprawdzic normalna sciezke storage::read.
      for (const auto &name : {"result", "result.meta", "result.shadow", "result.meta.shadow"}) {
        if (!shadow && std::string(name).ends_with("shadow")) continue;
        std::filesystem::copy_file(root / (name + suffix), root / "view" / name,
                                   std::filesystem::copy_options::overwrite_existing);
      }
      {
        rdb::storage view("result", "result", (root / "view").string(), type, false, false, -1);
        ASSERT_EQ(view.attachDescriptor(&desc), "");
        EXPECT_EQ(view.getRecordsCount(), 2U);
        EXPECT_EQ(view.read(0), rdb::ReadStatus::Ok);
        EXPECT_EQ(view.getPayload()->getNullBitset(), std::vector<bool>{session == 1});
        if (session == 0) EXPECT_EQ(std::any_cast<int>(view.getPayload()->getItem(0).value()), 77);
        EXPECT_EQ(view.read(1), rdb::ReadStatus::Ok);
        EXPECT_EQ(view.getPayload()->getNullBitset(), std::vector<bool>{false});
        EXPECT_EQ(std::any_cast<int>(view.getPayload()->getItem(0).value()), 100 + session);
      }
    }
    std::filesystem::remove_all(root);
  }
}

// When data and meta counts match (no rotation), configureGapDetection must NOT
// rotate the meta index - existing records remain accessible.
TEST(xrdb, storage_no_rotation_when_counts_match) {
  const std::string qryID2    = "ut-no-rotation";
  const std::string dataFile2 = "ut-no-rotation.bin";
  const std::string descFile2 = qryID2 + ".desc";
  const std::string metaFile2 = dataFile2 + ".meta";

  auto desc = rdb::Descriptor("v", 4, 1, rdb::INTEGER);

  // First lifecycle: percounter=-1 so data file is NOT renamed.
  {
    rdb::storage s(qryID2, dataFile2, ".", "POSIX", false, false, -1);
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    auto *pl = s.getPayload();
    for (int i = 0; i < 3; ++i) {
      pl->setItem(0, i);
      static_cast<void>(s.write());
    }
  }

  EXPECT_TRUE(std::filesystem::exists(dataFile2));  // not renamed
  EXPECT_TRUE(std::filesystem::exists(metaFile2));

  // Second lifecycle: counts match → no rotation.
  {
    rdb::storage s(qryID2, dataFile2, ".", "POSIX", false, false, -1);
    EXPECT_EQ(s.attachDescriptor(&desc), "");
    s.configureGapDetection({1, 10});

    // Meta still has the original 3 records (not rotated).
    EXPECT_FALSE(s.isMetaIndexEmpty());
    EXPECT_EQ(s.getRecordsCount(), 3U);
  }

  for (const auto &f : {dataFile2, descFile2, metaFile2})
    std::filesystem::remove(f);
}

// REF z wczytanego `.desc` a uprawnienia magazynu (#278). REF moze prowadzic poza katalog magazynu
// - tak opisuje sie zewnetrzne zrodla DECLARE - ale wziety wylacznie z pliku nie kieruje zapisu
// poza katalog magazynu i `storage.ref_dirs`, a porzadkowanie magazynu nie usuwa pliku spoza nich.
// REF podany przez wolajacego (plan) jest decyzja operatora i musi zgadzac sie z zachowanym `.desc`.
namespace {
struct RefScene {
  std::string store   = "ut-ref-store";
  std::string outside = "ut-ref-outside";
  std::string stream  = "ut-ref";
  std::string desc    = "ut-ref-store/ut-ref.desc";

  RefScene() {
    std::filesystem::remove_all(store);
    std::filesystem::remove_all(outside);
    std::filesystem::create_directory(store);
    std::filesystem::create_directory(outside);
  }
  ~RefScene() {
    std::filesystem::remove_all(store);
    std::filesystem::remove_all(outside);
  }
  RefScene(const RefScene &)            = delete;
  RefScene &operator=(const RefScene &) = delete;

  void keep(const std::string &ref, const std::string &type) const {
    std::ofstream(desc) << "{ INTEGER a REF \"" << ref << "\" TYPE " << type << " }\n";
  }
};
}  // namespace

// Odczyt zewnetrznego zrodla dziala jak dotad, ale `rox` nie usuwa pliku, ktorego instancja nie posiada.
TEST(xrdb, storage_ref_outside_store_reads_but_is_not_removed) {
  RefScene scene;
  const std::string source = scene.outside + "/source.txt";
  std::ofstream(source) << "1\n2\n";
  scene.keep(source, "TEXTSOURCE");

  {
    rdb::storage s(scene.stream, scene.stream, scene.store);
    ASSERT_EQ(s.attachDescriptor(), "");
    EXPECT_TRUE(s.isDeclared());
    s.setDisposable(true);
  }

  EXPECT_TRUE(std::filesystem::exists(source));
  EXPECT_FALSE(std::filesystem::exists(scene.desc));
}

// Zapisywalny magazyn przeniesiony przez REF poza katalog magazynu: odmowa przed pierwszym
// dotknieciem sciezki - plik nie powstaje, a `.desc` zostaje do wgladu operatora.
TEST(xrdb, storage_writable_ref_outside_store_is_refused) {
  RefScene scene;
  const std::string target = scene.outside + "/target.bin";
  scene.keep(target, "POSIX");

  {
    rdb::storage s(scene.stream, scene.stream, scene.store);
    const std::string error = s.attachDescriptor();
    EXPECT_EQ(error, "storage: " + scene.desc + " moves the POSIX data file to '" + target +
                         "', outside the storage directory; add its directory to storage.ref_dirs in retractor.toml to "
                         "allow it");
  }

  EXPECT_FALSE(std::filesystem::exists(target));
  EXPECT_FALSE(std::filesystem::exists(target + ".meta"));
  EXPECT_TRUE(std::filesystem::exists(scene.desc));
}

// Katalog z `storage.ref_dirs` to jawnie dozwolona relokacja: zapis tam trafia, a porzadkowanie
// magazynu usuwa plik jak kazdy inny plik instancji.
TEST(xrdb, storage_writable_ref_into_allowed_dir_works) {
  RefScene scene;
  const std::string target = scene.outside + "/target.bin";
  scene.keep(target, "POSIX");

  {
    rdb::storage s(scene.stream, scene.stream, scene.store);
    s.allowRefDirs({std::filesystem::absolute(scene.outside).string()});
    ASSERT_EQ(s.attachDescriptor(), "");
    *reinterpret_cast<int *>(s.getPayload()->span().data()) = 7;
    ASSERT_EQ(s.write(), rdb::WriteStatus::Ok);
    EXPECT_EQ(std::filesystem::file_size(target), sizeof(int));
    s.setDisposable(true);
  }

  EXPECT_FALSE(std::filesystem::exists(target));
  EXPECT_FALSE(std::filesystem::exists(scene.desc));
}

// REF z planu wygrywa: zachowany `.desc` o innym REF - albo z REF, ktorego plan nie daje (SELECT) -
// jest odmowa, zanim cokolwiek zostanie otwarte. Ten sam REF przechodzi.
TEST(xrdb, storage_kept_ref_must_match_the_plan) {
  RefScene scene;
  const std::string planned = scene.outside + "/planned.txt";
  const std::string foreign = scene.outside + "/foreign.txt";
  std::ofstream(planned) << "1\n";
  std::ofstream(foreign) << "1\n";

  const auto attach = [&](const rdb::Descriptor &plan) {
    rdb::storage s(scene.stream, scene.stream, scene.store);
    const std::string error = s.attachDescriptor(&plan);
    if (error.empty()) s.setDisposable(true);  // jak dataModel: DISPOSABLE dopiero po udanym otwarciu
    return error;
  };
  const rdb::Descriptor field("a", sizeof(int), 1, rdb::INTEGER);

  scene.keep(foreign, "TEXTSOURCE");
  EXPECT_EQ(attach(field + rdb::Descriptor(planned, 0, 0, rdb::REF) + rdb::Descriptor("TEXTSOURCE", 0, 0, rdb::TYPE)),
            "storage: " + scene.desc + " names data file '" + foreign + "', but the plan gives '" + planned + "'; remove " +
                scene.desc + " to start the stream afresh");

  scene.keep(foreign, "POSIX");
  EXPECT_EQ(attach(field), "storage: " + scene.desc + " names data file '" + foreign + "', but the plan gives none; remove " +
                               scene.desc + " to start the stream afresh");
  EXPECT_TRUE(std::filesystem::exists(foreign));
  EXPECT_TRUE(std::filesystem::exists(scene.desc)) << "odmowa nie usuwa zachowanego .desc";

  // REF planu zgodny z plikiem: odczyt dziala, a DISPOSABLE z planu usuwa zrodlo jak dotad.
  scene.keep(planned, "TEXTSOURCE");
  EXPECT_EQ(attach(field + rdb::Descriptor(planned, 0, 0, rdb::REF) + rdb::Descriptor("TEXTSOURCE", 0, 0, rdb::TYPE)), "");
  EXPECT_FALSE(std::filesystem::exists(planned));
  EXPECT_TRUE(std::filesystem::exists(foreign));
}

// Dowiazanie pod nazwa pliku magazynu (#374). Kto ma prawo zapisu do katalogu magazynu, kladzie
// tam dowiazanie do pliku dostepnego dla konta uslugi; pliki magazynu otwierane z O_NOFOLLOW nie
// zapisuja ani nie obcinaja jego celu. Katalog magazynu bedacy dowiazaniem i REF podany przez
// wolajacego (#278) dzialaja jak dotad.
namespace {
struct LinkScene : RefScene {
  std::string sentinel           = outside + "/sentinel";
  const std::string sentinelText = "wartownik\n";
  const rdb::Descriptor plan     = rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  const std::string eloopMessage = std::strerror(ELOOP);

  LinkScene() { std::ofstream(sentinel) << sentinelText; }

  // Dowiazanie wzgledne, jakie zalozylby ktos z prawem zapisu do katalogu magazynu.
  void link(const std::string &name, const std::string &target = "sentinel") const {
    std::filesystem::create_symlink("../" + outside + "/" + target, store + "/" + name);
  }
  [[nodiscard]] bool sentinelIntact() const {
    std::ifstream in(sentinel);
    std::stringstream content;
    content << in.rdbuf();
    return content.str() == sentinelText;
  }
};

void writeValue(rdb::storage &s, const int value, const size_t position = std::numeric_limits<size_t>::max()) {
  *reinterpret_cast<int *>(s.getPayload()->span().data()) = value;
  ASSERT_EQ(s.write(position), rdb::WriteStatus::Ok);
}
}  // namespace

// Plik danych i cien danych: otwarcie konczy sie odmowa z ELOOP, a nie zapisem do celu.
TEST(xrdb, storage_data_file_symlink_is_refused) {
  for (const std::string type : {"POSIX", "POSIXSHD", "DEFAULT", "DIRECT"}) {
    LinkScene scene;
    scene.link(scene.stream);
    {
      rdb::storage s(scene.stream, scene.stream, scene.store, type, false, false, -1);
      const std::string error = s.attachDescriptor(&scene.plan);
      EXPECT_NE(error.find(scene.eloopMessage), std::string::npos) << type << ": " << error;
    }
    EXPECT_TRUE(scene.sentinelIntact()) << type;
  }
  for (const std::string type : {"POSIXSHD", "DEFAULT"}) {
    LinkScene scene;
    scene.link(scene.stream + ".shadow");
    {
      rdb::storage s(scene.stream, scene.stream, scene.store, type, false, false, -1);
      const std::string error = s.attachDescriptor(&scene.plan);
      EXPECT_NE(error.find(scene.eloopMessage), std::string::npos) << type << ": " << error;
    }
    EXPECT_TRUE(scene.sentinelIntact()) << type;
  }
}

// GENERIC otwiera plik przy kazdej operacji: odmowa przychodzi z pierwszym zapisem.
TEST(xrdb, storage_generic_symlink_is_not_written) {
  LinkScene scene;
  scene.link(scene.stream);
  EXPECT_DEATH(
      {
        rdb::storage s(scene.stream, scene.stream, scene.store, "GENERIC", false, false, -1);
        if (s.attachDescriptor(&scene.plan).empty()) writeValue(s, 7);
      },
      "failed");
  EXPECT_TRUE(scene.sentinelIntact());
}

// Indeks `.meta` i jego cien: magazyn pracuje dalej (indeks jest wtorny), ale cel dowiazania
// nie zostaje obciety ani dopisany. DISPOSABLE usuwa samo dowiazanie, cel zostaje.
TEST(xrdb, storage_meta_symlink_is_not_written) {
  for (const std::string suffix : {".meta", ".meta.shadow"}) {
    LinkScene scene;
    scene.link(scene.stream + suffix);
    {
      rdb::storage s(scene.stream, scene.stream, scene.store, "POSIXSHD", false, false, -1);
      ASSERT_EQ(s.attachDescriptor(&scene.plan), "") << suffix;
      writeValue(s, 7);
      writeValue(s, 8, 0);  // aktualizacja rekordu dopisuje wpis do `.meta.shadow`
      s.setDisposable(true);
    }
    EXPECT_TRUE(scene.sentinelIntact()) << suffix;
    EXPECT_FALSE(std::filesystem::is_symlink(scene.store + "/" + scene.stream + suffix)) << suffix;
  }
}

// `.desc` bedacy dowiazaniem: istniejacy cel nie jest czytany jako deskryptor, a nieistniejacy
// nie powstaje przez dowiazanie.
TEST(xrdb, storage_descriptor_symlink_is_refused) {
  LinkScene scene;
  scene.link(scene.stream + ".desc");
  {
    rdb::storage s(scene.stream, scene.stream, scene.store, "POSIX", false, false, -1);
    const std::string error = s.attachDescriptor(&scene.plan);
    EXPECT_NE(error.find(scene.eloopMessage), std::string::npos) << error;
  }
  EXPECT_TRUE(scene.sentinelIntact());

  std::filesystem::remove(scene.desc);
  scene.link(scene.stream + ".desc", "created");
  EXPECT_DEATH(
      {
        rdb::storage s(scene.stream, scene.stream, scene.store, "POSIX", false, false, -1);
        (void)s.attachDescriptor(&scene.plan);
      },
      "failed to open descriptor file for writing");
  EXPECT_FALSE(std::filesystem::exists(scene.outside + "/created"));
}

// Katalog magazynu wskazany dowiazaniem dziala jak dotad - O_NOFOLLOW dotyczy ostatniego komponentu.
TEST(xrdb, storage_directory_symlink_works) {
  LinkScene scene;
  const std::string storeLink = scene.store + "-link";
  std::filesystem::remove(storeLink);
  std::filesystem::create_directory_symlink(scene.store, storeLink);
  {
    rdb::storage s(scene.stream, scene.stream, storeLink, "POSIX", false, false, -1);
    ASSERT_EQ(s.attachDescriptor(&scene.plan), "");
    writeValue(s, 7);
  }
  EXPECT_EQ(std::filesystem::file_size(scene.store + "/" + scene.stream), sizeof(int));
  std::filesystem::remove(storeLink);
}

// REF podany przez wolajacego jest decyzja operatora (#278): dowiazanie pod nim prowadzi zapis do celu.
TEST(xrdb, storage_caller_ref_symlink_works) {
  LinkScene scene;
  const std::string link   = scene.outside + "/link";
  const std::string target = scene.outside + "/target.bin";
  std::filesystem::create_symlink("target.bin", link);
  const rdb::Descriptor plan = scene.plan + rdb::Descriptor(link, 0, 0, rdb::REF) + rdb::Descriptor("POSIX", 0, 0, rdb::TYPE);
  {
    rdb::storage s(scene.stream, scene.stream, scene.store);
    ASSERT_EQ(s.attachDescriptor(&plan), "");
    writeValue(s, 7);
  }
  EXPECT_TRUE(std::filesystem::is_symlink(link));
  EXPECT_EQ(std::filesystem::file_size(target), sizeof(int));
}

// NOLINTEND(modernize-avoid-c-arrays,bugprone-unchecked-optional-access)
