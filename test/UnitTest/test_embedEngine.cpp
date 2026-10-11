#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "fatalError.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/embed/engine.hpp"
#include "rdb/memoryStore.hpp"

// ctest -R '^ut_embedEngine' -V

//
// Fasada osadzenia (#446, C2): jeden obiekt = jedna instancja silnika w procesie.
//
// Demon buduje swoj stan raz, notatnik przy kazdej komorce. Do fazy 2 nie dalo sie tego SPRAWDZIC -
// stan byl globalny i nie istnial obiekt, ktory moglby go posiadac. Ten plik jest pierwszym
// miejscem, w ktorym ta teza staje sie asercja.
//

namespace {

/// Pole TYPE nie jest danymi: attachStorage() czyta z niego nazwe typu magazynu, wiec "MEMORY"
/// wybiera backend pamieciowy (accessorFactory.cc).
rdb::Descriptor memoryBackedInteger() { return rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"MEMORY", 0, 0, rdb::TYPE}}; }

void writeOneRecord(rdb::storage &stream, const rdb::Descriptor &descriptor, int value) {
  auto *payload = stream.getPayload();
  payload->setNullBitset(std::vector<bool>(descriptor.size(), false));
  payload->setItem(0, value);
  ASSERT_EQ(stream.write(), rdb::WriteStatus::Ok);
}

}  // namespace

// Sedno: DWA silniki, TA SAMA nazwa strumienia MEMORY, zero wspolnego stanu. Przed faza 2 pamiec
// magazynu MEMORY byla mapa `static` w faccmemory.cc, wiec ten przypadek nie mial jak przejsc.
TEST(embedEngine, two_engines_do_not_share_a_memory_stream) {
  auto descriptor = memoryBackedInteger();

  rdb::embed::Engine first;
  rdb::embed::Engine second;

  auto firstStream  = first.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  auto secondStream = second.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  ASSERT_TRUE(firstStream->attachDescriptor(&descriptor).empty());
  ASSERT_TRUE(secondStream->attachDescriptor(&descriptor).empty());
  firstStream->setDisposable(true);

  writeOneRecord(*firstStream, descriptor, 11);

  // Nowy magazyn czyta liczbe rekordow z akcesora przy dolaczeniu deskryptora, wiec widzi dokladnie
  // kubelek Memory Store swojego silnika.
  auto firstProbe  = first.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  auto secondProbe = second.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  ASSERT_TRUE(firstProbe->attachDescriptor(&descriptor).empty());
  ASSERT_TRUE(secondProbe->attachDescriptor(&descriptor).empty());

  EXPECT_EQ(firstProbe->getRecordsCount(), 1U) << "zapis nie trafil do Memory Store wlasnego silnika";
  EXPECT_EQ(secondProbe->getRecordsCount(), 0U) << "drugi silnik zobaczyl zapis pierwszego";
}

// Engine jest OPCJONALNY: magazyn zbudowany bez niego nadal uzywa instancji domyslnej procesu.
// Dlatego ta zmiana nic nie zmienia serwerowi ani xtrdb - zaden z nich o Engine nie wie.
//
// Domyslny Memory Store jest stanem CALEGO PROCESU; przypadek opiera sie na tym, ze plik testowy
// ma wlasna binarke, a kubelki znikaja z ostatnia instancja magazynu.
TEST(embedEngine, storage_built_without_an_engine_uses_the_process_default) {
  auto descriptor = memoryBackedInteger();

  ASSERT_TRUE(rdb::MemoryStore::processDefault().empty()) << "domyslny Memory Store nie byl pusty na starcie";

  rdb::embed::Engine engine;
  rdb::storage loose("engine_default", "engine_default", "", "DEFAULT", false, false, -1);
  ASSERT_TRUE(loose.attachDescriptor(&descriptor).empty());
  loose.setDisposable(true);

  writeOneRecord(loose, descriptor, 37);

  EXPECT_FALSE(rdb::MemoryStore::processDefault().empty()) << "zapis bez silnika nie trafil do domyslnego Memory Store";
  EXPECT_TRUE(engine.memory().empty()) << "zapis bez silnika trafil do Memory Store silnika";
}

// Decyzja (a) z #463: K1 przy budowie magazynu przez fasade wychodzi jako EngineFatal, a proces
// hosta zyje - bez zatrzasku uslugi. Pusty qryID to K1 w StoragePaths (storagePaths.cc).
TEST(embedEngine, open_storage_failure_is_engine_fatal_not_process_exit) {
  rdb::embed::Engine engine;

  std::optional<rdb::EngineFatal> caught;
  try {
    static_cast<void>(engine.openStorage("", "engine_fatal", "", "DEFAULT", false, false, -1));
  } catch (const rdb::EngineFatal &fault) {
    caught = fault;
  }

  ASSERT_TRUE(caught.has_value()) << "openStorage nie zglosil bledu K1";
  EXPECT_EQ(caught->cls, rdb::ErrorClass::Internal);
  EXPECT_NE(caught->message.find("qryID"), std::string::npos) << caught->message;
  EXPECT_FALSE(fatalErrorRaised.load()) << "tryb rzucaj podniosl zatrzask uslugi";
  EXPECT_FALSE(rdb::detail::throwOnFatalActive()) << "zakres trybu rzucaj przezyl wywolanie fasady";
}
