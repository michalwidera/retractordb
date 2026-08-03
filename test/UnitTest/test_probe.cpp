#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"
#include "rdb/probe.hpp"
#include "retractor/lib/qTree.hpp"

// ctest -R '^ut_probe' -V

//
// Testy sond pomiarowych (rdb/probe.hpp).
//
// Każdy test uruchamia się w OBU wariantach kompilacji i w obu coś sprawdza. To celowe:
// wariant bez sond ma równie mocny kontrakt co wariant z sondami — wywołanie sondy nie
// może mieć ŻADNEGO efektu. Stąd mnożniki `workOn`/`materializeOn`: oczekiwana liczba to
// albo wartość wynikająca z geometrii zdarzeń (build z sondą), albo zero (build bez sondy).
// Testy pomijane przez GTEST_SKIP nie pilnowałyby tej drugiej połowy kontraktu, a to
// właśnie ona decyduje, czy sonda nie przecieka do produkcji.
//
namespace {

constexpr unsigned long long workOn        = rdb_probe_work ? 1 : 0;
constexpr unsigned long long materializeOn = rdb_probe_materialize ? 1 : 0;

/// Zmienna środowiskowa przywracana po teście — sondy czytają je przy uzbrajaniu,
/// więc przeciek między testami zmieniałby wynik następnego.
class envGuard {
 public:
  envGuard(const char *name, const char *value) : name_(name) {
    if (const char *previous = std::getenv(name); previous != nullptr) {
      had_      = true;
      previous_ = previous;
    }
    if (value)
      ::setenv(name, value, 1);
    else
      ::unsetenv(name);
  }
  ~envGuard() {
    if (had_)
      ::setenv(name_.c_str(), previous_.c_str(), 1);
    else
      ::unsetenv(name_.c_str());
  }
  envGuard(const envGuard &)            = delete;
  envGuard &operator=(const envGuard &) = delete;

 private:
  std::string name_;
  std::string previous_;
  bool had_ = false;
};

field makeField(const std::string &name, int tokens) {
  std::list<token> program;
  for (int i = 0; i < tokens; ++i)
    program.push_back(token(PUSH_STREAM, std::string("src")));
  return field(rdb::rField(name, sizeof(int), 1, rdb::INTEGER), program);
}

query makeQuery(const std::string &id, bool substrat, int fromTokens, const std::vector<int> &fieldTokens) {
  query q(boost::rational<int>(1, 1), id);
  q.isSubstrat = substrat;
  for (int i = 0; i < fromTokens; ++i)
    q.lProgram.push_back(token(PUSH_STREAM, std::string("src")));
  int fieldNo = 0;
  for (const auto tokens : fieldTokens)
    q.lSchema.push_back(makeField("f" + std::to_string(fieldNo++), tokens));
  return q;
}

std::vector<std::string> readLines(const std::filesystem::path &file) {
  std::vector<std::string> lines;
  std::ifstream in(file);
  for (std::string line; std::getline(in, line);)
    lines.push_back(line);
  return lines;
}

}  // namespace

//
// ─── E4: praca na slot ──────────────────────────────────────────────────────────
//

TEST(probeWork, counters_accumulate_per_event) {
  rdb::probe::workReset();

  // Okna o RÓŻNEJ długości: praca okna rośnie z liczbą odwiedzonych elementów, a nie
  // z liczbą okien — to jest cała teza sondy E4 (w planie oba okna to jeden token).
  rdb::probe::onAgseWindow(4);
  rdb::probe::onAgseWindow(6);
  rdb::probe::onAgseRead();
  rdb::probe::onAgseRead();
  rdb::probe::onEval(7);
  rdb::probe::onHashPick();
  rdb::probe::onAddMerge();

  const auto w = rdb::probe::workReport();
  EXPECT_EQ(w.agseWindows, 2 * workOn);
  EXPECT_EQ(w.agseElements, 10 * workOn);
  EXPECT_EQ(w.agseReads, 2 * workOn);
  EXPECT_EQ(w.evalCalls, 1 * workOn);
  EXPECT_EQ(w.evalTokens, 7 * workOn);
  EXPECT_EQ(w.hashPicks, 1 * workOn);
  EXPECT_EQ(w.addMerges, 1 * workOn);
}

TEST(probeWork, counters_accumulate_across_slots) {
  // Liczniki są procesowe i sumują się przez cały przebieg — analiza dzieli je dopiero
  // przez liczbę slotów. Gubienie zdarzeń zaniżałoby pracę, a to jest dokładnie ta klasa
  // błędu, przez którą upadł model kosztu K20 etap 1.
  rdb::probe::workReset();
  for (int slot = 0; slot < 3; ++slot) {
    rdb::probe::onAgseWindow(5);
    rdb::probe::onEval(2);
  }

  const auto w = rdb::probe::workReport();
  EXPECT_EQ(w.agseWindows, 3 * workOn);
  EXPECT_EQ(w.agseElements, 15 * workOn);
  EXPECT_EQ(w.evalCalls, 3 * workOn);
  EXPECT_EQ(w.evalTokens, 6 * workOn);
}

TEST(probeWork, reset_zeroes_every_counter) {
  rdb::probe::onAgseWindow(3);
  rdb::probe::onAgseRead();
  rdb::probe::onEval(1);
  rdb::probe::onHashPick();
  rdb::probe::onAddMerge();

  rdb::probe::workReset();

  const auto w = rdb::probe::workReport();
  EXPECT_EQ(w.agseWindows, 0u);
  EXPECT_EQ(w.agseElements, 0u);
  EXPECT_EQ(w.agseReads, 0u);
  EXPECT_EQ(w.evalCalls, 0u);
  EXPECT_EQ(w.evalTokens, 0u);
  EXPECT_EQ(w.hashPicks, 0u);
  EXPECT_EQ(w.addMerges, 0u);
}

TEST(probeWork, report_is_a_snapshot_not_a_view) {
  // Raport musi być kopią: kampania zdejmuje odczyt na końcu przebiegu i porównuje go
  // z odczytem wcześniejszym. Referencja dawałaby dwie identyczne liczby.
  rdb::probe::workReset();
  rdb::probe::onAgseWindow(2);
  const auto before = rdb::probe::workReport();

  rdb::probe::onAgseWindow(2);
  const auto after = rdb::probe::workReport();

  EXPECT_EQ(before.agseElements, 2 * workOn);
  EXPECT_EQ(after.agseElements, 4 * workOn);
}

//
// ─── K6: objętość materializacji ────────────────────────────────────────────────
//

TEST(probeMaterialization, persistent_and_memory_storage_counted_separately) {
  // Bez tego podziału plan z SUBSTRAT 'memory' raportowałby objętość, której nigdy nie
  // zapisał na dysk — a to jest liczba, którą artykuł nazywa „objętością materializacji".
  rdb::probe::materializationReset();
  rdb::probe::onMaterializedAppend(false, 100);
  rdb::probe::onMaterializedAppend(false, 40);
  rdb::probe::onMaterializedAppend(true, 7);

  const auto m = rdb::probe::materializationReport();
  EXPECT_EQ(m.appends, 2 * materializeOn);
  EXPECT_EQ(m.bytes, 140 * materializeOn);
  EXPECT_EQ(m.memoryAppends, 1 * materializeOn);
  EXPECT_EQ(m.memoryBytes, 7 * materializeOn);
}

TEST(probeMaterialization, overwrite_costs_a_write_but_no_volume) {
  // Nadpisanie nie zwiększa objętości magazynu, tylko koszt zapisu — dlatego nie ma
  // parametru z liczbą bajtów i nie wolno mu ruszyć licznika `bytes`.
  rdb::probe::materializationReset();
  rdb::probe::onMaterializedAppend(false, 50);
  rdb::probe::onMaterializedOverwrite(false);
  rdb::probe::onMaterializedOverwrite(true);

  const auto m = rdb::probe::materializationReport();
  EXPECT_EQ(m.appends, 1 * materializeOn);
  EXPECT_EQ(m.bytes, 50 * materializeOn);
  EXPECT_EQ(m.overwrites, 1 * materializeOn);
  EXPECT_EQ(m.memoryOverwrites, 1 * materializeOn);
  EXPECT_EQ(m.memoryBytes, 0u);
}

TEST(probeMaterialization, reset_zeroes_every_counter) {
  rdb::probe::onMaterializedAppend(false, 10);
  rdb::probe::onMaterializedAppend(true, 10);
  rdb::probe::onMaterializedOverwrite(false);

  rdb::probe::materializationReset();

  const auto m = rdb::probe::materializationReport();
  EXPECT_EQ(m.appends, 0u);
  EXPECT_EQ(m.overwrites, 0u);
  EXPECT_EQ(m.bytes, 0u);
  EXPECT_EQ(m.memoryAppends, 0u);
  EXPECT_EQ(m.memoryOverwrites, 0u);
  EXPECT_EQ(m.memoryBytes, 0u);
}

//
// ─── E3: kształt planu ──────────────────────────────────────────────────────────
//
// Te funkcje są czystymi przekształceniami danych, więc działają identycznie w obu
// wariantach kompilacji — mnożników tu nie ma. Mierzone na PRAWDZIWYCH typach planu
// (qTree/query/field), bo to z nimi szablon jest instancjonowany w compiler.cpp.
//

TEST(probePlanShape, directives_are_not_part_of_the_plan) {
  qTree plan;
  plan.push_back(makeQuery("public", false, 2, {3}));
  plan.push_back(makeQuery(":STORAGE", false, 5, {5, 5}));  // dyrektywa kompilatora

  const auto shape = rdb::probe::shapeOf(plan);
  EXPECT_EQ(shape.publicStreams, 1u);
  EXPECT_EQ(shape.substrates, 0u);
  EXPECT_EQ(shape.fromTokens, 2u);
  EXPECT_EQ(shape.fieldTokens, 3u);
}

TEST(probePlanShape, substrates_counted_apart_from_public_streams) {
  // Substrat nie ma tożsamości obserwowalnej, więc to on jest właściwą jednostką
  // redukcji strukturalnej — liczony osobno od strumieni publicznych.
  qTree plan;
  plan.push_back(makeQuery("public1", false, 1, {}));
  plan.push_back(makeQuery("public2", false, 1, {}));
  plan.push_back(makeQuery("STREAM_ADD_a_b", true, 3, {}));

  const auto shape = rdb::probe::shapeOf(plan);
  EXPECT_EQ(shape.publicStreams, 2u);
  EXPECT_EQ(shape.substrates, 1u);
  EXPECT_EQ(shape.fromTokens, 5u);
}

TEST(probePlanShape, field_tokens_counted_separately_from_from_tokens) {
  // Rozdział jest konieczny: współdzielenie równoważnych SELECT przenosi kosztowny
  // program pól do jednego substratu i NIE zmienia ani jednego tokenu w lProgram.
  // Metryka licząca same lProgram dawała identyczny odczyt dla programu pól
  // i dla programu pięciokrotnie droższego.
  qTree plan;
  plan.push_back(makeQuery("q", false, 2, {4, 6, 1}));

  const auto shape = rdb::probe::shapeOf(plan);
  EXPECT_EQ(shape.fromTokens, 2u);
  EXPECT_EQ(shape.fieldTokens, 11u);
}

TEST(probePlanShape, empty_plan_has_zero_shape) {
  const qTree plan;

  const auto shape = rdb::probe::shapeOf(plan);
  EXPECT_EQ(shape.publicStreams, 0u);
  EXPECT_EQ(shape.substrates, 0u);
  EXPECT_EQ(shape.fromTokens, 0u);
  EXPECT_EQ(shape.fieldTokens, 0u);
}

TEST(probePlanShape, capacities_summed_and_maximum_reported) {
  // Suma jest proporcjonalna do zajętości pamięci planu, maksimum wskazuje najgłębszy
  // bufor — ten decydujący o najgorszym przypadku.
  const std::map<std::string, int> capacities{{"a", 3}, {"b", 7}, {"c", 1}};

  const auto shape = rdb::probe::shapeOfCapacities(capacities);
  EXPECT_EQ(shape.streams, 3u);
  EXPECT_EQ(shape.total, 11u);
  EXPECT_EQ(shape.max, 7);
}

TEST(probePlanShape, empty_capacity_map_has_zero_shape) {
  const std::map<std::string, int> capacities;

  const auto shape = rdb::probe::shapeOfCapacities(capacities);
  EXPECT_EQ(shape.streams, 0u);
  EXPECT_EQ(shape.total, 0u);
  EXPECT_EQ(shape.max, 0);
}

//
// ─── Raporty na stderr ──────────────────────────────────────────────────────────
//
// Format tych wierszy jest kontraktem ze skryptami analizy kampanii (rdb-experiment),
// więc testowany jest tekst, a nie tylko fakt wypisania czegokolwiek.
//

TEST(probeReport, runtime_counters_are_silent_without_environment) {
  const envGuard noMaterialize("RDB_BENCH_MATERIALIZE", nullptr);
  const envGuard noWork("RDB_BENCH_WORK", nullptr);

  testing::internal::CaptureStderr();
  rdb::probe::reportRuntimeCounters();
  EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}

TEST(probeReport, work_line_printed_when_armed) {
  const envGuard noMaterialize("RDB_BENCH_MATERIALIZE", nullptr);
  const envGuard work("RDB_BENCH_WORK", "1");

  rdb::probe::workReset();
  rdb::probe::onAgseWindow(30);
  rdb::probe::onAgseRead();
  rdb::probe::onEval(4);
  rdb::probe::onHashPick();
  rdb::probe::onAddMerge();

  testing::internal::CaptureStderr();
  rdb::probe::reportRuntimeCounters();
  const std::string output = testing::internal::GetCapturedStderr();

  if constexpr (rdb_probe_work)
    EXPECT_EQ(output,
              "WORK agse: okna=1 elementy=30 odczyty=1  eval: wywolania=1 tokeny=4  "
              "hash: wybory=1  add: scalenia=1\n");
  else
    EXPECT_EQ(output, "");
}

TEST(probeReport, materialization_line_printed_when_armed) {
  const envGuard materialize("RDB_BENCH_MATERIALIZE", "1");
  const envGuard noWork("RDB_BENCH_WORK", nullptr);

  rdb::probe::materializationReset();
  rdb::probe::onMaterializedAppend(false, 64);
  rdb::probe::onMaterializedOverwrite(false);
  rdb::probe::onMaterializedAppend(true, 8);

  testing::internal::CaptureStderr();
  rdb::probe::reportRuntimeCounters();
  const std::string output = testing::internal::GetCapturedStderr();

  if constexpr (rdb_probe_materialize)
    EXPECT_EQ(output,
              "MATERIALIZED trwale: dopisania=1 nadpisania=1 bajty=64  "
              "pamieciowe: dopisania=1 nadpisania=0 bajty=8\n");
  else
    EXPECT_EQ(output, "");
}

TEST(probePlanReport, silent_without_environment) {
  const envGuard noPlan("RDB_BENCH_PLAN", nullptr);

  qTree plan;
  plan.push_back(makeQuery("q", false, 1, {1}));

  rdb::probe::planProbe bench;
  bench.capture(rdb::probe::planStage::entry, plan);

  testing::internal::CaptureStderr();
  bench.report(plan, plan.maxCapacity, true);
  EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}

TEST(probePlanReport, stages_and_rewrites_reported_when_armed) {
  const envGuard planEnv("RDB_BENCH_PLAN", "1");

  qTree plan;
  plan.push_back(makeQuery("q", false, 2, {3}));
  plan.maxCapacity = {{"q", 4}, {"r", 6}};

  rdb::probe::planProbe bench;  // konstruktor uzbraja sondę i ZERUJE liczniki przepisań

  bench.capture(rdb::probe::planStage::entry, plan);
  plan.push_back(makeQuery("STREAM_ADD_a_b", true, 1, {}));  // dekompozycja: przybył substrat
  bench.capture(rdb::probe::planStage::preDedup, plan);
  plan.pop_back();  // deduplikacja: substrat zniknął — to jest właściwa redukcja planu
  bench.capture(rdb::probe::planStage::postDedup, plan);

  rdb::probe::onRewriteR1();
  rdb::probe::onRewriteR1();
  rdb::probe::onRewriteR2("nodeA");
  rdb::probe::onRewriteR2("nodeA");  // ten sam węzeł — metryka liczy WĘZŁY, nie zastosowania
  rdb::probe::onRewriteR2("nodeB");

  testing::internal::CaptureStderr();
  bench.report(plan, plan.maxCapacity, true);
  const std::string output = testing::internal::GetCapturedStderr();

  if constexpr (rdb_probe_plan) {
    EXPECT_NE(output.find("PLAN bench (publiczne/substraty/tokeny-from/tokeny-pol, dedup=ON): "
                          "wejscie=1/0/2/3  przed-dedup=1/1/3/3  po-dedup=1/0/2/3  wyjscie=1/0/2/3\n"),
              std::string::npos)
        << output;
    EXPECT_NE(output.find("REWRITE_APPLIED r1=2 r2=2\n"), std::string::npos) << output;
    EXPECT_NE(output.find("COMPILE_NS "), std::string::npos) << output;
    EXPECT_NE(output.find("PLAN capacity: strumieni=2 suma=10 maks=6\n"), std::string::npos) << output;
  } else {
    EXPECT_EQ(output, "");
  }
}

TEST(probePlanReport, dedup_switch_is_visible_in_the_report) {
  // Profil ablacyjny musi dać się rozpoznać po samym wierszu raportu — bez tego wyniki
  // dwóch profili są nierozróżnialne w zbiorczym logu kampanii.
  const envGuard planEnv("RDB_BENCH_PLAN", "1");

  qTree plan;
  rdb::probe::planProbe bench;

  testing::internal::CaptureStderr();
  bench.report(plan, plan.maxCapacity, false);
  const std::string output = testing::internal::GetCapturedStderr();

  if constexpr (rdb_probe_plan)
    EXPECT_NE(output.find("dedup=OFF"), std::string::npos) << output;
  else
    EXPECT_EQ(output, "");
}

TEST(probePlanReport, construction_resets_rewrite_counters) {
  const envGuard planEnv("RDB_BENCH_PLAN", "1");

  rdb::probe::onRewriteR1();
  rdb::probe::onRewriteR2("stale");

  qTree plan;
  rdb::probe::planProbe bench;  // nowa kompilacja zaczyna liczenie od zera

  testing::internal::CaptureStderr();
  bench.report(plan, plan.maxCapacity, true);
  const std::string output = testing::internal::GetCapturedStderr();

  if constexpr (rdb_probe_plan)
    EXPECT_NE(output.find("REWRITE_APPLIED r1=0 r2=0\n"), std::string::npos) << output;
  else
    EXPECT_EQ(output, "");
}

//
// ─── E1/E2E: budżet czasowy slotu ───────────────────────────────────────────────
//

TEST(probeSlot, csv_has_header_and_one_row_per_slot) {
  const std::filesystem::path csv = "probe_slot.csv";
  std::filesystem::remove(csv);
  const envGuard armed("RDB_BENCH_CSV", csv.c_str());

  {
    rdb::probe::slotProbe bench;
    bench.open();
    std::timespec anchor{};
    ::clock_gettime(CLOCK_MONOTONIC, &anchor);
    bench.anchor(anchor);

    for (int slot = 0; slot < 3; ++slot) {
      bench.beginSlot(slot * 10);
      bench.beginCompute();
      bench.endCompute();
      bench.endSlot();
    }
  }  // destruktor domyka plik — bez tego ostatnie wiersze zostałyby w buforze

  if constexpr (!rdb_probe_slot) {
    // Build bez sondy nie tworzy pliku NAWET z ustawioną zmienną — to jest dowód, że
    // sonda nie przecieka do wariantu produkcyjnego.
    EXPECT_FALSE(std::filesystem::exists(csv));
    return;
  }

  ASSERT_TRUE(std::filesystem::exists(csv));
  const auto lines = readLines(csv);
  ASSERT_EQ(lines.size(), 4u);  // nagłówek + 3 sloty
  EXPECT_EQ(lines[0], "iter,compute_ns,wake_lag_ns,e2e_ns");
  for (std::size_t slot = 1; slot < lines.size(); ++slot)
    EXPECT_TRUE(lines[slot].starts_with(std::to_string(slot - 1) + ","))
        << "numer slotu musi rosnąć monotonicznie: " << lines[slot];

  std::filesystem::remove(csv);
}

TEST(probeSlot, measured_intervals_keep_their_order) {
  const std::filesystem::path csv = "probe_slot_order.csv";
  std::filesystem::remove(csv);
  const envGuard armed("RDB_BENCH_CSV", csv.c_str());

  {
    rdb::probe::slotProbe bench;
    bench.open();
    std::timespec anchor{};
    ::clock_gettime(CLOCK_MONOTONIC, &anchor);
    bench.anchor(anchor);

    bench.beginSlot(0);  // deadline == kotwica, więc pobudka jest po nim
    bench.beginCompute();
    bench.endCompute();
    bench.endSlot();
  }

  if constexpr (!rdb_probe_slot) {
    EXPECT_FALSE(std::filesystem::exists(csv));
    return;
  }

  ASSERT_TRUE(std::filesystem::exists(csv));
  const auto lines = readLines(csv);
  ASSERT_EQ(lines.size(), 2u);

  long iter = 0, computeNs = 0, wakeLagNs = 0, e2eNs = 0;
  ASSERT_EQ(std::sscanf(lines[1].c_str(), "%ld,%ld,%ld,%ld", &iter, &computeNs, &wakeLagNs, &e2eNs), 4);
  EXPECT_EQ(iter, 0);
  EXPECT_GE(computeNs, 0) << "czas obliczeń nie może być ujemny";
  EXPECT_GT(wakeLagNs, 0) << "pobudka następuje po deadline'ie równym kotwicy";
  EXPECT_GE(e2eNs, wakeLagNs) << "emisja wyniku nie może wyprzedzić pobudki tego samego slotu";

  std::filesystem::remove(csv);
}

TEST(probeSlot, disarmed_probe_writes_nothing_and_survives_use) {
  // Normalny tryb pracy usługi: sonda wkompilowana, ale zmiennej nie ma. Wywołania
  // muszą być bezpieczne i nie mogą zostawić po sobie pliku.
  const envGuard disarmed("RDB_BENCH_CSV", nullptr);
  const std::filesystem::path csv = "probe_slot_disarmed.csv";
  std::filesystem::remove(csv);

  {
    rdb::probe::slotProbe bench;
    bench.open();
    std::timespec anchor{};
    ::clock_gettime(CLOCK_MONOTONIC, &anchor);
    bench.anchor(anchor);
    bench.beginSlot(5);
    bench.beginCompute();
    bench.endCompute();
    bench.endSlot();
  }

  EXPECT_FALSE(std::filesystem::exists(csv));
}
