#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <variant>
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
// C3 (#491) dolozyl plan: compile() + step() nad tym samym cialem slotu, ktore wykonuje usluga.
// Testy grupy embedEnginePlan biegna w katalogu roboczym testu i zostawiaja w nim artefakty
// magazynu, tak jak zrobilby to xretractor.
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

/// Zrodlo tekstowe: jedna liczba na wiersz, jak data.txt w it_untileof_stop.
void writeSource(const std::string &name, const std::vector<int> &values) {
  std::ofstream out(name);
  for (const int value : values)
    out << value << '\n';
}

/// Plan z it_untileof_stop: jedno zrodlo co 1/2 s, jeden SELECT podwajajacy wartosc.
std::string doublingPlan(const std::string &source) {
  return "DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '" + source + "'\nSELECT a*2 STREAM dst FROM src\n";
}

/// Artefakty poprzedniego przebiegu w katalogu roboczym: magazyn liczylby dalej od nich.
void dropArtifacts(const std::string &stream) {
  for (const auto &suffix : {"", ".desc", ".meta", ".shadow"})
    std::filesystem::remove(stream + suffix);
}

int firstValue(const rdb::payload &record) { return std::get<int>(record.getItemVT(0).value()); }

/// Etap i klasa odmowy compile() albo nullopt, gdy plan przeszedl.
std::optional<rdb::embed::Refused> refusalOf(rdb::embed::Engine &engine, const std::string &rql) {
  try {
    engine.compile(rql);
  } catch (const rdb::embed::Refused &refused) {
    return refused;
  }
  return std::nullopt;
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

// Ten sam plan i to samo wejscie, co it_untileof_stop, przez step() zamiast przez petle uslugi.
// Osiem rekordow wejscia daje osiem rekordow wyjscia, zadnego policzonego z all-null wstawionego za
// koniec pliku - dokladnie artefakt `xretractor -u`.
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
  EXPECT_FALSE(engine.failed());
  EXPECT_EQ(engine.slotsDone(), slots);
  EXPECT_EQ(engine.time(), boost::rational<int>(static_cast<int>(slots), 2));
  // Po koncu wejscia step() odpowiada nullopt i niczego nie liczy.
  EXPECT_FALSE(engine.step().has_value());
  EXPECT_EQ(engine.slotsDone(), slots);

  ASSERT_EQ(engine.recordCount("dst"), input.size());
  for (std::size_t i = 0; i < input.size(); ++i)
    EXPECT_EQ(firstValue(engine.record("dst", i)), input[i] * 2);
  EXPECT_EQ(engine.schema("dst")[0].rname, "dst_0");

  // Projekcja gesta: te same wartosci jako double, wierszami.
  const auto block = engine.project("dst", {0}, 2, 3);
  ASSERT_EQ(block.size(), 3U);
  EXPECT_DOUBLE_EQ(block[0], 60.0);
  EXPECT_DOUBLE_EQ(block[2], 100.0);

  EXPECT_THROW((void)engine.record("dst", input.size()), rdb::embed::Refused);
  EXPECT_THROW((void)engine.record("nosuch", 0), rdb::embed::Refused);
  EXPECT_THROW((void)engine.project("dst", {7}, 0, 1), rdb::embed::Refused);

  engine.close();
  EXPECT_FALSE(engine.hasPlan());
  EXPECT_THROW(engine.step(), rdb::embed::Refused);
  engine.close();  // idempotentne
}

// Izolacja Memory Store dla DZIALAJACEGO planu: dwa silniki licza strumien VOLATILE o tej samej nazwie
// z roznych wejsc. Zanim dataModel dostal Memory Store, oba pisalyby do jednej mapy procesu i drugi
// silnik czytalby wartosci pierwszego.
TEST(embedEnginePlan, two_engines_with_the_same_volatile_stream_stay_isolated) {
  writeSource("low.txt", {1, 2, 3, 4});
  writeSource("high.txt", {100, 200, 300, 400});
  const auto plan = [](const std::string &source) {
    return "DECLARE a INTEGER STREAM src, 1 TEXTFILE '" + source + "'\nSELECT a*2 STREAM shared FROM src VOLATILE\n";
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
  EXPECT_FALSE(first.memory().empty()) << "strumien VOLATILE nie trafil do Memory Store pierwszego silnika";
  EXPECT_FALSE(second.memory().empty()) << "strumien VOLATILE nie trafil do Memory Store drugiego silnika";

  // Strumien VOLATILE jest pierscieniem o rozmiarze z kompilatora (tu: 1). Starszy rekord NIE
  // istnieje i nie wolno go udawac cudzym slotem - retainedFrom() to mowi, record() odmawia.
  EXPECT_EQ(first.retainedFrom("shared"), 3U);
  EXPECT_THROW((void)first.record("shared", 0), rdb::embed::Refused);
}

// Powtorne compile() w tym samym katalogu: artefakty poprzedniego planu schodza, TAKZE .desc
// deklaracji (#450). Bez tego zastany `src.desc` z REF na poprzedni plik odrzucilaby kontrola
// magazynu (#265) - a notatnik uruchamia komorki w jednym katalogu wielokrotnie.
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

// Odmowy K2/K3 sa jednym typem z etapem, w kolejnosci, w jakiej plan przez nie przechodzi: parser,
// kompilator, ograniczenia silnika osadzonego, kontrola magazynu; potem wywolania bez planu.
TEST(embedEnginePlan, refusals_carry_their_stage) {
  writeSource("plan_data.txt", {1, 2});
  dropArtifacts("dst");
  rdb::embed::Engine engine;
  using Stage = rdb::embed::Refused::Stage;

  const auto expectStage = [&engine](const std::string &rql, const Stage stage) {
    const auto refused = refusalOf(engine, rql);
    ASSERT_TRUE(refused.has_value()) << "plan przeszedl: " << rql;
    EXPECT_EQ(refused->stage(), stage) << refused->what();
    EXPECT_EQ(refused->refusal().cls, rdb::ErrorClass::Plan) << refused->what();
    EXPECT_FALSE(engine.hasPlan()) << "odrzucony plan nie ma prawa zostac planem silnika";
  };

  expectStage("SELEKT nonsense FROM nowhere\n", Stage::Parse);
  // Nieznany strumien w FROM: na master std::logic_error z qTree::getQuery (usluga: kod EINTR).
  expectStage("DECLARE a INTEGER STREAM src, 1 TEXTFILE 'plan_data.txt'\nSELECT a*2 STREAM dst FROM nosuch\n", Stage::Check);
  expectStage("# nothing but a comment\n", Stage::Check);
  expectStage(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO DUMP -1 TO 1\n", Stage::Check);
  expectStage(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO SYSTEM 'echo no'\n", Stage::Check);
  expectStage(doublingPlan("plan_data.txt") + "ROTATION 'counter.txt'\n", Stage::Check);
  expectStage("DECLARE a INTEGER STREAM src, 1 DEVICE '/dev/null'\nSELECT a*2 STREAM dst FROM src\n", Stage::Check);
  // Kontrola magazynu (checkPlanStorage): zrodlo zlego rodzaju i brak katalogu :STORAGE. Brak pliku
  // zrodla odmowa NIE jest - na master daje rekordy NULL (#346), tak samo w silniku osadzonym.
  expectStage(doublingPlan("/dev/null"), Stage::Check);
  expectStage("STORAGE 'engine_missing_dir'\n" + doublingPlan("plan_data.txt"), Stage::Check);

  const auto request = [](auto &&call) {
    try {
      call();
    } catch (const rdb::embed::Refused &refused) {
      EXPECT_EQ(refused.stage(), Stage::Request) << refused.what();
      EXPECT_EQ(refused.refusal().cls, rdb::ErrorClass::Request) << refused.what();
      return;
    }
    ADD_FAILURE() << "wywolanie bez planu nie zostalo odrzucone";
  };
  request([&] { static_cast<void>(engine.step()); });
  request([&] { static_cast<void>(engine.streams()); });
}

// Katalog magazynu silnika obowiazuje tylko plan bez :STORAGE, jak `[storage] dir` uslugi.
TEST(embedEnginePlan, storage_dir_is_a_default_and_the_directive_wins) {
  writeSource("plan_data.txt", {5, 6, 7});
  std::filesystem::create_directories("engine_default_dir");
  std::filesystem::create_directories("engine_directive_dir");
  dropArtifacts("engine_default_dir/dst");
  dropArtifacts("engine_directive_dir/dst");
  const std::string source = std::filesystem::absolute("plan_data.txt").string();

  {
    rdb::embed::Engine engine("engine_default_dir");
    engine.compile(doublingPlan(source));
    while (engine.step()) {}
    EXPECT_EQ(engine.recordCount("dst"), 3U);
  }
  EXPECT_TRUE(std::filesystem::exists("engine_default_dir/dst.desc"));

  {
    rdb::embed::Engine engine("engine_default_dir");
    engine.compile("STORAGE 'engine_directive_dir'\n" + doublingPlan(source));
    while (engine.step()) {}
  }
  EXPECT_TRUE(std::filesystem::exists("engine_directive_dir/dst.desc"));
}

// Decyzja (a): K4 w slocie wychodzi z step() jako EngineFatal, proces hosta zyje, a plan zostaje
// zatrzasniety - model moze stac w polowie slotu, wiec kolejne step() odmawia liczenia z trescia
// pierwszego bledu. Odczyty dzialaja dalej; compile() zaczyna od nowa.
TEST(embedEnginePlan, fault_in_a_slot_latches_the_plan) {
  writeSource("plan_data.txt", {1, 2, 3});
  dropArtifacts("dst");
  rdb::embed::Engine engine;
  engine.compile(doublingPlan("plan_data.txt"));

  ASSERT_EQ(setenv("RDB_FAULT_ENV_IN_SLOT", "1", 1), 0);
  std::optional<rdb::EngineFatal> first;
  try {
    static_cast<void>(engine.step());
  } catch (const rdb::EngineFatal &fault) {
    first = fault;
  }
  ASSERT_EQ(unsetenv("RDB_FAULT_ENV_IN_SLOT"), 0);

  ASSERT_TRUE(first.has_value()) << "blad K4 w slocie nie wyszedl z step()";
  EXPECT_EQ(first->cls, rdb::ErrorClass::Environment);
  EXPECT_TRUE(engine.failed());
  EXPECT_FALSE(fatalErrorRaised.load()) << "tryb rzucaj podniosl zatrzask uslugi";
  EXPECT_FALSE(rdb::detail::throwOnFatalActive()) << "zakres trybu rzucaj przezyl wywolanie fasady";

  std::optional<rdb::EngineFatal> latched;
  try {
    static_cast<void>(engine.step());
  } catch (const rdb::EngineFatal &fault) {
    latched = fault;
  }
  ASSERT_TRUE(latched.has_value()) << "zatrzasniety plan liczyl dalej";
  EXPECT_EQ(latched->cls, rdb::ErrorClass::Internal);
  EXPECT_NE(latched->message.find("RDB_FAULT_ENV_IN_SLOT"), std::string::npos) << latched->message;
  EXPECT_NO_THROW(static_cast<void>(engine.recordCount("dst")));

  engine.compile(doublingPlan("plan_data.txt"));
  EXPECT_FALSE(engine.failed());
  while (engine.step()) {}
  EXPECT_EQ(engine.recordCount("dst"), 3U);
}
