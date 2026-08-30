#include <algorithm>
#include <cctype>
#include <cerrno>
#include <format>
#include <fstream>
#include <map>
#include <sstream>
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

  // Nazwa substratu przesuniecia niesie wielkosc przesuniecia — `A>2` daje
  // STREAM_TIMEMOVE_2_A. Patrz compiler::composeStreamName().
#if RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES
  EXPECT_TRUE(instance.exists("STREAM_HASH_A_B"));
  EXPECT_FALSE(instance.exists("STREAM_TIMEMOVE_2_A"));
  EXPECT_FALSE(instance.exists("STREAM_TIMEMOVE_1_B"));
#else
  EXPECT_FALSE(instance.exists("STREAM_HASH_A_B"));
  EXPECT_TRUE(instance.exists("STREAM_TIMEMOVE_2_A"));
  EXPECT_TRUE(instance.exists("STREAM_TIMEMOVE_1_B"));
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
  // Różnica nie ma własnego członu: ogon wynika z kresu fazy odczytu, a ten po podzieleniu
  // przez iloraz taktów daje zero zarówno dla producenta deklarowanego, jak i obliczanego.
  // Do 2026-08-18 stały tu jedynki — dawna reguła dokładała slot deklaracji zawsze,
  // a fazę 3/2 traktowała jako pełne oczekiwanie (K24: zgodność 19,1%).
  EXPECT_EQ(instance.getQuery("sub_declared").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("sub_computed").startupLatency, 0);
  EXPECT_EQ(instance.getQuery("sub_fractional").startupLatency, 0);
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
  EXPECT_EQ(instance.getQuery("sub_same").startupLatency, 0);
  // K24/P1: czlon pojemnosci zrodla roznicy to floor((1+Wout)*ratio) + prefetch
  // deklaracji, brany jako maksimum ze stara formula. Wartosc 3 pokrywala odleglosc
  // wsteczna tylko dla ilorazu 1 i 2; od ilorazu 3 odczyt wypadal poza historia
  // i dawal cichy rekord all-NULL.
  //
  // K24/H10 (2026-08-18): wynik spadl z 4 na 3, bo Wout tego wezla spadl z 1 na 0 wraz
  // z naprawa ogona `-`. Pojemnosc jest FUNKCJA ogona, wiec zawyzony ogon zawyzal tez
  // wymagana historie; formula sie nie zmienila. Kierunek jest bezpieczny, bo konsument
  // czyta te same rekordy, tylko zaczyna o slot wczesniej — potwierdza to model
  // pojemnosci kampanii (bramka badawcza, `ninja test_gate`).
  EXPECT_EQ(instance.maxCapacity.at("subsrc"), 3);
}

// Ogon `-`, `Θ` i `~Θ` po wyprowadzeniu postaci dokładnych (K24/H10, faza 3).
//
// Wartości oczekiwane pochodzą z NIEZALEŻNEGO modelu zdarzeniowego kampanii K24
// (rdb-experiment/investigation_K24H10/PHASE2.md §4), nie z postaci zamkniętej silnika.
// Cztery pierwsze przypadki zmieniają wynik wobec reguł sprzed 2026-08-18, piąty jest
// bramką regresyjną: tam slot jest prawdziwy i ma zostać. Trybem porażki tego testu jest
// powrót zawyżenia — zaniżenie łapie osobna bramka `EXPECT_GE` w ut_h10aGate.
TEST(xcompiler, exact_tail_for_subtract_and_dehash) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE value INTEGER STREAM s58,  5/8  FILE 'a.txt'
        DECLARE value INTEGER STREAM s12,  1/2  FILE 'a.txt'
        DECLARE value INTEGER STREAM s116, 1/16 FILE 'a.txt'
        SELECT * STREAM sub_decl    FROM s58-5/2
        SELECT * STREAM theta_int   FROM s12&1
        SELECT * STREAM theta_frac  FROM s12&3/2
        SELECT * STREAM sub_over    FROM theta_frac-2
        SELECT * STREAM theta_deep  FROM s116&1/6
        SELECT * STREAM ntheta_deep FROM theta_deep%1/5
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // `-` nad deklaracją, iloraz 4: kres fazy zerowy, ogon zerowy (dawniej 1).
  EXPECT_EQ(instance.getQuery("sub_decl").startupLatency, 0);
  // `Θ` przy ilorazie całkowitym: własny ogon zerowy (dawniej 1).
  EXPECT_EQ(instance.getQuery("theta_int").startupLatency, 0);
  // `Θ` przy ilorazie ułamkowym: slot jest prawdziwy — bramka regresyjna.
  EXPECT_EQ(instance.getQuery("theta_frac").startupLatency, 1);
  // `-` nad składową obliczaną o ogonie 1: ogon nie dziedziczy się w skali 1:1 (dawniej 1).
  EXPECT_EQ(instance.getQuery("sub_over").startupLatency, 0);
  // `~Θ` nad składową o ogonie 1: dawna reguła zaokrąglała ogon składowej osobno i dawała 1.
  EXPECT_EQ(instance.getQuery("theta_deep").startupLatency, 1);
  EXPECT_EQ(instance.getQuery("ntheta_deep").startupLatency, 0);
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

// Tozsamosc R1 jest ROWNOSCIA WYNIKOW i NIEROWNOSCIA OPOZNIEN (`thm:shift-match`).
// Obie postaci zgadzaja sie co do interwalu, poczatku logicznego i ciagu rekordow,
// a strona prawa ma ogon NIE WIEKSZY od lewej — dla niektorych taktow scisle
// mniejszy. `def:observable` zada `Val(P) = Val(Q)` dokladnie, ale tylko
// `Lat(Q) <= Lat(P)`: przepisaniu wolno skrocic oczekiwanie, nigdy wydluzyc.
//
// Zadanie ROWNOSCI ogonow byloby wiec ostrzejsze niz relacja obserwowalnosci i
// odrzucaloby przepisanie, ktore teoria dopuszcza. Dokladnie tak oblala bramka
// `public_identity` kampanii K23 (znalezisko A, decyzja D1 z 2026-08-09,
// `research_plan.md` §14.20) — i dlatego test sprawdza nierownosc, nie rownosc.
//
// Przy R1 ON obie strony i tak schodza sie do jednego ksztaltu; niezmiennik jest
// nietrywialny dopiero przy R1 OFF, gdzie lewa ma ogon 2, a prawa 0.
TEST(xcompiler, shift_matching_identity_does_not_lengthen_startup_latency) {
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

  // Czesc wartosciowa: poczatek logiczny musi byc IDENTYCZNY po obu stronach — to on
  // niesie tozsamosc, w kazdej konfiguracji przelacznikow.
  EXPECT_EQ(instance.getQuery("lhs").logicalOrigin, instance.getQuery("rhs").logicalOrigin);
  // Czesc opoznieniowa: strona prawa (sfaktoryzowana) nie moze czekac DLUZEJ niz lewa.
  // Rownosc jest dozwolona i zachodzi przy R1 ON; przy R1 OFF nierownosc jest ostra
  // (2 wobec 0) i to jest przypadek przewidziany przez `thm:shift-match`, nie defekt.
  EXPECT_LE(instance.getQuery("rhs").startupLatency, instance.getQuery("lhs").startupLatency);
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

TEST(xcompiler, interleave_constituent_named_field_is_rejected) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a.v-b.v STREAM roznica FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "odwolanie A.pole do skladnika przeplotu przeszlo kompilacje";
}

TEST(xcompiler, interleave_constituent_index_wildcard_is_rejected) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a[_]-b[_] STREAM roznica FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "odwolanie A[_] do skladnika przeplotu przeszlo kompilacje";
}

TEST(xcompiler, sum_allows_constituent_index_wildcards) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a[_]-b[_] STREAM roznica FROM a+b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  const auto &result = instance.getQuery("roznica");
  ASSERT_EQ(result.lSchema.size(), 1u);
  std::vector<int> offsets;
  for (const auto &t : result.lSchema.front().lProgram)
    if (t.getCommandID() == PUSH_ID) offsets.push_back(std::get<std::pair<std::string, int>>(t.getVT()).second);

  ASSERT_EQ(offsets.size(), 2u);
  EXPECT_NE(offsets[0], offsets[1]) << "suma zgubila tozsamosc przy A[_]";
}

TEST(xcompiler, interleave_constituent_qualified_wildcard_is_rejected) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT a.* STREAM wynik FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "odwolanie A.* do skladnika przeplotu przeszlo kompilacje";
}

TEST(xcompiler, interleave_allows_qualified_wildcard_by_output_stream_name) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT wynik.* STREAM wynik FROM a#b
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_EQ(compilerInstance.compile(), "OK");
}

TEST(xcompiler, rule_rejects_interleave_constituent_reference) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/100 FILE 'a.txt'
        DECLARE v INTEGER STREAM b, 1/50  FILE 'b.txt'
        SELECT * STREAM wynik FROM a#b
        RULE niejednoznaczna ON wynik WHEN a[0] > 0 DO DUMP -1 TO 0
      )");
  ASSERT_EQ(parseResult, "OK");

  compiler compilerInstance(instance);
  EXPECT_NE(compilerInstance.compile(), "OK") << "RULE odwolujaca sie do skladnika przeplotu przeszla kompilacje";
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

// Uszkodzony program pośredni nie może cofnąć iteratora przed begin(). Operator binarny
// bez operandów ma zostać odrzucony przed próbą wycięcia zakresu z listy tokenów.
TEST(xcompiler, malformed_intermediate_operator_is_rejected_before_iterator_underflow) {
  qTree instance;
  query malformed(boost::rational<int>(1), "malformed");
  malformed.lProgram.emplace_back(STREAM_HASH);
  malformed.lProgram.emplace_back(STREAM_TIMEMOVE);
  instance.push_back(malformed);

  EXPECT_DEATH(
      {
        compiler compilerInstance(instance);
        (void)compilerInstance.compile();
      },
      "operator 'STREAM_HASH' in query 'malformed' has 0 preceding tokens, needs 2");
}

// --- Issue 236: reduktor w postaci funkcyjnej SUMC()/MIN()/MAX()/AVG() ------------
//
// Postac przyrostkowa `.sumc` siega tylko po operand poziomu postfiksowego, wiec okno
// trzeba bylo zmaterializowac osobnym, nazwanym zapytaniem. Postac funkcyjna domyka
// argument wlasnymi nawiasami, wiec ta sama para miesci sie w jednym zapytaniu. To zmiana
// WYLACZNIE w kompilacji: extractIntermediateStreams() rozbija program z powrotem
// na dwa wezly, wiec DAG ma byc identyczny.

namespace {

/// Program klauzuli FROM jako tekst — do porownan ksztaltu planu.
std::string fromProgram(query &q) {
  std::ostringstream out;
  for (auto &t : q.lProgram)
    out << t << ";";
  return out.str();
}

/// Kompiluje RQL i zwraca plan. Blad kompilacji jest bledem testu.
qTree compilePlan(const std::string &rql) {
  qTree instance;
  auto [parseResult, keyword, streamName] = parserRQLString(instance, rql);
  EXPECT_EQ(parseResult, "OK");
  compiler compilerInstance(instance);
  EXPECT_EQ(compilerInstance.compile(), "OK");
  return instance;
}

}  // namespace

// Rdzen zgloszenia. Jedno zapytanie `FROM SUMC(sq@(125,1000))` ma dac DOKLADNIE ten plan,
// co para zapytan z nazwanym oknem — az do brzegu (origin, ogon) i deskryptora wyjscia.
// Rozna jest tylko nazwa wezla okna: uzytkownik nadal moze ja nadac sam, ale nie musi.
TEST(xcompiler, stream_function_matches_the_two_step_form) {
  auto oneClause = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT v*v STREAM sq FROM a
        SELECT * STREAM s FROM SUMC(sq@(125,1000))
      )");
  auto twoStep   = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT v*v STREAM sq FROM a
        SELECT * STREAM w FROM sq@(125,1000)
        SELECT * STREAM s FROM w.sumc
      )");

  auto &oneResult = oneClause.getQuery("s");
  auto &twoResult = twoStep.getQuery("s");

  EXPECT_EQ(oneResult.rInterval, twoResult.rInterval);
  EXPECT_EQ(oneResult.logicalOrigin, twoResult.logicalOrigin);
  EXPECT_EQ(oneResult.startupLatency, twoResult.startupLatency);
  EXPECT_EQ(oneResult.descriptorFrom(oneClause), twoResult.descriptorFrom(twoStep));

  // Wezel okna powstaje jako substrat kompilatora, a nie znika: rachunek brzegu
  // ma na czym stanac. Nazwa niesie parametry okna — patrz composeStreamName().
  ASSERT_TRUE(oneClause.exists("STREAM_AGSE_125_1000_sq"));
  auto &window = oneClause.getQuery("STREAM_AGSE_125_1000_sq");
  EXPECT_TRUE(window.isSubstrat);
  EXPECT_EQ(fromProgram(window), "PUSH_STREAM(sq);STREAM_AGSE(125,1000);");
  EXPECT_EQ(fromProgram(oneResult), "PUSH_STREAM(STREAM_AGSE_125_1000_sq);STREAM_SUM(0);");

  // Brzeg wezla okna tez musi sie zgadzac, nie tylko brzeg wyniku — inaczej rownosc
  // na `s` mogla by wyjsc z dwoch bledow znoszacych sie nawzajem.
  auto &namedWindow = twoStep.getQuery("w");
  EXPECT_EQ(window.rInterval, namedWindow.rInterval);
  EXPECT_EQ(window.logicalOrigin, namedWindow.logicalOrigin);
  EXPECT_EQ(window.startupLatency, namedWindow.startupLatency);
}

// Reduktor jest zwyklym operatorem klauzuli FROM, wiec sklada sie z pozostalymi.
// SUMC(x)+SUMC(y) jest tu przypadkiem wprost zamowionym: `+` laczy dwa WYNIKI
// reduktorow, wiec kazdy z nich musi wczesniej trafic do wlasnego substratu.
TEST(xcompiler, stream_functions_compose_with_other_from_operators) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        DECLARE w INTEGER STREAM b, 1/500 FILE 'b.txt'
        SELECT * STREAM p FROM SUMC(a@(125,1000))+SUMC(b@(125,1000))
        SELECT * STREAM q FROM MIN(a)>2
        SELECT * STREAM r FROM AVG(SUMC(a))
        SELECT * STREAM t FROM MIN(a)#(MAX(b))
        SELECT * STREAM u FROM SUMC(a+b)
      )");

  // Kazdy wezel planu ma po wydzieleniu DOKLADNIE jeden operator — tego wymaga
  // wykonanie (GetArgs odrzuca program dluzszy niz trzy tokeny).
  for (auto &q : plan)
    if (!q.isDeclaration() && !q.isCompilerDirective()) EXPECT_LE(q.lProgram.size(), 3u) << "wezel " << q.id;

  EXPECT_EQ(fromProgram(plan.getQuery("p")),
            "PUSH_STREAM(STREAM_SUM_STREAM_AGSE_125_1000_a);PUSH_STREAM(STREAM_SUM_STREAM_AGSE_125_1000_b);STREAM_ADD(0);");
  EXPECT_EQ(fromProgram(plan.getQuery("q")), "PUSH_STREAM(STREAM_MIN_a);STREAM_TIMEMOVE(2);");
  EXPECT_EQ(fromProgram(plan.getQuery("r")), "PUSH_STREAM(STREAM_SUM_a);STREAM_AVG(0);");
  EXPECT_EQ(fromProgram(plan.getQuery("t")), "PUSH_STREAM(STREAM_MIN_a);PUSH_STREAM(STREAM_MAX_b);STREAM_HASH(0);");
  EXPECT_EQ(fromProgram(plan.getQuery("u")), "PUSH_STREAM(STREAM_ADD_a_b);STREAM_SUM(0);");
}

// Postac przyrostkowa jest wygaszana, ale dopoki dziala, ma dawac TEN SAM plan.
TEST(xcompiler, deprecated_dot_notation_matches_the_function_form) {
  auto dotted = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT * STREAM s FROM a.sumc
      )");
  auto called = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT * STREAM s FROM SUMC(a)
      )");

  EXPECT_EQ(fromProgram(dotted.getQuery("s")), fromProgram(called.getQuery("s")));
  EXPECT_EQ(dotted.getQuery("s").descriptorFrom(dotted), called.getQuery("s").descriptorFrom(called));
}

// `(a.sumc)>2` konczylo sie zabiciem procesu w computeRequiredCapacities: program
// [PUSH_STREAM, STREAM_SUM, STREAM_TIMEMOVE] nie byl uznawany za wymagajacy redukcji,
// bo isReductionRequired() nie liczyl reduktorow. Ten sam brak przewracal forme funkcyjna
// nad oknem, czyli caly sens zgloszenia. TRYBEM PORAZKI JEST SMIERC PROCESU.
TEST(xcompiler, reducer_over_another_from_operator_is_extracted_not_fatal) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT * STREAM m FROM (a.sumc)>2
      )");
  ASSERT_TRUE(plan.exists("STREAM_SUM_a"));
  EXPECT_EQ(fromProgram(plan.getQuery("m")), "PUSH_STREAM(STREAM_SUM_a);STREAM_TIMEMOVE(2);");
}

// --- Nazwa substratu musi identyfikowac wezel ------------------------------------
//
// Do 2026-08-29 nazwa skladala sie z operatora i operandow, BEZ parametru operatora.
// Dwa rozne okna nad tym samym zrodlem dawaly wiec jeden substrat STREAM_AGSE_a,
// a drugie zapytanie po cichu liczylo okno pierwszego. Defekt byl osiagalny juz przez
// `(a@(k,L))>N`, ale forma funkcyjna czyni go typowym, bo SUMC(x@(...)) obok AVG(x@(...))
// to zwykly zapis. TESTEM JEST ROZNICA WYNIKU, nie sama liczba wezlow: interwaly
// i brzegi obu galezi musza zostac rozne.
TEST(xcompiler, distinct_windows_over_one_source_get_distinct_substrates) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT * STREAM m1 FROM SUMC(a@(1,4))
        SELECT * STREAM m2 FROM SUMC(a@(2,8))
      )");

  ASSERT_TRUE(plan.exists("STREAM_AGSE_1_4_a"));
  ASSERT_TRUE(plan.exists("STREAM_AGSE_2_8_a"));
  EXPECT_EQ(fromProgram(plan.getQuery("STREAM_AGSE_1_4_a")), "PUSH_STREAM(a);STREAM_AGSE(1,4);");
  EXPECT_EQ(fromProgram(plan.getQuery("STREAM_AGSE_2_8_a")), "PUSH_STREAM(a);STREAM_AGSE(2,8);");
  EXPECT_NE(plan.getQuery("m1").rInterval, plan.getQuery("m2").rInterval);
}

// Identyczne wezly nadal maja sie scalac — parametr w nazwie nie moze wylaczyc
// deduplikacji, bo wtedy naprawa kolizji kosztowalaby powielenie planu.
TEST(xcompiler, identical_windows_over_one_source_still_share_one_substrate) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT * STREAM m1 FROM SUMC(a@(1,4))
        SELECT * STREAM m2 FROM AVG(a@(1,4))
      )");

  EXPECT_EQ(std::ranges::count_if(plan, [](const query &q) { return q.id.starts_with("STREAM_AGSE_"); }), 1);
  EXPECT_TRUE(plan.exists("STREAM_AGSE_1_4_a"));
}

// Nazwa substratu jest zarazem nazwa artefaktu na dysku, wiec musi byc identyfikatorem.
// `-` liczby ujemnej idzie na "N", a `/` liczby wymiernej na "_" — do 2026-08-29 nazwa
// substratu `&` niosla kreske ulamkowa, czyli separator sciezki, wprost z token::getStr_().
TEST(xcompiler, substrate_names_stay_identifiers) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        DECLARE w INTEGER STREAM b, 1/500 FILE 'b.txt'
        SELECT * STREAM g FROM (a-1/4)>1
        SELECT * STREAM h FROM ((a#b)&2)>1
        SELECT * STREAM i FROM SUMC(a@(1,-10))
      )");

  for (auto &q : plan) {
    if (!q.isSubstrat) continue;
    EXPECT_TRUE(std::ranges::all_of(q.id, [](unsigned char c) { return std::isalnum(c) != 0 || c == '_'; }))
        << "nazwa substratu nie jest identyfikatorem: " << q.id;
  }
  EXPECT_TRUE(plan.exists("STREAM_SUBTRACT_1_4_a"));
  EXPECT_TRUE(plan.exists("STREAM_AGSE_1_N10_a"));
  // `&` niesie swoj parametr jako OPERAND, nie w tokenie operatora, wiec liczba wymierna
  // stoi na koncu nazwy — po nazwie strumienia, a nie po nazwie operatora.
  EXPECT_TRUE(plan.exists("STREAM_DEHASH_DIV_STREAM_HASH_a_b_2_1"));
}

// MIN/MAX/AVG/SUMC sa tokenami leksera stojacymi PRZED ID, wiec zaden strumien nie moze
// sie tak nazywac — reguly stream_factor przyjmuja wylacznie ID. Zastrzezenie jest
// dzialaniem gramatyki, nie osobna kontrola w kompilatorze, i ten test je przypina:
// gdyby ktos zdjal MIN z leksera albo dodal go do ID, `SUMC(x)` przestaloby byc
// jednoznaczne. parserRQLString konczy proces przy bledzie skladni, stad EXPECT_EXIT.
TEST(xparser, aggregate_keywords_are_reserved_stream_names) {
  for (const char *rql : {
           "DECLARE v INTEGER STREAM min, 1/500 FILE 'a.txt'",
           "DECLARE v INTEGER STREAM MAX, 1/500 FILE 'a.txt'",
           "DECLARE v INTEGER STREAM avg, 1/500 FILE 'a.txt'",
           "DECLARE v INTEGER STREAM SUMC, 1/500 FILE 'a.txt'",
       }) {
    EXPECT_EXIT(
        {
          qTree instance;
          (void)parserRQLString(instance, rql);
          exit(0);
        },
        ::testing::ExitedWithCode(EPERM), "expecting ID")
        << rql;
  }
}

namespace {

/// Sparsuj i skompiluj `rql`, zwracajac wynik compiler::compile().
///
/// Nazwa funkcji jest w gramatyce zwyklym ID, wiec bledna nazwa NIE jest bledem skladni
/// i nie konczy procesu — wychodzi lagodnie przez wartosc zwracana z compile(), tym samym
/// kanalem co pozostale kontrole planu. Dlatego te testy nie potrzebuja EXPECT_EXIT.
std::string compileRql(const std::string &rql) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, rql);
  if (parseResult != "OK") return parseResult;
  compiler compilerInstance(instance);
  return compilerInstance.compile();
}

std::string selectRql(const std::string &selectList) {
  return "SUBSTRAT 'memory'\n"
         "DECLARE a INTEGER, b INTEGER STREAM src, 1 FILE 'src.txt'\n"
         "SELECT " +
         selectList + " STREAM out FROM src\n";
}

}  // namespace

// Wielkosc liter w nazwie funkcji przestala byc czescia skladni. Do 2026-08-29 gramatyka
// miala literaly 'Sqrt', 'Ceil', 'Floor', a ewaluator skladal nazwe do malych liter przed
// dopasowaniem — `Sqrt(x)` przechodzilo, `sqrt(x)` bylo bledem skladni, a dla `to_integer`
// i `isnull` bylo odwrotnie.
TEST(xparser, function_name_is_case_insensitive) {
  for (const char *call : {"Sqrt(a)", "sqrt(a)", "SQRT(a)", "SqRt(a)"}) {
    EXPECT_EQ(compileRql(selectRql(call)), "OK") << call;
  }
}

// Do tokena idzie postac KANONICZNA z rqlFunctions.hpp, a nie ta napisana przez autora.
// Trzyma to zrzuty planu stabilne — wzorce testow integracyjnych i zapisy planow pilota H9
// pokazuja `CALL(Sqrt)` niezaleznie od pisowni w zrodle. Bez kanonizacji porownania
// `getStr_() == "to_string"` w exitExpression i exprSimplify przestalyby dzialac.
TEST(xparser, function_name_is_canonicalized_in_token) {
  for (const char *call : {"sqrt(a)", "SQRT(a)"}) {
    qTree instance;
    auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, selectRql(call));
    ASSERT_EQ(parseResult, "OK") << call;

    bool found = false;
    for (const auto &q : instance)
      for (const auto &f : q.lSchema)
        for (const auto &tk : f.lProgram)
          if (tk.getCommandID() == CALL && tk.getStr_() == "Sqrt") found = true;
    EXPECT_TRUE(found) << call;
  }
}

// Siedem funkcji bylo zaimplementowanych w ewaluatorze, ale nie stalo ich w gramatyce,
// wiec byly z RQL nieosiagalne. To odwrotna polowa tej samej rozbieznosci list.
TEST(xparser, implemented_functions_are_reachable_from_rql) {
  for (const char *call : {"round(a)", "trunc(a)", "sin(a)", "cos(a)", "tan(a)", "log(a)", "log2(a)"}) {
    EXPECT_EQ(compileRql(selectRql(call)), "OK") << call;
  }
}

// Funkcje dopisane 2026-08-30. `Abs` liczy sie wprost na wariancie, zeby nie tracic
// dokladnosci wartosci wymiernej; `IsZero`/`IsNonZero` wnosza predykat do wyrazenia
// w SELECT, gdzie porownania z `term_logic` nie sa dostepne.
TEST(xparser, newly_implemented_functions_compile) {
  for (const char *call : {"Abs(a)", "abs(a)", "IsZero(a)", "isnonzero(a)"}) {
    EXPECT_EQ(compileRql(selectRql(call)), "OK") << call;
  }
}

// Sedno pozycji 1 z usecases/requested.md: `-c` jest bramka. Te nazwy stały w gramatyce
// bez implementacji, wiec plan kompilowal sie czysto i ginal dopiero w wykonaniu na
// `Unsupported function call`. Teraz odpadaja na kompilacji, kanalem `Check result:`.
TEST(xparser, unknown_function_is_rejected_at_compile_time) {
  for (const char *call : {"Crc(a)", "Sum(a)", "Sign(a)", "Chr(a)", "Count(a)", "IntCast(a)", "FloatCast(a)", "ToNumber(a)",
                           "ToTimeStamp(a)", "Length(a)", "Sqrtt(a)"}) {
    const std::string result = compileRql(selectRql(call));
    EXPECT_NE(result, "OK") << call;
    EXPECT_NE(result.find("not a known RQL function"), std::string::npos) << call << " -> " << result;
  }
}

// Nieznana nazwa w warunku reguly musi odpasc tak samo jak w liscie SELECT: warunek jest
// osobnym programem tokenow i przebieg kontrolny musi go obejsc.
TEST(xparser, unknown_function_in_rule_condition_is_rejected) {
  const std::string result = compileRql(
      "SUBSTRAT 'memory'\n"
      "DECLARE a INTEGER, b INTEGER STREAM src, 1 FILE 'src.txt'\n"
      "SELECT a, b STREAM out FROM src\n"
      "RULE alarm ON out WHEN Crc(out[0]) >= 1 DO DUMP -1 TO 1 RETENTION 8\n");
  EXPECT_NE(result, "OK");
  EXPECT_NE(result.find("not a known RQL function"), std::string::npos) << result;
}

// Zadeklarowana szerokosc `f(expr : N)` nalezy wylacznie do `to_string` — N jest szerokoscia
// pola wyjsciowego, a nie wartoscia na stosie. Arnosc sprawdza tabela w rqlFunctions.hpp,
// a nie ksztalt gramatyki, wiec dodanie funkcji o innej arnosci nie wymaga regeneracji ANTLR.
TEST(xparser, declared_width_is_rejected_for_functions_without_width) {
  const std::string result = compileRql(selectRql("Sqrt(a:8)"));
  EXPECT_NE(result, "OK");
  EXPECT_NE(result.find("takes no width argument"), std::string::npos) << result;

  EXPECT_EQ(compileRql(selectRql("to_string(a:8)")), "OK");
  EXPECT_EQ(compileRql(selectRql("TO_STRING(a:8)")), "OK");
}

namespace {

/// Plan z jedna szeroka klauzula FROM: `str01 + str02 + ... + strNN`.
///
/// Nazwa substratu rosnie LINIOWO z liczba skladnikow — kazdy poziom doklada
/// "STREAM_ADD_" (11), podkreslenie i piecioznakowa nazwe operandu, czyli 17 bajtow.
/// Dwa skladniki daja 22 bajty, wiec prog 200 wypada miedzy 12. a 13. skladnikiem.
std::string wideFromClause(int operandCount) {
  std::string clause("str01");
  for (int index = 2; index <= operandCount; ++index)
    clause += std::format("+str{:02}", index);
  return clause;
}

std::string wideAdditionPlan(int operandCount) {
  std::string rql("SUBSTRAT 'memory'\n");
  for (int index = 1; index <= operandCount; ++index)
    rql += std::format("DECLARE v INTEGER STREAM str{:02}, 1 FILE 'd{:02}.txt'\n", index, index);
  return rql + "SELECT * STREAM wide FROM " + wideFromClause(operandCount) + "\n";
}

/// Nazwa szczytowego substratu planu — pierwszego operandu programu zapytania `wide`.
///
/// To ten wezel niesie CALY lancuch skladnikow, wiec to on jako pierwszy uderza w NAME_MAX.
/// Ostatni STREAM_ADD zostaje w samym zapytaniu publicznym i substratu nie dostaje, wiec
/// szczyt lancucha ma o jeden skladnik mniej niz klauzula FROM.
std::string topSubstratName(qTree &plan) { return plan.getQuery("wide").lProgram.front().getStr_(); }

}  // namespace

// Ponizej progu nazwa ma zostac DOKLADNIE taka jak dotad. To przypina brak przemianowania:
// gałąź skrótu wolno wprowadzić tylko tam, gdzie nazwa czytelna i tak jest nie do zapisania,
// bo kazde przemianowanie unieważnia wzorce testow integracyjnych i artefakty na dysku.
TEST(xcompiler, substrate_name_stays_readable_below_threshold) {
  auto plan = compilePlan(wideAdditionPlan(12));

  const auto longest = topSubstratName(plan);
  EXPECT_EQ(longest,
            "STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_"
            "str01_str02_str03_str04_str05_str06_str07_str08_str09_str10_str11");
  // Blisko progu, ale ponizej — inaczej test przestaje pilnowac granicy.
  EXPECT_GT(longest.size(), 150u);
  EXPECT_LE(longest.size(), 200u);
}

// Powyzej progu nazwa czytelna ustepuje skrotowi. Bez tego plan jest NIEZAPISYWALNY:
// nazwa substratu jest nazwa pliku, a NAME_MAX to 255 bajtow.
TEST(xcompiler, wide_from_clause_falls_back_to_digest) {
  auto plan = compilePlan(wideAdditionPlan(14));

  bool sawDigest = false;
  for (auto &q : plan) {
    if (!q.isSubstrat) continue;
    EXPECT_LE(q.id.size(), 200u) << "nazwa substratu ponad progiem: " << q.id;
    EXPECT_TRUE(std::ranges::all_of(q.id, [](unsigned char c) { return std::isalnum(c) != 0 || c == '_'; }))
        << "nazwa substratu nie jest identyfikatorem: " << q.id;
    if (q.id.starts_with("STREAM_ADD_x")) {
      sawDigest = true;
      // Prefiks operatora zostaje czytelny; skrot to 16 znakow szesnastkowych.
      EXPECT_EQ(q.id.size(), std::string("STREAM_ADD_x").size() + 16);
      EXPECT_TRUE(std::ranges::all_of(q.id.substr(std::string("STREAM_ADD_x").size()), [](unsigned char c) {
        return std::isxdigit(c) != 0;
      })) << q.id;
    }
  }
  EXPECT_TRUE(sawDigest) << "prog nie zadzialal — zaden wezel nie dostal skrotu";
}

// Skrot liczy sie z CALEJ nazwy czytelnej, wiec pozostaje CZYSTA FUNKCJA trojki
// (operator, parametr, operandy). Na tym stoja dwa niezmienniki: deduplikacja substratow
// oraz odtwarzanie nazwy wezla przeplotu w factorMatchedHashTimeMoves(), ktore sklada ja
// od nowa z nazw zrodel i wyszukuje po niej istniejacy wezel.
TEST(xcompiler, digest_name_is_a_pure_function_of_the_readable_name) {
  auto digestNames = [](qTree &plan) {
    std::vector<std::string> names;
    for (auto &q : plan)
      if (q.isSubstrat && q.id.starts_with("STREAM_ADD_x")) names.push_back(q.id);
    std::ranges::sort(names);
    return names;
  };

  // Ta sama szeroka klauzula w dwoch zapytaniach — jeden wspolny wezel, nie dwa.
  auto shared            = compilePlan(wideAdditionPlan(14) + "SELECT * STREAM twin FROM " + wideFromClause(14) + "\n");
  const auto sharedNames = digestNames(shared);
  ASSERT_EQ(sharedNames.size(), 1u);

  // Powtorna kompilacja daje ten sam skrot — nie zalezy on od kolejnosci w planie,
  // od adresow ani od liczby konsumentow.
  auto again = compilePlan(wideAdditionPlan(14));
  EXPECT_EQ(digestNames(again), sharedNames);

  // Inna kolejnosc skladnikow to INNY program, wiec musi dac inna nazwe. Gdyby skrot
  // nie zalezal od operandow, oba plany uzylyby jednej nazwy dla dwoch roznych wezlow —
  // dokladnie ta cicha zla odpowiedz, ktorej pilnuje validateSubstratNameUniqueness().
  std::string reversedClause("str14");
  for (int index = 13; index >= 1; --index)
    reversedClause += std::format("+str{:02}", index);
  auto reversed = compilePlan(wideAdditionPlan(14) + "SELECT * STREAM mirror FROM " + reversedClause + "\n");
  for (const auto &name : digestNames(reversed))
    if (name != sharedNames.front()) return;
  FAIL() << "odwrocona klauzula nie dostala wlasnego skrotu";
}

// Reduktor nad wezlem o skroconej nazwie. Sam substrat okna dziedziczy dluga nazwe zrodla,
// wiec rowniez przechodzi w galaz skrotu — a plan ma nadal miec ksztalt "okno, potem suma".
TEST(xcompiler, reducer_over_a_digest_named_substrate_keeps_its_shape) {
  auto plan = compilePlan(wideAdditionPlan(14) + "SELECT * STREAM reduced FROM SUMC(wide@(1,3))\n");

  auto &reduced = plan.getQuery("reduced");
  ASSERT_EQ(reduced.lProgram.size(), 2u);
  EXPECT_EQ(reduced.lProgram.back().getCommandID(), STREAM_SUM);

  const auto windowName = reduced.lProgram.front().getStr_();
  EXPECT_TRUE(plan.exists(windowName)) << windowName;
  EXPECT_LE(windowName.size(), 200u);
  EXPECT_TRUE(plan.getQuery(windowName).isSubstrat);
}

// --- Issue 236: drabina priorytetow operatorow strumieniowych ---------------------
//
// Do 2026-08-29 szesc operatorow o jednej postaci — operator plus literal nad jednym
// strumieniem — bylo rozrzuconych na dwa pietra gramatyki: `@`, `&`, `%` i `.agg` wiazaly
// mocniej niz `#`, a `>` i `-` slabiej. Zaden z nich nie dawal sie lancuchowac, bo wszystkie
// zadaly `stream_factor`, a wywolanie reduktora stalo o pietro za wysoko, wiec `a#MAX(b)`
// bylo bledem skladni. RQL.g4 opisuje docelowa drabine; ponizsze testy przypinaja KAZDA
// jej granice od strony planu.
//
// Metoda jest wszedzie ta sama: zapis bez nawiasow ma dac DOKLADNIE ten plan, co zapis
// z nawiasami stawiajacymi grupowanie wprost. Nazwy substratow sa pochodna struktury
// wyrazenia, wiec rownosc ksztaltow planu jest rownowaznoscia grupowania.

namespace {

/// Plan jednego wyrazenia strumieniowego nad trzema zrodlami o rownym takcie.
qTree streamExpressionPlan(const std::string &fromClause) {
  return compilePlan(
      "SUBSTRAT 'memory'\n"
      "DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'\n"
      "DECLARE w INTEGER STREAM b, 1/500 FILE 'b.txt'\n"
      "DECLARE x INTEGER STREAM c, 1/500 FILE 'c.txt'\n"
      "SELECT * STREAM t FROM " +
      fromClause + "\n");
}

/// Ksztalt planu: kazdy wezel niebedacy deklaracja jako "id=program", posortowany.
std::string planShape(qTree &plan) {
  std::vector<std::string> nodes;
  for (auto &q : plan)
    if (!q.isDeclaration() && !q.isCompilerDirective()) nodes.push_back(q.id + "=" + fromProgram(q));
  std::ranges::sort(nodes);
  std::string out;
  for (const auto &node : nodes)
    out += node + "\n";
  return out;
}

/// Zapis bez nawiasow grupuje tak samo, jak zapis z nawiasami.
void expectSameGrouping(const std::string &bare, const std::string &parenthesized) {
  auto lhs = streamExpressionPlan(bare);
  auto rhs = streamExpressionPlan(parenthesized);
  EXPECT_EQ(planShape(lhs), planShape(rhs)) << bare << "   vs   " << parenthesized;
}

}  // namespace

// Poziom 1. Wywolanie reduktora domykaja wlasne nawiasy, wiec jest PRYMITYWEM i stoi tam,
// gdzie nazwa strumienia — po obu stronach `#` i pod kazdym postfiksem. Nawias w
// `MIN(a)#(MAX(b))` byl dokladany wylacznie po to, zeby zejsc na poziom stream_factor.
TEST(xparser, function_call_is_a_primary) {
  expectSameGrouping("MIN(a)#MAX(b)", "(MIN(a))#(MAX(b))");
  expectSameGrouping("a#MAX(b)", "a#(MAX(b))");
  expectSameGrouping("SUMC(a)#SUMC(b)", "(SUMC(a))#(SUMC(b))");
  expectSameGrouping("MIN(a)@(1,4)", "(MIN(a))@(1,4)");
  expectSameGrouping("MIN(a)&2", "(MIN(a))&2");
}

// Granica 2|3 — JEDYNA zmiana znaczenia wzgledem stanu sprzed 2026-08-29. `>` i `-` sa
// postfiksami unarnymi tak samo jak `&` i `%`, wiec wiaza mocniej niz przeplot.
//
// Test musi pokazac ROZNICE, a nie tylko rownowaznosc: oba grupowania sa poprawne i daja
// rozne plany (rozny ogon), wiec sama zgodnosc z nawiasami niczego by nie rozstrzygnela.
TEST(xparser, shift_binds_tighter_than_hash) {
  expectSameGrouping("a#b>1", "a#(b>1)");
  expectSameGrouping("a#b-1/4", "a#(b-1/4)");

  auto tight = streamExpressionPlan("a#b>1");
  auto loose = streamExpressionPlan("(a#b)>1");
  EXPECT_NE(planShape(tight), planShape(loose));
  EXPECT_TRUE(tight.exists("STREAM_TIMEMOVE_1_b"));
  EXPECT_TRUE(loose.exists("STREAM_HASH_a_b"));

  // Dominujacy zapis korpusu traci nawiasy i ma znaczyc dokladnie to samo.
  expectSameGrouping("a>1#b>2", "(a>1)#(b>2)");
}

// Granica 3|4 wynika z typowania, nie z konwencji: `#` zada zgodnych schematow, `+` schemat
// poszerza, wiec `a#b+c` ma tylko jeden dobrze otypowany odczyt.
TEST(xparser, hash_binds_tighter_than_concatenation) {
  expectSameGrouping("a#b+c", "(a#b)+c");
  expectSameGrouping("a+b#c", "a+(b#c)");
}

// Lacznosc operatorow binarnych MUSI zostac lewostronna: compiler sklada nazwe substratu
// lewostronnie, a ta nazwa jest nazwa pliku na dysku. Zmiana lacznosci przemianowalaby
// kazdy istniejacy artefakt, wiec ten test broni zgodnosci wstecz, a nie samej skladni.
TEST(xparser, binary_stream_operators_are_left_associative) {
  expectSameGrouping("a#b#c", "(a#b)#c");
  expectSameGrouping("a+b+c", "(a+b)+c");

  auto plan = streamExpressionPlan("a#b#c");
  EXPECT_TRUE(plan.exists("STREAM_HASH_a_b")) << "przeplot nie jest lewostronny";
}

namespace {

/// Program pola SELECT-a w postaci ONP — do porownan grupowania w wyrazeniu skalarnym.
///
/// Sam PARSER, bez kompilacji: uproszczenia algebraiczne (R3) zwinelyby stale i zatarly
/// roznice, o ktore chodzi. Zrodlem jest jeden strumien o dwoch polach, wiec `v` i `w` sa
/// zwyklymi odwolaniami FieldID.
std::string selectFieldProgram(const std::string &expression) {
  qTree instance;
  auto [parseResult, keyword, streamName] = parserRQLString(instance,
                                                            "DECLARE v INTEGER, w INTEGER STREAM a, 1/500 FILE 'a.txt'\n"
                                                            "SELECT " +
                                                                expression + " STREAM t FROM a\n");
  EXPECT_EQ(parseResult, "OK") << expression;

  std::ostringstream out;
  for (auto &f : instance.getQuery("t").lSchema)
    for (auto &tk : f.lProgram)
      out << tk << ";";
  return out.str();
}

}  // namespace

// `^` stoi PIERWSZE w regule `term`, wiec wiaze mocniej niz `*` i `/` — a te z kolei
// mocniej niz `+`. Test pokazuje ROZNICE, nie tylko rownowaznosc: `v*w^2` i `(v*w)^2` to
// dwa rozne wyrazenia i tylko pierwsze ma znaczyc to, co zapis bez nawiasow.
TEST(xparser, power_binds_tighter_than_multiplication) {
  EXPECT_EQ(selectFieldProgram("v*w^2"), selectFieldProgram("v*(w^2)"));
  EXPECT_NE(selectFieldProgram("v*w^2"), selectFieldProgram("(v*w)^2"));

  EXPECT_EQ(selectFieldProgram("v/w^2"), selectFieldProgram("v/(w^2)"));
  EXPECT_EQ(selectFieldProgram("v+w^2"), selectFieldProgram("v+(w^2)"));
}

// Jedyny prawostronnie laczny operator tej gramatyki. Wolno tak, bo drabina `term` buduje
// program ONP dla expressionEvaluator, a nie nazwe substratu — lewostronnosc jest wymogiem
// tylko po stronie stream_expression, gdzie nazwa wezla jest nazwa pliku na dysku.
TEST(xparser, power_is_right_associative) {
  EXPECT_EQ(selectFieldProgram("v^w^2"), selectFieldProgram("v^(w^2)"));
  EXPECT_NE(selectFieldProgram("v^w^2"), selectFieldProgram("(v^w)^2"));
}

// Literal ujemny jest PRYMITYWEM tego samego pietra, a `unary_op_expression` siega po cale
// `expression` — stad asymetria, ktorej dla `*` nie bylo widac, bo tam nie zmieniala wyniku.
// Test utrwala stan faktyczny: dla `^` zamiar zapisuje sie nawiasem.
TEST(xparser, power_and_unary_minus_group_differently_for_literals_and_fields) {
  EXPECT_EQ(selectFieldProgram("-2^2"), selectFieldProgram("(-2)^2"));
  EXPECT_EQ(selectFieldProgram("-v^2"), selectFieldProgram("-(v^2)"));
}

// Poziom 2 jest lancuchowalny. Do 2026-08-29 kazdy z tych zapisow byl bledem skladni,
// bo postfiksy zadaly `stream_factor`, czyli nazwy albo nawiasu.
TEST(xparser, postfix_stream_operators_chain) {
  expectSameGrouping("a>1>2", "(a>1)>2");
  expectSameGrouping("a&2&2", "(a&2)&2");
  expectSameGrouping("a@(1,4)&2", "(a@(1,4))&2");
  // `-` przetaktowuje do ZADANEGO interwalu, wiec ogniwa lancucha musza isc od szybszego
  // do wolniejszego — inaczej plan pada na wiezie SUBTRACT, a nie na skladni.
  expectSameGrouping("a-1/4-1/2", "(a-1/4)-1/2");
  expectSameGrouping("a@(1,4).sumc", "(a@(1,4)).sumc");
}

// --- Issue 236: `#` przestaje kolidowac z komentarzem ----------------------------
//
// Lekser mial regule `'# ' ~[\r\n]*`, wiec o znaczeniu `#` decydowala SPACJA po nim.
// Skutek nie byl bledem, tylko cisza: `FROM a # b` kompilowalo sie jako `FROM a`, gubiac
// drugi operand bez zadnego komunikatu. Komentarz `#` obsluguje dzis wylacznie
// readLogicalLines() i zajmuje caly wiersz; w lekserze `#` znaczy zawsze przeplot.

// Odstep wokol `#` nie moze zmieniac planu. TRYBEM PORAZKI JEST CICHA ZLA ODPOWIEDZ:
// przed naprawa lewa strona dawala plan jednooperandowy i przechodzila kompilacje.
TEST(xparser, hash_operator_is_not_whitespace_sensitive) {
  expectSameGrouping("a # b", "a#b");
  expectSameGrouping("a # b > 1", "a#(b>1)");

  auto plan = streamExpressionPlan("a # b");
  EXPECT_EQ(fromProgram(plan.getQuery("t")), "PUSH_STREAM(a);PUSH_STREAM(b);STREAM_HASH(0);");
}

// Komentarz na koncu wiersza po `#` ma byc odrzucony GLOSNO. Tresc komentarza jest tu
// wielowyrazowa, wiec konczy sie bledem skladni; komentarz jednowyrazowy trafia dalej jako
// nazwa strumienia i ginie na nierozwiazanym odwolaniu — takze glosno. Komentarz konczacy
// wiersz zapisuje sie `//`.
TEST(xparser, trailing_hash_comment_is_rejected) {
  EXPECT_EXIT(
      {
        qTree instance;
        (void)parserRQLString(instance, "SELECT * STREAM t FROM a # komentarz na koncu wiersza");
        exit(0);
      },
      ::testing::ExitedWithCode(EPERM), "extraneous input");
}

// Komentarz zajmujacy caly wiersz — takze wciety — nadal jest komentarzem. Przechodzi
// przez readLogicalLines(), a nie przez lekser, wiec wymaga sciezki plikowej.
TEST(xparser, whole_line_hash_comment_survives) {
  const std::string fileName("ut_hash_comment.rql");
  {
    std::ofstream out(fileName);
    out << "# komentarz pelnowierszowy\n"
        << "   # komentarz wciety\n"
        << "SUBSTRAT 'memory'\n"
        << "DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'\n"
        << "DECLARE w INTEGER STREAM b, 1/500 FILE 'b.txt'\n"
        << "SELECT * STREAM t FROM a # b\n";
  }

  qTree instance;
  ASSERT_EQ(parserRQLFile_4Test(instance, fileName), "OK");
  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Komentarze zniknely, a `#` w klauzuli FROM zostal przeplotem.
  EXPECT_EQ(fromProgram(instance.getQuery("t")), "PUSH_STREAM(a);PUSH_STREAM(b);STREAM_HASH(0);");
}

// ---------------------------------------------------------------------------------------
// Generator strumieni: `SELECT cells[$] STREAM cell[24] FROM cells`
// ---------------------------------------------------------------------------------------

namespace {

/// Plan w postaci porownywalnej: strumienie posortowane po nazwie, kazdy z pelna trescia.
///
/// Poza operatorem query::operator<< (id, plik, interwal, schemat, program) doklada ogon
/// i poczatek logiczny, bo to one niosa skutki czasowe planu — a wlasnie o brak roznicy
/// w skutkach chodzi w tescie rownowaznosci.
std::string renderPlan(qTree &plan) {
  std::vector<std::string> rendered;
  rendered.reserve(plan.size());
  for (auto &q : plan) {
    std::ostringstream os;
    os << q << ",tail:" << q.startupLatency << ",origin:" << q.logicalOrigin;
    rendered.push_back(os.str());
  }
  std::ranges::sort(rendered);
  std::string out;
  for (const auto &entry : rendered)
    out += entry + "\n";
  return out;
}

}  // namespace

/// Wlasciwosc, dla ktorej generator zostal zrobiony tak, a nie inaczej: po ekspansji plan
/// jest NIE DO ODROZNIENIA od recznie rozpisanych SELECT-ow. Gdyby ten test zaczal padac,
/// generator przestalby byc skrotem zapisu, a stalby sie osobnym bytem w silniku.
TEST(xcompiler, generator_plan_is_identical_to_hand_written_plan) {
  qTree generated;
  auto [genParse, genKeyword, genName] = parserRQLString(generated, R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[4] FROM cells
        SELECT * STREAM grp FROM cell[0]#cell[1]#cell[2]#cell[3]
      )");
  ASSERT_EQ(genParse, "OK");
  compiler generatedCompiler(generated);
  ASSERT_EQ(generatedCompiler.compile(), "OK");

  qTree handWritten;
  auto [manParse, manKeyword, manName] = parserRQLString(handWritten, R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[0] STREAM cell$0 FROM cells
        SELECT cells[1] STREAM cell$1 FROM cells
        SELECT cells[2] STREAM cell$2 FROM cells
        SELECT cells[3] STREAM cell$3 FROM cells
        SELECT * STREAM grp FROM cell$0#cell$1#cell$2#cell$3
      )");
  ASSERT_EQ(manParse, "OK");
  compiler handWrittenCompiler(handWritten);
  ASSERT_EQ(handWrittenCompiler.compile(), "OK");

  EXPECT_EQ(renderPlan(generated), renderPlan(handWritten));
}

/// `$` poza nawiasami kwadratowymi jest WARTOSCIA, wiec kazda instancja liczy co innego.
TEST(xcompiler, generator_substitutes_ordinal_as_a_value) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[0]+$ STREAM cell[3] FROM cells
      )");
  ASSERT_EQ(parseResult, "OK");
  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  // Po ekspansji nie ma juz sladu po PUSH_GENIDX w calym planie.
  for (const auto &qry : instance)
    for (const auto &f : qry.lSchema)
      EXPECT_EQ(std::ranges::count_if(f.lProgram, [](const token &t) { return t.getCommandID() == PUSH_GENIDX; }), 0) << qry.id;

  // Wartosci nie sprawdzamy tokenem, tylko rownowaznoscia planow: dla instancji zerowej
  // `cells[0]+0` zwija simplifyFieldExpressions() i PUSH_VAL(0) prawidlowo znika. Porownanie
  // z recznym zapisem jest odporne na przebiegi upraszczajace, bo dotykaja obu planow tak samo.
  qTree handWritten;
  auto [manParse, manKeyword, manName] = parserRQLString(handWritten, R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[0]+0 STREAM cell$0 FROM cells
        SELECT cells[0]+1 STREAM cell$1 FROM cells
        SELECT cells[0]+2 STREAM cell$2 FROM cells
      )");
  ASSERT_EQ(manParse, "OK");
  compiler handWrittenCompiler(handWritten);
  ASSERT_EQ(handWrittenCompiler.compile(), "OK");

  EXPECT_EQ(renderPlan(instance), renderPlan(handWritten));
}

/// `$` w klauzuli FROM buduje kaskade rodzin: okno per instancja poprzedniej rodziny.
TEST(xcompiler, generator_expands_ordinal_in_from_clause) {
  qTree instance;
  auto [parseResult, firstKeyword, streamName] = parserRQLString(instance, R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[4] FROM cells
        SELECT * STREAM w[4] FROM cell[$]@(2,4)
      )");
  ASSERT_EQ(parseResult, "OK");
  compiler compilerInstance(instance);
  ASSERT_EQ(compilerInstance.compile(), "OK");

  for (int ordinal = 0; ordinal < 4; ++ordinal) {
    EXPECT_TRUE(instance.exists("cell$" + std::to_string(ordinal)));
    EXPECT_TRUE(instance.exists("w$" + std::to_string(ordinal)));
  }
  // Odwolanie `cell[3]` zostalo nazwa fizyczna, a nie zapisem z nawiasem.
  EXPECT_EQ(std::ranges::count_if(instance, [](const query &qry) { return qry.id.find('[') != std::string::npos; }), 0);
}

/// Generator bez `$` wyprodukowalby N identycznych strumieni pod roznymi nazwami — to zawsze
/// pomylka zapisu, nigdy zamiar, wiec kompilator ma ja zatrzymac.
TEST(xcompiler, rejects_generator_without_ordinal) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[0] STREAM cell[4] FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("uses no '$'"), std::string::npos) << verdict;
}

/// `[0]` musi byc odrozniane od braku generatora — inaczej stalby sie po cichu strumieniem `cell`.
TEST(xcompiler, rejects_zero_sized_generator) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[0] FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("positive size"), std::string::npos) << verdict;
}

/// Zwiniety indeks musi miescic sie w zrodle. Kontrola dziala, bo szerokosc DECLARE jest
/// znana juz po parsowaniu.
TEST(xcompiler, rejects_generated_field_index_beyond_source) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[9] FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("has only 4 element"), std::string::npos) << verdict;
}

/// Wyrazenie moze zejsc ponizej zera, zanim wyjdzie poza zrodlo — osobny komunikat.
TEST(xcompiler, rejects_negative_generated_field_index) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[1-$] STREAM cell[4] FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("must not be negative"), std::string::npos) << verdict;
}

/// Poza szablonem `$` nie ma czego oznaczac.
TEST(xcompiler, rejects_ordinal_outside_generator) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM plain FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("outside a stream generator"), std::string::npos) << verdict;
}

/// Odwolanie do nieistniejacej instancji rodziny.
TEST(xcompiler, rejects_reference_beyond_family_range) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[4] FROM cells
        SELECT * STREAM w FROM cell[9]
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("outside the range 0..3"), std::string::npos) << verdict;
}

/// Jedna nazwa pliku nie obsluzy N strumieni.
TEST(xcompiler, rejects_file_directive_on_generator) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[4] FROM cells FILE 'one.dat'
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("FILE directive"), std::string::npos) << verdict;
}

/// Nazwa fizyczna instancji jest zwyklym identyfikatorem, wiec moze zderzyc sie z recznie
/// zadeklarowanym strumieniem. Ciche nadpisanie byloby tu najgorszym z wyjsc.
TEST(xcompiler, rejects_collision_with_existing_stream) {
  const std::string verdict = compileRql(R"(
        DECLARE cell INTEGER[4] STREAM cells, 1/10 FILE 'cells.txt'
        SELECT cells[$] STREAM cell[4] FROM cells
        SELECT cells[0] STREAM cell$2 FROM cells
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("collides"), std::string::npos) << verdict;
}

/// Strumien o schemacie z dzikiego indeksu `[_]` skonsumowany przez zlaczenie. Rozwiniecie `[_]`
/// bylo kiedys osobnym przebiegiem piec krokow po materializacji schematow, wiec buildOutputSchema()
/// kopiowal liste pol zrodla, gdy ta miala jeszcze jedno pole zastepcze: zlaczenie dostawalo dwa
/// pola zamiast trzech, a przesuniecie drugiego argumentu bylo juz liczone z prawidlowej szerokosci.
/// Schemat rozjezdzal sie z ukladem rekordu i wykonanie zamieralo bez bledu.
TEST(xcompiler, index_wildcard_schema_survives_being_joined) {
  qTree wildcard;
  auto [wildParse, wildKeyword, wildName] = parserRQLString(wildcard, R"(
        DECLARE c INTEGER[4] STREAM src, 1 FILE 'src.txt'
        SELECT src[0] STREAM x FROM src
        SELECT src[1] STREAM y FROM src
        SELECT * STREAM j FROM x+y
        SELECT to_integer(j[_]) STREAM w FROM j
        SELECT * STREAM out FROM w+x
      )");
  ASSERT_EQ(wildParse, "OK");
  compiler wildcardCompiler(wildcard);
  ASSERT_EQ(wildcardCompiler.compile(), "OK");

  // Zlaczenie ma trzy pola, a nie dwa, i offsety sa ciagle: w_0,w_1 z lewej, x_2 z prawej.
  auto &joined = wildcard.getQuery("out");
  ASSERT_EQ(joined.lSchema.size(), 3u);
  EXPECT_EQ(joined.descriptorStorage().flatElementCount(), 3);

  // Rownowaznosc z recznym zapisem: `[_]` ma byc czystym skrotem, nie osobna sciezka kompilacji.
  qTree handWritten;
  auto [manParse, manKeyword, manName] = parserRQLString(handWritten, R"(
        DECLARE c INTEGER[4] STREAM src, 1 FILE 'src.txt'
        SELECT src[0] STREAM x FROM src
        SELECT src[1] STREAM y FROM src
        SELECT * STREAM j FROM x+y
        SELECT to_integer(j[0]), to_integer(j[1]) STREAM w FROM j
        SELECT * STREAM out FROM w+x
      )");
  ASSERT_EQ(manParse, "OK");
  compiler handWrittenCompiler(handWritten);
  ASSERT_EQ(handWrittenCompiler.compile(), "OK");

  EXPECT_EQ(renderPlan(wildcard), renderPlan(handWritten));
}

// --- Szerokosc `[_]` liczona ze schematu klauzuli FROM ---------------------------
//
// `x[_]` znaczy „wszystkie sloty, ktore x wnosi do rekordu czytanego przez to zapytanie",
// a nie „wlasna szerokosc strumienia x". Do 2026-08-29 expandIndexWildcards() bralo te druga,
// wiec `x[_]` przy `FROM x@(1,5)+y` rozwijalo sie do JEDNEGO skladnika zamiast pieciu.
// Trybem porazki byl CICHY ZLY WYNIK: plan sie kompilowal, offsety liczone pozniej przez
// collectTransitiveOffsets() byly poprawne, tylko splot miec mial piec tapow, a mial jeden.

namespace {

/// Programy pol zapytania jako tekst — do porownan schematu miedzy dwoma zapisami planu.
/// Sam program klauzuli FROM sie rozni (raz nazwane okno, raz substrat kompilatora), wiec
/// porownanie idzie po tym, co zapytanie LICZY, a nie po nazwie wezla, z ktorego czyta.
std::string fieldPrograms(query &q) {
  std::ostringstream out;
  for (auto &f : q.lSchema) {
    out << f.field_.rname << ":";
    for (auto &t : f.lProgram)
      out << t << ";";
    out << "\n";
  }
  return out.str();
}

}  // namespace

TEST(xcompiler, index_wildcard_width_comes_from_the_from_clause) {
  auto namedWindow  = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        DECLARE c INTEGER[5] STREAM coef, 1 FILE 'c.txt'
        SELECT * STREAM w FROM a@(1,5)
        SELECT w[_]*coef[_] STREAM prod FROM w+coef
        SELECT prod[0] STREAM out FROM SUMC(prod)
      )");
  auto inlineWindow = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        DECLARE c INTEGER[5] STREAM coef, 1 FILE 'c.txt'
        SELECT a[_]*coef[_] STREAM prod FROM a@(1,5)+coef
        SELECT prod[0] STREAM out FROM SUMC(prod)
      )");

  auto &namedProduct  = namedWindow.getQuery("prod");
  auto &inlineProduct = inlineWindow.getQuery("prod");

  // Piec tapow, nie jeden — i te same offsety, bo `[_]` ma byc czystym skrotem.
  EXPECT_EQ(inlineProduct.lSchema.size(), 5u);
  EXPECT_EQ(fieldPrograms(inlineProduct), fieldPrograms(namedProduct));
  EXPECT_EQ(inlineProduct.descriptorFrom(inlineWindow), namedProduct.descriptorFrom(namedWindow));
  EXPECT_EQ(inlineProduct.rInterval, namedProduct.rInterval);
  EXPECT_EQ(inlineProduct.logicalOrigin, namedProduct.logicalOrigin);
  EXPECT_EQ(inlineProduct.startupLatency, namedProduct.startupLatency);

  // Rownosc musi siegac wyniku, nie tylko wezla posredniego.
  EXPECT_EQ(fieldPrograms(inlineWindow.getQuery("out")), fieldPrograms(namedWindow.getQuery("out")));
  EXPECT_EQ(inlineWindow.getQuery("out").startupLatency, namedWindow.getQuery("out").startupLatency);
}

// Bezposredni operand zachowuje sie jak dotad — to jest zapis uzywany w calym korpusie
// (`examples/ecg/*`, `select_cse_commutative_add`), wiec poprawka nie moze go ruszyc.
TEST(xcompiler, index_wildcard_over_direct_operands_is_unchanged) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE c INTEGER[4] STREAM src, 1 FILE 'src.txt'
        DECLARE d INTEGER[4] STREAM other, 1 FILE 'other.txt'
        SELECT src[_]*other[_] STREAM p FROM src+other
      )");
  EXPECT_EQ(plan.getQuery("p").lSchema.size(), 4u);
}

// Reduktor zwija krotnosc do jednego slotu, wiec `a[_]` pod nim znaczy JEDEN skladnik.
// Szerokosc bierze sie z wezla stojacego w FROM, a nie z dna lancucha.
TEST(xcompiler, index_wildcard_under_a_reducer_is_one_slot) {
  auto plan = compilePlan(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        SELECT a[_] STREAM s FROM SUMC(a@(1,5))
      )");
  EXPECT_EQ(plan.getQuery("s").lSchema.size(), 1u);
}

// Nazwa nieobecna w klauzuli FROM nie ma w tym schemacie zadnej szerokosci. Do 2026-08-29
// dostawala wlasna szerokosc strumienia o tej nazwie, czyli liczbe wzieta z innego schematu.
TEST(xcompiler, index_wildcard_over_a_stream_outside_from_is_refused) {
  const auto verdict = compileRql(R"(
        SUBSTRAT 'memory'
        DECLARE c INTEGER[4] STREAM src, 1 FILE 'src.txt'
        DECLARE d INTEGER[4] STREAM other, 1 FILE 'other.txt'
        SELECT other[_] STREAM p FROM src
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("contiguous"), std::string::npos) << verdict;
}

// Okno nad konkatenacja przeplata sloty skladowych (a,b,a,b,...), wiec wklad `a` nie jest
// spojnym blokiem i nie opisuje go zadna pojedyncza szerokosc. Odmowa zamiast zgadywania.
TEST(xcompiler, index_wildcard_through_a_width_changing_join_is_refused) {
  const auto verdict = compileRql(R"(
        SUBSTRAT 'memory'
        DECLARE v INTEGER STREAM a, 1/500 FILE 'a.txt'
        DECLARE w INTEGER STREAM b, 1/500 FILE 'b.txt'
        SELECT a[_] STREAM p FROM (a+b)@(1,5)
      )");
  EXPECT_NE(verdict, "OK");
  EXPECT_NE(verdict.find("contiguous"), std::string::npos) << verdict;
}

// Indeks pola w PUSH_ID jest PLASKI: `f FLOAT[4]` zajmuje cztery indeksy, wiec `src[1]` to
// `f[1]`, a nie kolejne pole schematu. Odwzorowanie liczone wprost po pozycji w lSchema
// dawalo tu FLOAT-owi typ nastepnego pola — a to nie jest „typ nieznany", tylko typ ZLY:
// regula D przepisywalaby na `^` mnozenie zmiennoprzecinkowe, ktorego przepisac nie wolno.
TEST(xcompiler, field_type_lookup_uses_flat_element_index) {
  auto floatArray = compilePlan(
      "SUBSTRAT 'memory'\n"
      "DECLARE f FLOAT[4], n INTEGER STREAM src, 1 FILE 'a.txt'\n"
      "SELECT src[1]*src[1] STREAM t FROM src\n");
  const auto &floatField = floatArray.getQuery("t").lSchema.front();
  EXPECT_EQ(std::ranges::count_if(floatField.lProgram, [](const token &tk) { return tk.getCommandID() == POWER; }), 0)
      << "mnozenie FLOAT nie moze zostac przepisane na potege";

  // Kontrola dodatnia na tym samym ksztalcie schematu: dla tablicy INTEGER przepisanie
  // ma zadzialac na KAZDYM elemencie, nie tylko na zerowym. Bez agresywnych przepisan
  // zostaje mnozenie — przelaczniki zmieniaja POSTAC programu, nigdy jego wynik.
  auto intArray = compilePlan(
      "SUBSTRAT 'memory'\n"
      "DECLARE v INTEGER[4], n INTEGER STREAM src, 1 FILE 'a.txt'\n"
      "SELECT src[0]*src[0], src[3]*src[3] STREAM t FROM src\n");
#if RDB_OPT_SIMPLIFY_EXPRESSIONS && aggressive_expr_optimization
  const command_id expected = POWER;
#else
  const command_id expected = MULTIPLY;
#endif
  for (const auto &item : intArray.getQuery("t").lSchema)
    EXPECT_EQ(std::ranges::count_if(item.lProgram, [expected](const token &tk) { return tk.getCommandID() == expected; }), 1)
        << "pole " << item.field_.rname;
}
