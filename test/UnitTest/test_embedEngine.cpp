#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/embed/engine.hpp"

// ctest -R '^ut_embedEngine' -V

//
// L2 architektury osadzania: jeden obiekt = jedna instancja silnika w procesie.
//
// Teza fazy 2 brzmi: demon buduje swoj stan raz, notatnik przy kazdej komorce. Do tej pory
// nie dalo sie tego SPRAWDZIC - stan byl globalny i nie istnial obiekt, ktory moglby go
// posiadac. Ten plik jest pierwszym miejscem, w ktorym teza staje sie asercja.
//

namespace {

/// Pole TYPE nie jest danymi: attachStorage() czyta z niego nazwe typu magazynu, wiec
/// "MEMORY" wybiera backend pamieciowy (accessorFactory.cc).
rdb::Descriptor memoryBackedInteger() { return rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"MEMORY", 0, 0, rdb::TYPE}}; }

void writeOneRecord(rdb::storage &stream, const rdb::Descriptor &descriptor, int value) {
  auto *payload = stream.getPayload();
  payload->setNullBitset(std::vector<bool>(descriptor.size(), false));
  payload->setItem(0, value);
  stream.write();
}

}  // namespace

// Sedno: DWA silniki, TA SAMA nazwa strumienia MEMORY, zero wspolnego stanu. Przed faza 2
// pamiec magazynu byla trzema `static` w faccmemory.cc, wiec ten przypadek nie mial jak
// przejsc - i nie mial jak byc napisany, bo nie bylo czym nazwac "silnika".
TEST(embedEngine, two_engines_do_not_share_a_memory_stream) {
  auto descriptor = memoryBackedInteger();

  rdb::embed::Engine first;
  rdb::embed::Engine second;

  auto firstStream  = first.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  auto secondStream = second.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1);
  firstStream->attachDescriptor(&descriptor);
  secondStream->attachDescriptor(&descriptor);
  firstStream->setDisposable(true);

  writeOneRecord(*firstStream, descriptor, 11);

  EXPECT_FALSE(first.memory().empty()) << "zapis nie trafil do sklepu wlasnego silnika";
  EXPECT_TRUE(second.memory().empty()) << "drugi silnik zobaczyl zapis pierwszego";
}

// Engine jest OPCJONALNY: magazyn zbudowany bez niego nadal uzywa instancji domyslnej
// procesu. To jest powod, dla ktorego ta zmiana nic nie psuje serwerowi ani dotychczasowemu
// wiazaniu - zaden z nich o Engine nie wie.
//
// Sklep domyslny jest stanem CALEGO PROCESU, wiec ten przypadek opiera sie na tym, ze plik
// testowy ma wlasna binarke i startuje z pustym sklepem.
TEST(embedEngine, storage_built_without_an_engine_uses_the_process_default) {
  auto descriptor = memoryBackedInteger();

  ASSERT_TRUE(rdb::MemoryStore::processDefault().empty()) << "sklep domyslny nie byl pusty na starcie binarki";

  rdb::embed::Engine engine;
  rdb::storage loose("engine_default", "engine_default", "", "DEFAULT", false, false, -1);
  loose.attachDescriptor(&descriptor);
  loose.setDisposable(true);

  writeOneRecord(loose, descriptor, 37);

  EXPECT_FALSE(rdb::MemoryStore::processDefault().empty()) << "zapis bez silnika nie trafil do sklepu domyslnego";
  EXPECT_TRUE(engine.memory().empty()) << "zapis bez silnika trafil do sklepu silnika";
}
