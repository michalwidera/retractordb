#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <boost/rational.hpp>
#include <boost/system/error_code.hpp>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/qTree.hpp"

// ctest -R '^ut-test_compiler' -V

extern std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

qTree coreInstance;

bool compiled = false;

bool check_compile_function() {
  // assure compile only once
  std::ifstream infl("ut_compiler.rql");
  for (std::string line; std::getline(infl, line);) {
    SPDLOG_INFO("{}", line);
  }

  if (!compiled) {
    coreInstance.clear();
    compiled = parserRQLFile_4Test(coreInstance, "ut_compiler.rql") == "OK";
  }
  return compiled;
}

// ut_compiler.rql:
// DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'
// DECLARE c INTEGER,d BYTE STREAM core1, 0.5 FILE '/dev/urandom'
// SELECT core0[0],core0[1] STREAM str1 FROM core0#core1
// SELECT * STREAM test1 FROM core@(1,-10)
// SELECT * STREAM test2 FROM core@(1,10)

TEST(xparser, check_compile) { EXPECT_TRUE(check_compile_function()); }

TEST(xparser, check_compile_result) {
  EXPECT_TRUE(check_compile_function());

  SPDLOG_INFO("coreInstance.size() {}", coreInstance.size());

  for (auto q : coreInstance) {
    SPDLOG_INFO("coreInstance[] {}, {}", q.id, q.filename);
    if (q.id == "test1") {
      EXPECT_TRUE(q.isDeclaration() == false);
    }
  }
}

TEST(xparser, check_parserRQLString) {
  auto [result, first_keyword, stream_name] =
      parserRQLString(coreInstance, "DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE '/dev/urandom'");
  EXPECT_TRUE(result == "OK");
  EXPECT_TRUE(first_keyword == "DECLARE");
  EXPECT_TRUE(stream_name == "core0");
}

TEST(xparser, check_topological_sort) { qTree myInstance; }

TEST(xparser, check_multiline_backslash) {
  qTree instance;
  auto result = parserRQLFile_4Test(instance, "ut_multiline.rql");
  EXPECT_EQ(result, "OK");
  EXPECT_TRUE(instance.exists("core0"));
  EXPECT_TRUE(instance.exists("str1"));
}

TEST(xcompiler, shares_commutative_add_select_computation) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT a[_]*b[_] STREAM c1 FROM a+b
        SELECT a[_]*b[_] STREAM c2 FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");
  ASSERT_TRUE(instance.exists("c1"));
  ASSERT_TRUE(instance.exists("c2"));

#if RDB_OPT_SHARE_EQUIVALENT_SELECTS && RDB_OPT_COMMUTATIVE_ADD
  auto &c1 = instance.getQuery("c1");
  auto &c2 = instance.getQuery("c2");
  ASSERT_EQ(c1.lProgram.size(), 1);
  ASSERT_EQ(c2.lProgram.size(), 1);

  const auto sharedId = c1.lProgram.front().getStr_();
  EXPECT_EQ(c2.lProgram.front().getStr_(), sharedId);
  EXPECT_TRUE(sharedId.starts_with("STREAM_SELECT_"));
  ASSERT_TRUE(instance.exists(sharedId));
  EXPECT_TRUE(instance.getQuery(sharedId).isSubstrat);
  EXPECT_EQ(instance.getQuery(sharedId).lSchema.size(), 2);
#else
  EXPECT_EQ(instance.getQuery("c1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("c2").lProgram.size(), 3);
  EXPECT_EQ(std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); }), 0);
#endif
}

TEST(xcompiler, shares_syntactically_identical_add_select_computation) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT a[_]*b[_] STREAM c1 FROM a+b
        SELECT a[_]*b[_] STREAM c2 FROM a+b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

#if RDB_OPT_SHARE_EQUIVALENT_SELECTS
  ASSERT_EQ(instance.getQuery("c1").lProgram.size(), 1);
  ASSERT_EQ(instance.getQuery("c2").lProgram.size(), 1);
  EXPECT_EQ(instance.getQuery("c1").lProgram.front().getStr_(), instance.getQuery("c2").lProgram.front().getStr_());
#else
  EXPECT_EQ(instance.getQuery("c1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("c2").lProgram.size(), 3);
  EXPECT_EQ(std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); }), 0);
#endif
}

TEST(xcompiler, does_not_share_order_sensitive_selects) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT * STREAM d1 FROM a+b
        SELECT * STREAM d2 FROM b+a
        SELECT a[0],b[1] STREAM e1 FROM a+b
        SELECT b[1],a[0] STREAM e2 FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  const auto sharedCount =
      std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); });
  EXPECT_EQ(sharedCount, 0);
  EXPECT_EQ(instance.getQuery("d1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("d2").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("e1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("e2").lProgram.size(), 3);
}

TEST(xcompiler, does_not_share_different_result_shapes) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT to_string(a[0]:8) STREAM narrow FROM a+b
        SELECT to_string(a[0]:16) STREAM wide FROM b+a
        SELECT to_float(a[0]) STREAM as_float FROM a+b
        SELECT to_double(a[0]) STREAM as_double FROM b+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  const auto sharedCount =
      std::ranges::count_if(instance, [](const query &qry) { return qry.id.starts_with("STREAM_SELECT_"); });
  EXPECT_EQ(sharedCount, 0);
  EXPECT_EQ(instance.getQuery("narrow").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("wide").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("as_float").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("as_double").lProgram.size(), 3);
}

TEST(xcompiler, preserves_add_grouping_in_computation_fingerprint) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        DECLARE cx INTEGER, cy INTEGER STREAM c, 3 FILE 'c.txt'
        SELECT a[_]*b[_]+c[_] STREAM x1 FROM (a+b)+c
        SELECT a[_]*b[_]+c[_] STREAM x2 FROM (b+a)+c
        SELECT a[_]*b[_]+c[_] STREAM x3 FROM (c+b)+a
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

#if RDB_OPT_SHARE_EQUIVALENT_SELECTS && RDB_OPT_COMMUTATIVE_ADD
  auto &x1 = instance.getQuery("x1");
  auto &x2 = instance.getQuery("x2");
  auto &x3 = instance.getQuery("x3");
  ASSERT_EQ(x1.lProgram.size(), 1);
  ASSERT_EQ(x2.lProgram.size(), 1);
  EXPECT_EQ(x1.lProgram.front().getStr_(), x2.lProgram.front().getStr_());
  EXPECT_EQ(x3.lProgram.size(), 3);
  EXPECT_FALSE(instance.exists("STREAM_ADD_b_a"));
  EXPECT_TRUE(instance.exists("STREAM_ADD_c_b"));
#else
  EXPECT_EQ(instance.getQuery("x1").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("x2").lProgram.size(), 3);
  EXPECT_EQ(instance.getQuery("x3").lProgram.size(), 3);
#endif
}

TEST(xcompiler, toggles_substrate_deduplication) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE av INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bv INTEGER STREAM b, 1 FILE 'b.txt'
        SELECT * STREAM sum FROM a+b
        SELECT shifted[0] STREAM shifted FROM (a+b)>1
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

#if RDB_OPT_DEDUP_SUBSTRATES
  EXPECT_FALSE(instance.exists("STREAM_ADD_a_b"));
#else
  EXPECT_TRUE(instance.exists("STREAM_ADD_a_b"));
#endif
}

TEST(xcompiler, toggles_matched_hash_time_move_factorization) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE av INTEGER STREAM A, 0.1 FILE 'a.txt'
        DECLARE bv INTEGER STREAM B, 0.2 FILE 'b.txt'
        SELECT * STREAM matched FROM (A>2)#(B>1)
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

#if RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES
  EXPECT_TRUE(instance.exists("STREAM_HASH_A_B"));
  EXPECT_FALSE(instance.exists("STREAM_TIMEMOVE_A"));
  EXPECT_FALSE(instance.exists("STREAM_TIMEMOVE_B"));
#else
  EXPECT_FALSE(instance.exists("STREAM_HASH_A_B"));
  EXPECT_TRUE(instance.exists("STREAM_TIMEMOVE_A"));
  EXPECT_TRUE(instance.exists("STREAM_TIMEMOVE_B"));
#endif
}

TEST(xcompiler, does_not_rewrite_existing_queries_during_import) {
  qTree live;
  auto [baseParseResult, baseKeyword, baseStream] = parserRQLString(live, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 2 FILE 'b.txt'
        SELECT a[_]*b[_] STREAM c1 FROM a+b
      )");
  ASSERT_EQ(baseParseResult, "OK");

  compiler liveCompiler(live);
  ASSERT_EQ(liveCompiler.compile(), "OK");
  ASSERT_EQ(live.getQuery("c1").lProgram.size(), 3);

  qTree importedPlan                                    = live;
  auto [importParseResult, importKeyword, importStream] = parserRQLString(importedPlan, "SELECT a[_]*b[_] STREAM c2 FROM b+a");
  ASSERT_EQ(importParseResult, "OK");

  compiler importedCompiler(importedPlan);
  ASSERT_EQ(importedCompiler.compile(), "OK");
  auto importedIds = liveCompiler.importFrom(importedPlan);
  ASSERT_FALSE(importedIds.empty());
  ASSERT_EQ(liveCompiler.compile(), "OK");

  EXPECT_EQ(live.getQuery("c1").lProgram.size(), 3);
  ASSERT_TRUE(live.exists("c2"));
  EXPECT_EQ(live.getQuery("c2").lProgram.size(), 3);
}

// Ogon strumienia (query::startupLatency): liczba poczatkowych slotow wlasnego interwalu bez
// zdefiniowanego wyniku. Jest obserwowalna semantyka runtime, a nie prefiks rekordow-zastepnikow.
TEST(xcompiler, computes_startup_latency) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE value INTEGER STREAM a, 1/10 FILE 'a.txt'
        DECLARE value INTEGER STREAM b, 1/5  FILE 'b.txt'
        DECLARE value INTEGER STREAM c, 1/10 FILE 'c.txt'
        DECLARE value INTEGER STREAM a35, 3/100   FILE 'a.txt'
        DECLARE value INTEGER STREAM b35, 1/20    FILE 'b.txt'
        DECLARE value INTEGER STREAM a32, 3/100   FILE 'a.txt'
        DECLARE value INTEGER STREAM b32, 1/50    FILE 'b.txt'
        DECLARE value INTEGER STREAM a711, 7/100  FILE 'a.txt'
        DECLARE value INTEGER STREAM b711, 11/100 FILE 'b.txt'
        DECLARE value INTEGER STREAM a160, 4/25   FILE 'a.txt'
        DECLARE value INTEGER STREAM b147, 147/1000 FILE 'b.txt'
        DECLARE x INTEGER, y INTEGER, z INTEGER STREAM wide, 1/10 FILE 'a.txt'
        DECLARE value INTEGER STREAM subsrc, 1/10 FILE 'a.txt'
        SELECT a[0]+0 STREAM mid       FROM a
        SELECT * STREAM shifted        FROM mid>3
        SELECT * STREAM shifted_twice  FROM shifted>2
        SELECT * STREAM hash_slow_snd  FROM a#b
        SELECT * STREAM hash_fast_snd  FROM b#a
        SELECT * STREAM hash_equal     FROM a#c
        SELECT * STREAM hash_3_5       FROM a35#b35
        SELECT * STREAM hash_3_2       FROM a32#b32
        SELECT * STREAM hash_7_11      FROM a711#b711
        SELECT * STREAM hash_160_147   FROM a160#b147
        SELECT * STREAM added          FROM a+c
        SELECT * STREAM agse4          FROM a@(1,4)
        SELECT * STREAM agse4_mirror   FROM a@(1,-4)
        SELECT * STREAM sub_declared   FROM a-1/5
        SELECT * STREAM sub_computed   FROM mid-1/5
        SELECT * STREAM sub_fractional FROM mid-3/20
        SELECT * STREAM reduced        FROM agse4.sumc
        SELECT * STREAM wide_shifted   FROM wide>1
        SELECT * STREAM wide_nested    FROM wide_shifted@(1,2)
        SELECT * STREAM sub_same       FROM subsrc-1/10
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Zrodlo deklarowane emituje od pierwszego slotu.
  EXPECT_EQ(instance.getQuery("a").startupLatency, 0);
  // Przepisanie bez operatora nie wnosi opoznienia.
  EXPECT_EQ(instance.getQuery("mid").startupLatency, 0);
  // tau_N to N slotow opoznienia, kumulowanych wzdluz lancucha — ale po przestemplowaniu
  // to opoznienie jest ORIGIN, nie ogonem: rekord n ma tresc rekordu n-N, wiec rekordy ponizej
  // N nie maja definicji. Ogon wynosi max(0, W_src - N): rekord n-N jest STARSZY od biezacego,
  // wiec producent o ogonie nie wiekszym od N nie kaze na niego czekac ani slotu.
  EXPECT_EQ(instance.getQuery("shifted").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("shifted").logicalOrigin, 3);
  EXPECT_EQ(instance.getQuery("shifted_twice").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("shifted_twice").logicalOrigin, 5);
  // phi: ogon obejmuje najgorsza faze odczytu drugiego argumentu w jednym okresie.
  // Dla zredukowanego delta1/delta2=p/q wynosi ceil((p+q-1)/p).
  EXPECT_EQ(instance.getQuery("hash_slow_snd").startupLatency, 2);
  EXPECT_EQ(instance.getQuery("hash_fast_snd").startupLatency, 1);
  EXPECT_EQ(instance.getQuery("hash_equal").startupLatency, 1);
  EXPECT_EQ(instance.getQuery("hash_3_5").startupLatency, 3);
  EXPECT_EQ(instance.getQuery("hash_3_2").startupLatency, 2);
  EXPECT_EQ(instance.getQuery("hash_7_11").startupLatency, 3);
  EXPECT_EQ(instance.getQuery("hash_160_147").startupLatency, 2);
  // Suma o zgodnych interwalach i zerowych ogonach zrodel nie wnosi opoznienia.
  EXPECT_EQ(instance.getQuery("added").startupLatency, 0);
  // AGSE po przestemplowaniu na koniec przedzialu: rekord n obejmuje pozycje
  // n-3 ... n, wiec czeka wylacznie na pole najnowsze (pozycja n), dostepne w
  // jego wlasnym slocie — ogon 0. Czekanie na komplet okna przeszlo do origin:
  // pierwsze pelne okno konczy sie na pozycji 3, czyli O = ceil(3/1) = 3.
  // Liczba i tresc emitowanych rekordow sie nie zmienia, zmienia sie ich indeks
  // logiczny — i to on naprawia zlaczenie okna z wlasnym zrodlem.
  EXPECT_EQ(instance.getQuery("agse4").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("agse4").logicalOrigin, 3);
  // Znak dlugosci jest wylacznie konwencja kolejnosci pol — ogon i origin te same.
  EXPECT_EQ(instance.getQuery("agse4_mirror").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("agse4_mirror").logicalOrigin, 3);
  // Różnica ma jeden slot dla deklaracji; całkowita faza producenta
  // obliczanego jest dostępna topologicznie, a faza 3/2 wymaga slotu.
  EXPECT_EQ(instance.getQuery("sub_declared").startupLatency, 1);
  EXPECT_EQ(instance.getQuery("sub_computed").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("sub_fractional").startupLatency, 1);
  // Redukcja działa na bieżącej pełnej krotce i tylko propaguje ogon ORAZ origin.
  EXPECT_EQ(instance.getQuery("reduced").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("reduced").logicalOrigin, 3);
  // Przesuniecie nad deklaracja: ogon producenta (0) i wlasne opoznienie w origin.
  EXPECT_EQ(instance.getQuery("wide_shifted").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("wide_shifted").logicalOrigin, 1);
  // Okno 2-polowe nad producentem o szerokosci 3 i ogonie 0: najnowsze pole rekordu n
  // to pozycja n, czyli rekord producenta floor(n/3), okreslony w chwili
  // (floor(n/3)+1+0)*D_src; stad ogon ceil(1*3/1)-1 = 2.
  EXPECT_EQ(instance.getQuery("wide_nested").startupLatency, 2);
  // Origin: producent ma origin 1, wiec okno musi zmiescic i jego, i wlasna rozpietosc:
  // O = ceil((1*3 + 2 - 1)/1) = 4.
  EXPECT_EQ(instance.getQuery("wide_nested").logicalOrigin, 4);
  EXPECT_EQ(instance.maxCapacity.at("wide_shifted"), 2);
  EXPECT_EQ(instance.getQuery("sub_same").startupLatency, 1);
  // K24/P1: pojemnosc zrodla roznicy wzrosla z 3 na 4. Wartosc 3 pokrywala
  // odleglosc wsteczna tylko dla ilorazu 1 i 2; od ilorazu 3 odczyt wypadal
  // poza historia i dawal cichy rekord all-NULL. Nowy czlon to
  // floor((1+Wout)*ratio) + prefetch deklaracji, brany jako maksimum ze stara
  // formula — pojemnosc nigdy nie maleje.
  EXPECT_EQ(instance.maxCapacity.at("subsrc"), 4);
}

// Pojemnosc bufora zrodla okna to odleglosc do najstarszego pola okna PLUS jeden:
// w buforze musza sie zmiescic oba konce zakresu. Regresja (plaski wykres detekcji
// QRS w examples/ecg): dla zrodla o szerokosci 1 odleglosc wypada calkowita, wiec
// zaokraglenie w gore dawalo o jeden rekord za malo — kolowy bufor MEMORY
// nadpisywal najstarsze pole okna rekordem najnowszym.
TEST(xcompiler, agse_capacity_covers_whole_window_over_computed_source) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE value INTEGER STREAM src, 1 FILE 'a.txt'
        SELECT * STREAM win1     FROM src@(1,3)
        SELECT win1[0] STREAM mid FROM win1
        SELECT * STREAM win2     FROM mid@(1,3)
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Okno 3-polowe nad deklaracja o szerokosci 1: rekord n czyta pozycje n-2..n,
  // wiec czeka tylko na pozycje n — ogon 0, a niedefiniowalny poczatek to origin
  // ceil(2/1) = 2. Przepisanie (mid) propaguje jedno i drugie.
  EXPECT_EQ(instance.getQuery("win1").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("win1").logicalOrigin, 2);
  EXPECT_EQ(instance.getQuery("mid").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("mid").logicalOrigin, 2);
  // Okno nad producentem o szerokosci 1 i ogonie 0 rowniez nie czeka; origin sklada
  // sie z origin producenta i rozpietosci wlasnego okna: ceil((2*1 + 2)/1) = 4.
  EXPECT_EQ(instance.getQuery("win2").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("win2").logicalOrigin, 4);
  // win2 czyta rekordy mid o indeksach n-2..n. W chwili emisji rekordu n=4 mid ma
  // wydane rekordy do indeksu 4, a najstarszy potrzebny to 2 — dystans 2, plus jeden
  // rekord na drugi koniec zakresu.
  EXPECT_EQ(instance.maxCapacity.at("mid"), 3);
}

// Tozsamosc R1 musi zachowywac ogon — inaczej przepisanie zmienialoby obserwowalna
// deklaracje opoznienia, nawet gdyby sekwencja rekordow byla identyczna.
TEST(xcompiler, startup_latency_is_preserved_by_shift_matching_identity) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE value INTEGER STREAM fa,  1/10 FILE 'a.txt'
        DECLARE value INTEGER STREAM fb,  1/5  FILE 'b.txt'
        DECLARE value INTEGER STREAM fa2, 1/10 FILE 'a.txt'
        DECLARE value INTEGER STREAM fb2, 1/5  FILE 'b.txt'
        SELECT * STREAM lhs FROM (fa>2)#(fb>1)
        SELECT * STREAM rhs FROM (fa2#fb2)>3
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Tozsamosc ma zachowywac CALA deklaracje opoznienia, a nie tylko jej sume — po
  // przestemplowaniu tau_N sklada sie ona z ogona (przeplot) i origin (przesuniecie).
  EXPECT_EQ(instance.getQuery("lhs").startupLatency, instance.getQuery("rhs").startupLatency);
  EXPECT_EQ(instance.getQuery("lhs").logicalOrigin, instance.getQuery("rhs").logicalOrigin);
  // Ogon przeplotu to 2, ale tau_3 nad nim NIE dokłada nic i wręcz go pochłania:
  // W = max(0, 2 - 3) = 0. Rekord 3 niesie rekord 0 przeplotu, dostepny w chwili 3*Delta,
  // a slot 3 konczy sie w 4*Delta — czekac nie ma na co. Do 2026-08-07 stalo tu 2, bo
  // adresowanie wzgledne w fetchBack wymuszalo W = W_src (K24p §2.2).
  EXPECT_EQ(instance.getQuery("rhs").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("rhs").logicalOrigin, 3);  // przesuniecie 2+1 slotow wyjscia
}

// Niezmiennik D3: przepisania planu nie zmieniaja nazw pol nazwanych strumieni uzytkownika.
//
// Scenariusz najbardziej narazony: deduplikacja scala substrat STREAM_ADD_s1_s2 z uzytkownikowym
// `mysum`, mimo ze ich schematy roznia sie NAZWAMI pol (predykat scalania porownuje tylko typ,
// dlugosc i krotnosc — i ma do tego prawo, bo scala wezly wewnetrzne). Po scaleniu `out` czyta
// z `mysum`, ale jego wlasny deskryptor musi pozostac nietkniety.
//
// Kontrola niepustosci sprawdzana mutacyjnie: wstrzykniecie zmiany nazwy pola w
// deduplicateSubstrats() konczy kompilacje bledem "changed observable field names".
TEST(xcompiler, rewrites_preserve_observable_field_names) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE a INTEGER STREAM s1, 1 FILE 'd1.dat'
        DECLARE b INTEGER STREAM s2, 1 FILE 'd2.dat'
        SELECT s1[0], s2[0] STREAM mysum FROM s1+s2
        SELECT out[0] STREAM out FROM (s1+s2)>1
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  auto fieldNames = [&instance](const std::string &id) {
    std::vector<std::string> names;
    for (const auto &f : instance.getQuery(id).lSchema)
      names.push_back(f.field_.rname);
    return names;
  };

  EXPECT_EQ(fieldNames("mysum"), (std::vector<std::string>{"mysum_0", "mysum_1"}));
  EXPECT_EQ(fieldNames("out"), (std::vector<std::string>{"out_0"}));

#if RDB_OPT_DEDUP_SUBSTRATES
  // Scalenie faktycznie zaszlo — inaczej test nie sprawdzalby niczego o przepisaniu.
  ASSERT_EQ(instance.getQuery("out").lProgram.front().getStr_(), "mysum");
  EXPECT_EQ(std::ranges::count_if(instance, [](const query &q) { return q.id.starts_with("STREAM_ADD_"); }), 0);
#endif
}

// Rozwiazywanie interwalow nie moze zalezec od kolejnosci planu.
//
// resolveStreamIntervals() liczy interwaly iteracyjnie; nierozwiazane zrodlo daje delte 0.
// Dwie sciezki przyjmowaly to zero bez zadania kolejnego przebiegu — program jednoelementowy
// (SELECT expr STREAM x FROM y) oraz STREAM_AGSE — wiec strumien dostawal interwal 0 na stale.
// Zaleznie od kolejnosci po coreInstance.sort() konczylo sie to albo zerowym mianownikiem,
// albo falszywym "Circular dependency": warunek konca (unresolvedCount >= prevUnresolved)
// wymagal SCISLEGO spadku licznika w kazdym przebiegu, czyli byl heurystyka postepu.
//
// Ten sam plan z 3 lancuchami kompilowal sie poprawnie, z 4, 5, 6, 8 i 12 — nie; z 7, 9, 10,
// 16, 20 i 32 znowu tak. Niemonotonicznosc w liczbie zapytan jest wlasnie objawem zaleznosci
// od kolejnosci, a nie od poprawnosci planu.
TEST(xcompiler, resolves_intervals_independently_of_plan_order) {
  // Liczby lancuchow dobrane tak, by trafic w defekt: przy dyrektywach planu zawodzily
  // 4, 5, 6, 8 i 12, a 1, 2, 3, 7, 9, 10 i 11 przechodzily. Ta niemonotonicznosc jest
  // objawem zaleznosci od kolejnosci; pojedyncza liczba lancuchow bylaby krucha.
  for (int chains : {4, 5, 6, 8, 12}) {
    std::string source = "STORAGE 'temp'\nSUBSTRAT 'memory'\n";
    source += "DECLARE value INTEGER STREAM a, 1/10 FILE 'a.txt'\n";
    source += "DECLARE value INTEGER STREAM b, 1/5  FILE 'b.txt'\n";
    for (int j = 0; j < chains; ++j) {
      const std::string n = std::to_string(j);
      source += "SELECT * STREAM out" + n + " FROM (a>2)#(b>1)\n";
      source += "SELECT out" + n + "[0] STREAM proj" + n + " FROM out" + n + "\n";
      source += "SELECT * STREAM win" + n + " FROM proj" + n + "@(1,30)\n";
      source += "SELECT win" + n + "[0] STREAM avg" + n + " FROM win" + n + ".avg\n";
    }

    qTree instance;
    auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, source);
    ASSERT_EQ(parseResult, "OK") << "lancuchow: " << chains;

    compiler compilerInstance(instance);
    EXPECT_EQ(compilerInstance.compile(), "OK") << "lancuchow: " << chains;

    // Zerowy interwal jest drugim objawem tej samej wady: strumien nigdy nie zostal
    // rozwiazany, a mimo to przepuszczono go dalej — konczylo sie zerowym mianownikiem.
    for (const auto &q : instance)
      if (!q.isCompilerDirective()) EXPECT_NE(q.rInterval, 0) << "nierozwiazany interwal: " << q.id << ", lancuchow: " << chains;
  }
}

// Zluzowanie warunku konca nie moze uczynic detektora cykli slepym.
// str2 czyta samo siebie, wiec zaden przebieg nie rozwiaze go nigdy.
TEST(xcompiler, still_reports_true_circular_dependency) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE value INTEGER STREAM src, 1 FILE 'a.txt'
        SELECT * STREAM loop FROM src + loop
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_EQ(compilerInstance.compile(), "Circular dependency in stream definitions");
}

// Iloczyn interwalow wychodzil poza int juz dla licznikow rzedu 10^4, a
// boost::rational<int> nie wykrywa przepelnienia. Objawem byl niezwiazany
// komunikat walidacji planu ("faster div from slower source") dla planu, ktory
// jest poprawny. Interwaly liczone sa teraz w 64 bitach.
TEST(xcompiler, deep_dehash_chain_does_not_overflow_interval) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE value INTEGER STREAM src, 147/1000 FILE 'a.txt'
        SELECT * STREAM d1 FROM src&441/1000
        SELECT * STREAM d2 FROM d1&1029/2000
        SELECT * STREAM d3 FROM d2&9261/16000
        SELECT * STREAM d4 FROM d3&27783/16000
        SELECT * STREAM d5 FROM d4&83349/8000
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");
  EXPECT_EQ(instance.getQuery("d5").rInterval, boost::rational<int>(83349, 16000));
}

// Interwal, ktorego nie da sie zapisac w typie interwalu, musi dawac komunikat
// o zakresie — a nie cichy smiec ani komunikat o innej wadzie planu.
TEST(xcompiler, unrepresentable_interval_reports_range_error) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE value INTEGER STREAM src, 46341/46339 FILE 'a.txt'
        SELECT * STREAM wide FROM src&46342/46339
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_THROW(
      {
        try {
          compilerInstance.compile();
        } catch (const std::out_of_range &error) {
          EXPECT_NE(std::string(error.what()).find("out of representable range"), std::string::npos);
          throw;
        }
      },
      std::out_of_range);
}

// --- L5: nierozwiazany wezel planu nie moze degradowac po cichu ---------------------
//
// compiler::computeLogicalOrigin() i compiler::computeStartupLatency() konczyly sie
// petla, ktora dla wezla nieobecnego w mapie wynikow robila SPDLOG_WARN i przechodzila
// dalej. query::logicalOrigin i query::startupLatency maja wartosc domyslna 0, wiec taki
// wezel dostawal ZERO — rezim ZANIZAJACY, ten sam, ktory tabela dokladnosci ogona
// wyklucza dla wszystkich dziewieciu klas. Ostrzezenie szlo do logu, ktorego ctest nie
// czyta.
//
// Reguly „plan nie zostawia wezla nierozwiazanego" NIE DA SIE zlamac zapytaniem RQL:
// program klauzuli FROM ma 1, 2 albo 3 tokeny i zawsze zaczyna sie od PUSH_STREAM
// (kazdy inny ksztalt zatrzymuje wczesniej resolveStreamIntervals), a deklaracje
// i dyrektywy sa zaszczepiane zerem przed petla. Przeglad 233 ksztaltow planu nad
// wszystkimi dziewiecioma klasami nie znalazl ani jednego przypadku nierozwiazanego.
// Dlatego bramka wola requireResolvedForEveryNode() wprost, z mapa podana recznie —
// inaczej test broniacy tej reguly nie mialby jak jej naruszyc.

TEST(xcompiler, unresolved_node_is_a_compilation_error) {
  qTree plan;
  plan.push_back(query(boost::rational<int>(1), "src"));
  plan.push_back(query(boost::rational<int>(1), "consumer"));

  // Mapa pokrywa producenta, ale nie konsumenta — dokladnie stan, w ktorym stara
  // wersja przepisywala konsumentowi ciche zero.
  const std::map<std::string, int> partial{{"src", 0}};

  EXPECT_DEATH(
      { requireResolvedForEveryNode(plan, partial, "test", "startup latency"); }, "unresolved startup latency for 'consumer'");
}

// Kontrola aparatury do testu wyzej: przy komplecie wynikow bramka musi milczec.
// Bez niej test smierci przechodzilby takze dla bramki, ktora zabija zawsze.
TEST(xcompiler, complete_resolution_passes_the_gate) {
  qTree plan;
  plan.push_back(query(boost::rational<int>(1), "src"));
  plan.push_back(query(boost::rational<int>(1), "consumer"));

  const std::map<std::string, int> complete{{"src", 0}, {"consumer", 3}};

  requireResolvedForEveryNode(plan, complete, "test", "startup latency");
  SUCCEED();
}

// Kontrola pozytywna na korpusie planow poprawnych: plan obejmujacy wszystkie dziewiec
// klas operatorow musi wyjsc z kompilatora bez ani jednego wezla nierozwiazanego.
//
// Trybem porazki tego testu jest SMIERC PROCESU: requireResolvedForEveryNode() konczy
// kompilacje przez FatalError, wiec nierozwiazany wezel przerywa binarke testu, zamiast
// zapisac ostrzezenie w logu. Kontrola mutacyjna (zaszczepienie deklaracji usuniete
// z computeStartupLatency) pokazuje, ze ten tryb porazki dziala.
TEST(xcompiler, every_node_of_a_nine_class_plan_is_resolved) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE ax INTEGER, ay INTEGER STREAM a, 1/10 FILE 'a.txt'
        DECLARE bx INTEGER, by INTEGER STREAM b, 1/5  FILE 'b.txt'
        SELECT a[0] STREAM proj        FROM a
        SELECT * STREAM win            FROM a@(1,3)
        SELECT * STREAM red            FROM win.sumc
        SELECT * STREAM shifted        FROM proj>2
        SELECT * STREAM merged         FROM shifted+b
        SELECT * STREAM inter          FROM a#b
        SELECT * STREAM recovered_a    FROM inter&1/5
        SELECT * STREAM recovered_b    FROM inter%1/10
        SELECT * STREAM slower         FROM a-1/5
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Plan po kompilacji zawiera takze wezly wygenerowane (STREAM_*) — one rowniez
  // przechodza przez ta sama bramke, wiec korpus jest szerszy niz lista SELECT-ow.
  ASSERT_GE(instance.size(), 11u);
  for (const auto &q : instance) {
    EXPECT_GE(q.logicalOrigin, 0) << "origin ujemny dla " << q.id;
    EXPECT_GE(q.startupLatency, 0) << "ogon ujemny dla " << q.id;
  }
}

// --- F9/S3: odwolanie do skladnika przeplotu jest bledem kompilacji ------------------
//
// `A[0]` na liscie pol NIE znaczy „biezaca wartosc strumienia A" — znaczy pozycje
// w schemacie strumienia z FROM, liczona od miejsca wejscia A do zlaczenia (aliasowanie).
// Przeplot wymaga IDENTYCZNYCH schematow obu argumentow i wydaje jeden strumien o tym
// samym schemacie, wiec pozycja k skladnika lewego i pozycja k skladnika prawego to
// TA SAMA pozycja: compiler::localizeFieldOffsets() zeruje offsety obu skladnikow.
//
// Skutkiem bylo, ze `A[0]-B[0]` nad `A#B` kompilowalo sie po cichu do `roznica[0]-roznica[0]`,
// czyli tozsamosciowego zera, mimo ze A i B to rozne strumienie. Dwa syntaktycznie rozne
// odwolania dawaly tozsamy wynik i kompilator nie mowil o tym ani slowa. Odzyskanie
// skladnika ma w algebrze wlasny operator — rozplot & / % — a siegania po skladnik nazwa
// przez wezel # algebra nie przewiduje wcale.
//
// Rozstrzygniecie F9 (D-F1 = S3, 2026-08-09): takie odwolanie jest bledem kompilacji,
// nie wynikiem. Nie zmienia sie wartosc — odmawia sie planu.

TEST(xcompiler, interleave_constituent_reference_is_a_compilation_error) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a[0]-b[0] STREAM roznica FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  const auto result = compilerInstance.compile();
  EXPECT_NE(result, "OK") << "odwolanie do skladnika przeplotu skompilowalo sie po cichu";
  EXPECT_NE(result.find("roznica"), std::string::npos) << "komunikat nie wskazuje strumienia: " << result;
}

// Ten sam defekt w ksztalcie z kampanii K23 (rodzina F9-X): odwolania siegaja skladnikow
// przez DWA wezly przeplotu ukryte w substratach generowanych przez kompilator, a klauzula
// FROM na wierzchu jest suma. Sciezka przechodzi przez collectTransitiveOffsets(), wiec
// bramka pilnujaca wylacznie bezposrednich argumentow by tego nie zlapala.
TEST(xcompiler, interleave_constituent_reference_is_rejected_through_substrates) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        DECLARE v INTEGER STREAM c, 1/100 FILE 'c.txt'
        DECLARE v INTEGER STREAM d, 1/50  FILE 'd.txt'
        SELECT a[0]*c[0]+b[0]*d[0] STREAM m1 FROM ((a>2)#(b>1)) + ((c>2)#(d>1))
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "ksztalt F9-X skompilowal sie po cichu";
}

// KONTROLA NEGATYWNA — bramka, ktora odrzuca takze plany poprawne, jest bezwartosciowa.
// Suma sklada schematy przez konkatenacje, wiec `a[0]` i `b[0]` maja ROZNE offsety
// i pozostaja rozroznialne. To jest udokumentowane aliasowanie (Pattern7) i musi dzialac.
TEST(xcompiler, sum_keeps_constituent_identity) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a[0]-b[0] STREAM roznica FROM a+b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  std::vector<int> offsets;
  for (const auto &q : instance)
    if (q.id == "roznica")
      for (const auto &f : q.lSchema)
        for (const auto &t : f.lProgram)
          if (t.getCommandID() == PUSH_ID) offsets.push_back(std::get<std::pair<std::string, int>>(t.getVT()).second);

  ASSERT_EQ(offsets.size(), 2u);
  EXPECT_NE(offsets[0], offsets[1]) << "suma zgubila tozsamosc skladnikow";
}

// KONTROLA NEGATYWNA — nad przeplotem legalne pozostaje odwolanie nazwa strumienia
// WYNIKOWEGO. Ono nie jest dwuznaczne: wskazuje pozycje w jedynym schemacie, jaki po #
// istnieje. Gdyby bramka odrzucala i to, odcielaby przeplot od listy pol w ogole.
TEST(xcompiler, interleave_allows_reference_by_output_stream_name) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT h[0]+1 STREAM h FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_EQ(compilerInstance.compile(), "OK");
}

// Ta sama utrata tozsamosci zapisana gola nazwa pola. `v` jest polem A, `w` polem B,
// wiec autor jawnie wskazal DWA rozne strumienie — a nad `A#B` oba odwolania trafialy
// na pozycje 0 tego samego schematu i roznica byla tozsamosciowym zerem. Bramka, ktora
// lapie `A[0]-B[0]`, ale przepuszcza `v-w`, nie zamyka defektu.
TEST(xcompiler, interleave_constituent_reference_is_rejected_for_bare_field_names) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE w INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT v-w STREAM roznica FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "gola nazwa pola skompilowala sie po cichu";
}

// KONTROLA NEGATYWNA do testu wyzej: nad suma gole nazwy pol pozostaja legalne i musza
// wskazywac ROZNE pozycje. Bez tej kontroli bramka odcinajaca gole nazwy w ogole
// przechodzilaby test wyzej, nic nie naprawiajac.
TEST(xcompiler, sum_allows_bare_field_names) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE w INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT v-w STREAM roznica FROM a+b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  std::vector<int> offsets;
  for (const auto &q : instance)
    if (q.id == "roznica")
      for (const auto &f : q.lSchema)
        for (const auto &t : f.lProgram)
          if (t.getCommandID() == PUSH_ID) offsets.push_back(std::get<std::pair<std::string, int>>(t.getVT()).second);

  ASSERT_EQ(offsets.size(), 2u);
  EXPECT_NE(offsets[0], offsets[1]) << "suma zgubila tozsamosc przy golych nazwach pol";
}

// --- ZnA/uboczne: `>N` nalozone wprost na `@` psulo sterte -------------------------
//
// extractIntermediateStreams() wydziela operatory z klauzuli FROM do substratow. Liczbe
// poprzedzajacych tokenow, ktore operator konsumuje, ustalala CZARNA LISTA: wszystko poza
// `>N` i `-` uznawano za dwuargumentowe. `@` niesie jednak swoje parametry (krok, szerokosc)
// W SAMYM TOKENIE, wiec konsumuje JEDEN token — tak samo jak `>N`.
//
// Dla `(A@(1,4))>1` program ma trzy tokeny [PUSH_STREAM, STREAM_AGSE, STREAM_TIMEMOVE].
// Po wydzieleniu `@` i zdjeciu jego jedynego argumentu lista ma juz tylko [STREAM_TIMEMOVE],
// a kod siegal po drugi argument: dereferencjonowal WARTOWNIKA listy i go kasowal. Skutkiem
// bylo uszkodzenie sterty — `SIGSEGV` albo `free(): invalid size`, zaleznie od parametrow —
// ujawniane dopiero w qTree::topologicalSort() jako odczyt zwolnionej pamieci.
//
// Defekt jest WCZESNIEJSZY niz naprawa F9: odtworzony na `ebd8aab` (abort) i `530c80e`
// (SIGSEGV). Zaden test ani plan integracyjny nie zestawial `@` z `>N` w jednej klauzuli
// FROM, wiec `ctest` 186/186 tego nie lapal — to byla luka pokrycia, nie tylko defekt.
//
// TRYBEM PORAZKI TEGO TESTU JEST SMIERC PROCESU, nie asercja: przed naprawa binarka testu
// przerywa sie w tym miejscu.

TEST(xcompiler, shift_over_agse_in_one_from_clause_does_not_corrupt_heap) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        SELECT * STREAM m FROM (a@(1,4))>2
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");
  EXPECT_TRUE(instance.exists("m"));
}

// KONTROLA POZYTYWNA — sam brak wywrotki nie wystarczy. Postac jednoklauzulowa musi dac
// DOKLADNIE ten sam brzeg co rownowazna postac dwuetapowa, ktora dzialala takze przed
// naprawa. Bez tej kontroli przechodzilaby rowniez „naprawa”, ktora tylko przestaje
// kasowac wartownika, ale gubi argument albo przesuwa origin.
TEST(xcompiler, shift_over_agse_matches_the_two_step_form) {
  auto boundaryOf = [](const char *rql, const char *name) {
    qTree instance;
    auto [parseResult, kw, sn] = parserRQLString(instance, rql);
    EXPECT_EQ(parseResult, "OK");
    compiler compilerInstance(instance);
    EXPECT_EQ(compilerInstance.compile(), "OK");
    const auto &q = instance.getQuery(name);
    return std::make_pair(q.logicalOrigin, q.startupLatency);
  };

  const auto oneClause = boundaryOf(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        SELECT * STREAM m FROM (a@(1,4))>2
      )",
                                    "m");
  const auto twoStep   = boundaryOf(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        SELECT * STREAM w FROM a@(1,4)
        SELECT * STREAM m FROM w>2
      )",
                                    "m");

  EXPECT_EQ(oneClause.first, twoStep.first) << "origin rozny miedzy postacia jedno- i dwuetapowa";
  EXPECT_EQ(oneClause.second, twoStep.second) << "ogon rozny miedzy postacia jedno- i dwuetapowa";
}
