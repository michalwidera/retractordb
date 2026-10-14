#include <gtest/gtest.h>

#include <cerrno>
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
#include "rdbResult.hpp"

// ctest -R '^ut_embedEngine' -V

//
// Fasada osadzenia (#446, C2): jeden obiekt = jedna instancja silnika w procesie.
//
// Demon buduje swoj stan raz, notatnik przy kazdej komorce. Do fazy 2 nie dalo sie tego SPRAWDZIC -
// stan byl globalny i nie istnial obiekt, ktory moglby go posiadac. Ten plik jest pierwszym
// miejscem, w ktorym ta teza staje sie asercja.
//
// C3 (#491) dolozyl plan: compile() + step() nad tym samym cialem slotu, ktore wykonuje usluga.
// Testy grupy embedEnginePlan czytaja zrodla z katalogu roboczego, a wyniki trzymaja
// w osobnych katalogach silnikow (#450).
//

namespace {

/// Pole TYPE nie jest danymi: attachStorage() czyta z niego nazwe typu magazynu, wiec "MEMORY"
/// wybiera backend pamieciowy (accessorFactory.cc).
rdb::Descriptor memoryBackedInteger() { return rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"MEMORY", 0, 0, rdb::TYPE}}; }

void writeOneRecord(rdb::storage &stream, const rdb::Descriptor &descriptor, int value) {
  auto *payload = stream.getPayload();
  payload->setNullBitset(std::vector<bool>(descriptor.size(), false));
  payload->setItem(0, value);
  ASSERT_EQ(rdbtest::ok(stream.write()), rdb::WriteStatus::Ok);
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

int firstValue(const rdb::payload &record) { return std::get<int>(record.getItemVT(0).value()); }

/// Kategoria i przyczyna odmowy compile() albo nullopt, gdy plan przeszedl.
std::optional<rdb::Error> refusalOf(rdb::embed::Engine &engine, const std::string &rql) {
  const auto compiled = engine.compile(rql);
  if (!compiled) return compiled.error();
  return std::nullopt;
}

}  // namespace

// Sedno: DWA silniki, TA SAMA nazwa strumienia MEMORY, zero wspolnego stanu. Przed faza 2 pamiec
// magazynu MEMORY byla mapa `static` w faccmemory.cc, wiec ten przypadek nie mial jak przejsc.
TEST(embedEngine, two_engines_do_not_share_a_memory_stream) {
  auto descriptor = memoryBackedInteger();

  auto firstOwner            = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &first  = *firstOwner;
  auto secondOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &second = *secondOwner;

  auto firstStream  = rdbtest::ok(first.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1));
  auto secondStream = rdbtest::ok(second.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1));
  ASSERT_RDB_OK(firstStream->attachDescriptor(&descriptor));
  ASSERT_RDB_OK(secondStream->attachDescriptor(&descriptor));
  firstStream->setDisposable(true);

  writeOneRecord(*firstStream, descriptor, 11);

  // Nowy magazyn czyta liczbe rekordow z akcesora przy dolaczeniu deskryptora, wiec widzi dokladnie
  // kubelek Memory Store swojego silnika.
  auto firstProbe  = rdbtest::ok(first.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1));
  auto secondProbe = rdbtest::ok(second.openStorage("engine_iso", "engine_iso", "", "DEFAULT", false, false, -1));
  ASSERT_RDB_OK(firstProbe->attachDescriptor(&descriptor));
  ASSERT_RDB_OK(secondProbe->attachDescriptor(&descriptor));

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

  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  auto looseOwner     = rdbtest::ok(rdb::storage::create("engine_default", "engine_default", "", "DEFAULT", false, false, -1));
  rdb::storage &loose = *looseOwner;
  ASSERT_RDB_OK(loose.attachDescriptor(&descriptor));
  loose.setDisposable(true);

  writeOneRecord(loose, descriptor, 37);

  EXPECT_FALSE(rdb::MemoryStore::processDefault().empty()) << "zapis bez silnika nie trafil do domyslnego Memory Store";
  EXPECT_TRUE(engine.memory().empty()) << "zapis bez silnika trafil do Memory Store silnika";
}

// Pusty qryID jest odrzucany na granicy API: Errc::Config z przyczyna, zanim powstanie magazyn.
// Odmowa nie podnosi zatrzasku uslugi ani nie konczy procesu hosta.
TEST(embedEngine, open_storage_failure_is_engine_fatal_not_process_exit) {
  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  const auto opened          = engine.openStorage("", "engine_fatal", "", "DEFAULT", false, false, -1);
  ASSERT_RDB_ERROR(opened, rdb::Errc::Config, "qryID");
  EXPECT_FALSE(fatalErrorRaised.load()) << "odmowa API podniosla zatrzask uslugi";
}

// Ten sam plan i to samo wejscie, co it_untileof_stop, przez step() zamiast przez petle uslugi.
// Osiem rekordow wejscia daje osiem rekordow wyjscia, zadnego policzonego z all-null wstawionego za
// koniec pliku - dokladnie artefakt `xretractor -u`.
TEST(embedEnginePlan, compile_step_and_read_the_result) {
  const std::vector<int> input{10, 20, 30, 40, 50, 60, 70, 80};
  writeSource("plan_data.txt", input);

  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  ASSERT_FALSE(engine.hasPlan());
  ASSERT_RDB_OK(engine.compile(doublingPlan("plan_data.txt")));
  ASSERT_TRUE(engine.hasPlan());
  EXPECT_EQ(rdbtest::ok(engine.streams()), (std::vector<std::string>{"src", "dst"}));
  EXPECT_TRUE(rdbtest::ok(engine.isDeclared("src")));
  EXPECT_FALSE(rdbtest::ok(engine.isDeclared("dst")));

  std::uint64_t slots = 0;
  while (const auto slot = rdbtest::ok(engine.step())) {
    EXPECT_EQ(*slot, slots);
    ++slots;
  }
  EXPECT_TRUE(engine.endOfInput());
  EXPECT_FALSE(engine.failed());
  EXPECT_EQ(engine.slotsDone(), slots);
  EXPECT_EQ(engine.time(), boost::rational<int>(static_cast<int>(slots), 2));
  // Po koncu wejscia step() odpowiada nullopt i niczego nie liczy.
  EXPECT_FALSE(rdbtest::ok(engine.step()).has_value());
  EXPECT_EQ(engine.slotsDone(), slots);

  ASSERT_EQ(rdbtest::ok(engine.recordCount("dst")), input.size());
  for (std::size_t i = 0; i < input.size(); ++i)
    EXPECT_EQ(firstValue(rdbtest::ok(engine.record("dst", i))), input[i] * 2);
  EXPECT_EQ((*rdbtest::ok(engine.schema("dst")))[0].rname, "dst_0");

  // Projekcja gesta: te same wartosci jako double, wierszami.
  const auto block = rdbtest::ok(engine.project("dst", {0}, 2, 3));
  ASSERT_EQ(block.size(), 3U);
  EXPECT_DOUBLE_EQ(block[0], 60.0);
  EXPECT_DOUBLE_EQ(block[2], 100.0);

  EXPECT_RDB_ERROR(engine.record("dst", input.size()), rdb::Errc::Config);
  EXPECT_RDB_ERROR(engine.record("nosuch", 0), rdb::Errc::Config);
  EXPECT_RDB_ERROR(engine.project("dst", {7}, 0, 1), rdb::Errc::Config);

  engine.close();
  EXPECT_FALSE(engine.hasPlan());
  EXPECT_RDB_ERROR(engine.step(), rdb::Errc::Config);
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

  auto firstOwner            = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &first  = *firstOwner;
  auto secondOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &second = *secondOwner;
  ASSERT_RDB_OK(first.compile(plan("low.txt")));
  ASSERT_RDB_OK(second.compile(plan("high.txt")));
  while (rdbtest::ok(first.step())) {}
  while (rdbtest::ok(second.step())) {}

  ASSERT_EQ(rdbtest::ok(first.recordCount("shared")), 4U);
  ASSERT_EQ(rdbtest::ok(second.recordCount("shared")), 4U);
  EXPECT_EQ(firstValue(rdbtest::ok(first.record("shared", 3))), 8);
  EXPECT_EQ(firstValue(rdbtest::ok(second.record("shared", 3))), 800);
  EXPECT_FALSE(first.memory().empty()) << "strumien VOLATILE nie trafil do Memory Store pierwszego silnika";
  EXPECT_FALSE(second.memory().empty()) << "strumien VOLATILE nie trafil do Memory Store drugiego silnika";

  // Strumien VOLATILE jest pierscieniem o rozmiarze z kompilatora (tu: 1). Starszy rekord NIE
  // istnieje i nie wolno go udawac cudzym slotem - retainedFrom() to mowi, record() odmawia.
  EXPECT_EQ(rdbtest::ok(first.retainedFrom("shared")), 3U);
  EXPECT_RDB_ERROR(first.record("shared", 0), rdb::Errc::Config);
}

// Powtorne compile() w tym samym katalogu: artefakty poprzedniego planu schodza, TAKZE .desc
// deklaracji (#450). Bez tego zastany `src.desc` z REF na poprzedni plik odrzucilaby kontrola
// magazynu (#265) - a notatnik uruchamia komorki w jednym katalogu wielokrotnie.
TEST(embedEnginePlan, recompile_in_the_same_directory_reads_the_new_source) {
  writeSource("first_source.txt", {1, 2, 3});
  writeSource("second_source.txt", {10, 20, 30});

  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  ASSERT_RDB_OK(engine.compile(doublingPlan("first_source.txt")));
  while (rdbtest::ok(engine.step())) {}
  EXPECT_EQ(firstValue(rdbtest::ok(engine.record("dst", 2))), 6);

  ASSERT_RDB_OK(engine.compile(doublingPlan("second_source.txt")));
  EXPECT_EQ(engine.slotsDone(), 0U);
  while (rdbtest::ok(engine.step())) {}
  ASSERT_EQ(rdbtest::ok(engine.recordCount("dst")), 3U) << "magazyn SELECT-a liczyl dalej od starych rekordow";
  EXPECT_EQ(firstValue(rdbtest::ok(engine.record("dst", 2))), 60) << "deklaracja czytala poprzedni plik zrodlowy";
}

// Odmowy planu i API maja kategorie wedlug miejsca wykrycia: parser, kompilator,
// ograniczenia osadzenia i magazyn. Brak planu jest odmowa wywolania API.
TEST(embedEnginePlan, refusals_carry_their_stage) {
  writeSource("plan_data.txt", {1, 2});
  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;

  const auto expectCategory = [&engine](const std::string &rql, const rdb::Errc code) {
    const auto refused = refusalOf(engine, rql);
    ASSERT_TRUE(refused.has_value()) << "plan przeszedl: " << rql;
    EXPECT_EQ(refused->code(), code) << refused->message();
    EXPECT_TRUE(rdb::isConfigFamily(refused->code())) << refused->message();
    EXPECT_FALSE(engine.hasPlan()) << "odrzucony plan nie ma prawa zostac planem silnika";
  };

  expectCategory("SELEKT nonsense FROM nowhere\n", rdb::Errc::Syntax);
  expectCategory("DECLARE a INTEGER STREAM src, 1 TEXTFILE 'plan_data.txt'\nSELECT a*2 STREAM dst FROM nosuch\n",
                 rdb::Errc::Compile);
  expectCategory("# nothing but a comment\n", rdb::Errc::Compile);
  expectCategory(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO DUMP -1 TO 1\n", rdb::Errc::Compile);
  expectCategory(doublingPlan("plan_data.txt") + "RULE r ON dst WHEN dst[0] > 100 DO SYSTEM 'echo no'\n", rdb::Errc::Compile);
  expectCategory(doublingPlan("plan_data.txt") + "ROTATION 'counter.txt'\n", rdb::Errc::Compile);
  expectCategory("DECLARE a INTEGER STREAM src, 1 DEVICE '/dev/null'\nSELECT a*2 STREAM dst FROM src\n", rdb::Errc::Compile);
  // Zrodlo zlego rodzaju i brak katalogu :STORAGE to bledy planu. Brak pliku zrodla
  // pozostaje Errc::IO/ENOENT, sprawdzany osobno ponizej.
  expectCategory(doublingPlan("/dev/null"), rdb::Errc::Compile);
  expectCategory("STORAGE 'engine_missing_dir'\n" + doublingPlan("plan_data.txt"), rdb::Errc::Compile);

  EXPECT_RDB_ERROR(engine.step(), rdb::Errc::Config, "no plan");
  EXPECT_RDB_ERROR(engine.streams(), rdb::Errc::Config, "no plan");
}

// Katalog magazynu silnika obowiazuje tylko plan bez :STORAGE, jak `[storage] dir` uslugi.
TEST(embedEnginePlan, storage_dir_is_a_default_and_the_directive_wins) {
  writeSource("plan_data.txt", {5, 6, 7});
  auto defaultEngineOwner            = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &defaultOwner   = *defaultEngineOwner;
  auto directiveEngineOwner          = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &directiveOwner = *directiveEngineOwner;
  const std::string defaultDir       = defaultOwner.storageDir();
  const std::string directiveDir     = directiveOwner.storageDir();
  const std::string source           = std::filesystem::absolute("plan_data.txt").string();

  {
    auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create(defaultDir));
    rdb::embed::Engine &engine = *engineOwner;
    ASSERT_RDB_OK(engine.compile(doublingPlan(source)));
    while (rdbtest::ok(engine.step())) {}
    EXPECT_EQ(rdbtest::ok(engine.recordCount("dst")), 3U);
  }
  EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(defaultDir) / "dst.desc"));

  {
    auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create(defaultDir));
    rdb::embed::Engine &engine = *engineOwner;
    ASSERT_RDB_OK(engine.compile("STORAGE '" + directiveDir + "'\n" + doublingPlan(source)));
    while (rdbtest::ok(engine.step())) {}
  }
  EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(directiveDir) / "dst.desc"));
}

// Blokada dotyczy obiektu katalogu, takze dla dyrektywy STORAGE i aliasu sciezki.
// Odmowa drugiego silnika nie zmienia pierwszego planu; close() oddaje katalog.
TEST(embedEnginePlan, directory_lock_covers_storage_directives_and_aliases) {
  writeSource("ownership_source.txt", {3, 4});
  auto firstOwner             = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &first   = *firstOwner;
  const std::string directory = first.storageDir();
  const std::string alias     = directory + "/../" + std::filesystem::path(directory).filename().string();
  auto secondOwner            = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &second  = *secondOwner;
  const auto plan             = doublingPlan("ownership_source.txt");
  const auto alternate        = "STORAGE '" + alias + "'\n" + plan;
  ASSERT_RDB_OK(first.compile(plan));
  const auto failure = refusalOf(second, alternate);
  ASSERT_TRUE(failure.has_value());
  EXPECT_EQ(failure->code(), rdb::Errc::Config);
  EXPECT_EQ(failure->err(), EBUSY);
  EXPECT_FALSE(second.hasPlan());
  while (rdbtest::ok(first.step())) {}
  EXPECT_EQ(firstValue(rdbtest::ok(first.record("dst", 1))), 8);
  first.close();
  ASSERT_RDB_OK(second.compile(alternate));
  while (rdbtest::ok(second.step())) {}
  EXPECT_EQ(firstValue(rdbtest::ok(second.record("dst", 1))), 8);
}

// Katalog zastany bez znacznika nie daje prawa do nadpisania .desc ani danych.
TEST(embedEnginePlan, foreign_artifacts_are_preserved) {
  writeSource("ownership_source.txt", {3, 4});
  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  const auto descriptor      = std::filesystem::path(engine.storageDir()) / "src.desc";
  std::ofstream(descriptor) << "{ a DOUBLE[1] }\n";
  const auto failure = refusalOf(engine, doublingPlan("ownership_source.txt"));
  ASSERT_TRUE(failure.has_value());
  EXPECT_EQ(failure->code(), rdb::Errc::Compile);
  EXPECT_FALSE(engine.hasPlan());
  std::ifstream input(descriptor);
  std::string content;
  std::getline(input, content);
  EXPECT_EQ(content, "{ a DOUBLE[1] }");
  EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(engine.storageDir()) / ".retractordb-embedded"));
  std::filesystem::remove(descriptor);
  ASSERT_RDB_OK(engine.compile(doublingPlan("ownership_source.txt")));
  while (rdbtest::ok(engine.step())) {}
  EXPECT_EQ(firstValue(rdbtest::ok(engine.record("dst", 1))), 8);
}

// Serwis daje przy braku zrodla rekordy NULL (#346); silnik osadzony zwraca Errc::IO
// z errno (#451), a po poprawionym planie pracuje dalej.
TEST(embedEnginePlan, missing_source_is_refused_unlike_the_service) {
  writeSource("ownership_source.txt", {3, 4});
  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  ASSERT_RDB_OK(engine.compile(doublingPlan("ownership_source.txt")));
  while (rdbtest::ok(engine.step())) {}
  for (const std::string source : {"engine_missing_source.txt", "engine_missing_dir/source.txt"}) {
    const auto refused = refusalOf(engine, doublingPlan(source));
    ASSERT_TRUE(refused.has_value()) << source;
    EXPECT_EQ(refused->code(), rdb::Errc::IO);
    EXPECT_EQ(refused->err(), ENOENT) << refused->message();
    EXPECT_NE(refused->message().find("source file does not exist"), std::string::npos) << refused->message();
    EXPECT_FALSE(engine.hasPlan());
  }
  ASSERT_RDB_OK(engine.compile(doublingPlan("ownership_source.txt")));
  ASSERT_TRUE(rdbtest::ok(engine.step()).has_value());
  EXPECT_EQ(firstValue(rdbtest::ok(engine.record("dst", 0))), 6);
}

// Blad w slocie wraca wartoscia i zatrzaskuje plan. Kolejne step() zachowuje
// kategorie, errno i przyczyne pierwszego bledu; odczyty dzialaja, compile() zaczyna od nowa.
TEST(embedEnginePlan, fault_in_a_slot_latches_the_plan) {
  writeSource("plan_data.txt", {1, 2, 3});
  auto engineOwner           = rdbtest::ok(rdb::embed::Engine::create());
  rdb::embed::Engine &engine = *engineOwner;
  struct Fault {
    const char *hook;
    const char *value;
    rdb::Errc code;
    int err;
  };
  for (const Fault &fault :
       {Fault{"RDB_FAULT_ENV_IN_SLOT", "1", rdb::Errc::IO, EIO}, Fault{"RDB_FAULT_ERROR_IN_SLOT", "0", rdb::Errc::Logic, 0}}) {
    SCOPED_TRACE(fault.hook);
    ASSERT_RDB_OK(engine.compile(doublingPlan("plan_data.txt")));

    ASSERT_EQ(setenv(fault.hook, fault.value, 1), 0);
    const auto first = engine.step();
    ASSERT_EQ(unsetenv(fault.hook), 0);

    ASSERT_RDB_ERROR(first, fault.code, fault.hook);
    EXPECT_EQ(first.error().err(), fault.err);
    EXPECT_TRUE(engine.failed());
    EXPECT_FALSE(fatalErrorRaised.load()) << "blad zwrocony przez Result podniosl zatrzask uslugi";

    const auto latched = engine.step();
    ASSERT_RDB_ERROR(latched, fault.code, fault.hook);
    EXPECT_EQ(latched.error().err(), fault.err);
    EXPECT_RDB_OK(engine.recordCount("dst"));

    ASSERT_RDB_OK(engine.compile(doublingPlan("plan_data.txt")));
    EXPECT_FALSE(engine.failed());
    while (rdbtest::ok(engine.step())) {}
    EXPECT_EQ(rdbtest::ok(engine.recordCount("dst")), 3U);
  }
}
