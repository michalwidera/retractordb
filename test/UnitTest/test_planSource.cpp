#include <sys/stat.h>  // mkfifo

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
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
  EXPECT_TRUE(check("{\n INTEGER }\n", true).contains("Fail: line 2:9"));
  // Bez rotacji pliki i tak znikaja przy starcie, wiec nie ma czego porownywac.
  EXPECT_EQ(check("{ INTEGER seg_0 RETENTION 5 3 }", false), "OK");

  fs::remove_all(dir);
}

TEST(PlanSource, invalid_declared_descriptor_is_rejected_before_plan_activation) {
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
  const std::string error = checkDescriptorFiles(plan, {});
  EXPECT_TRUE(error.contains("src.desc")) << error;
  EXPECT_TRUE(error.contains("Fail: line 2:9")) << error;

  fs::remove_all(dir);
}

// Wstepne sprawdzenie ma dawac ten sam werdykt co storage::attachDescriptor -> verifyDescriptorMatch
// (plan == plik). Descriptor::operator== jest asymetryczny: odwrocony kierunek odrzucal plan, ktory
// silnik przyjmuje, i przepuszczal taki, na ktorym silnik konczyl sie FatalError-em.
TEST(PlanSource, declared_descriptor_check_uses_the_runtime_direction) {
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
    return checkDescriptorFiles(plan, {});
  };

  EXPECT_TRUE(check("INTEGER", "DOUBLE").contains("descriptor schema mismatch")) << "silnik konczy sie FatalError-em";
  EXPECT_EQ(check("DOUBLE", "INTEGER"), "OK") << "silnik przyjmuje ten plan";

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
// znakowe albo FIFO. Brak pliku nie jest odmowa - akcesor ostrzega i daje NULL jak dotad.
TEST(PlanSource, declared_source_kind_is_checked_without_opening_the_path) {
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
    return withinFiveSeconds([&plan] { return checkDeclaredSources(plan); });
  };

  EXPECT_EQ(check("BINFILE '" + fifo + "'"), "stream 'src': BINFILE '" + fifo + "' is a FIFO, not a regular file");
  EXPECT_EQ(check("TEXTFILE '" + fifo + "'"), "stream 'src': TEXTFILE '" + fifo + "' is a FIFO, not a regular file");
  EXPECT_EQ(check("BINFILE '/dev/null'"), "stream 'src': BINFILE '/dev/null' is a character device, not a regular file");
  EXPECT_EQ(check("TEXTFILE '/dev/null'"), "stream 'src': TEXTFILE '/dev/null' is a character device, not a regular file");
  EXPECT_EQ(check("BINFILE '" + dir.string() + "'"),
            "stream 'src': BINFILE '" + dir.string() + "' is a directory, not a regular file");
  EXPECT_EQ(check("DEVICE '" + regular + "'"),
            "stream 'src': DEVICE '" + regular + "' is a regular file, not a character device or FIFO");

  EXPECT_EQ(check("DEVICE '" + fifo + "'"), "OK");
  EXPECT_EQ(check("DEVICE '/dev/null'"), "OK");
  EXPECT_EQ(check("BINFILE '" + regular + "'"), "OK");
  EXPECT_EQ(check("TEXTFILE '" + regular + "'"), "OK");
  EXPECT_EQ(check("BINFILE '" + (dir / "missing.bin").string() + "'"), "OK");

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
  EXPECT_EQ(checkDeclaredSources(plan, {"good"}), "OK");
  EXPECT_NE(checkDeclaredSources(plan), "OK");

  fs::remove_all(dir);
}

// Magazyn bierze TYPE i REF deklaracji z wczytanego `.desc`, a Descriptor::operator== porownuje
// tylko sloty danych. Do #346 plan wskazujacy `w.txt` czytal wiec po cichu `v.txt` sprzed zmiany.
// Jedyny przepuszczany rozjazd to dawny TYPE DEVICE zwyklego pliku binarnego - ten `.desc`
// zastepuje start.
TEST(PlanSource, declared_descriptor_must_match_the_source_kind_and_path) {
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
  EXPECT_EQ(checkDescriptorFiles(legacy, {}), "OK");
  dropStalePlanArtifacts(legacy);
  EXPECT_FALSE(fs::exists(descFile));

  // Ten sam dawny zapis, ale plan czyta plik jako tekst - to juz inne zrodlo.
  keep("rec.bin", "DEVICE");
  qTree text = plan("TEXTFILE 'rec.bin'");
  EXPECT_EQ(checkDescriptorFiles(text, {}), "stream 'src': " + descFile.string() +
                                                " was written for TYPE DEVICE and the plan declares TYPE TEXTSOURCE; remove " +
                                                descFile.string() + " to start the stream afresh");
  dropStalePlanArtifacts(text);
  EXPECT_TRUE(fs::exists(descFile)) << "start nie kasuje .desc, ktory nie jest dawnym zapisem BINFILE";

  // Dawny TYPE DEVICE dla innej sciezki nie jest dawnym zapisem tego strumienia.
  keep("old.bin", "DEVICE");
  qTree moved = plan("FILE 'rec.bin'");
  EXPECT_TRUE(checkDescriptorFiles(moved, {}).contains("was written for TYPE DEVICE and the plan declares TYPE BINFILE"));

  // Zmiana sciezki przy tym samym rodzaju.
  keep("v.txt", "TEXTSOURCE");
  qTree path = plan("FILE 'w.txt'");
  EXPECT_EQ(checkDescriptorFiles(path, {}), "stream 'src': " + descFile.string() +
                                                " was written for source 'v.txt' and the plan reads 'w.txt'; remove " +
                                                descFile.string() + " to start the stream afresh");

  // Urzadzenie bylo DEVICE przed zmiana i zostaje nim - `.desc` pasuje i zostaje.
  keep("/dev/urandom", "DEVICE");
  qTree device = plan("FILE '/dev/urandom'");
  EXPECT_EQ(checkDescriptorFiles(device, {}), "OK");
  dropStalePlanArtifacts(device);
  EXPECT_TRUE(fs::exists(descFile));

  fs::remove_all(dir);
}

// Ostrzezenia o formie przestarzalej ida w kolejnosci wierszy planu, choc kompilator sortuje
// wezly po interwale. Wiersz to pierwszy wiersz instrukcji takze przy kontynuacji `\`.
TEST(PlanSource, deprecated_file_warnings_follow_the_plan_lines) {
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
