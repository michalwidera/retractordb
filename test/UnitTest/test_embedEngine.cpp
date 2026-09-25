#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/embed/engine.hpp"
#include "rdb/exceptions.hpp"

// ctest -R '^ut_embedEngine' -V

//
// L2 architektury osadzania: jeden obiekt = jedna instancja silnika w procesie.
//
// Teza fazy 2 brzmi: demon buduje swoj stan raz, notatnik przy kazdej komorce. Do tej pory
// nie dalo sie tego SPRAWDZIC - stan byl globalny i nie istnial obiekt, ktory moglby go
// posiadac. Ten plik jest pierwszym miejscem, w ktorym teza staje sie asercja.
//
// Faza 3 dolozyla plan: compile() + step() nad tym samym cialem slotu, ktore wykonuje demon.
// Testy ponizej (grupa embedEnginePlan) biegna w katalogu roboczym testu i zostawiaja w nim
// artefakty magazynu, tak jak zrobilby to xretractor.
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

/// Zrodlo tekstowe: jedna liczba na wiersz, jak data.txt w it_untileof_stop.
void writeSource(const std::string &name, const std::vector<int> &values) {
  std::ofstream out(name);
  for (const int value : values)
    out << value << '\n';
}

/// Plan z it_untileof_stop: jedno zrodlo co 1/2 s, jeden SELECT podwajajacy wartosc.
std::string doublingPlan(const std::string &source) {
  return "DECLARE a INTEGER STREAM src, 1/2 FILE '" + source + "'\nSELECT a*2 STREAM dst FROM src\n";
}

/// Artefakty poprzedniego przebiegu w katalogu roboczym: magazyn liczylby dalej od nich.
void dropArtifacts(const std::string &stream) {
  for (const auto &suffix : {"", ".desc", ".meta", ".shadow"})
    std::filesystem::remove(stream + suffix);
}

int firstValue(const rdb::payload &record) { return std::get<int>(record.getItemVT(0).value()); }

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

// Faza 3: ten sam plan i to samo wejscie, co it_untileof_stop, przez step() zamiast przez
// petle demona. Osiem rekordow wejscia daje osiem rekordow wyjscia, zadnego policzonego z
// all-null wstawionego za koniec pliku - dokladnie artefakt `xretractor -u`.
TEST(embedEnginePlan, compile_step_and_read_the_result) {
  const std::vector<int> input{10, 20, 30, 40, 50, 60, 70, 80};
  writeSource("plan_data.txt", input);
  dropArtifacts("dst");

  rdb::embed::Engine engine;
  ASSERT_FALSE(engine.hasPlan());
  engine.compile(doublingPlan("plan_data.txt"));
  ASSERT_TRUE(engine.hasPlan());
  EXPECT_EQ(engine.streams(), (std::vector<std::string>{"src", "dst"}));
  EXPECT_TRUE(engine.isDeclared("src"));
  EXPECT_FALSE(engine.isDeclared("dst"));

  std::uint64_t slots = 0;
  while (const auto slot = engine.step()) {
    EXPECT_EQ(*slot, slots);
    ++slots;
  }
  EXPECT_TRUE(engine.endOfInput());
  EXPECT_EQ(engine.slotsDone(), slots);
  EXPECT_EQ(engine.time(), boost::rational<int>(static_cast<int>(slots), 2));
  // Po koncu wejscia step() odpowiada nullopt i niczego nie liczy.
  EXPECT_FALSE(engine.step().has_value());
  EXPECT_EQ(engine.slotsDone(), slots);

  ASSERT_EQ(engine.recordCount("dst"), input.size());
  for (std::size_t i = 0; i < input.size(); ++i)
    EXPECT_EQ(firstValue(engine.record("dst", i)), input[i] * 2);
  EXPECT_EQ(engine.schema("dst").front().rname, "dst_0");

  // Projekcja gesta: te same wartosci jako double, wierszami.
  const auto block = engine.project("dst", {0}, 2, 3);
  ASSERT_EQ(block.size(), 3U);
  EXPECT_DOUBLE_EQ(block[0], 60.0);
  EXPECT_DOUBLE_EQ(block[2], 100.0);

  EXPECT_THROW((void)engine.record("dst", input.size()), rdb::ConfigError);
  EXPECT_THROW((void)engine.record("nosuch", 0), rdb::ConfigError);
  EXPECT_THROW((void)engine.project("dst", {7}, 0, 1), rdb::ConfigError);

  engine.close();
  EXPECT_FALSE(engine.hasPlan());
  EXPECT_THROW(engine.step(), rdb::ConfigError);
  engine.close();  // idempotentne
}

// Izolacja z fazy 2 dla DZIALAJACEGO planu: dwa silniki licza strumien VOLATILE o tej samej
// nazwie z roznych wejsc. Zanim dataModel dostal sklep MEMORY, oba pisalyby do jednej mapy
// procesu i drugi silnik czytalby wartosci pierwszego.
TEST(embedEnginePlan, two_engines_with_the_same_volatile_stream_stay_isolated) {
  writeSource("low.txt", {1, 2, 3, 4});
  writeSource("high.txt", {100, 200, 300, 400});
  const auto plan = [](const std::string &source) {
    return "DECLARE a INTEGER STREAM src, 1 FILE '" + source + "'\nSELECT a*2 STREAM shared FROM src VOLATILE\n";
  };

  rdb::embed::Engine first;
  rdb::embed::Engine second;
  first.compile(plan("low.txt"));
  second.compile(plan("high.txt"));
  while (first.step()) {}
  while (second.step()) {}

  ASSERT_EQ(first.recordCount("shared"), 4U);
  ASSERT_EQ(second.recordCount("shared"), 4U);
  EXPECT_EQ(firstValue(first.record("shared", 3)), 8);
  EXPECT_EQ(firstValue(second.record("shared", 3)), 800);
  EXPECT_FALSE(first.memory().empty()) << "strumien VOLATILE nie trafil do sklepu pierwszego silnika";
  EXPECT_FALSE(second.memory().empty()) << "strumien VOLATILE nie trafil do sklepu drugiego silnika";

  // Strumien VOLATILE jest pierscieniem o rozmiarze z kompilatora (tu: 1). Starszy rekord
  // NIE istnieje i nie wolno go udawac cudzym slotem - retainedFrom() to mowi, record() odmawia.
  EXPECT_EQ(first.retainedFrom("shared"), 3U);
  EXPECT_THROW((void)first.record("shared", 0), rdb::ConfigError);
}

// Powtorne compile() w tym samym katalogu: artefakty poprzedniego planu schodza, TAKZE .desc
// deklaracji. Bez tego zastany `src.desc` z REF na poprzedni plik kazalby czytac stare
// zrodlo mimo nowego FILE - a notatnik uruchamia komorki w jednym katalogu wielokrotnie.
TEST(embedEnginePlan, recompile_in_the_same_directory_reads_the_new_source) {
  writeSource("first_source.txt", {1, 2, 3});
  writeSource("second_source.txt", {10, 20, 30});
  dropArtifacts("dst");

  rdb::embed::Engine engine;
  engine.compile(doublingPlan("first_source.txt"));
  while (engine.step()) {}
  EXPECT_EQ(firstValue(engine.record("dst", 2)), 6);

  engine.compile(doublingPlan("second_source.txt"));
  EXPECT_EQ(engine.slotsDone(), 0U);
  while (engine.step()) {}
  ASSERT_EQ(engine.recordCount("dst"), 3U) << "magazyn SELECT-a liczyl dalej od starych rekordow";
  EXPECT_EQ(firstValue(engine.record("dst", 2)), 60) << "deklaracja czytala poprzedni plik zrodlowy";
}

// Bledy planu przychodza jako TYPY, w kolejnosci, w jakiej plan przez nie przechodzi:
// parser, kompilator, ograniczenia silnika osadzonego.
TEST(embedEnginePlan, plan_errors_are_typed) {
  writeSource("plan_data.txt", {1, 2});
  rdb::embed::Engine engine;

  EXPECT_THROW(engine.compile("SELEKT nonsense FROM nowhere\n"), rdb::embed::SyntaxError);
  EXPECT_THROW(engine.compile("SELECT a*2 STREAM dst FROM nosuch\n"), rdb::embed::CompileError);
  EXPECT_THROW(engine.compile("# nothing but a comment\n"), rdb::embed::CompileError);
  EXPECT_THROW(engine.compile(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO DUMP -1 TO 1\n"),
               rdb::embed::CompileError);
  EXPECT_THROW(engine.compile(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO SYSTEM 'echo no'\n"),
               rdb::embed::CompileError);
  EXPECT_THROW(engine.compile(doublingPlan("plan_data.txt") + "ROTATION 'counter.txt'\n"), rdb::embed::CompileError);
  EXPECT_FALSE(engine.hasPlan()) << "odrzucony plan nie ma prawa zostac planem silnika";

  // Oba typy sa ConfigError: wejscie jest zle, silnik jest caly.
  EXPECT_THROW(engine.compile("SELEKT\n"), rdb::ConfigError);
  EXPECT_THROW(engine.step(), rdb::ConfigError);
  EXPECT_THROW((void)engine.streams(), rdb::ConfigError);
}

// Katalog magazynu silnika obowiazuje tylko plan bez :STORAGE, jak `[storage] dir` demona;
// a nieistniejacy katalog jest ConfigError z fazy 1, nie martwym procesem.
TEST(embedEnginePlan, storage_dir_is_a_default_and_the_directive_wins) {
  writeSource("plan_data.txt", {5, 6, 7});
  std::filesystem::create_directories("engine_default_dir");
  std::filesystem::create_directories("engine_directive_dir");
  dropArtifacts("engine_default_dir/dst");
  dropArtifacts("engine_directive_dir/dst");

  {
    rdb::embed::Engine engine("engine_default_dir");
    engine.compile(doublingPlan("plan_data.txt"));
    while (engine.step()) {}
    EXPECT_EQ(engine.recordCount("dst"), 3U);
  }
  EXPECT_TRUE(std::filesystem::exists("engine_default_dir/dst.desc"));

  {
    rdb::embed::Engine engine("engine_default_dir");
    engine.compile("STORAGE 'engine_directive_dir'\n" + doublingPlan("plan_data.txt"));
    while (engine.step()) {}
  }
  EXPECT_TRUE(std::filesystem::exists("engine_directive_dir/dst.desc"));

  rdb::embed::Engine missing("engine_missing_dir");
  EXPECT_THROW(missing.compile(doublingPlan("plan_data.txt")), rdb::ConfigError);
  EXPECT_FALSE(missing.hasPlan());
}
