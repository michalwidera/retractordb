#include <sys/stat.h>  // mkfifo
#include <unistd.h>    // geteuid, chdir, getpid

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "fatalError.hpp"
#include "logCapture.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"
#include "retractor/lib/serviceControl.hpp"

// ANTLR usuwa makro EOF; fmt musi byc wczytany wczesniej przez naglowki projektu.
#include <antlr4-runtime/antlr4-runtime.h>

// Kopia jednostki parsera z podmienionym wylacznie w tescie strumieniem ANTLR.
// Nie dodaje zadnego haka ani konwersji do biblioteki produkcyjnej.
namespace {
enum class ParserFault { none, internal, standard, unknown };
ParserFault parserFault = ParserFault::none;
class BoundaryInputStream : public antlr4::ANTLRInputStream {
 public:
  explicit BoundaryInputStream(const std::string &text);
};
}  // namespace

#define ANTLRInputStream BoundaryInputStream
#include "retractor/lib/RQLParser.cpp"
#undef ANTLRInputStream

namespace {
BoundaryInputStream::BoundaryInputStream(const std::string &text) : antlr4::ANTLRInputStream(text) {
  switch (std::exchange(parserFault, ParserFault::none)) {
    case ParserFault::internal:
      throw RQLInternalError{"injected listener invariant"};
    case ParserFault::standard:
      throw std::runtime_error("injected ANTLR failure");
    case ParserFault::unknown:
      throw 521;
    case ParserFault::none:
      break;
  }
}
}  // namespace

/// @brief Wczytywanie zestawu RQL z tekstu - wspolne zrodlo startu z pliku i przeladowania
/// w locie (`xqry --reset`).
///
/// Przedmiotem badania jest jedna rzecz, ktora rozni sie tu od dawnego kodu launchera: zestaw
/// PUSTY nie jest bledem. Do 2026-09-05 pusty plik zapytan konczyl proces bledem parsowania
/// ("Empty file."), przez co udokumentowana w jednostce systemd sciezka "pusty plik = tryb
/// bezczynny" nie dzialala wcale, a Restart=on-failure zapetlal start uslugi.

TEST(PlanSource, empty_text_is_an_empty_plan_not_an_error) {
  // covers: plansource.parse-plan-text.4~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, rotation_counter_preflight_is_read_only_and_preserves_error_class) {
  // covers: plansource.check-rotation-counter.1~1, plansource.check-rotation-counter.2~1, plansource.check-rotation-counter.3~1
  // covers: plansource.check-rotation-counter.4~1, plansource.check-rotation-counter.5~1
  namespace fs           = std::filesystem;
  const std::string file = "preflight_counter";
  fs::remove_all(file);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan, "ROTATION 'preflight_counter'\n").status, "OK");
  EXPECT_TRUE(checkRotationCounter(plan).refusal.empty());
  EXPECT_FALSE(fs::exists(file));
  std::ofstream(file) << "7\n";
  EXPECT_TRUE(checkRotationCounter(plan).refusal.empty());
  std::string saved;
  std::ifstream(file) >> saved;
  EXPECT_EQ(saved, "7");
  std::ofstream(file) << "invalid";
  const auto invalid = checkRotationCounter(plan);
  EXPECT_FALSE(invalid.environment);
  EXPECT_TRUE(invalid.refusal.contains(file));
  fs::remove(file);
  fs::create_directory(file);
  const auto inaccessible = checkRotationCounter(plan);
  EXPECT_TRUE(inaccessible.environment);
  EXPECT_TRUE(inaccessible.refusal.contains(file));
  fs::remove(file);
  qTree empty;
  EXPECT_TRUE(checkRotationCounter(empty).refusal.empty());
}

TEST(PlanSource, timeout_report_covers_sources_filters_and_no_clock) {
  // covers: plansource.device-timeout-report.1~1, plansource.device-timeout-report.2~1, query.effective-device-timeout.1~1
  // covers: query.effective-device-timeout.2~1
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "DECLARE a BYTE STREAM explicit_timeout, 1 DEVICE '/dev/zero' TIMEOUT 0.5\n"
                          "DECLARE a BYTE STREAM inherited, 1 DEVICE '/dev/zero'\n"
                          "DECLARE a BYTE STREAM file_source, 1 BINFILE 'file.bin'\n")
                .status,
            "OK");
  EXPECT_EQ(deviceTimeoutReport(plan, 0.25, false),
            (std::vector<std::string>{"DEVICE stream 'explicit_timeout': effective TIMEOUT 0.5 s (RQL)",
                                      "DEVICE stream 'inherited': effective TIMEOUT 0.25 s (config)"}));
  EXPECT_EQ(deviceTimeoutReport(plan, std::nullopt, false, {"inherited"}),
            (std::vector<std::string>{"DEVICE stream 'inherited': effective TIMEOUT 0 s (default)"}));
  EXPECT_EQ(deviceTimeoutReport(plan, 0.25, true, {"explicit_timeout"}),
            (std::vector<std::string>{"DEVICE stream 'explicit_timeout': effective TIMEOUT 0 s (no-clock)"}));
}

TEST(PlanSource, logical_lines_keep_start_numbers_and_drop_unfinished_statements) {
  // covers: rqlparser.read-logical-lines.1~1, rqlparser.read-logical-lines.2~1, rqlparser.read-logical-lines.3~1
  // covers: rqlparser.read-logical-lines.4~1, rqlparser.read-logical-lines.5~1
  std::istringstream input(" \t\r\n  # ignored\nSELECT \\\n  a\n\nRULE b\nunfinished\\\n");
  EXPECT_EQ(readLogicalLines(input), (std::vector<std::pair<std::string, size_t>>{{"SELECT    a", 3}, {"RULE b", 6}}));
}

TEST(PlanSource, parser_contains_internal_standard_and_unknown_exceptions) {
  // covers: rqlparser.parser-rql-string.8~1
  for (const auto &[kind, expected] :
       std::vector<std::pair<ParserFault, std::string>>{{ParserFault::internal, "injected listener invariant"},
                                                        {ParserFault::standard, "RQL parser: injected ANTLR failure"},
                                                        {ParserFault::unknown, "RQL parser: unknown failure"}}) {
    parserFault = kind;
    qTree plan;
    std::vector<std::string> keywords{"stale"};
    const auto [status, keyword, name] =
        parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 BINFILE 'src.bin'", keywords, 7, "fault.rql");
    EXPECT_EQ(status, std::string(kInternalCompilerError) + expected);
    EXPECT_EQ(keyword, "UNRECOGNIZED");
    EXPECT_TRUE(name.empty());
    EXPECT_TRUE(keywords.empty());
    EXPECT_TRUE(plan.empty());
    EXPECT_EQ(std::get<0>(parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 BINFILE 'src.bin'")), "OK");
    EXPECT_TRUE(plan.exists("src"));
  }
}

TEST(PlanSource, parser_overloads_use_line_one_or_the_explicit_source_offset) {
  // covers: rqlparser.parser-rql-string.7~1
  qTree plan;
  std::vector<std::string> keywords;
  LogCapture log;
  const std::string invalid = "\n?";
  EXPECT_TRUE(std::get<0>(parserRQLString(plan, invalid, keywords, 41, "fragment.rql")).starts_with("line 42:"));
  EXPECT_TRUE(log.text().contains("fragment.rql: line 42:"));
  EXPECT_TRUE(std::get<0>(parserRQLString(plan, invalid, keywords)).starts_with("line 2:"));
  EXPECT_TRUE(std::get<0>(parserRQLString(plan, invalid)).starts_with("line 2:"));
}

TEST(PlanSource, parser_status_keywords_and_partial_plan_are_explicit) {
  // covers: rqlparser.parser-rql-string.1~1, rqlparser.parser-rql-string.2~1, rqlparser.parser-rql-string.3~1
  // covers: rqlparser.parser-rql-string.4~1, rqlparser.parser-rql-string.5~1, rqlparser.parser-rql-string.6~1
  // covers: rqlparser.parser-rql-string.9~1
  qTree plan;
  std::vector<std::string> keywords{"stale"};
  const auto [ok, first, name] =
      parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\nSELECT a STREAM dst FROM src\n", keywords);
  EXPECT_EQ(ok, "OK");
  EXPECT_EQ(first, "DECLARE");
  EXPECT_EQ(name, "src");
  EXPECT_EQ(keywords, (std::vector<std::string>{"DECLARE", "SELECT"}));
  EXPECT_TRUE(plan.exists("src"));
  EXPECT_TRUE(plan.exists("dst"));
  const auto [empty, unrecognized, emptyName] = parserRQLString(plan, "", keywords);
  EXPECT_NE(empty, "OK");
  EXPECT_EQ(unrecognized, "UNRECOGNIZED");
  EXPECT_TRUE(emptyName.empty());
  EXPECT_TRUE(keywords.empty());
  const auto [bad, unknown, noName] = parserRQLString(plan, "SELEKT", keywords);
  EXPECT_NE(bad, "OK");
  EXPECT_EQ(unknown, "UNRECOGNIZED");
  EXPECT_TRUE(noName.empty());
  EXPECT_TRUE(plan.exists("src"));
  const auto [semantic, declared, invalid] =
      parserRQLString(plan, "DECLARE a INTEGER STREAM invalid, 0 TEXTFILE 'data.txt'\n", keywords);
  EXPECT_NE(semantic, "OK");
  EXPECT_EQ(declared, "DECLARE");
  EXPECT_EQ(invalid, "invalid");
}

TEST(PlanSource, file_parser_reports_missing_empty_and_first_invalid_statement) {
  // covers: rqlparser.parser-rql-file-4-test.1~1, rqlparser.parser-rql-file-4-test.2~1, rqlparser.parser-rql-file-4-test.3~1
  // covers: rqlparser.parser-rql-file-4-test.4~1
  const std::string file = "parser_contract.rql";
  std::filesystem::remove(file);
  qTree plan;
  LogCapture log;
  EXPECT_EQ(parserRQLFile_4Test(plan, file), "Unable to open file.");
  EXPECT_TRUE(log.text().contains("Unable to open file"));
  std::ofstream(file) << "# comment only\n";
  EXPECT_EQ(parserRQLFile_4Test(plan, file), "Empty file.");
  std::ofstream(file) << "DECLARE a INTEGER STREAM before, 1 TEXTFILE 'data.txt'\n"
                         "SELEKT broken\n"
                         "DECLARE a INTEGER STREAM after, 1 TEXTFILE 'data.txt'\n";
  EXPECT_NE(parserRQLFile_4Test(plan, file), "OK");
  EXPECT_TRUE(plan.exists("before"));
  EXPECT_FALSE(plan.exists("after"));
  EXPECT_TRUE(log.text().contains("Parsing failed"));
  std::filesystem::remove(file);
}

TEST(PlanSource, internal_compiler_status_is_prefixed_logged_and_recognized) {
  // covers: compiler.internal-compiler-error.1~1, compiler.internal-compiler-error.2~1, compiler.is-internal-compiler-error.1~1
  LogCapture log;
  const auto status = internalCompilerError("audit boundary");
  EXPECT_EQ(status, std::string(kInternalCompilerError) + "audit boundary");
  EXPECT_TRUE(log.text().contains(status));
  EXPECT_TRUE(isInternalCompilerError(status));
  EXPECT_FALSE(isInternalCompilerError("OK"));
  EXPECT_FALSE(isInternalCompilerError("prefix " + status));
}

TEST(PlanSource, comments_and_blank_lines_alone_are_an_empty_plan) {
  // covers: plansource.parse-plan-text.4~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "# tylko komentarz\n\n   \n# i jeszcze jeden\n");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, statements_land_in_the_plan_and_in_the_line_list) {
  // covers: plansource.parse-plan-text.1~1, plansource.parse-plan-text.2~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM dst FROM src\n");

  ASSERT_EQ(loaded.status, "OK");
  ASSERT_EQ(loaded.lines.size(), 2U);
  EXPECT_EQ(loaded.lines[0].first, "src");
  EXPECT_EQ(loaded.lines[1].first, "dst");
  EXPECT_TRUE(plan.exists("src"));
  EXPECT_TRUE(plan.exists("dst"));
}

TEST(PlanSource, a_broken_statement_stops_the_load_and_reports_the_reason) {
  qTree plan;
  testing::internal::CaptureStderr();
  const PlanSource loaded = parsePlanText(plan, "SELECT ((( STREAM dst FROM src\n");
  testing::internal::GetCapturedStderr();

  EXPECT_NE(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
}

/// Instrukcje po pierwszej bledna nie sa juz czytane. Zestaw czesciowo wczytany bylby gorszy
/// od zadnego: skompilowalby sie i policzyl COS INNEGO, niz operator zapisal w pliku.
TEST(PlanSource, load_stops_at_the_first_bad_statement) {
  // covers: plansource.parse-plan-text.3~1
  qTree plan;
  testing::internal::CaptureStderr();
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT ((( STREAM broken FROM src\n"
                                          "SELECT a+2 STREAM never FROM src\n");
  testing::internal::GetCapturedStderr();

  EXPECT_NE(loaded.status, "OK");
  ASSERT_EQ(loaded.lines.size(), 1U);
  EXPECT_EQ(loaded.lines[0].first, "src");
  EXPECT_FALSE(plan.exists("never"));
}

/// Nazwy roszczone na magistrali to wszystkie wezly poza dyrektywami: dyrektywa nie jest
/// strumieniem i nie ma czego roscic, a nazwanie jej tak konczyloby sie kolizja kazdych
/// dwoch planow uzywajacych STORAGE.
TEST(PlanSource, directives_do_not_claim_stream_names) {
  // covers: plansource.plan-stream-names.1~1, plansource.plan-counter-path.1~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "STORAGE 'temp'\n"
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n");
  ASSERT_EQ(loaded.status, "OK");

  const std::vector<std::string> claimed = planStreamNames(plan);
  EXPECT_EQ(claimed.size(), 1U);
  EXPECT_EQ(claimed.front(), "src");
  EXPECT_TRUE(planCounterPath(plan).empty());
}

/// Nazwa strumienia nie chroni pliku danych: klauzula `FILE` odrywa nazwe pliku od nazwy
/// zapytania, wiec magistrala musi dostac osobno ZNORMALIZOWANE sciezki magazynow.
TEST(PlanSource, stores_follow_the_file_clause_and_the_storage_directive) {
  // covers: plansource.plan-store-paths.1~1, plansource.plan-store-paths.2~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "STORAGE 'temp'\n"
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM dst FROM src\n"
                                          "SELECT a+2 STREAM aliased FROM src FILE 'shared'\n");
  ASSERT_EQ(loaded.status, "OK");

  // Deklaracji nie ma na liscie: `data.txt` jest zrodlem TYLKO DO ODCZYTU (TEXTSOURCE),
  // a wiele serwerow czytajacych jeden plik jest poprawne.
  EXPECT_EQ(planStorePaths(plan, {}), (std::vector<std::string>{absolutePathOf("temp/dst"), absolutePathOf("temp/shared")}));
}

/// Strumien MEMORY zyje w pamieci procesu i nie dotyka systemu plikow - roszczenie jego
/// sciezki byloby konfliktem o nic. Obie drogi do tego typu (VOLATILE i STORAGE memory)
/// musza dawac ten sam wynik, bo w wykonaniu obie koncza sie tym samym akcesorem.
TEST(PlanSource, memory_streams_do_not_claim_a_store) {
  // covers: plansource.plan-store-paths.2~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM vol FROM src VOLATILE\n"
                                          "SELECT a+2 STREAM mem FROM src STORAGE memory\n"
                                          "SELECT a+3 STREAM disk FROM src\n");
  ASSERT_EQ(loaded.status, "OK");

  EXPECT_EQ(planStorePaths(plan, {}), (std::vector<std::string>{absolutePathOf("disk")}));
}

/// Domyslny katalog z konfiguracji wchodzi tylko wtedy, gdy plan nie ma wlasnej dyrektywy -
/// ta sama regula pierwszenstwa, co przy budowie planu w launcherze. Bez tego parametru
/// rezerwacja wskazywalaby katalog roboczy, a plan pisalby gdzie indziej.
TEST(PlanSource, default_storage_dir_yields_to_the_storage_directive) {
  // covers: plansource.plan-storage-dir.1~1, plansource.plan-store-paths.1~1
  qTree bare;
  ASSERT_EQ(parsePlanText(bare,
                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                          "SELECT a+1 STREAM dst FROM src\n")
                .status,
            "OK");
  EXPECT_EQ(planStorageDir(bare, "/opt/rdb"), "/opt/rdb");
  EXPECT_EQ(planStorePaths(bare, "/opt/rdb"), (std::vector<std::string>{absolutePathOf("/opt/rdb/dst")}));

  qTree directed;
  ASSERT_EQ(parsePlanText(directed,
                          "STORAGE 'temp'\n"
                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                          "SELECT a+1 STREAM dst FROM src\n")
                .status,
            "OK");
  EXPECT_EQ(planStorePaths(directed, "/opt/rdb"), (std::vector<std::string>{absolutePathOf("temp/dst")}));
}

/// Dwa strumienie jednego planu wskazujace jeden plik daja JEDEN wpis. Roszczenie porownuje
/// plany miedzy soba, wiec duplikat zajalby wpis w slocie i niczego nie wniosl.
TEST(PlanSource, aliased_streams_claim_one_store_once) {
  // covers: plansource.plan-store-paths.3~1
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM one FROM src FILE 'shared'\n"
                                          "SELECT a+2 STREAM two FROM src FILE 'shared'\n");
  ASSERT_EQ(loaded.status, "OK");

  EXPECT_EQ(planStorePaths(plan, {}), (std::vector<std::string>{absolutePathOf("shared")}));
}

/// :ROTATION zachowuje historie strumieni plikowych, wiec ich artefakty zostaja. Strumien MEMORY
/// danych na dysku nie ma, a jego .desc i .meta to konfiguracja POPRZEDNIEGO przebiegu: magazyn bierze
/// TYPE i RETMEMORY z wczytanego .desc, nie z planu, wiec .desc sprzed 108a5e94 przywracal przy
/// rotacji pierscien bez granicy. Obie drogi do MEMORY (VOLATILE i STORAGE memory) musza dac to samo.
TEST(PlanSource, rotation_drops_only_the_configuration_of_memory_streams) {
  // covers: plansource.drop-stale-plan-artifacts.4~1
  namespace fs       = std::filesystem;
  const fs::path dir = "rotation_store";
  fs::remove_all(dir);
  fs::create_directory(dir);

  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "ROTATION 'rotation_counter.txt'\n"
                                          "STORAGE 'rotation_store'\n"
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM vol FROM src VOLATILE\n"
                                          "SELECT a+2 STREAM mem FROM src STORAGE memory\n"
                                          "SELECT a+3 STREAM disk FROM src\n");
  ASSERT_EQ(loaded.status, "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");

  for (const std::string file : {"vol.desc", "vol.meta", "mem.desc", "mem.meta", "disk", "disk.desc", "disk.meta"})
    std::ofstream(dir / file) << "stale";

  dropStalePlanArtifacts(plan);

  for (const std::string file : {"vol.desc", "vol.meta", "mem.desc", "mem.meta"})
    EXPECT_FALSE(fs::exists(dir / file)) << file;
  for (const std::string file : {"disk", "disk.desc", "disk.meta"})
    EXPECT_TRUE(fs::exists(dir / file)) << file;

  fs::remove_all(dir);
}

/// Sprawdzenie `.desc` deklaracji przy starcie nie rzuca (#511). stat() na dowiazaniu do samego
/// siebie konczy sie ELOOP, a std::filesystem::exists bez error_code zamienia to w filesystem_error,
/// ktorego rdzen nie obsluguje. Wpis zostaje na miejscu - odmowe z przyczyna da dalej kontrola
/// deskryptorow przy otwarciu magazynu.
TEST(PlanSource, unreadable_declaration_descriptor_does_not_throw) {
  // covers: plansource.drop-stale-plan-artifacts.2~1
  namespace fs       = std::filesystem;
  const fs::path dir = "eloop_store";
  fs::remove_all(dir);
  fs::create_directory(dir);

  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'eloop_store'\n"
                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a+1 STREAM dst FROM src\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");

  fs::create_symlink("src.desc", dir / "src.desc");  // dowiazanie do samego siebie: stat() daje ELOOP
  std::error_code ec;
  ASSERT_FALSE(fs::exists(dir / "src.desc", ec));
  ASSERT_EQ(ec, std::errc::too_many_symbolic_link_levels) << "kontrola dodatnia: wpis ma dawac ELOOP";

  EXPECT_NO_THROW(dropStalePlanArtifacts(plan));
  EXPECT_TRUE(fs::is_symlink(dir / "src.desc"));

  fs::remove_all(dir);
}

namespace {

/// Plan z trzema rodzajami wezlow plikowych: segmenty retencji, klauzula FILE i wezel posredni.
const std::string kFamilyPlan =
    "STORAGE 'family_store'\n"
    "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
    "SELECT a+1 STREAM seg FROM src RETENTION 5 2 STORAGE DIRECT\n"
    "SELECT a+2 STREAM named FROM src FILE 'named.dat'\n"
    "SELECT * STREAM sum FROM SUMC(src@(1,3))\n";

/// Nazwa wezla posredniego, ktory kompilator wydzielil z `src@(1,3)`.
std::string intermediateOf(const qTree &plan) {
  for (const auto &q : plan)
    if (q.isSubstrat) return q.id;
  return {};
}

std::vector<std::string> familyFiles(const std::string &intermediate) {
  return {"seg",
          "seg.shadow",
          "seg.desc",
          "seg.meta",
          "seg.meta.shadow",
          "seg_segment_0",
          "seg_segment_0.shadow",
          "seg_segment_12",
          "named.desc",
          "named.dat",
          "named.dat.meta",
          "named.dat.shadow",
          "named.dat_segment_3",
          intermediate,
          intermediate + ".desc",
          intermediate + ".meta"};
}

}  // namespace

/// Bez :ROTATION start kasuje PELNA rodzine plikow kazdego wezla planu. Do 2026-09-27 znikaly tylko
/// `<id>`, `<id>.desc` i `<id>.meta` strumieni z linii zapisu: segmenty `RETENTION c s` wracaly pod
/// swieze metadane (FatalError przy drugim starcie), a pliki wezlow posrednich rosly miedzy restartami.
/// Pliki podobne z nazwy, ale spoza rodziny, zostaja.
TEST(PlanSource, start_drops_whole_file_families_of_every_plan_node) {
  // covers: plansource.drop-stale-plan-artifacts.1~1, plansource.drop-stale-plan-artifacts.3~1
  namespace fs       = std::filesystem;
  const fs::path dir = "family_store";
  fs::remove_all(dir);
  fs::create_directory(dir);

  qTree plan;
  ASSERT_EQ(parsePlanText(plan, kFamilyPlan).status, "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const std::string intermediate = intermediateOf(plan);
  ASSERT_FALSE(intermediate.empty());

  const std::vector<std::string> strangers{"seg_segment_x", "seg_segment_", "segment", "seg2.desc", "src.desc", "other"};
  for (const auto &file : familyFiles(intermediate))
    std::ofstream(dir / file) << "stale";
  for (const auto &file : strangers)
    std::ofstream(dir / file) << "keep";

  dropStalePlanArtifacts(plan);

  for (const auto &file : familyFiles(intermediate))
    EXPECT_FALSE(fs::exists(dir / file)) << file;
  for (const auto &file : strangers)
    EXPECT_TRUE(fs::exists(dir / file)) << file;

  fs::remove_all(dir);
}

/// Mirror: przy :ROTATION cala rodzina wezla plikowego zostaje, takze posredniego.
TEST(PlanSource, rotation_keeps_whole_file_families) {
  // covers: plansource.drop-stale-plan-artifacts.4~1
  namespace fs       = std::filesystem;
  const fs::path dir = "family_store";
  fs::remove_all(dir);
  fs::create_directory(dir);

  qTree plan;
  ASSERT_EQ(parsePlanText(plan, "ROTATION 'family_counter.txt'\n" + kFamilyPlan).status, "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const std::string intermediate = intermediateOf(plan);
  ASSERT_FALSE(intermediate.empty());

  for (const auto &file : familyFiles(intermediate))
    std::ofstream(dir / file) << "kept";

  dropStalePlanArtifacts(plan);

  for (const auto &file : familyFiles(intermediate))
    EXPECT_TRUE(fs::exists(dir / file)) << file;

  fs::remove_all(dir);
}

/// P5: pliki, ktore :ROTATION zachowa, musza miec konfiguracje z planu. TYPE i RETENTION z zachowanego
/// .desc wygrywaja w magazynie, a zmiana pojemnosci przy starych segmentach przestawilaby adresowanie
/// rekordow - wiec rozjazd jest odmowa z obiema konfiguracjami, zanim cokolwiek zostanie zmienione.
TEST(PlanSource, rotation_refuses_kept_files_written_with_another_configuration) {
  // covers: plansource.check-kept-stores.1~1, plansource.check-kept-stores.5~1, plansource.check-kept-stores.6~1
  // covers: plansource.check-kept-stores.7~1
  namespace fs       = std::filesystem;
  const fs::path dir = "kept_store";
  fs::remove_all(dir);
  fs::create_directory(dir);

  const auto check = [&dir](const std::string &keptDesc, bool rotation) {
    qTree plan;
    EXPECT_EQ(parsePlanText(plan, std::string(rotation ? "ROTATION 'kept_counter.txt'\n" : "") +
                                      "STORAGE 'kept_store'\n"
                                      "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                      "SELECT a+1 STREAM seg FROM src RETENTION 5 2 STORAGE DIRECT\n"
                                      "SELECT a+2 STREAM vol FROM src VOLATILE\n")
                  .status,
              "OK");
    compiler cm(plan);
    EXPECT_EQ(cm.compile(), "OK");
    std::ofstream(dir / "seg.desc") << keptDesc;
    std::ofstream(dir / "vol.desc") << "{ INTEGER vol_0 RETMEMORY 1 TYPE MEMORY }";
    return checkKeptStores(plan, {}).message;
  };

  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 2 }", true), "");
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 2 TYPE DIRECT }", true), "");
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 3 }", true),
            "Stream 'seg' keeps its files under ROTATION, but kept_store/seg.desc was written with STORAGE DIRECT RETENTION 5 3 "
            "and the plan asks for STORAGE DIRECT RETENTION 5 2: put STORAGE DIRECT RETENTION 5 3 back in the plan, or remove "
            "the stream's files to start it afresh");
  EXPECT_TRUE(check("{ INTEGER seg_0 }", true).contains("written with STORAGE DIRECT without RETENTION")) << "no retention";
  EXPECT_TRUE(
      check("{ INTEGER seg_0 RETMEMORY 5 TYPE DEFAULT }", true).contains("written with STORAGE DEFAULT without RETENTION"))
      << ".desc sprzed D7";
  EXPECT_TRUE(check("{\n INTEGER }\n", true).contains("Fail: line 2:9"));
  // Bez rotacji pliki i tak znikaja przy starcie, wiec nie ma czego porownywac.
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 3 }", false), "");

  fs::remove_all(dir);
}

TEST(PlanSource, invalid_declared_descriptor_is_rejected_before_plan_activation) {
  // covers: plansource.check-descriptor-files.2~1
  namespace fs       = std::filesystem;
  const fs::path dir = "bad_declared_descriptor";
  fs::remove_all(dir);
  fs::create_directory(dir);

  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'bad_declared_descriptor'\n"
                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");

  std::ofstream(dir / "src.desc") << "{\n INTEGER }\n";
  const std::string error = checkDescriptorFiles(plan, {}).message;
  EXPECT_TRUE(error.contains("src.desc")) << error;
  EXPECT_TRUE(error.contains("Fail: line 2:9")) << error;

  fs::remove_all(dir);
}

// Wstepne sprawdzenie ma dawac ten sam werdykt co storage::attachDescriptor -> verifyDescriptorMatch
// (plan == plik). Descriptor::operator== jest asymetryczny: odwrocony kierunek odrzucal plan, ktory
// silnik przyjmuje, i przepuszczal taki, na ktorym silnik konczyl sie FatalError-em.
TEST(PlanSource, declared_descriptor_check_uses_the_runtime_direction) {
  // covers: plansource.check-descriptor-files.5~1
  namespace fs       = std::filesystem;
  const fs::path dir = "declared_descriptor_direction";
  fs::remove_all(dir);
  fs::create_directory(dir);

  const auto check = [&](const std::string &plannedType, const std::string &keptType) {
    qTree plan;
    EXPECT_EQ(parsePlanText(
                  plan, "STORAGE 'declared_descriptor_direction'\nDECLARE a " + plannedType + " STREAM src, 1 FILE 'data.txt'\n")
                  .status,
              "OK");
    compiler cm(plan);
    EXPECT_EQ(cm.compile(), "OK");
    std::ofstream(dir / "src.desc") << "{ " + keptType + " a REF \"data.txt\" TYPE TEXTSOURCE }\n";
    return checkDescriptorFiles(plan, {}).message;
  };

  EXPECT_TRUE(check("INTEGER", "DOUBLE").contains("descriptor schema mismatch")) << "silnik konczy sie FatalError-em";
  EXPECT_EQ(check("DOUBLE", "INTEGER"), "") << "silnik przyjmuje ten plan";

  fs::remove_all(dir);
}

namespace {

/// Wynik `call` albo przerwanie procesu testu, gdy nie wroci w 5 s. Kontrola rodzaju pliku nie moze
/// otwierac sciezki: open() na FIFO bez pisarza wisi, a wiszacego watku nie da sie zakonczyc inaczej.
template <typename F>
auto withinFiveSeconds(F call) {
  auto pending = std::async(std::launch::async, std::move(call));
  if (pending.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
    std::cerr << "call blocked for 5 s - a FIFO without a writer was opened\n";
    std::_Exit(EXIT_FAILURE);
  }
  return pending.get();
}

}  // namespace

// Rodzaj pliku pod sciezka deklaracji (#346): BINFILE i TEXTFILE - plik zwykly, DEVICE - urzadzenie
// znakowe albo FIFO. Brak pliku jest odmowa K4 przed budowa modelu (#488).
TEST(PlanSource, declared_source_kind_is_checked_without_opening_the_path) {
  // covers: plansource.check-declared-sources.1~1, plansource.check-declared-sources.3~2, plansource.check-declared-sources.4~1
  // covers: plansource.check-declared-sources.5~1
  namespace fs       = std::filesystem;
  const fs::path dir = "declared_source_kind";
  fs::remove_all(dir);
  fs::create_directory(dir);
  const std::string fifo    = (dir / "feed.fifo").string();
  const std::string regular = (dir / "values.dat").string();
  ASSERT_EQ(::mkfifo(fifo.c_str(), 0600), 0);
  std::ofstream(regular) << "1\n";

  const auto check = [](const std::string &declaration) {
    qTree plan;
    EXPECT_EQ(parsePlanText(plan, "DECLARE a INTEGER STREAM src, 1 " + declaration + "\n").status, "OK") << declaration;
    return withinFiveSeconds([&plan] { return checkDeclaredSources(plan).message; });
  };

  EXPECT_EQ(check("BINFILE '" + fifo + "'"), "stream 'src': BINFILE '" + fifo + "' is a FIFO, not a regular file");
  EXPECT_EQ(check("TEXTFILE '" + fifo + "'"), "stream 'src': TEXTFILE '" + fifo + "' is a FIFO, not a regular file");
  EXPECT_EQ(check("BINFILE '/dev/null'"), "stream 'src': BINFILE '/dev/null' is a character device, not a regular file");
  EXPECT_EQ(check("TEXTFILE '/dev/null'"), "stream 'src': TEXTFILE '/dev/null' is a character device, not a regular file");
  EXPECT_EQ(check("BINFILE '" + dir.string() + "'"),
            "stream 'src': BINFILE '" + dir.string() + "' is a directory, not a regular file");
  EXPECT_EQ(check("DEVICE '" + regular + "'"),
            "stream 'src': DEVICE '" + regular + "' is a regular file, not a character device or FIFO");

  EXPECT_EQ(check("DEVICE '" + fifo + "'"), "");
  EXPECT_EQ(check("DEVICE '/dev/null'"), "");
  EXPECT_EQ(check("BINFILE '" + regular + "'"), "");
  EXPECT_EQ(check("TEXTFILE '" + regular + "'"), "");
  EXPECT_TRUE(check("BINFILE '" + (dir / "missing.bin").string() + "'").contains("source file does not exist"));

  // Forma przestarzala: odmowa mowi, co wybrala regula i jakie slowo pasuje do pliku.
  EXPECT_EQ(check("FILE '" + fifo + "'"), "stream 'src': BINFILE '" + fifo +
                                              "' is a FIFO, not a regular file (deprecated FILE resolved this path as "
                                              "BINFILE; declare it with DEVICE)");

  // Ad-hoc sprawdza tylko nowo dodawane strumienie.
  qTree plan;
  ASSERT_EQ(parsePlanText(plan, "DECLARE a INTEGER STREAM bad, 1 BINFILE '" + fifo +
                                    "'\nDECLARE a INTEGER STREAM good, 1 BINFILE '" + regular + "'\n")
                .status,
            "OK");
  EXPECT_EQ(checkDeclaredSources(plan, {"good"}).message, "");
  EXPECT_NE(checkDeclaredSources(plan).message, "");

  fs::remove_all(dir);
}

// Magazyn bierze TYPE i REF deklaracji z wczytanego `.desc`, a Descriptor::operator== porownuje
// tylko sloty danych. Do #346 plan wskazujacy `w.txt` czytal wiec po cichu `v.txt` sprzed zmiany.
// Jedyny przepuszczany rozjazd to dawny TYPE DEVICE zwyklego pliku binarnego - ten `.desc`
// zastepuje start.
TEST(PlanSource, declared_descriptor_must_match_the_source_kind_and_path) {
  // covers: plansource.drop-stale-plan-artifacts.2~1, plansource.check-descriptor-files.7~1
  namespace fs       = std::filesystem;
  const fs::path dir = "declared_descriptor_kind";
  fs::remove_all(dir);
  fs::create_directory(dir);
  const fs::path descFile = dir / "src.desc";

  const auto plan = [](const std::string &declaration) {
    qTree retVal;
    EXPECT_EQ(parsePlanText(retVal, "STORAGE 'declared_descriptor_kind'\nDECLARE a INTEGER STREAM src, 1 " + declaration + "\n")
                  .status,
              "OK");
    return retVal;
  };
  const auto keep = [&](const std::string &ref, const std::string &type) {
    std::ofstream(descFile) << "{ INTEGER a REF \"" + ref + "\" TYPE " + type + " }\n";
  };

  // Dawny zapis: przepuszczony i zastapiony przy starcie.
  keep("rec.bin", "DEVICE");
  qTree legacy = plan("FILE 'rec.bin'");
  EXPECT_EQ(checkDescriptorFiles(legacy, {}).message, "");
  dropStalePlanArtifacts(legacy);
  EXPECT_FALSE(fs::exists(descFile));

  // Ten sam dawny zapis, ale plan czyta plik jako tekst - to juz inne zrodlo.
  keep("rec.bin", "DEVICE");
  qTree text = plan("TEXTFILE 'rec.bin'");
  EXPECT_EQ(checkDescriptorFiles(text, {}).message,
            "stream 'src': " + descFile.string() +
                " was written for TYPE DEVICE and the plan declares TYPE TEXTSOURCE; remove " + descFile.string() +
                " to start the stream afresh");
  dropStalePlanArtifacts(text);
  EXPECT_TRUE(fs::exists(descFile)) << "start nie kasuje .desc, ktory nie jest dawnym zapisem BINFILE";

  // Dawny TYPE DEVICE dla innej sciezki nie jest dawnym zapisem tego strumienia.
  keep("old.bin", "DEVICE");
  qTree moved = plan("FILE 'rec.bin'");
  EXPECT_TRUE(
      checkDescriptorFiles(moved, {}).message.contains("was written for TYPE DEVICE and the plan declares TYPE BINFILE"));

  // Zmiana sciezki przy tym samym rodzaju.
  keep("v.txt", "TEXTSOURCE");
  qTree path = plan("FILE 'w.txt'");
  EXPECT_EQ(checkDescriptorFiles(path, {}).message, "stream 'src': " + descFile.string() +
                                                        " was written for source 'v.txt' and the plan reads 'w.txt'; remove " +
                                                        descFile.string() + " to start the stream afresh");

  // Urzadzenie bylo DEVICE przed zmiana i zostaje nim - `.desc` pasuje i zostaje.
  keep("/dev/urandom", "DEVICE");
  qTree device = plan("FILE '/dev/urandom'");
  EXPECT_EQ(checkDescriptorFiles(device, {}).message, "");
  dropStalePlanArtifacts(device);
  EXPECT_TRUE(fs::exists(descFile));

  fs::remove_all(dir);
}

// Ad-hoc sprawdza tylko nowo dodawane strumienie (#278): niezgodny `.desc` strumienia spoza listy
// nie blokuje importu, a dodawanego - tak, tym samym komunikatem co przy starcie.
TEST(PlanSource, descriptor_check_can_be_limited_to_named_streams) {
  // covers: plansource.check-descriptor-files.1~1, plansource.check-descriptor-files.8~1
  namespace fs       = std::filesystem;
  const fs::path dir = "descriptor_check_names";
  fs::remove_all(dir);
  fs::create_directory(dir);
  std::ofstream(dir / "moved.desc") << "{ INTEGER a REF \"foreign.txt\" TYPE TEXTSOURCE }\n";

  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'descriptor_check_names'\n"
                          "DECLARE a INTEGER STREAM moved, 1 TEXTFILE 'planned.txt'\n"
                          "DECLARE a INTEGER STREAM fresh, 1 TEXTFILE 'fresh.txt'\n")
                .status,
            "OK");

  const std::string refusal =
      "stream 'moved': descriptor_check_names/moved.desc was written for source 'foreign.txt' and "
      "the plan reads 'planned.txt'; remove descriptor_check_names/moved.desc to start the stream "
      "afresh";
  EXPECT_EQ(checkDescriptorFiles(plan, {}, {"fresh"}).message, "");
  EXPECT_EQ(checkDescriptorFiles(plan, {}, {"fresh", "moved"}).message, refusal);
  EXPECT_EQ(checkDescriptorFiles(plan, {}).message, refusal);

  fs::remove_all(dir);
}

// Plan nie daje SELECT-owi REF, a magazyn wzialby go z zachowanego `.desc` i przeniosl zapis wyniku
// w dowolne miejsce (#278). Pod :ROTATION, gdy pliki zostaja, jest to odmowa przed startem. Bez
// rotacji start i tak kasuje `.desc` wezla.
TEST(PlanSource, kept_select_descriptor_must_not_move_the_data_file) {
  // covers: plansource.check-descriptor-files.1~1, plansource.check-descriptor-files.6~1
  namespace fs       = std::filesystem;
  const fs::path dir = "kept_select_ref";
  fs::remove_all(dir);
  fs::create_directory(dir);

  const auto check = [&dir](const std::string &keptDesc, bool rotation) {
    qTree plan;
    EXPECT_EQ(parsePlanText(plan, std::string(rotation ? "ROTATION 'kept_select_ref_counter.txt'\n" : "") +
                                      "STORAGE 'kept_select_ref'\n"
                                      "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                      "SELECT a+1 STREAM out FROM src\n")
                  .status,
              "OK");
    compiler cm(plan);
    EXPECT_EQ(cm.compile(), "OK");
    std::ofstream(dir / "out.desc") << keptDesc;
    return checkDescriptorFiles(plan, {}).message;
  };

  EXPECT_EQ(check("{ INTEGER out_0 }", true), "");
  EXPECT_EQ(check("{ INTEGER out_0 REF \"../../etc/rdb\" }", true),
            "stream 'out': kept_select_ref/out.desc moves the stream's data file to '../../etc/rdb', which the plan does "
            "not do; remove kept_select_ref/out.desc to start the stream afresh");
  EXPECT_EQ(check("{ INTEGER out_0 REF \"../../etc/rdb\" }", false), "");

  fs::remove_all(dir);
}

// Ostrzezenia o formie przestarzalej ida w kolejnosci wierszy planu, choc kompilator sortuje
// wezly po interwale. Wiersz to pierwszy wiersz instrukcji takze przy kontynuacji `\`.
TEST(PlanSource, deprecated_file_warnings_follow_the_plan_lines) {
  // covers: plansource.deprecated-file-warnings.1~1, plansource.deprecated-file-warnings.2~1
  // covers: plansource.deprecated-file-warnings.3~1
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "DECLARE a INTEGER STREAM slow, 2 FILE 'a.txt'\n"
                          "# komentarz\n"
                          "DECLARE b BYTE STREAM fast, 1/10 \\\n"
                          "  FILE '/dev/urandom'\n"
                          "DECLARE c INTEGER STREAM fresh, 1 BINFILE 'c.bin'\n"
                          "SELECT slow[0] STREAM out FROM slow\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");

  EXPECT_EQ(deprecatedFileWarnings(plan),
            (std::vector<std::string>{"line 1: DECLARE slow: FILE 'a.txt' is deprecated, resolved as TEXTFILE",
                                      "line 3: DECLARE fast: FILE '/dev/urandom' is deprecated, resolved as DEVICE"}));
  EXPECT_EQ(deprecatedFileWarnings(plan, {"fast", "fresh"}),
            (std::vector<std::string>{"line 3: DECLARE fast: FILE '/dev/urandom' is deprecated, resolved as DEVICE"}));
}

// TIMEOUT dluzszy od interwalu przekracza slot (#347). Liczy sie termin EFEKTYWNY: jawna klauzula,
// a bez niej `[sources] timeout_s`; jawne `TIMEOUT 0` wylacza wartosc z konfiguracji. Termin rowny
// interwalowi jeszcze miesci sie w slocie.
TEST(PlanSource, device_timeout_warnings_use_the_effective_timeout) {
  // covers: plansource.device-timeout-warnings.1~1, plansource.device-timeout-warnings.3~1
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "DECLARE a BYTE STREAM late, 1/10 DEVICE '/dev/zero' TIMEOUT 0.5\n"
                          "DECLARE b BYTE STREAM ontime, 1 DEVICE '/dev/zero' TIMEOUT 1\n"
                          "DECLARE c BYTE STREAM inherit, 1/10 DEVICE '/dev/zero'\n"
                          "DECLARE d BYTE STREAM zeroed, 1/10 DEVICE '/dev/zero' TIMEOUT 0\n"
                          "DECLARE e BYTE STREAM recorded, 1/10 BINFILE 'e.bin'\n"
                          "SELECT late[0] STREAM out FROM late\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");

  const std::string late =
      "line 1: DECLARE late: TIMEOUT 0.5 s (RQL) is longer than the interval 0.1 s; waiting overruns the slot";
  const std::string inherit =
      "line 3: DECLARE inherit: TIMEOUT 0.2 s (config) is longer than the interval 0.1 s; waiting overruns the slot";
  EXPECT_EQ(deviceTimeoutWarnings(plan, std::nullopt), (std::vector<std::string>{late}));
  EXPECT_EQ(deviceTimeoutWarnings(plan, 0.2), (std::vector<std::string>{late, inherit}));
  EXPECT_EQ(deviceTimeoutWarnings(plan, 0.2, {"inherit", "zeroed"}), (std::vector<std::string>{inherit}));
  EXPECT_EQ(deviceTimeoutWarnings(plan, 0.1, {"inherit"}), (std::vector<std::string>{}));
}

// K2 katalogu dotyczy takze MEMORY, ktore nadal zapisuje .desc. Pusty plan pozostaje idle.
TEST(PlanSource, storage_directory_refusal_precedes_all_storage_operations) {
  // covers: plansource.check-storage-directory.1~1, plansource.check-storage-directory.2~1
  // covers: plansource.check-storage-directory.3~1, plansource.check-storage-directory.4~1, plansource.check-plan-storage.1~1
  namespace fs       = std::filesystem;
  const fs::path dir = "preflight_directory";
  fs::remove_all(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'preflight_directory/'\n"
                          "DECLARE a DOUBLE STREAM src, 1 TEXTFILE 'preflight_directory_input.txt'\n"
                          "SELECT a STREAM out FROM src VOLATILE\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto missing = checkPlanStorage(plan, {});
  EXPECT_EQ(missing.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(missing.err, ENOENT);
  EXPECT_TRUE(missing.message.contains("does not exist"));
  EXPECT_FALSE(fs::exists(dir));
  std::ofstream(dir) << "not a directory";
  const auto wrongKind = checkPlanStorage(plan, {});
  EXPECT_EQ(wrongKind.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(wrongKind.err, ENOTDIR);
  EXPECT_TRUE(wrongKind.message.contains("not a directory"));
  fs::remove(dir);
  fs::create_directory(dir);
  std::ofstream("preflight_directory_input.txt") << "1\n";
  EXPECT_TRUE(checkPlanStorage(plan, {}).message.empty());
  EXPECT_TRUE(fs::is_empty(dir)) << "preflight nie tworzy artefaktow";
  fs::remove(dir);
  fs::remove("preflight_directory_input.txt");
  qTree idle;
  EXPECT_TRUE(checkStorageDirectory(idle, dir.string()).message.empty());
}

TEST(PlanSource, output_preflight_distinguishes_plan_and_environment_without_creating_files) {
  // covers: plansource.check-output-files-openable.4~1, plansource.check-output-files-openable.5~1
  // covers: plansource.check-output-files-openable.6~1
  namespace fs       = std::filesystem;
  const fs::path dir = "preflight_output";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'preflight_output'\n"
                          "DECLARE a DOUBLE STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a STREAM out FROM src FILE 'nested/result' STORAGE DIRECT\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto missingParent = checkOutputFilesOpenable(plan, {});
  EXPECT_EQ(missingParent.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(missingParent.err, ENOENT);
  fs::create_directory(dir / "nested");
  EXPECT_TRUE(checkOutputFilesOpenable(plan, {}).message.empty());
  EXPECT_FALSE(fs::exists(dir / "nested/result"));
  fs::create_directory(dir / "nested/result");
  const auto directory = checkOutputFilesOpenable(plan, {});
  EXPECT_EQ(directory.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(directory.err, EISDIR);
  fs::remove(dir / "nested/result");
  std::ofstream(dir / "target") << "untouched";
  fs::create_symlink("../target", dir / "nested/result");
  const auto linked = checkOutputFilesOpenable(plan, {});
  EXPECT_EQ(linked.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(linked.err, ELOOP);
  EXPECT_FALSE(linked.message.empty());
  fs::remove_all(dir);
}

// Ad-hoc nie usuwa .desc wynikow: bez ROTATION oraz dla MEMORY musza przejsc kontrole.
TEST(PlanSource, adhoc_checks_retained_select_descriptors_and_only_the_new_streams) {
  // covers: plansource.check-descriptor-files.1~1, plansource.check-descriptor-files.3~1, plansource.check-descriptor-files.5~1
  // covers: plansource.check-plan-storage.1~1
  namespace fs       = std::filesystem;
  const fs::path dir = "preflight_adhoc_descriptor";
  fs::remove_all(dir);
  fs::create_directory(dir);
  std::ofstream(dir / "input.txt") << "1\n";
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'preflight_adhoc_descriptor'\n"
                          "DECLARE a DOUBLE STREAM src, 1 TEXTFILE 'preflight_adhoc_descriptor/input.txt'\n"
                          "SELECT a STREAM disk FROM src\n"
                          "SELECT a STREAM mem FROM src VOLATILE\n"
                          "SELECT a STREAM fresh FROM src VOLATILE\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  for (const std::string name : {"disk", "mem"}) {
    std::ofstream(dir / (name + ".desc")) << "{ STRING v[32] TYPE MEMORY RETMEMORY 1 }";
    const auto mismatch = checkPlanStorage(plan, {}, {name});
    EXPECT_EQ(mismatch.cls, rdb::ErrorClass::Plan);
    EXPECT_TRUE(mismatch.message.contains("schema mismatch"));
    std::ofstream(dir / (name + ".desc")) << "{ DOUBLE v TYPE UNKNOWN }";
    const auto type = checkPlanStorage(plan, {}, {name});
    EXPECT_EQ(type.cls, rdb::ErrorClass::Plan);
    EXPECT_TRUE(type.message.contains("unsupported storage type"));
  }
  EXPECT_TRUE(checkPlanStorage(plan, {}, {"fresh"}).message.empty());
  EXPECT_TRUE(checkPlanStorage(plan, {}).message.empty()) << "start usunie te deskryptory";
  fs::remove_all(dir);
}

TEST(PlanSource, text_source_refuses_a_kept_rational_even_when_its_schema_fits_double) {
  // covers: plansource.check-descriptor-files.4~1
  namespace fs       = std::filesystem;
  const fs::path dir = "preflight_text_fields";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'preflight_text_fields'\n"
                          "DECLARE a DOUBLE STREAM src, 1 TEXTFILE 'data.txt'\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  std::ofstream(dir / "src.desc") << "{ RATIONAL a REF \"data.txt\" TYPE TEXTSOURCE }";
  const auto refusal = checkPlanStorage(plan, {});
  EXPECT_EQ(refusal.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(refusal.err, 0);
  EXPECT_TRUE(refusal.message.contains("unsupported field type RATIONAL")) << refusal.message;
  EXPECT_TRUE(checkPlanStorage(plan, {}, {"src"}).message.contains("unsupported field type"));
  fs::remove_all(dir);
}

// Polityka startu jednostki: odmowa K2 czysci plik planu bez zatrzasku FatalError (#473).
TEST(PlanSource, missing_storage_is_a_service_plan_refusal_without_the_fatal_latch) {
  namespace fs        = std::filesystem;
  const fs::path file = "preflight_service_plan.rql";
  const fs::path dir  = "preflight_service_missing";
  fs::remove_all(dir);
  const std::string text =
      "STORAGE 'preflight_service_missing'\n"
      "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n";
  std::ofstream(file) << text;
  qTree plan;
  ASSERT_EQ(parsePlanText(plan, text).status, "OK");
  const auto refusal = checkPlanStorage(plan, {});
  ASSERT_FALSE(refusal.message.empty());
  EXPECT_EQ(refusal.cls, rdb::ErrorClass::Plan);
  EXPECT_FALSE(fatalErrorRaised.load());
  EXPECT_TRUE(servicecontrol::dropRefusedPlan("xretractor.service", file.string()));
  EXPECT_EQ(fs::file_size(file), 0U);
  EXPECT_FALSE(fatalErrorRaised.load());
  fs::remove(file);
}

// Blad skladni wskazuje wiersz PLIKU i sam plik: parsePlanText podaje parserowi numer pierwszego
// wiersza instrukcji i nazwe zrodla. Udane instrukcje sprzed bledu trafiaja na liste z trescia.
TEST(PlanSource, syntax_error_names_the_file_line_and_keeps_earlier_statements) {
  // covers: rqlparser.parser-rql-string.7~1, plansource.parse-plan-text.1~1, plansource.parse-plan-text.2~1
  // covers: plansource.parse-plan-text.3~1
  qTree plan;
  LogCapture log;
  testing::internal::CaptureStderr();
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                                          "# komentarz\n"
                                          "SELECT a+1 \\\n"
                                          "  STREAM dst FROM src\n"
                                          "SELECT ((( STREAM broken FROM src\n",
                                          "plan.rql");
  testing::internal::GetCapturedStderr();

  EXPECT_TRUE(loaded.status.starts_with("line 5:")) << loaded.status;
  EXPECT_TRUE(log.text().contains("Parser: plan.rql: line 5:")) << log.text();
  EXPECT_EQ(loaded.lines,
            (std::vector<std::pair<std::string, std::string>>{{"src", "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'"},
                                                              {"dst", "SELECT a+1    STREAM dst FROM src"}}));
  EXPECT_TRUE(plan.exists("dst"));
  EXPECT_FALSE(plan.exists("broken"));
}

// Bez :STORAGE rodziny leza w katalogu roboczym: katalog domyslny konfiguracji dopisuje do planu
// wolajacy. Instancje generatora sa wezlami planu jak kazde inne. `.desc` deklaracji, ktorego nie
// da sie wczytac, zostaje - odmowe z przyczyna da kontrola przy otwarciu magazynu.
TEST(PlanSource, start_drops_generator_instances_in_the_working_directory) {
  // covers: plansource.drop-stale-plan-artifacts.1~1, plansource.drop-stale-plan-artifacts.2~1
  // covers: plansource.drop-stale-plan-artifacts.3~1
  namespace fs = std::filesystem;
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "DECLARE a INTEGER STREAM gen_src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT gen_src[0]+$ STREAM gen_cell[2] FROM gen_src\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  std::vector<std::string> instances;
  for (const auto &q : plan)
    if (q.id.starts_with("gen_cell")) instances.push_back(q.id);
  ASSERT_EQ(instances.size(), 2U);
  for (const auto &id : instances)
    for (const std::string suffix : {"", ".desc", ".meta"})
      std::ofstream(id + suffix) << "stale";
  std::ofstream("gen_src.desc") << "{\n INTEGER }\n";

  dropStalePlanArtifacts(plan);

  for (const auto &id : instances)
    for (const std::string suffix : {"", ".desc", ".meta"})
      EXPECT_FALSE(fs::exists(id + suffix)) << id + suffix;
  EXPECT_TRUE(fs::exists("gen_src.desc"));
  fs::remove("gen_src.desc");
}

// Sprzatanie nie zatrzymuje sie na bledzie: plik, ktorego nie da sie usunac, zostaje, a katalog,
// ktorego nie da sie czytac, konczy tylko szukanie segmentow. Rodzina dalszego wezla znika.
TEST(PlanSource, cleanup_errors_leave_files_but_do_not_stop_the_start) {
  // covers: plansource.drop-stale-plan-artifacts.5~1
  if (::geteuid() == 0) GTEST_SKIP() << "root ignores directory permissions";
  namespace fs       = std::filesystem;
  const fs::path dir = "drop_errors";
  fs::remove_all(dir);
  fs::create_directories(dir / "locked");
  fs::create_directories(dir / "blind");
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'drop_errors'\n"
                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a+1 STREAM stuck FROM src FILE 'locked/stuck'\n"
                          "SELECT a+2 STREAM unlisted FROM src FILE 'blind/unlisted'\n"
                          "SELECT a+3 STREAM free FROM src\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  ASSERT_EQ(planStreamNames(plan).back(), "free") << "kontrola: rodzina do usuniecia stoi za wezlami z bledami";
  for (const std::string file : {"locked/stuck", "blind/unlisted", "blind/unlisted_segment_0", "free", "free.desc"})
    std::ofstream(dir / file) << "stale";
  fs::permissions(dir / "locked", fs::perms::owner_read | fs::perms::owner_exec);
  fs::permissions(dir / "blind", fs::perms::owner_write | fs::perms::owner_exec);

  dropStalePlanArtifacts(plan);

  fs::permissions(dir / "locked", fs::perms::owner_all);
  fs::permissions(dir / "blind", fs::perms::owner_all);
  EXPECT_TRUE(fs::exists(dir / "locked/stuck"));
  EXPECT_FALSE(fs::exists(dir / "blind/unlisted"));
  EXPECT_TRUE(fs::exists(dir / "blind/unlisted_segment_0"));
  EXPECT_FALSE(fs::exists(dir / "free"));
  EXPECT_FALSE(fs::exists(dir / "free.desc"));
  fs::remove_all(dir);
}

// Zakres i klasy odmowy checkKeptStores: deklaracje, strumienie MEMORY i nazwy spoza listy nie sa
// badane, wezel bez `.desc` jest pomijany, `.desc` niebedacy plikiem to K2, a blad lstat inny niz
// ENOENT - K4 z errno. Z kilku rozjazdow wraca pierwszy w kolejnosci planu.
TEST(PlanSource, kept_store_check_scope_and_error_classes) {
  // covers: plansource.check-kept-stores.2~1, plansource.check-kept-stores.3~1, plansource.check-kept-stores.4~1
  // covers: plansource.check-kept-stores.6~1, plansource.check-kept-stores.7~1
  namespace fs       = std::filesystem;
  const fs::path dir = "kept_scope";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "ROTATION 'kept_scope_counter.txt'\n"
                          "STORAGE 'kept_scope'\n"
                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a+1 STREAM one FROM src RETENTION 5 2 STORAGE DIRECT\n"
                          "SELECT a+2 STREAM two FROM src RETENTION 5 2 STORAGE DIRECT\n"
                          "SELECT a+3 STREAM vol FROM src VOLATILE\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto names = planStreamNames(plan);
  ASSERT_LT(std::ranges::find(names, "one"), std::ranges::find(names, "two")) << "kontrola kolejnosci planu";

  const std::string foreign = "{ INTEGER x RETENTION 7 7 TYPE DIRECT }";
  std::ofstream(dir / "src.desc") << foreign;
  std::ofstream(dir / "vol.desc") << foreign;
  EXPECT_EQ(checkKeptStores(plan, {}).message, "") << "deklaracja, MEMORY i wezly bez .desc nie sa badane";
  EXPECT_EQ(checkKeptStores(plan, {}, {"vol", "src"}).message, "");

  std::ofstream(dir / "two.desc") << foreign;
  std::ofstream(dir / "one.desc") << foreign;
  const auto mismatch = checkKeptStores(plan, {});
  EXPECT_EQ(mismatch.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(mismatch.message.starts_with("Stream 'one'")) << mismatch.message;
  EXPECT_TRUE(checkKeptStores(plan, {}, {"two"}).message.starts_with("Stream 'two'"));

  fs::remove(dir / "one.desc");
  fs::create_directory(dir / "one.desc");
  const auto notFile = checkKeptStores(plan, {});
  EXPECT_EQ(notFile.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(notFile.message.contains("is not a regular file")) << notFile.message;

  fs::remove_all(dir);
  std::ofstream(dir) << "not a directory";
  const auto lstatError = checkKeptStores(plan, {});
  EXPECT_EQ(lstatError.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(lstatError.err, ENOTDIR);
  fs::remove(dir);
}

// Zakres i klasy odmowy checkDescriptorFiles, ktorych nie dotykaja testy wyzej: MEMORY pod rotacja
// przy pustej liscie, TYPE spoza typow deklaracji, klasa K2 rozjazdu TYPE, REF i REF wyniku oraz
// `.desc` niebedacy plikiem i blad lstat.
TEST(PlanSource, descriptor_file_check_scope_and_error_classes) {
  // covers: plansource.check-descriptor-files.1~1, plansource.check-descriptor-files.2~1, plansource.check-descriptor-files.3~1
  // covers: plansource.check-descriptor-files.6~1, plansource.check-descriptor-files.7~1, plansource.check-descriptor-files.8~1
  namespace fs       = std::filesystem;
  const fs::path dir = "desc_scope";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "ROTATION 'desc_scope_counter.txt'\n"
                          "STORAGE 'desc_scope'\n"
                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a+1 STREAM one FROM src\n"
                          "SELECT a+2 STREAM two FROM src\n"
                          "SELECT a+3 STREAM vol FROM src VOLATILE\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto names = planStreamNames(plan);
  ASSERT_LT(std::ranges::find(names, "one"), std::ranges::find(names, "two")) << "kontrola kolejnosci planu";

  std::ofstream(dir / "vol.desc") << "{ DOUBLE v TYPE UNKNOWN }";
  EXPECT_EQ(checkDescriptorFiles(plan, {}).message, "") << "MEMORY pod rotacja znika przy starcie";
  EXPECT_TRUE(checkDescriptorFiles(plan, {}, {"vol"}).message.contains("unsupported storage type"));
  fs::remove(dir / "vol.desc");

  std::ofstream(dir / "two.desc") << "{ INTEGER two_0 REF \"elsewhere\" }";
  std::ofstream(dir / "one.desc") << "{ INTEGER one_0 REF \"elsewhere\" }";
  const auto moved = checkDescriptorFiles(plan, {});
  EXPECT_EQ(moved.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(moved.message.starts_with("stream 'one'")) << moved.message;
  fs::remove(dir / "one.desc");
  fs::remove(dir / "two.desc");

  const auto declared = [&](const std::string &desc) {
    std::ofstream(dir / "src.desc") << desc;
    return checkDescriptorFiles(plan, {});
  };
  const auto wrongKind = declared("{ INTEGER a REF \"data.txt\" TYPE DIRECT }");
  EXPECT_EQ(wrongKind.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(wrongKind.message.contains("unsupported storage type 'DIRECT'")) << wrongKind.message;
  const auto wrongType = declared("{ INTEGER a REF \"data.txt\" TYPE BINFILE }");
  EXPECT_EQ(wrongType.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(wrongType.message.contains("was written for TYPE BINFILE")) << wrongType.message;
  const auto wrongRef = declared("{ INTEGER a REF \"other.txt\" TYPE TEXTSOURCE }");
  EXPECT_EQ(wrongRef.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(wrongRef.message.contains("was written for source 'other.txt'")) << wrongRef.message;

  fs::remove(dir / "src.desc");
  fs::create_directory(dir / "src.desc");
  const auto notFile = checkDescriptorFiles(plan, {});
  EXPECT_EQ(notFile.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(notFile.message.contains("is not a regular file")) << notFile.message;

  fs::remove_all(dir);
  std::ofstream(dir) << "not a directory";
  const auto lstatError = checkDescriptorFiles(plan, {});
  EXPECT_EQ(lstatError.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(lstatError.err, ENOTDIR);
  fs::remove(dir);
}

// Kontrola zrodel deklarowanych: wyniki nie sa zrodlami, ENOTDIR to K4 jak brak pliku,
// inny blad stat to K4 z errno, rozjazd rodzaju to K2, a forma przestarzala podpowiada slowo.
TEST(PlanSource, declared_source_check_scope_classes_and_deprecated_hints) {
  // covers: plansource.check-declared-sources.1~1, plansource.check-declared-sources.3~2, plansource.check-declared-sources.4~1
  // covers: plansource.check-declared-sources.5~1
  namespace fs       = std::filesystem;
  const fs::path dir = "declared_scope";
  fs::remove_all(dir);
  fs::create_directory(dir);
  const std::string fifo    = (dir / "feed.fifo").string();
  const std::string regular = (dir / "values.dat").string();
  const std::string loop    = (dir / "loop").string();
  const std::string device  = "/dev/shm/rdb_planSource_" + std::to_string(::getpid());
  ASSERT_EQ(::mkfifo(fifo.c_str(), 0600), 0);
  std::ofstream(regular) << "1\n";
  std::ofstream(device) << "1\n";
  fs::create_symlink("loop", loop);

  const auto check = [](const std::string &text) {
    qTree plan;
    EXPECT_EQ(parsePlanText(plan, text).status, "OK") << text;
    return withinFiveSeconds([&plan] { return checkDeclaredSources(plan); });
  };
  const auto declare = [&check](const std::string &type, const std::string &source) {
    return check("DECLARE a " + type + " STREAM src, 1 " + source + "\n");
  };

  EXPECT_EQ(
      check("DECLARE a INTEGER STREAM src, 1 BINFILE '" + regular + "'\nSELECT a STREAM out FROM src FILE '" + fifo + "'\n")
          .message,
      "")
      << "plik wyniku nie jest zrodlem";
  const auto underFile = declare("INTEGER", "BINFILE '" + regular + "/x'");
  EXPECT_EQ(underFile.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(underFile.err, ENOTDIR);
  const auto looped = declare("INTEGER", "BINFILE '" + loop + "'");
  EXPECT_EQ(looped.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(looped.err, ELOOP);
  EXPECT_EQ(declare("INTEGER", "BINFILE '" + fifo + "'").cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(declare("INTEGER", "FILE '" + device + "'").message,
            "stream 'src': DEVICE '" + device +
                "' is a regular file, not a character device or FIFO (deprecated FILE resolved this path as DEVICE; declare "
                "it with BINFILE or TEXTFILE)");
  EXPECT_EQ(declare("INTEGER", "FILE '" + dir.string() + "'").message,
            "stream 'src': BINFILE '" + dir.string() +
                "' is a directory, not a regular file (deprecated FILE resolved this path as BINFILE)");

  fs::remove(device);
  fs::remove_all(dir);
}

// Brzegi checkStorageDirectory: plan z samych dyrektyw i katalog roboczy nie maja czego sprawdzac,
// "/" zostaje "/", odmowa braku podaje sciezke bezwzgledna i polecenie, ENOTDIR ze stat to K2, a
// inny blad stat - K4.
TEST(PlanSource, storage_directory_check_edges) {
  // covers: plansource.check-storage-directory.1~1, plansource.check-storage-directory.2~1
  // covers: plansource.check-storage-directory.3~1, plansource.check-storage-directory.4~1
  namespace fs              = std::filesystem;
  const std::string missing = "storage_edges_missing";
  const std::string file    = "storage_edges_file";
  const std::string loop    = "storage_edges_loop";
  fs::remove_all(missing);
  fs::remove(file);
  fs::remove(loop);
  std::ofstream(file) << "not a directory";
  fs::create_symlink(loop, loop);

  qTree directives;
  ASSERT_EQ(parsePlanText(directives, "STORAGE '" + missing + "'\n").status, "OK");
  EXPECT_EQ(checkStorageDirectory(directives, {}).message, "");
  qTree bare;
  ASSERT_EQ(parsePlanText(bare, "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n").status, "OK");
  EXPECT_EQ(checkStorageDirectory(bare, {}).message, "");
  EXPECT_EQ(checkStorageDirectory(bare, "//").message, "");

  const auto absent = checkStorageDirectory(bare, missing);
  EXPECT_EQ(absent.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(absent.err, ENOENT);
  EXPECT_TRUE(absent.message.contains(absolutePathOf(missing))) << absent.message;
  EXPECT_TRUE(absent.message.contains("mkdir -p " + missing)) << absent.message;
  EXPECT_FALSE(fs::exists(missing));

  const auto underFile = checkStorageDirectory(bare, file + "/sub");
  EXPECT_EQ(underFile.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(underFile.err, ENOTDIR);
  const auto looped = checkStorageDirectory(bare, loop);
  EXPECT_EQ(looped.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(looped.err, ELOOP);

  fs::remove(file);
  fs::remove(loop);
}

// checkPlanStorage zwraca odmowe pierwszej kontroli w porzadku: zachowane magazyny, deskryptory,
// zrodla, pliki wyjscia. Kazdy krok usuwa jedna przyczyne. Licznika rotacji nie czyta.
TEST(PlanSource, plan_storage_check_order_and_counter_left_to_the_caller) {
  // covers: plansource.check-plan-storage.1~1, plansource.check-plan-storage.2~1
  namespace fs       = std::filesystem;
  const fs::path dir = "storage_order";
  fs::remove_all(dir);
  fs::create_directory(dir);
  const std::string source = (dir / "source").string();
  ASSERT_EQ(::mkfifo(source.c_str(), 0600), 0);
  std::ofstream(dir / "counter") << "invalid";
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "ROTATION 'storage_order/counter'\n"
                          "STORAGE 'storage_order'\n"
                          "DECLARE a INTEGER STREAM src, 1 BINFILE '" +
                              source +
                              "'\n"
                              "SELECT a+1 STREAM out FROM src FILE 'missing/out' STORAGE DIRECT\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  ASSERT_FALSE(checkRotationCounter(plan).refusal.empty()) << "kontrola: licznik jest wadliwy";

  std::ofstream(dir / "out.desc") << "{ INTEGER out_0 RETENTION 5 3 REF \"x\" }";
  EXPECT_TRUE(withinFiveSeconds([&plan] { return checkPlanStorage(plan, {}); }).message.starts_with("Stream 'out' keeps"));
  std::ofstream(dir / "out.desc") << "{ INTEGER out_0 REF \"x\" }";
  EXPECT_TRUE(withinFiveSeconds([&plan] { return checkPlanStorage(plan, {}); }).message.contains("moves the stream's data"));
  fs::remove(dir / "out.desc");
  EXPECT_TRUE(withinFiveSeconds([&plan] { return checkPlanStorage(plan, {}); }).message.contains("is a FIFO"));
  fs::remove(source);
  std::ofstream(source) << "1\n";
  EXPECT_TRUE(checkPlanStorage(plan, {}).message.contains("cannot open output file"));
  fs::create_directory(dir / "missing");
  EXPECT_EQ(checkPlanStorage(plan, {}).message, "");

  fs::remove_all(dir);
}

// Ostrzezenia o TIMEOUT ida w kolejnosci wierszy planu, choc kompilator stawia szybszy strumien
// wczesniej.
TEST(PlanSource, device_timeout_warnings_follow_the_plan_lines) {
  // covers: plansource.device-timeout-warnings.2~1
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "DECLARE a BYTE STREAM slow, 1 DEVICE '/dev/zero' TIMEOUT 2\n"
                          "DECLARE b BYTE STREAM fast, 1/10 DEVICE '/dev/zero' TIMEOUT 0.5\n"
                          "SELECT slow[0] STREAM out FROM slow\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto names = planStreamNames(plan);
  ASSERT_LT(std::ranges::find(names, "fast"), std::ranges::find(names, "slow")) << "kontrola: plan stoi w innej kolejnosci";

  EXPECT_EQ(deviceTimeoutWarnings(plan, std::nullopt),
            (std::vector<std::string>{
                "line 1: DECLARE slow: TIMEOUT 2 s (RQL) is longer than the interval 1 s; waiting overruns the slot",
                "line 2: DECLARE fast: TIMEOUT 0.5 s (RQL) is longer than the interval 0.1 s; waiting overruns the slot"}));
}

// Sciezki z dyrektyw: licznik z :ROTATION bezwzgledny, katalog z ostatniej :STORAGE, magazyny bez
// przestawienia przez REF zachowanego `.desc`. absolutePathOf: pusty napis, normalizacja, a przy
// bledzie weakly_canonical sciezka bezwzgledna, przy bledzie absolute - wejscie bez zmian.
TEST(PlanSource, path_helpers_follow_directives_and_normalize) {
  // covers: plansource.plan-stream-names.1~1, plansource.plan-counter-path.1~1, plansource.plan-storage-dir.1~1
  // covers: plansource.plan-store-paths.4~1, plansource.absolute-path-of.1~1, plansource.absolute-path-of.2~1
  namespace fs       = std::filesystem;
  const fs::path cwd = fs::current_path();
  const fs::path dir = "helpers_store";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "ROTATION 'helpers_counter.txt'\n"
                          "STORAGE 'helpers_ignored'\n"
                          "STORAGE 'helpers_store'\n"
                          "DECLARE a INTEGER STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a+1 STREAM out FROM src\n")
                .status,
            "OK");
  EXPECT_EQ(planStreamNames(plan), (std::vector<std::string>{"src", "out"}));
  EXPECT_EQ(planCounterPath(plan), (cwd / "helpers_counter.txt").string());
  EXPECT_EQ(planStorageDir(plan, "/opt/rdb"), "helpers_store/") << "parser zapisuje katalog z separatorem";
  std::ofstream(dir / "out.desc") << "{ INTEGER out_0 REF \"elsewhere\" }";
  EXPECT_EQ(planStorePaths(plan, {}), (std::vector<std::string>{(cwd / "helpers_store/out").string()}));

  EXPECT_EQ(absolutePathOf(""), "");
  EXPECT_EQ(absolutePathOf("helpers_store/../helpers_x"), (cwd / "helpers_x").string());
  fs::create_symlink("helpers_loop", "helpers_loop");
  EXPECT_EQ(absolutePathOf("helpers_loop/../x"), (cwd / "helpers_loop/../x").string()) << "bez normalizacji";
  fs::remove("helpers_loop");

  fs::create_directory("helpers_gone");
  ASSERT_EQ(::chdir("helpers_gone"), 0);
  fs::remove(cwd / "helpers_gone");
  const std::string orphan = absolutePathOf("x");
  ASSERT_EQ(::chdir(cwd.c_str()), 0);
  EXPECT_EQ(orphan, "x");

  fs::remove_all(dir);
}

// Zakres checkOutputFilesOpenable i sprawdzane pliki: deklaracje i MEMORY pominiete nawet z listy,
// DEFAULT z RETENTION sprawdza `_segment_0` i jego cien, DIRECT bez cienia. Istniejace FIFO jest
// otwierane i odrzucane jako K2, ENOTDIR z open to K2, katalog bez prawa zapisu - K4.
TEST(PlanSource, output_preflight_scope_segments_shadows_and_permissions) {
  // covers: plansource.check-output-files-openable.1~1, plansource.check-output-files-openable.2~1
  // covers: plansource.check-output-files-openable.3~1, plansource.check-output-files-openable.4~1
  // covers: plansource.check-output-files-openable.5~1, plansource.check-output-files-openable.6~1
  namespace fs       = std::filesystem;
  const fs::path dir = "output_scope";
  fs::remove_all(dir);
  fs::create_directory(dir);
  qTree plan;
  ASSERT_EQ(parsePlanText(plan,
                          "STORAGE 'output_scope'\n"
                          "DECLARE a DOUBLE STREAM src, 1 TEXTFILE 'data.txt'\n"
                          "SELECT a STREAM seg FROM src RETENTION 5 2\n"
                          "SELECT a STREAM raw FROM src STORAGE DIRECT\n"
                          "SELECT a STREAM vol FROM src VOLATILE\n"
                          "SELECT a STREAM nested FROM src FILE 'plain/out' STORAGE DIRECT\n"
                          "SELECT a STREAM guarded FROM src FILE 'readonly/out' STORAGE DIRECT\n")
                .status,
            "OK");
  compiler cm(plan);
  ASSERT_EQ(cm.compile(), "OK");
  const auto check = [&plan](const std::string &name) { return checkOutputFilesOpenable(plan, {}, {name}); };

  fs::create_directory(dir / "src");
  fs::create_directory(dir / "vol");
  EXPECT_EQ(check("src").message, "");
  EXPECT_EQ(check("vol").message, "");

  EXPECT_EQ(check("seg").message, "");
  fs::create_directory(dir / "seg_segment_0");
  const auto segment = check("seg");
  EXPECT_EQ(segment.err, EISDIR);
  EXPECT_TRUE(segment.message.contains("seg_segment_0'")) << segment.message;
  fs::remove(dir / "seg_segment_0");
  fs::create_directory(dir / "seg_segment_0.shadow");
  EXPECT_TRUE(check("seg").message.contains("seg_segment_0.shadow")) << "DEFAULT sprawdza cien";
  fs::create_directory(dir / "seg_segment_0");
  EXPECT_TRUE(check("seg").message.contains("seg_segment_0'")) << "pierwsza odmowa: segment przed cieniem";
  fs::create_directory(dir / "raw.shadow");
  EXPECT_EQ(check("raw").message, "") << "DIRECT nie ma cienia";

  ASSERT_EQ(::mkfifo((dir / "raw").c_str(), 0600), 0);
  const auto fifo = withinFiveSeconds([&check] { return check("raw"); });
  EXPECT_EQ(fifo.cls, rdb::ErrorClass::Plan);
  EXPECT_TRUE(fifo.message.contains("is not a regular file")) << fifo.message;

  std::ofstream(dir / "plain") << "not a directory";
  const auto underFile = check("nested");
  EXPECT_EQ(underFile.cls, rdb::ErrorClass::Plan);
  EXPECT_EQ(underFile.err, ENOTDIR);

  if (::geteuid() != 0) {
    fs::create_directory(dir / "readonly");
    fs::permissions(dir / "readonly", fs::perms::owner_read | fs::perms::owner_exec);
    const auto readOnly = check("guarded");
    fs::permissions(dir / "readonly", fs::perms::owner_all);
    EXPECT_EQ(readOnly.cls, rdb::ErrorClass::Environment);
    EXPECT_EQ(readOnly.err, EACCES);
    EXPECT_FALSE(fs::exists(dir / "readonly/out"));
  }
  fs::remove_all(dir);
}

// Niedostepne wejscie nie oznacza EOF ani poprawnego strumienia NULL (#488).
TEST(PlanSource, missing_declared_sources_preserve_environment_class_errno_and_context) {
  // covers: plansource.check-declared-sources.3~2
  namespace fs       = std::filesystem;
  const fs::path dir = "declared_missing";
  fs::remove_all(dir);
  fs::create_directory(dir);
  std::ofstream(dir / "file") << "1\n";
  fs::create_symlink("absent", dir / "dangling");
  for (const std::string keyword : {"TEXTFILE", "BINFILE", "DEVICE"}) {
    for (const auto &[path, expected] : std::vector<std::pair<fs::path, int>>{{dir / "absent", ENOENT},
                                                                              {dir / "parent/input", ENOENT},
                                                                              {dir / "file/input", ENOTDIR},
                                                                              {dir / "dangling", ENOENT}}) {
      qTree plan;
      ASSERT_EQ(parsePlanText(plan, "DECLARE a INTEGER STREAM src, 1 " + keyword + " '" + path.string() + "'\n").status, "OK");
      const auto refusal = checkDeclaredSources(plan);
      EXPECT_EQ(refusal.cls, rdb::ErrorClass::Environment) << keyword << ": " << path;
      EXPECT_EQ(refusal.err, expected);
      EXPECT_TRUE(refusal.message.contains("stream 'src'")) << refusal.message;
      EXPECT_TRUE(refusal.message.contains(path.string())) << refusal.message;
      EXPECT_TRUE(refusal.message.contains(std::strerror(expected))) << refusal.message;
      EXPECT_TRUE(checkDeclaredSources(plan, {"other"}).message.empty());
    }
  }
  fs::remove_all(dir);
}
