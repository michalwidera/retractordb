#include <gtest/gtest.h>

#include <array>
#include <cerrno>
#include <optional>
#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/faccmemory.hpp"
#include "rdb/memoryStore.hpp"
#include "rdb/storage.hpp"
#include "rdbResult.hpp"

static rdb::Descriptor makeDesc(size_t size) { return {"f", static_cast<int>(size), 1, rdb::BYTE}; }

using BYTE = unsigned char;

// ctest -R '^ut-test_faccmemory' -V

// Source under test:
// src/rdb/lib/faccmemory.cc

// ============================================================
// memoryFile tests
// ============================================================

// Verify write and sequential read of 4 records without retention limit
TEST(MemoryTest, test_faccmemory_infinite) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 4);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 1);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 2);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 4);

  mfa->write(nullptr);                            // Clear the storage
  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 0);  // After clearing, count should be 0
}

// Verify retention ring buffer: after 4 writes with size 2, ring holds last 2 records.
// Positions outside the last 2 writes are refused with ERANGE instead of returning the record
// that took over their ring slot.
TEST(MemoryTest, test_faccmemory_retention) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 2);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(&record.data), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 4);

  // Positions 2 and 3 map to ring slots 0 and 1 - current values 3 and 4
  GTEST_ASSERT_EQ(mfa->read(&record.data, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(&record.data, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record.data, 4);

  // Positions 0 and 1 left the ring; slot 0 now holds record 2, not record 0
  record.data = 0;
  GTEST_ASSERT_EQ(mfa->read(&record.data, 0), ERANGE);
  GTEST_ASSERT_EQ(mfa->read(&record.data, 1), ERANGE);
  GTEST_ASSERT_EQ(record.data, 0);
  // Position 4 has not been written yet
  GTEST_ASSERT_EQ(mfa->read(&record.data, 4), ERANGE);

  mfa->write(nullptr);                            // Clear the storage
  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 0);  // After clearing, count should be 0
}

// Verify write and read of multi-byte records (4-byte integers) without retention
TEST(MemoryTest, test_faccmemory_multibyte_record) {
  int record;

  std::string filename = "test_file_memory_multi";

  auto recsize   = sizeof(int);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record = 1000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 2000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record = 3000;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 3);

  // Read back using byte positions (position = index * recordSize)
  mfa->read(reinterpret_cast<uint8_t *>(&record), 0 * sizeof(int));
  GTEST_ASSERT_EQ(record, 1000);

  mfa->read(reinterpret_cast<uint8_t *>(&record), 1 * sizeof(int));
  GTEST_ASSERT_EQ(record, 2000);

  mfa->read(reinterpret_cast<uint8_t *>(&record), 2 * sizeof(int));
  GTEST_ASSERT_EQ(record, 3000);

  mfa->write(nullptr);
  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 0);
}

// Verify update-in-place overwrites existing record at given position
TEST(MemoryTest, test_faccmemory_update_in_place) {
  BYTE record;

  std::string filename = "test_file_memory_update";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record = 10;
  mfa->write(&record);
  record = 20;
  mfa->write(&record);
  record = 30;
  mfa->write(&record);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 3);

  // Update record at position 1 (byte offset = 1 * recsize = 1)
  record = 99;
  GTEST_ASSERT_EQ(mfa->write(&record, 1), EXIT_SUCCESS);

  // Count should remain the same after update
  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 3);

  // Verify updated record
  mfa->read(&record, 1);
  GTEST_ASSERT_EQ(record, 99);

  // Verify other records are untouched
  mfa->read(&record, 0);
  GTEST_ASSERT_EQ(record, 10);

  mfa->read(&record, 2);
  GTEST_ASSERT_EQ(record, 30);

  mfa->write(nullptr);
}

// Verify name() returns the filename passed at construction
TEST(MemoryTest, test_faccmemory_name) {
  std::string filename = "test_file_memory_name";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  EXPECT_EQ(mfa->name(), "test_file_memory_name");

  mfa->write(nullptr);
}

// Verify count is 0 for freshly created memory accessor
TEST(MemoryTest, test_faccmemory_empty_count) {
  std::string filename = "test_file_memory_empty";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 0);

  mfa->write(nullptr);
}

// Verify retention ring boundary: ring of size 3 overwrites slot 0 on the 4th write.
// The overwritten position is refused; the positions still in the ring read their own records.
TEST(MemoryTest, test_faccmemory_retention_boundary) {
  BYTE record;

  std::string filename = "test_file_memory_boundary";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 3);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  // Write 3 records - ring fills up, no overwrite yet
  for (BYTE i = 1; i <= 3; i++) {
    record = i;
    mfa->write(&record);
  }

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 3);

  // Ring slots 0,1,2 hold values 1,2,3
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 1);
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 3);

  // Write 4th record - wraps to slot 0, overwrites value 1 with value 4
  record = 4;
  mfa->write(&record);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 4);

  // Position 0 shares slot 0 with position 3 and left the ring with the 4th write
  GTEST_ASSERT_EQ(mfa->read(&record, 0), ERANGE);
  GTEST_ASSERT_EQ(mfa->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 2);

  // Position 3 is the record now held in slot 0
  GTEST_ASSERT_EQ(mfa->read(&record, 3), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 4);

  // Write 5th record - wraps to slot 1, overwrites value 2 with value 5
  record = 5;
  mfa->write(&record);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 5);

  GTEST_ASSERT_EQ(mfa->read(&record, 4), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 5);

  // Slot 2 still holds value 3 (not yet overwritten)
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 3);

  mfa->write(nullptr);
}

// Verify read beyond storage bounds returns EXIT_FAILURE
TEST(MemoryTest, test_faccmemory_read_beyond_bounds) {
  BYTE record;

  std::string filename = "test_file_memory_bounds";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);
  auto mfa       = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);

  record = 10;
  mfa->write(&record);
  record = 20;
  mfa->write(&record);
  record = 30;
  mfa->write(&record);

  GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 3);

  // Valid reads
  GTEST_ASSERT_EQ(mfa->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(mfa->read(&record, 2), EXIT_SUCCESS);

  // Beyond upper bound should fail
  GTEST_ASSERT_EQ(mfa->read(&record, 3), ERANGE);
  GTEST_ASSERT_EQ(mfa->read(&record, 99), ERANGE);

  // Empty storage should fail
  mfa->write(nullptr);
  GTEST_ASSERT_EQ(mfa->read(&record, 0), ERANGE);
}

// Verify two LIVE instances with the same filename share one bucket
TEST(MemoryTest, test_faccmemory_shared_between_live_instances) {
  BYTE record;

  std::string filename = "test_file_memory_shared";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  auto mfa = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
  record   = 0x42;
  mfa->write(&record);
  record = 0x43;
  mfa->write(&record);

  auto mfa2 = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
  GTEST_ASSERT_EQ(rdbtest::ok(mfa2->count()), 2);

  GTEST_ASSERT_EQ(mfa2->read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x42);

  GTEST_ASSERT_EQ(mfa2->read(&record, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x43);
}

// Verify the bucket dies with the last instance using it: a stream of the same name in the next
// plan starts empty. Until 2026-09-27 it inherited the records and writeCount of the previous plan,
// which after `xqry --reset` shifted its ring and ended the server with FatalError.
TEST(MemoryTest, test_faccmemory_bucket_dies_with_last_instance) {
  BYTE record = 0x42;

  std::string filename = "test_file_memory_last_instance";

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  {
    auto mfa = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
    mfa->write(&record);
    mfa->write(&record);
    GTEST_ASSERT_EQ(rdbtest::ok(mfa->count()), 2);
  }  // mfa destroyed - the last user of the bucket

  auto mfa2 = std::make_unique<rdb::memoryFile>(filename, makeDesc(recsize), retention);
  GTEST_ASSERT_EQ(rdbtest::ok(mfa2->count()), 0);
  GTEST_ASSERT_EQ(mfa2->read(&record, 0), ERANGE);
}

// Verify an instance with a different record size never sees records of the old size: read()
// copies a whole stored record, so a wider one would overrun the narrower caller's buffer.
TEST(MemoryTest, test_faccmemory_other_record_size_starts_empty) {
  std::string filename = "test_file_memory_other_size";

  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  auto wide = std::make_unique<rdb::memoryFile>(filename, makeDesc(8), retention);
  std::array<BYTE, 8> wideRecord{};
  wide->write(wideRecord.data());
  GTEST_ASSERT_EQ(rdbtest::ok(wide->count()), 1);

  auto narrow = std::make_unique<rdb::memoryFile>(filename, makeDesc(1), retention);
  GTEST_ASSERT_EQ(rdbtest::ok(narrow->count()), 0);
}

// Verify read() refuses a stored record of another size instead of copying it past the caller's
// buffer. Two LIVE instances of one name with different record sizes do not occur in the engine
// (one storage per stream), and the constructor's clearing cannot help once both are alive.
TEST(MemoryTest, test_faccmemory_read_refuses_record_of_other_size) {
  std::string filename = "test_file_memory_read_other_size";

  auto retention = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  auto wide   = std::make_unique<rdb::memoryFile>(filename, makeDesc(8), retention);
  auto narrow = std::make_unique<rdb::memoryFile>(filename, makeDesc(1), retention);

  std::array<BYTE, 8> wideRecord{};
  GTEST_ASSERT_EQ(wide->write(wideRecord.data()), EXIT_SUCCESS);

  // On the heap, so that an overrun is visible to valgrind (ut_faccmemory-valgrind).
  auto narrowBuffer = std::make_unique<BYTE[]>(1);
  GTEST_ASSERT_EQ(narrow->read(narrowBuffer.get(), 0), EINVAL);
}

// Verify the number of buckets stays constant over plan swaps that bring new stream names:
// each round builds the instances of one plan and drops them, as dataModel does at the end
// of an epoch. Until 2026-09-27 every name ever used kept its bucket until process exit.
TEST(MemoryTest, test_faccmemory_bucket_count_constant_across_plan_swaps) {
  BYTE record = 0x42;

  auto recsize   = sizeof(BYTE);
  auto retention = std::pair<std::string, size_t>("MEMORY", 2);

  const auto baseline = rdb::memoryFile::bucketCountForUnitTest();

  for (int plan = 0; plan < 50; ++plan) {
    {
      auto a = std::make_unique<rdb::memoryFile>("plan_" + std::to_string(plan) + "/a", makeDesc(recsize), retention);
      auto b = std::make_unique<rdb::memoryFile>("plan_" + std::to_string(plan) + "/b", makeDesc(recsize), retention);
      a->write(&record);
      b->write(&record);
      GTEST_ASSERT_EQ(rdb::memoryFile::bucketCountForUnitTest(), baseline + 2);
    }
    GTEST_ASSERT_EQ(rdb::memoryFile::bucketCountForUnitTest(), baseline);
  }
}

// ============================================================
// MemoryStore - wlascicielstwo pamieci MEMORY (faza 2, #446 C2)
// ============================================================

// Sedno fazy 2: dwa Memory Store to dwie pamieci. Ta sama nazwa strumienia, te same zapisy, zadnego
// wspolnego stanu. Przed faza 2 pamiec byla mapa `static` w faccmemory.cc, wiec notatnik, ktory
// zbudowal dwa silniki w jednym procesie, dostawal jeden strumien sklejony z dwoch.
TEST(MemoryStoreTest, separate_stores_do_not_share_a_stream) {
  BYTE record;
  const std::string filename = "shared_name_two_stores";
  auto recsize               = sizeof(BYTE);
  auto retention             = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  rdb::MemoryStore storeA;
  rdb::MemoryStore storeB;

  rdb::memoryFile fileA(filename, makeDesc(recsize), retention, storeA);
  rdb::memoryFile fileB(filename, makeDesc(recsize), retention, storeB);

  record = 0x11;
  fileA.write(&record);
  GTEST_ASSERT_EQ(rdbtest::ok(fileA.count()), 1);
  GTEST_ASSERT_EQ(rdbtest::ok(fileB.count()), 0) << "Memory Store B zobaczyl zapis do Memory Store A";

  record = 0x22;
  fileB.write(&record);
  GTEST_ASSERT_EQ(rdbtest::ok(fileA.count()), 1);
  GTEST_ASSERT_EQ(rdbtest::ok(fileB.count()), 1);

  GTEST_ASSERT_EQ(fileA.read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x11);
  GTEST_ASSERT_EQ(fileB.read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x22);
}

// Druga polowa kontraktu: W GRANICACH jednego Memory Store wspoldzielenie po nazwie zostaje - na tym
// stoi plan, w ktorym piszacy i czytajacy ten sam strumien MEMORY to rozne obiekty. Kubelek
// znika z ostatnia instancja, tak jak w domyslnym Memory Store.
TEST(MemoryStoreTest, one_store_still_shares_by_stream_name) {
  BYTE record;
  const std::string filename = "shared_name_one_store";
  auto recsize               = sizeof(BYTE);
  auto retention             = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  rdb::MemoryStore store;
  {
    rdb::memoryFile writer(filename, makeDesc(recsize), retention, store);
    rdb::memoryFile reader(filename, makeDesc(recsize), retention, store);

    record = 0x37;
    writer.write(&record);

    GTEST_ASSERT_EQ(store.size(), 1);
    GTEST_ASSERT_EQ(rdbtest::ok(reader.count()), 1);
    GTEST_ASSERT_EQ(reader.read(&record, 0), EXIT_SUCCESS);
    GTEST_ASSERT_EQ(record, 0x37);
  }
  GTEST_ASSERT_TRUE(store.empty()) << "kubelek przezyl ostatnia instancje";
}

// Bez podanego Memory Store obowiazuje instancja domyslna procesu - zachowanie sprzed fazy 2, na ktorym
// stoja xretractor, xtrdb i testy powyzej.
TEST(MemoryStoreTest, default_is_the_process_store) {
  BYTE record;
  const std::string filename = "default_store_stream";
  auto recsize               = sizeof(BYTE);
  auto retention             = std::pair<std::string, size_t>("DEFAULT", rdb::memoryFile::no_retention);

  rdb::memoryFile implicitStore(filename, makeDesc(recsize), retention);
  rdb::memoryFile explicitStore(filename, makeDesc(recsize), retention, rdb::MemoryStore::processDefault());

  record = 0x5A;
  implicitStore.write(&record);

  GTEST_ASSERT_EQ(rdbtest::ok(explicitStore.count()), 1);
  GTEST_ASSERT_EQ(explicitStore.read(&record, 0), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(record, 0x5A);
}

// Memory Store podany do rdb::storage ma dojechac az do akcesora. Bez tego memoryFile umie przyjac
// Memory Store, ale nikt mu go nie podaje.
TEST(MemoryStoreTest, storage_passes_its_store_down_to_the_accessor) {
  // Pole TYPE nie jest danymi - attachStorage() czyta z niego nazwe typu magazynu,
  // wiec "MEMORY" wybiera backend pamieciowy (accessorFactory.cc).
  auto descriptor = rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"MEMORY", 0, 0, rdb::TYPE}};

  rdb::MemoryStore given;
  rdb::MemoryStore untouched;

  auto streamOwner = rdbtest::ok(rdb::storage::create("memstore_iso", "memstore_iso", "", "DEFAULT", false, false, -1, &given));

  rdb::storage &stream = *streamOwner;
  ASSERT_RDB_OK(stream.attachDescriptor(&descriptor));
  stream.setDisposable(true);

  auto *payload = stream.getPayload();
  payload->setNullBitset(std::vector<bool>(descriptor.size(), false));
  payload->setItem(0, 7);
  ASSERT_EQ(rdbtest::ok(stream.write()), rdb::WriteStatus::Ok);

  EXPECT_FALSE(given.empty()) << "zapis nie trafil do Memory Store podanego magazynowi";
  EXPECT_TRUE(untouched.empty()) << "zapis trafil do cudzego Memory Store";
}

// Nadpisanie w miejscu rekordu, ktory opuscil pierscien, trafialo do slotu nowszego rekordu
// i zglaszalo sukces. Teraz jest odmowa, a rekordy w oknie zostaja nietkniete.
TEST(MemoryTest, update_outside_retention_window_keeps_newer_records) {
  BYTE record = 0;
  rdb::MemoryStore store;
  rdb::memoryFile mfa("update_outside_window", makeDesc(sizeof(BYTE)), {"MEMORY", 2}, store);
  for (BYTE value = 1; value <= 3; ++value)
    GTEST_ASSERT_EQ(mfa.write(&value), EXIT_SUCCESS);

  BYTE stale = 0x7F;
  EXPECT_EQ(mfa.write(&stale, 0), ERANGE);
  EXPECT_EQ(mfa.write(&stale, 3), ERANGE);  // jeszcze niezapisany
  GTEST_ASSERT_EQ(mfa.read(&record, 1), EXIT_SUCCESS);
  EXPECT_EQ(record, 2);
  GTEST_ASSERT_EQ(mfa.read(&record, 2), EXIT_SUCCESS);
  EXPECT_EQ(record, 3);

  BYTE fresh = 0x55;
  EXPECT_EQ(mfa.write(&fresh, 1), EXIT_SUCCESS);
  GTEST_ASSERT_EQ(mfa.read(&record, 1), EXIT_SUCCESS);
  EXPECT_EQ(record, 0x55);
  EXPECT_EQ(rdbtest::ok(mfa.count()), 3U);
}

// Ta sama granica widziana przez rdb::storage: rekord spoza retencji MEMORY to Errc::IO z ERANGE,
// jak rekord z usunietego segmentu magazynu dyskowego - a nie cudze dane ze statusem Ok.
TEST(MemoryTest, storage_read_outside_memory_retention_is_an_io_error) {
  auto descriptor = rdb::Descriptor("a", 4, 1, rdb::INTEGER) + rdb::Descriptor("MEMORY", 0, 1, rdb::TYPE) +
                    rdb::Descriptor("retention-mem", 2, 1, rdb::RETMEMORY);
  rdb::MemoryStore store;
  auto streamOwner = rdbtest::ok(rdb::storage::create("memretention", "memretention", "", "DEFAULT", false, false, -1, &store));
  rdb::storage &stream = *streamOwner;
  ASSERT_RDB_OK(stream.attachDescriptor(&descriptor));
  stream.setDisposable(true);

  for (int value = 10; value <= 30; value += 10) {
    stream.getPayload()->setNullBitset(std::vector<bool>(descriptor.size(), false));
    stream.getPayload()->setItem(0, value);
    ASSERT_EQ(rdbtest::ok(stream.write()), rdb::WriteStatus::Ok);
  }

  ASSERT_EQ(rdbtest::ok(stream.revRead(1)), rdb::ReadStatus::Ok);
  EXPECT_EQ(stream.getPayload()->getItemVT(0), std::optional<rdb::descFldVT>(20));
  const auto outside = stream.revRead(2);
  ASSERT_RDB_ERROR(outside, rdb::Errc::IO);
  EXPECT_EQ(outside.error().err(), ERANGE);
}
