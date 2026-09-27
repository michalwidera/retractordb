#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"

/// @brief Wczytywanie zestawu RQL z tekstu - wspolne zrodlo startu z pliku i przeladowania
/// w locie (`xqry --reset`).
///
/// Przedmiotem badania jest jedna rzecz, ktora rozni sie tu od dawnego kodu launchera: zestaw
/// PUSTY nie jest bledem. Do 2026-09-05 pusty plik zapytan konczyl proces bledem parsowania
/// ("Empty file."), przez co udokumentowana w jednostce systemd sciezka "pusty plik = tryb
/// bezczynny" nie dzialala wcale, a Restart=on-failure zapetlal start uslugi.

TEST(PlanSource, empty_text_is_an_empty_plan_not_an_error) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, comments_and_blank_lines_alone_are_an_empty_plan) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "# tylko komentarz\n\n   \n# i jeszcze jeden\n");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, statements_land_in_the_plan_and_in_the_line_list) {
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
    return checkKeptStores(plan, {});
  };

  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 2 }", true), "OK");
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 2 TYPE DIRECT }", true), "OK");
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 3 }", true),
            "Stream 'seg' keeps its files under ROTATION, but kept_store/seg.desc was written with STORAGE DIRECT RETENTION 5 3 "
            "and the plan asks for STORAGE DIRECT RETENTION 5 2: put STORAGE DIRECT RETENTION 5 3 back in the plan, or remove "
            "the stream's files to start it afresh");
  EXPECT_TRUE(check("{ INTEGER seg_0 }", true).contains("written with STORAGE DIRECT without RETENTION")) << "no retention";
  EXPECT_TRUE(
      check("{ INTEGER seg_0 RETMEMORY 5 TYPE DEFAULT }", true).contains("written with STORAGE DEFAULT without RETENTION"))
      << ".desc sprzed D7";
  // Bez rotacji pliki i tak znikaja przy starcie, wiec nie ma czego porownywac.
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 3 }", false), "OK");

  fs::remove_all(dir);
}
