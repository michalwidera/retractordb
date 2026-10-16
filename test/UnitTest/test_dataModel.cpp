#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "config.h"
#include "logCapture.hpp"
#include "rdb/convertTypes.hpp"
#include "rdb/fainterface.hpp"
#include "rdb/memoryStore.hpp"
#include "rdb/payload.hpp"
#include "rdb/probe.hpp"  // sonda E4 (liczy tylko w buildzie RDB_BENCH_PROBE)
#include "rdb/storage.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"  // coreInstance
#include "retractor/lib/RQLParser.hpp"

// ctest -R '^ut-test_dataModel' -V

qTree coreInstance;

/*
file path: test/UnitTest/Data/dataModel/ut_example_schema.rql

DECLARE a INTEGER, b BYTE STREAM core0, 1 FILE 'datafile1.txt'
DECLARE c INTEGER,d INTEGER STREAM core1, 0.5 FILE 'datafile2.txt'
SELECT str1[0],str1[1] STREAM str1 FROM core0#core1
SELECT str2[0]+5 STREAM str2 FROM core0
SELECT str3[0] STREAM str3 FROM core0+core1
SELECT str4[0] STREAM str4 FROM core0%2
SELECT str5[0] STREAM str5 FROM core0>1
SELECT str6[0] STREAM str6 FROM core0-1/2
SELECT str7[0] STREAM str7 FROM core0.max
*/
namespace {

std::unique_ptr<dataModel> dataArea;

class xschema : public ::testing::Test {
 protected:
  xschema() {
    SPDLOG_INFO("Constructor");
    std::vector<std::string> cleanFilesSet = {
        "core0.desc",   //
        "core1.desc",   //
        "str1",         //
        "str1.desc",    //
        "str2",         //
        "str2.desc",    //
        "str1a",        //
        "str1a.desc",   //
        "file_A",       //
        "file_A.desc",  //
        "file_B",       //
        "file_B.desc",  //
        "agse1",        //
        "agse1.desc",   //
        "agse1.meta",   //
        "agse1.shadow"  //
    };

    for (auto i : cleanFilesSet)
      if (std::filesystem::exists(i)) {
        std::filesystem::remove(i);
        SPDLOG_INFO("Drop file {}", i);
      } else
        SPDLOG_WARN("Not found {}", i);

    // This simplified dataModel::load
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea = rdbtest::ok(dataModel::create(coreInstance));

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 11);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 12);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 13);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 14);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 15);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 16);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 111);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 222);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 333);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());

    for (const auto &i : coreInstance)
      if (!i.isDeclaration()) rdbtest::ok(dataArea->constructInputPayload(i, *dataArea->qSet[i.id]));

    pProc = dataArea.get();
  }

  ~xschema() override { pProc = nullptr; }

  void SetUp() override { SPDLOG_INFO("SetUp"); }

  void TearDown() override { SPDLOG_INFO("TearDown"); }
};

TEST_F(xschema, check_construct_payload) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // Okno @(1,4) stemplowane koncem przedzialu: rekord o indeksie logicznym 5 obejmuje
  // pozycje splaszczone 2..5, czyli pola 13..16. Indeks nazywa teraz KONIEC okna, wiec
  // ta sama zawartosc siedzi pod indeksem 5, nie 2.
  // Dodatnia szerokosc uklada najnowsze pole jako pierwsze.
  {
    std::unique_ptr<rdb::payload> payload =
        std::make_unique<rdb::payload>(rdbtest::ok(data.constructAgsePayload(4, 1, "str1", 5)));
    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload->descriptor;
    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *payload;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:16 str1_1:15 str1_2:14 str1_3:13 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

TEST_F(xschema, check_construct_payload_mirror) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16

  // Ujemna szerokosc jest odbiciem lustrzanym tego samego pelnego okna (pozycje 2..5).
  {
    std::unique_ptr<rdb::payload> payload =
        std::make_unique<rdb::payload>(rdbtest::ok(data.constructAgsePayload(-4, 1, "str1", 5)));
    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload->descriptor;

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << *payload;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:13 str1_1:14 str1_2:15 str1_3:16 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str1_2 INTEGER str1_3 }");
  }
}

// Sonda E4 (issue_219). Test o ZNANEJ ODPOWIEDZI, nie o "jakiejś liczbie": dla okna
// @(1,4) nad strumieniem str1 o trzech rekordach po dwa pola liczby wynikają z geometrii
// okna, nie z pomiaru.
//
// str1: 11,12 / 13,14 / 15,16   -> recordsCount=3, descriptorSrcSize=2
// Okno (length=4, step=1, windowIndex=5) jest stemplowane koncem przedzialu:
// windowStart = 5*1 - (4-1) = 2, pozycje płaskie 2,3,4,5 -> recordIndex = 2/2, 3/2, 4/2, 5/2 = 1,1,2,2.
// Geometria (a wiec i praca) jest ta sama co przed przestemplowaniem - zmienil sie
// wylacznie indeks, pod ktorym to okno wystepuje.
//
// Stąd dokładnie:
//   agseWindows  = 1  (jedna konstrukcja okna)
//   agseElements = 4  (cztery odwiedziny elementów - TO jest praca, której nie widzą
//                      liczniki planu: w planie okno jest jednym tokenem)
//   agseReads    = 2  (dwa różne rekordy źródła; cache lastReadPosition oszczędza dwa
//                      odczyty z czterech odwiedzin - dlatego odczyty i odwiedziny są
//                      liczone ROZDZIELNIE)
TEST_F(xschema, probe_e4_agse_window_work_counts) {
  // Test kompiluje się i URUCHAMIA w obu wariantach - to jest sens stałych rdb_probe_*
  // zamiast #ifdef. Mnożnik `on` koduje drugą połowę kontraktu: w buildzie bez sond
  // przejście przez to samo okno nie ma prawa ruszyć żadnego licznika.
  constexpr unsigned long long on = rdb_probe_work ? 1 : 0;

  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  rdb::probe::workReset();
  { auto payload = rdbtest::ok(data.constructAgsePayload(4, 1, "str1", 5)); }
  const auto after = rdb::probe::workReport();

  EXPECT_EQ(after.agseWindows, 1 * on);
  EXPECT_EQ(after.agseElements, 4 * on);
  EXPECT_EQ(after.agseReads, 2 * on);

  // Drugie okno musi DOŁOŻYĆ dokładnie tyle samo - liczniki są procesowe i akumulują,
  // a analiza dzieli je przez liczbę slotów. Gdyby akumulacja gubiła wywołania, model
  // kosztu dostałby zaniżoną pracę i to jest dokładnie ta klasa błędu, przez którą
  // upadł model K20 etap 1.
  { auto payload = rdbtest::ok(data.constructAgsePayload(4, 1, "str1", 5)); }
  const auto twice = rdb::probe::workReport();

  EXPECT_EQ(twice.agseWindows, 2 * on);
  EXPECT_EQ(twice.agseElements, 8 * on);
  EXPECT_EQ(twice.agseReads, 4 * on);

  // Okno lustrzane ma tę samą geometrię, więc tę samą pracę - znak steruje kolejnością
  // pól w wyniku, nie liczbą odwiedzin.
  rdb::probe::workReset();
  { auto payload = rdbtest::ok(data.constructAgsePayload(-4, 1, "str1", 5)); }
  const auto mirrored = rdb::probe::workReport();

  EXPECT_EQ(mirrored.agseElements, 4 * on);
  EXPECT_EQ(mirrored.agseReads, 2 * on);

  // Reset musi naprawdę zerować - bez tego przebiegi kampanii sumowałyby się nawzajem.
  rdb::probe::workReset();
  EXPECT_EQ(rdb::probe::workReport().agseElements, 0u);
  EXPECT_EQ(rdb::probe::workReport().agseWindows, 0u);
}

// Praca okna musi rosnąć LINIOWO z jego długością - to jest cała teza tej sondy: cecha
// rośnie z pracą wykonywaną w slocie, a nie z rozmiarem planu (który dla obu tych okien
// jest identyczny: jeden token STREAM_AGSE).
TEST_F(xschema, probe_e4_agse_elements_scale_with_window_length) {
  constexpr unsigned long long on = rdb_probe_work ? 1 : 0;

  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  rdb::probe::workReset();
  { auto payload = rdbtest::ok(data.constructAgsePayload(2, 1, "str1", 2)); }
  const auto shortWindow = rdb::probe::workReport().agseElements;

  rdb::probe::workReset();
  { auto payload = rdbtest::ok(data.constructAgsePayload(6, 1, "str1", 2)); }
  const auto longWindow = rdb::probe::workReport().agseElements;

  EXPECT_EQ(shortWindow, 2 * on);
  EXPECT_EQ(longWindow, 6 * on);
  EXPECT_EQ(longWindow, shortWindow * 3u);
}

TEST_F(xschema, check_sum) {
  auto dataStr1Owner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &dataStr1 = *dataStr1Owner;
  dataStr1.outputPayload->setDisposable(false);
  static_cast<void>(dataStr1.outputPayload->revRead(0));

  auto dataStr2Owner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str2"]));
  streamInstance &dataStr2 = *dataStr2Owner;
  dataStr2.outputPayload->setDisposable(false);
  static_cast<void>(dataStr2.outputPayload->revRead(0));

  // str1
  // [0] [1]
  //  11, 12
  //  13, 14
  //  15, 16 <-

  // str2
  // 11
  // 22
  // 33 <-
  // 44
  {
    auto payload = *(dataStr1.outputPayload->getPayload()) + *(dataStr2.outputPayload->getPayload());

    std::stringstream coutstring1;
    coutstring1 << rdb::singleLineFormat << payload.descriptor;

    std::stringstream coutstring2;
    coutstring2 << rdb::singleLineFormat << payload;

    EXPECT_TRUE(coutstring2.str() == "{ str1_0:15 str1_1:16 str2_0:333 }");
    EXPECT_TRUE(coutstring1.str() == "{ INTEGER str1_0 INTEGER str1_1 INTEGER str2_0 }");
  }
}

auto print(const std::vector<rdb::descFldVT> &row) {
  std::string res("{ ");
  for (const auto &v : row) {
    std::stringstream coutstring;

    std::visit(
        Overload{                                                                                                           //
                 [&coutstring](std::monostate) { coutstring << "null"; },                                                   //
                 [&coutstring](uint8_t a) { coutstring << (unsigned)a; },                                                   //
                 [&coutstring](int a) { coutstring << a; },                                                                 //
                 [&coutstring](unsigned a) { coutstring << a; },                                                            //
                 [&coutstring](float a) { coutstring << a; },                                                               //
                 [&coutstring](double a) { coutstring << a; },                                                              //
                 [&coutstring](const std::pair<int, int> &a) { coutstring << a.first << "," << a.second; },                 //
                 [&coutstring](const std::pair<std::string, int> &a) { coutstring << a.first << "[" << a.second << "]"; },  //
                 [&coutstring](const std::string &a) { coutstring << a; },                                                  //
                 [&coutstring](const boost::rational<int> &a) { coutstring << a; }},
        v);

    coutstring << " ";
    res.append(coutstring.str());
  }
  res.append("}");
  return res;
}

TEST_F(xschema, getRow_1) {
  /* datafile1.txt contents:
  20 31
  21 32
  22 33
  */
  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());

  // processRows bierze maske pozycyjna rownolegla do planu, nie zbior nazw.
  std::vector<char> rowMask(coreInstance.size(), 0);
  for (std::size_t position = 0; position < coreInstance.size(); ++position)
    if (coreInstance.at(position).id == "core0") rowMask[position] = 1;

  rdbtest::ok(dataArea->processZeroStep());
  auto row1 = rdbtest::ok(dataArea->getRow("core0", 0));
  rdbtest::ok(dataArea->processRows(rowMask));
  auto row2 = rdbtest::ok(dataArea->getRow("core0", 1));

  std::string res1 = print(row1);
  std::string res2 = print(row2);

  EXPECT_TRUE("{ 20 31 }" == res1);
  EXPECT_TRUE("{ 21 32 }" == res2);

  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());
}

// ============================================================
// Tablica uchwytow streamInstance (akcelerator processRows)
// ============================================================
//
// processRows siega po instancje wykonawcza po POZYCJI w planie, a nie po nazwie. Tablica jest
// przebudowywana przy rozjezdzie rewizji planu (qTree::planRevision()), wiec musi byc odporna
// na kazda zmiane ukladu planu - a jej kontrola krzyzowa w Debug musi umiec sie CZERWIENIC,
// inaczej nie jest kontrola, tylko ozdoba. Para testow ponizej pokazuje oba wyniki na tym samym
// ukladzie: rozni je wylacznie to, czy tablica zostala oznaczona jako aktualna wbrew prawdzie.

namespace {

/// Odwraca kolejnosc wezlow planu. Dlugosc bez zmian, wiec sama dlugosc tablicy niczego nie
/// wykryje - rozjazd widac dopiero po tozsamosci instancji na pozycji.
void reversePlanOrder() {
  std::vector<query> reordered(coreInstance.begin(), coreInstance.end());
  std::ranges::reverse(reordered);
  coreInstance.replaceAll(std::move(reordered));
}

std::vector<char> dueMaskFor(const std::string &id) {
  std::vector<char> mask(coreInstance.size(), 0);
  for (std::size_t position = 0; position < coreInstance.size(); ++position)
    if (coreInstance.at(position).id == id) mask[position] = 1;
  return mask;
}

}  // namespace

TEST_F(xschema, handleTable_survives_plan_reorder) {
  // Kontrola NEGATYWNA: po przestawieniu planu tablica przebudowuje sie sama i takt liczy sie
  // poprawnie. Sprawdzany jest wynik, nie samo "nie zginelo" - ta sama para rekordow co
  // w getRow_1, tyle ze policzona na odwroconym planie.
  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());

  // Uzbrojenie tablicy dla UKLADU SPRZED zmiany: maska pusta, wiec zaden strumien nie liczy.
  rdbtest::ok(dataArea->processRows(std::vector<char>(coreInstance.size(), 0)));

  reversePlanOrder();

  rdbtest::ok(dataArea->processZeroStep());
  auto row1 = rdbtest::ok(dataArea->getRow("core0", 0));
  rdbtest::ok(dataArea->processRows(dueMaskFor("core0")));
  auto row2 = rdbtest::ok(dataArea->getRow("core0", 1));

  EXPECT_TRUE("{ 20 31 }" == print(row1));
  EXPECT_TRUE("{ 21 32 }" == print(row2));

  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());
}

TEST_F(xschema, handleTable_stale_entry_is_caught) {
  // Kontrola DODATNIA. Ten sam uklad co wyzej, jedna roznica: tablica zostaje oznaczona jako
  // zbudowana dla biezacej rewizji, choc jej zawartosc opisuje plan sprzed przestawienia.
  // Niezmiennik zabrania takiego stanu, wiec wytworzyc go moze tylko hak testowy - i wlasnie
  // dlatego ten test jest dowodem, ze kontrola w handleAt() ma jak zawiesc.
#ifdef NDEBUG
  GTEST_SKIP() << "kontrola krzyzowa uchwytow zyje tylko w Debug - w Release nie ma czego czerwienic";
#else
  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());
  rdbtest::ok(dataArea->processRows(std::vector<char>(coreInstance.size(), 0)));  // uzbrojenie tablicy

  // Kontrola to RDB_ASSERT: zlamany niezmiennik konczy proces (do 2026-10 rzucala LogicError).
  // Przestawienie planu zachodzi wylacznie w procesie potomnym testu smierci, wiec ten proces -
  // i nastepne testy, ktore dziela z nim plan i model - zostaje z ukladem sprzed testu.
  EXPECT_DEATH(
      {
        reversePlanOrder();
        dataArea->markHandlesFreshForUnitTest();
        static_cast<void>(dataArea->processRows(dueMaskFor("core0")));
      },
      "does not match plan node");
#endif
}

// ============================================================
// Strumien nieobecny w modelu (issue #252)
// ============================================================
//
// Dostep po nazwie konczy proces z nazwa strumienia (RDB_ASSERT w streamRuntime()) - nazwy przychodza
// z planu, wiec brak to blad w kodzie. `qSet[nazwa]` na nieobecnym kluczu WSTAWIAL pusty unique_ptr
// i zaraz go dereferencjonowal - SIGSEGV zamiast bledu. Do 2026-10 byl tu rzut std::logic_error.
// Sprawdzane sa: zatrzymanie z komunikatem, mapa bez wstawionego wpisu i model, ktory dalej
// odpowiada dla strumienia, ktory w nim jest.

TEST_F(xschema, missingStream_is_reported_not_inserted) {
  const std::string ghost = "no_such_stream";
  const auto sizeBefore   = dataArea->qSet.size();

  EXPECT_DEATH(static_cast<void>(dataArea->getPayload(ghost)), "FATAL: .*no_such_stream");
  EXPECT_DEATH(static_cast<void>(dataArea->fetchForward(ghost, 0)), "FATAL: .*no_such_stream");
  EXPECT_DEATH(static_cast<void>(dataArea->getRow(ghost, 0)), "FATAL: .*no_such_stream");

  EXPECT_FALSE(dataArea->qSet.contains(ghost));
  EXPECT_EQ(dataArea->qSet.size(), sizeBefore);

  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());
  rdbtest::ok(dataArea->processZeroStep());
  EXPECT_TRUE("{ 20 31 }" == print(rdbtest::ok(dataArea->getRow("core0", 0))));
  rdbtest::ok(dataArea->qSet["core0"]->outputPayload->resetForUnitTest());
}

// Dolaczenie ad-hoc wpisuje instancje do modelu wszystkie albo zadnej. getAdHoc() przy porazce
// wycofuje PLAN i polega na tym, ze model zostal nietkniety - z qSet nic sie nie usuwa, bo
// tablica uchwytow trzyma surowe wskazniki. Poprawna nazwa przed bledna nie moze wiec zostac
// w modelu sama.
TEST_F(xschema, addQueriesToModel_is_all_or_nothing) {
  query extra = coreInstance["str2"];
  extra.id    = "str2_extra";
  coreInstance.push_back(extra);
  const auto sizeBefore = dataArea->qSet.size();

  EXPECT_EQ(dataArea->addQueriesToModel({"str2_extra", "no_such_stream"}), "no_such_stream");
  EXPECT_FALSE(dataArea->qSet.contains("str2_extra"));
  EXPECT_EQ(dataArea->qSet.size(), sizeBefore);
}

// ============================================================
// Pozycja splaszczona okna AGSE poza zakresem int
// ============================================================
//
// Rekord okna o indeksie logicznym n siega do pozycji splaszczonej n*step, a ta rosnie jak liczba
// rekordow zrodla razy F (szerokosc zrodla w elementach plaskich). W int pekala po ok. 2^31/F
// rekordach zrodla, niezaleznie od kroku - przy F = 65536 (legalne po M11) juz po 32768.
// Tu F = 2 i krok 2, wiec te sama granice przekracza indeks n = 2^30+10: n*step = 2^31+20.
// Sam indeks logiczny miesci sie w int - przepelnic mogl sie wylacznie iloczyn.
//
// Zawartosc okna zalezy tylko od odleglosci n od bazy zrodla, wiec okno daleko na osi ma byc
// identyczne z ta sama geometria blisko poczatku:
// pozycje 2n-3 .. 2n -> rekordy n-2, n-1, n-1, n -> pola 12, 13, 14, 15.

TEST_F(xschema, agse_window_flat_position_beyond_int) {
  constexpr int n = (1 << 30) + 10;

  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  const auto nearWindow = rdbtest::ok(data.constructAgsePayload(4, 2, "str1", 2, 0));
  const auto farWindow  = rdbtest::ok(data.constructAgsePayload(4, 2, "str1", n, n - 2));

  std::stringstream nearText;
  nearText << rdb::singleLineFormat << nearWindow;
  std::stringstream farText;
  farText << rdb::singleLineFormat << farWindow;

  EXPECT_EQ(nearText.str(), "{ str1_0:15 str1_1:14 str1_2:13 str1_3:12 }");
  EXPECT_EQ(farText.str(), nearText.str());
}

// Druga strona tego samego rachunku: queryInputsAvailable decyduje, czy wezel dolaczany ad hoc
// ma juz komplet rekordow zrodla. Przy zawinietej pozycji odpowiadal "nie" na zawsze - okno
// nigdy nie dostawalo bazy, wiec milczalo bez bledu.
TEST_F(xschema, agse_adhoc_join_flat_position_beyond_int) {
  constexpr int n = (1 << 30) + 10;

  const auto [status, keyword, name] = parserRQLString(coreInstance, "SELECT agse1[0] STREAM agse1 FROM str1@(2,4)");
  ASSERT_EQ(status, "OK");
  // Fikstura nie uruchamia kompilatora, wiec interwal ustawiamy recznie. Przy interwale 1 pierwszy
  // nalezny slot n+1 daje indeks n (T = (n + 1 + W) * Delta, W = 0).
  coreInstance["agse1"].rInterval = 1;
  ASSERT_EQ(dataArea->addQueriesToModel({"agse1"}), "");

  // Rekord fizyczny 0 zrodla nosi indeks logiczny n-2, wiec okno n ma komplet rekordow n-2..n.
  dataArea->qSet["str1"]->logicalIndexBase = n - 2;

  rdbtest::ok(dataArea->processRows(dueMaskFor("agse1"), boost::rational<int>(n + 1)));

  // Baze dostaje tylko wezel, dla ktorego queryInputsAvailable odpowiedzialo "tak".
  EXPECT_EQ(dataArea->qSet["agse1"]->logicalIndexBase, std::optional<int>(n));

  std::stringstream window;
  window << rdb::singleLineFormat << *dataArea->qSet["agse1"]->inputPayload;
  EXPECT_EQ(window.str(), "{ agse1_0:15 agse1_1:14 agse1_2:13 agse1_3:12 }");
}

TEST_F(xschema, reduceFieldsToPayload_max) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MAX = 16 (pole RATIONAL, patrz K24/D4)
  auto result = rdbtest::ok(data.reduceFieldsToPayload(STREAM_MAX, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:16/1 }");
}

TEST_F(xschema, reduceFieldsToPayload_min) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → MIN = 15 (pole RATIONAL, patrz K24/D4)
  auto result = rdbtest::ok(data.reduceFieldsToPayload(STREAM_MIN, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:15/1 }");
}

TEST_F(xschema, reduceFieldsToPayload_sum) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → SUM = 31 (pole RATIONAL, patrz K24/D4)
  auto result = rdbtest::ok(data.reduceFieldsToPayload(STREAM_SUM, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:31/1 }");
}

TEST_F(xschema, reduceFieldsToPayload_avg) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);
  // str1 last record: {15, 16} → AVG = 31/2.
  // K24/D4: wynik redukcji jest polem RATIONAL i pozostaje dokladny. Wczesniej
  // przechodzil przez rational_cast<int> i dawal 15, mimo ze pole wyjsciowe
  // zadeklarowane przez kompilator bylo RATIONAL - stad mianownik zawsze 1.
  auto result = rdbtest::ok(data.reduceFieldsToPayload(STREAM_AVG, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:31/2 }");
}

TEST_F(xschema, constructOutputPayload_expression) {
  // str2: SELECT str2[0]+5 FROM core0 → core0.a=20, result=25
  rdbtest::ok(dataArea->processZeroStep());
  rdbtest::ok(dataArea->constructInputPayload(coreInstance["str2"], *dataArea->qSet["str2"]));
  rdbtest::ok(dataArea->qSet["str2"]->constructOutputPayload(coreInstance["str2"].lSchema));
  std::stringstream ss;
  ss << rdb::singleLineFormat << *(dataArea->qSet["str2"]->outputPayload->getPayload());
  EXPECT_EQ(ss.str(), "{ str2_0:25 }");
}

// Przyczyny odmowy budowy modelu: wadliwy deskryptor i sciezka wyjscia sprzeczna z systemem
// plikow (K2) oraz dowiazanie w miejscu pliku wyjscia (K4, ELOOP). Klasa i errno jak
// w checkOutputFilesOpenable. Czesciowo zbudowany model nie moze byc publikowany (#474).
TEST(dataModel, initialization_refusal_preserves_class_and_discards_partial_model) {
  const std::filesystem::path directory("model_init_refusal");
  std::filesystem::remove_all(directory);
  std::filesystem::create_directory(directory);
  std::ofstream(directory / "source.txt") << "7\n8\n";
  const std::string prefix =
      "STORAGE 'model_init_refusal'\n"
      "DECLARE v INTEGER STREAM src, 1 TEXTFILE 'model_init_refusal/source.txt'\n";

  {
    std::ofstream(directory / "bad.desc") << "invalid descriptor\n";
    qTree plan;
    ASSERT_EQ(parsePlanText(plan, prefix + "SELECT * STREAM bad FROM src\n").status, "OK");
    compiler cm(plan);
    ASSERT_EQ(cm.compile(), "OK");
    const auto model = dataModel::create(plan);
    ASSERT_RDB_ERROR(model, rdb::Errc::CorruptDescriptor, "bad.desc");
  }
  std::filesystem::remove(directory / "bad.desc");
  {
    qTree plan;
    ASSERT_EQ(parsePlanText(plan, prefix + "SELECT * STREAM bad FROM src FILE 'missing/out'\n").status, "OK");
    compiler cm(plan);
    ASSERT_EQ(cm.compile(), "OK");
    const auto model = dataModel::create(plan);
    ASSERT_RDB_ERROR(model, rdb::Errc::Compile, "cannot open output file");
    EXPECT_EQ(model.error().err(), ENOENT);
  }
  std::filesystem::create_symlink("source.txt", directory / "link_out");
  {
    qTree plan;
    ASSERT_EQ(parsePlanText(plan, prefix + "SELECT * STREAM bad FROM src FILE 'link_out' STORAGE POSIX\n").status, "OK");
    compiler cm(plan);
    ASSERT_EQ(cm.compile(), "OK");
    const auto model = dataModel::create(plan);
    ASSERT_RDB_ERROR(model, rdb::Errc::IO, "link_out");
    EXPECT_EQ(model.error().err(), ELOOP);
  }
  {
    qTree plan;
    ASSERT_EQ(parsePlanText(plan, prefix + "SELECT * STREAM good FROM src STORAGE MEMORY\n").status, "OK");
    compiler cm(plan);
    ASSERT_EQ(cm.compile(), "OK");
    const auto model = dataModel::create(plan);
    ASSERT_RDB_OK(model);
    EXPECT_TRUE((*model)->qSet.contains("src"));
    EXPECT_TRUE((*model)->qSet.contains("good"));
  }
  std::filesystem::remove_all(directory);
}

TEST(dataModel, empty_plan_remains_an_internal_error) {
  qTree plan;
  EXPECT_DEATH({ (void)dataModel::create(plan); }, "coreInstance is empty");
}

TEST(dataModel, empty_directive_value_remains_an_internal_error) {
  qTree plan;
  query directive;
  directive.id = ":STORAGE";
  plan.push_back(directive);
  EXPECT_DEATH({ (void)dataModel::create(plan); }, "has empty value");
}

TEST_F(xschema, constructRulesAndUpdate_empty_rules) {
  // str2 has no RULE declarations → constructRulesAndUpdate is a no-op (no crash)
  rdbtest::ok(dataArea->qSet["str2"]->constructRulesAndUpdate(coreInstance["str2"]));
  SUCCEED();
}

TEST_F(xschema, reduceFieldsToPayload_single_field) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str2"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);
  // str2 last record: {333} → single-field aggregate
  auto result = rdbtest::ok(data.reduceFieldsToPayload(STREAM_MAX, "str2"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str2:333/1 }");
}

std::unique_ptr<dataModel> dataArea_rules;

class xschema_rules : public ::testing::Test {
 protected:
  xschema_rules() {
    for (const auto *f :
         {"rule_marker1.txt", "rule_marker2.txt", "str_rule", "str_rule.desc", "rules_core0.desc", "datafile1.txt.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);

    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_rules_schema.rql");
    dataArea_rules = rdbtest::ok(dataModel::create(coreInstance));
    pProc          = dataArea_rules.get();
  }
  ~xschema_rules() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(xschema_rules, constructRulesAndUpdate_system_rule_fires) {
  // core0 first row: a=20 → str_rule[0]=20 > 0 → rule1 fires, rule2 does not
  std::vector<char> ruleMask(coreInstance.size(), 0);
  for (std::size_t position = 0; position < coreInstance.size(); ++position)
    if (coreInstance.at(position).id == "str_rule") ruleMask[position] = 1;

  rdbtest::ok(dataArea_rules->processZeroStep());
  rdbtest::ok(dataArea_rules->processRows(ruleMask));
  EXPECT_TRUE(std::filesystem::exists("rule_marker1.txt")) << "rule1 (>0) should fire for positive data";
  EXPECT_FALSE(std::filesystem::exists("rule_marker2.txt")) << "rule2 (<0) should not fire for positive data";
}

std::unique_ptr<dataModel> dataArea_null;

class xschema_all_null : public ::testing::Test {
 protected:
  xschema_all_null() {
    // Destroy first so its metaData flushes before files are removed
    dataArea_null.reset();
    for (const auto *f : {"core0.desc", "core1.desc", "str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea_null = rdbtest::ok(dataModel::create(coreInstance));
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(0, std::nullopt);
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(1, std::nullopt);
    rdbtest::ok(dataArea_null->qSet["str1"]->outputPayload->write());
    pProc = dataArea_null.get();
  }
  ~xschema_all_null() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

// Logika trójwartościowa: agregacja na samych NULL-ach → wynik NULL
// Null bity są w currentEntry_ metaData tej samej instancji storage,
// więc revRead(0) odczyta je poprawnie bez potrzeby flushu na dysk.
TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_sum) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_SUM, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_min) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MIN, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_max) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MAX, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_all_null, reduceFieldsToPayload_all_null_avg) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_AVG, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

class xschema_partial_null : public ::testing::Test {
 protected:
  xschema_partial_null() {
    dataArea_null.reset();
    for (const auto *f : {"core0.desc", "core1.desc", "str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea_null = rdbtest::ok(dataModel::create(coreInstance));
    // pole 0 = NULL, pole 1 = 10 (nie-NULL)
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(0, std::nullopt);
    dataArea_null->qSet["str1"]->outputPayload->getPayload()->setItem(1, 10);
    rdbtest::ok(dataArea_null->qSet["str1"]->outputPayload->write());
    pProc = dataArea_null.get();
  }
  ~xschema_partial_null() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

// Logika trójwartościowa: NULL ignorowany, agregacja tylko na wartościach niezerowych
TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_sum) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_SUM, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10/1 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_avg) {
  // AVG: tylko 1 pole niezerowe (10), mianownik = 1, wynik = 10
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_AVG, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10/1 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_min) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MIN, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10/1 }");
}

TEST_F(xschema_partial_null, reduceFieldsToPayload_partial_null_max) {
  auto result = rdbtest::ok(dataArea_null->qSet["str1"]->reduceFieldsToPayload(STREAM_MAX, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10/1 }");
}

// Regression: null bits must survive flush to disk so a second storage reader
// sees them correctly. Before fix: second reader saw totalRecords()==0 →
// all-non-null fallback → SUM(null,null)=0 instead of null.
TEST_F(xschema_all_null, null_bits_flushed_to_disk_second_reader_sum) {
  auto secondOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &second = *secondOwner;
  second.outputPayload->setDisposable(false);
  auto result = rdbtest::ok(second.reduceFieldsToPayload(STREAM_SUM, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:null }");
}

TEST_F(xschema_partial_null, null_bits_flushed_to_disk_second_reader_ignores_null) {
  auto secondOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &second = *secondOwner;
  second.outputPayload->setDisposable(false);
  auto result = rdbtest::ok(second.reduceFieldsToPayload(STREAM_SUM, "str1"));
  std::stringstream ss;
  ss << rdb::singleLineFormat << result;
  EXPECT_EQ(ss.str(), "{ str1:10/1 }");
}

// ============================================================
// Kontrakt etapu 9 (#521): streamInstance - okna, reguly, pola wyjscia
// ============================================================

// Okno siegajace przed poczatek strumienia: brakujace pozycje sa NULL. Krok mnozy indeks okna
// (pozycje windowIndex*step-(N-1) .. windowIndex*step). str1 = 11,12 / 13,14 / 15,16.
TEST_F(xschema, constructAgsePayload_marks_missing_elements_null_and_scales_by_step) {
  auto dataOwner       = rdbtest::ok(streamInstance::create(coreInstance, coreInstance["str1"]));
  streamInstance &data = *dataOwner;
  data.outputPayload->setDisposable(false);

  const auto early = rdbtest::ok(data.constructAgsePayload(4, 1, "str1", 1));  // pozycje -2..1
  EXPECT_EQ(early.getItemVT(0), std::optional<rdb::descFldVT>(12));
  EXPECT_EQ(early.getItemVT(1), std::optional<rdb::descFldVT>(11));
  EXPECT_FALSE(early.getItemVT(2).has_value());
  EXPECT_FALSE(early.getItemVT(3).has_value());

  const auto mirrored = rdbtest::ok(data.constructAgsePayload(-4, 1, "str1", 1));
  EXPECT_FALSE(mirrored.getItemVT(0).has_value());
  EXPECT_FALSE(mirrored.getItemVT(1).has_value());
  EXPECT_EQ(mirrored.getItemVT(2), std::optional<rdb::descFldVT>(11));
  EXPECT_EQ(mirrored.getItemVT(3), std::optional<rdb::descFldVT>(12));

  const auto stepped = rdbtest::ok(data.constructAgsePayload(2, 2, "str1", 2));  // pozycje 3..4
  EXPECT_EQ(stepped.getItemVT(0), std::optional<rdb::descFldVT>(15));
  EXPECT_EQ(stepped.getItemVT(1), std::optional<rdb::descFldVT>(14));
}

class StreamRuntimeContract : public ::testing::Test {
 protected:
  std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_stream_runtime_contract";
  qTree plan;
  std::unique_ptr<dataModel> model;

  void SetUp() override {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "input.bin", std::ios::binary) << std::string(12, '\0');
    const auto source = parsePlanText(plan, "STORAGE '" + dir.string() +
                                                "'\n"
                                                "DECLARE v INTEGER, s STRING[8] STREAM src, 1 BINFILE '" +
                                                (dir / "input.bin").string() +
                                                "'\n"
                                                "SELECT src[0], src[1] STREAM out FROM src RETENTION 8 STORAGE MEMORY\n"
                                                "RULE onNumber ON out WHEN out[0] DO DUMP 0 TO 3 RETENTION 2\n"
                                                "RULE onText ON out WHEN out[1] DO SYSTEM 'touch " +
                                                (dir / "text_fired").string() + "'\n");
    ASSERT_EQ(source.status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    std::filesystem::remove_all(dir);
  }

  [[nodiscard]] streamInstance &out() { return *model->qSet.at("out"); }

  // Zapisuje rekord strumienia `out` i zwraca jego bajty, tak jak trafiaja do zrzutu.
  std::string writeRecord(std::optional<int> number, std::optional<std::string> text) {
    auto *record = out().outputPayload->getPayload();
    record->setItemVT(0, number ? std::optional<rdb::descFldVT>(*number) : std::nullopt);
    record->setItemVT(1, text ? std::optional<rdb::descFldVT>(*text) : std::nullopt);
    EXPECT_EQ(rdbtest::ok(out().outputPayload->write()), rdb::WriteStatus::Ok);
    return {reinterpret_cast<const char *>(record->span().data()), record->descriptor.getSizeInBytes()};
  }

  [[nodiscard]] std::string fileBytes(const char *name) const {
    std::ifstream file(dir / name, std::ios::binary);
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  }

  // Kopia wezla `out` z jedna regula o podanej nazwie (pusta nazwa = bez regul).
  [[nodiscard]] query withRule(const std::string &name) {
    query copy = plan.getQuery("out");
    copy.lRules.remove_if([&](const rule &r) { return r.name != name; });
    return copy;
  }
};

// Warunek: NULL, zero i pusty STRING nie uruchamiaja akcji; liczba niezerowa (takze ujemna)
// i niepusty STRING uruchamiaja. DUMP rejestruje zadanie z nazwa reguly, zakresem [0, 3)
// i RETENTION 2 w katalogu STORAGE; kazde zadanie dopisuje biezacy rekord w swoim slocie.
TEST_F(StreamRuntimeContract, rule_condition_truthiness_selects_dump_and_system_actions) {
  const query &node = plan.getQuery("out");
  const auto marker = dir / "text_fired";

  writeRecord(0, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(node));
  writeRecord(std::nullopt, std::nullopt);
  ASSERT_RDB_OK(out().constructRulesAndUpdate(node));
  EXPECT_FALSE(std::filesystem::exists(marker));
  EXPECT_FALSE(std::filesystem::exists(dir / "out_onNumber_dump_0.tmp"));

  writeRecord(0, "x");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(node));
  EXPECT_TRUE(std::filesystem::exists(marker));
  EXPECT_FALSE(std::filesystem::exists(dir / "out_onNumber_dump_0.tmp"));
  std::filesystem::remove(marker);

  const auto negative = writeRecord(-3, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(node));
  EXPECT_FALSE(std::filesystem::exists(marker));
  EXPECT_EQ(fileBytes("out_onNumber_dump_0.tmp"), negative);

  const auto seven = writeRecord(7, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(node));
  EXPECT_EQ(fileBytes("out_onNumber_dump_0.tmp"), negative + seven);
  EXPECT_EQ(fileBytes("out_onNumber_dump_1.tmp"), seven);
  EXPECT_FALSE(std::filesystem::exists(dir / "out_onNumber_dump.tmp"));
}

// Regula z armAtCount N jest pomijana, dopoki strumien nie ma N rekordow.
TEST_F(StreamRuntimeContract, rule_waits_until_records_reach_armAtCount) {
  query armed                     = withRule("onNumber");
  armed.lRules.front().armAtCount = 3;
  writeRecord(1, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(armed));
  writeRecord(2, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(armed));
  EXPECT_FALSE(std::filesystem::exists(dir / "out_onNumber_dump_0.tmp"));
  const auto third = writeRecord(3, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(armed));
  EXPECT_EQ(fileBytes("out_onNumber_dump_0.tmp"), third);
}

// Zadanie zrzutu biegnie dalej w slotach, w ktorych wezel nie ma juz zadnej reguly.
TEST_F(StreamRuntimeContract, pending_dump_advances_with_an_empty_rule_list) {
  const query fireOnce = withRule("onNumber");
  const query noRules  = withRule("");
  ASSERT_TRUE(noRules.lRules.empty());

  std::string expected = writeRecord(1, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(fireOnce));
  expected += writeRecord(2, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(noRules));
  expected += writeRecord(3, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(noRules));
  writeRecord(4, "");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(noRules));
  EXPECT_EQ(fileBytes("out_onNumber_dump_0.tmp"), expected);
}

// Warunek bez wartosci logicznej (para) to Errc::Eval, a porazka utworzenia zrzutu to Errc::IO -
// oba wracaja w Result zamiast konczyc proces.
TEST_F(StreamRuntimeContract, rule_errors_return_in_result) {
  query pairCondition                    = withRule("onNumber");
  pairCondition.lRules.front().condition = {token(PUSH_VAL, rdb::descFldVT(std::pair<int, int>{1, 2}))};
  writeRecord(1, "");
  EXPECT_RDB_ERROR(out().constructRulesAndUpdate(pairCondition), rdb::Errc::Eval, "pair");

  std::filesystem::remove_all(dir);
  writeRecord(1, "");
  const auto missingStorage = out().constructRulesAndUpdate(withRule("onNumber"));
  ASSERT_RDB_ERROR(missingStorage, rdb::Errc::IO, "failed to create");
  EXPECT_EQ(missingStorage.error().err(), ENOENT);
}

// SYSTEM konczacy sie bledem nie przerywa slotu, ale zostawia wpis w logu z kodem wyjscia.
TEST_F(StreamRuntimeContract, failing_system_command_is_logged_without_failing_the_slot) {
  query failing                        = withRule("onText");
  failing.lRules.front().systemCommand = "exit 3";
  const LogCapture log;
  writeRecord(0, "x");
  ASSERT_RDB_OK(out().constructRulesAndUpdate(failing));
  EXPECT_THAT(log.text(), ::testing::HasSubstr("system() command exited with status: 3"));
}

// Okno rekordowe: jedna wartosc slotu na rekord z zakresu last-(width-1)..last, z przesunieciem
// o baze logiczna; NULL i rekordy spoza historii sa pomijane, a puste okno daje same NULL.
TEST_F(StreamRuntimeContract, record_window_skips_nulls_and_missing_records) {
  writeRecord(1, "");
  writeRecord(std::nullopt, "");
  writeRecord(4, "");
  writeRecord(7, "");

  windowGroup group;
  group.source    = "out";
  group.slot      = 0;
  group.width     = 3;
  group.valueType = rdb::RATIONAL;
  using rational  = boost::rational<int>;

  const auto full = rdbtest::ok(out().reduceRecordWindow(group, 3, 0));  // null, 4, 7
  EXPECT_EQ(full.count, 2);
  EXPECT_EQ(full.minValue, rdb::descFldVT(rational(4)));
  EXPECT_EQ(full.maxValue, rdb::descFldVT(rational(7)));
  EXPECT_EQ(full.sumValue, rdb::descFldVT(rational(11)));
  EXPECT_EQ(full.avgValue, rdb::descFldVT(rational(11, 2)));

  const auto ahead = rdbtest::ok(out().reduceRecordWindow(group, 5, 0));  // 7 i dwa rekordy przyszle
  EXPECT_EQ(ahead.count, 1);
  EXPECT_EQ(ahead.avgValue, rdb::descFldVT(rational(7)));

  const auto shifted = rdbtest::ok(out().reduceRecordWindow(group, 4, 2));  // fizyczne 0..2: 1, null, 4
  EXPECT_EQ(shifted.count, 2);
  EXPECT_EQ(shifted.sumValue, rdb::descFldVT(rational(5)));
  EXPECT_EQ(shifted.avgValue, rdb::descFldVT(rational(5, 2)));

  const auto empty = rdbtest::ok(out().reduceRecordWindow(group, 0, 2));
  EXPECT_EQ(empty.count, 0);
  for (const auto &value : {empty.minValue, empty.maxValue, empty.sumValue, empty.avgValue})
    EXPECT_TRUE(std::holds_alternative<std::monostate>(value));

  // Wyrazenie liczone na kazdym rekordzie okna, typ wyniku z grupy (DOUBLE, FLOAT).
  group.program   = {token(PUSH_ID, rdb::descFldVT(std::pair<std::string, int>(out().outputPayload->descriptor[0].rname, 0))),
                     token(PUSH_VAL, rdb::descFldVT(10)), token(MULTIPLY)};
  group.valueType = rdb::DOUBLE;
  const auto computed = rdbtest::ok(out().reduceRecordWindow(group, 3, 0));
  EXPECT_EQ(computed.count, 2);
  EXPECT_EQ(computed.maxValue, rdb::descFldVT(70.0));
  EXPECT_EQ(computed.avgValue, rdb::descFldVT(55.0));
  group.valueType = rdb::FLOAT;
  EXPECT_EQ(rdbtest::ok(out().reduceRecordWindow(group, 3, 0)).minValue, rdb::descFldVT(40.0F));
}

// Pola wyjscia: NULL z wyrazenia i nieudana konwersja na typ pola daja NULL pola; blad
// przerywa obliczenia bez cofania pol juz ustawionych, a magazyn nie dostaje rekordu.
TEST_F(StreamRuntimeContract, output_fields_null_on_failed_conversion_and_stop_on_error) {
  auto &runtime = out();
  runtime.inputPayload->setItemVT(0, rdb::descFldVT(5));
  runtime.inputPayload->setItemVT(1, rdb::descFldVT(std::string("in")));
  auto *record = runtime.outputPayload->getPayload();

  std::list<field> fields = plan.getQuery("out").lSchema;
  ASSERT_EQ(fields.size(), 2U);
  ASSERT_RDB_OK(runtime.constructOutputPayload(fields));
  EXPECT_EQ(record->getItemVT(0), std::optional<rdb::descFldVT>(5));
  EXPECT_EQ(record->getItemVT(1), std::optional<rdb::descFldVT>(std::string("in")));

  fields.front().lProgram = {token(PUSH_VAL, rdb::descFldVT(1)), token(PUSH_VAL, rdb::descFldVT(0)), token(DIVIDE)};
  ASSERT_RDB_OK(runtime.constructOutputPayload(fields));
  EXPECT_FALSE(record->getItemVT(0).has_value());

  record->setItemVT(0, rdb::descFldVT(99));
  fields.front().lProgram = {token(PUSH_VAL, rdb::descFldVT(1e20))};
  ASSERT_RDB_OK(runtime.constructOutputPayload(fields));
  EXPECT_FALSE(record->getItemVT(0).has_value());

  fields                              = plan.getQuery("out").lSchema;
  std::next(fields.begin())->lProgram = {token(PUSH_VAL, rdb::descFldVT(std::string("a"))),
                                         token(PUSH_VAL, rdb::descFldVT(std::string("b"))), token(SUBTRACT)};
  record->setItemVT(0, rdb::descFldVT(99));
  record->setItemVT(1, rdb::descFldVT(std::string("old")));
  EXPECT_RDB_ERROR(runtime.constructOutputPayload(fields), rdb::Errc::Eval);
  EXPECT_EQ(record->getItemVT(0), std::optional<rdb::descFldVT>(5));
  EXPECT_EQ(record->getItemVT(1), std::optional<rdb::descFldVT>(std::string("old")));
  EXPECT_EQ(runtime.outputPayload->getRecordsCount(), 0U);
}

// ============================================================
// Kontrakt etapu 9 (#521): dataModel - maska, kolejnosc slotu, origin + ogon, odczyty
// ============================================================

class DataModelSlotContract : public ::testing::Test {
 protected:
  std::filesystem::path dir = std::filesystem::temp_directory_path() / "ut_datamodel_slot_contract";
  qTree plan;
  std::unique_ptr<dataModel> model;

  [[nodiscard]] std::string planText() const {
    return "STORAGE '" + dir.string() +
           "'\n"
           "SUBSTRAT 'memory'\n"
           "DECLARE v INTEGER STREAM src, 1 TEXTFILE '" +
           (dir / "input.txt").string() +
           "' ONESHOT\n"
           "SELECT src[0] STREAM a FROM src RETENTION 8 STORAGE MEMORY\n"
           "SELECT a[0]*2 STREAM b FROM a STORAGE MEMORY\n"
           "SELECT * STREAM w FROM src@(1,3) STORAGE MEMORY\n";
  }

  void SetUp() override {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "input.txt") << "10\n20\n30\n40\n50\n";
    ASSERT_EQ(parsePlanText(plan, planText()).status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model = rdbtest::ok(dataModel::create(plan));
    pProc = model.get();
  }

  void TearDown() override {
    pProc = nullptr;
    model.reset();
    std::filesystem::remove_all(dir);
  }

  [[nodiscard]] std::vector<char> due(std::initializer_list<const char *> ids) const {
    std::vector<char> mask(plan.size(), 0);
    for (std::size_t position = 0; position < plan.size(); ++position)
      for (const char *id : ids)
        if (plan.at(position).id == id) mask[position] = 1;
    return mask;
  }

  [[nodiscard]] std::vector<char> allDue() const { return std::vector<char>(plan.size(), 1); }

  [[nodiscard]] std::optional<rdb::descFldVT> latest(const char *id, std::size_t back = 0) {
    auto &out = *model->qSet.at(id)->outputPayload;
    EXPECT_EQ(rdbtest::ok(out.revRead(back)), rdb::ReadStatus::Ok);
    return out.getPayload()->getItemVT(0);
  }

  [[nodiscard]] std::optional<rdb::descFldVT> sourceValue() {
    return model->qSet.at("src")->outputPayload->getPayload()->getItemVT(0);
  }
};

// Dyrektywy znikaja z planu, kazdy pozostaly wezel ma instancje, a magazyny MEMORY trafiaja do
// podanego Memory Store, nie do domyslnego procesu. Blad otwarcia magazynu niesie nazwe strumienia.
TEST_F(DataModelSlotContract, create_strips_directives_and_builds_every_stream_in_the_given_memory_store) {
  EXPECT_TRUE(std::ranges::none_of(plan, [](const query &q) { return q.isCompilerDirective(); }));
  EXPECT_EQ(model->qSet.size(), plan.size());
  for (const auto &q : plan)
    EXPECT_TRUE(model->qSet.contains(q.id)) << q.id;

  rdb::MemoryStore store;
  const auto defaultBuckets = rdb::MemoryStore::processDefault().size();
  qTree isolated;
  ASSERT_EQ(parsePlanText(isolated, planText()).status, "OK");
  ASSERT_EQ(compiler(isolated).compile(), "OK");
  const auto isolatedModel = rdbtest::ok(dataModel::create(isolated, &store));
  EXPECT_EQ(store.size(), 3U);  // a, b, w
  EXPECT_EQ(rdb::MemoryStore::processDefault().size(), defaultBuckets);

  std::ofstream(dir / "a.desc") << "invalid descriptor\n";
  qTree broken;
  ASSERT_EQ(parsePlanText(broken, "STORAGE '" + dir.string() + "'\nDECLARE v INTEGER STREAM src, 1 TEXTFILE '" +
                                      (dir / "input.txt").string() + "'\nSELECT src[0] STREAM a FROM src\n")
                .status,
            "OK");
  ASSERT_EQ(compiler(broken).compile(), "OK");
  EXPECT_RDB_ERROR(dataModel::create(broken), rdb::Errc::CorruptDescriptor, "stream 'a'");
}

// Slot liczy wylacznie wezly z maski. Wezly obliczane ida w porzadku planu (b czyta rekord a
// z tego samego slotu), czytajac biezacy rekord zrodla; zrodlo przesuwa sie dopiero na koncu.
TEST_F(DataModelSlotContract, processRows_computes_due_nodes_in_plan_order_before_advancing_sources) {
  ASSERT_RDB_OK(model->processZeroStep());
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(10));

  ASSERT_RDB_OK(model->processRows(allDue(), 1));
  EXPECT_EQ(latest("a"), std::optional<rdb::descFldVT>(10));
  EXPECT_EQ(latest("b"), std::optional<rdb::descFldVT>(20));
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(20));

  ASSERT_RDB_OK(model->processRows(due({"src", "a"}), 2));
  EXPECT_EQ(model->getStreamCount("a"), 2U);
  EXPECT_EQ(latest("a"), std::optional<rdb::descFldVT>(20));
  EXPECT_EQ(model->getStreamCount("b"), 1U);
  EXPECT_EQ(model->getStreamCount("w"), 0U);
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(30));

  ASSERT_RDB_OK(model->processRows(due({"a"}), 3));  // zrodlo poza maska: nie przesuwa sie
  EXPECT_EQ(latest("a"), std::optional<rdb::descFldVT>(30));
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(30));
}

// Okno @(1,3) nad zrodlem o jednym polu: pierwszy kompletny rekord okna ma indeks logiczny 2,
// wiec dwa pierwsze nalezne sloty nie emituja niczego - ani zera, ani NULL.
TEST_F(DataModelSlotContract, processRows_emits_nothing_during_origin_plus_tail_slots) {
  ASSERT_RDB_OK(model->processZeroStep());
  ASSERT_RDB_OK(model->processRows(allDue(), 1));
  ASSERT_RDB_OK(model->processRows(allDue(), 2));
  EXPECT_EQ(model->getStreamCount("w"), 0U);
  EXPECT_EQ(model->streamStoredSize("w"), 0U);

  ASSERT_RDB_OK(model->processRows(allDue(), 3));
  ASSERT_EQ(model->getStreamCount("w"), 1U);
  const auto *window = rdbtest::ok(model->getPayload("w"));
  EXPECT_EQ(window->getItemVT(0), std::optional<rdb::descFldVT>(30));
  EXPECT_EQ(window->getItemVT(1), std::optional<rdb::descFldVT>(20));
  EXPECT_EQ(window->getItemVT(2), std::optional<rdb::descFldVT>(10));
}

// Pierwszy blad ewaluacji konczy slot: nastepne wezly nie licza sie, zrodlo sie nie przesuwa,
// a blad wraca w Result.
TEST_F(DataModelSlotContract, processRows_stops_the_slot_at_the_first_error) {
  ASSERT_RDB_OK(model->processZeroStep());
  plan["a"].lSchema.front().lProgram = {token(PUSH_VAL, rdb::descFldVT(std::string("x"))),
                                        token(PUSH_VAL, rdb::descFldVT(std::string("y"))), token(SUBTRACT)};
  EXPECT_RDB_ERROR(model->processRows(allDue(), 1), rdb::Errc::Eval);
  EXPECT_EQ(model->getStreamCount("a"), 0U);
  EXPECT_EQ(model->getStreamCount("b"), 0U);
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(10));
}

TEST_F(DataModelSlotContract, processRows_mask_of_another_length_is_an_invariant_failure) {
  EXPECT_DEATH(static_cast<void>(model->processRows(std::vector<char>(plan.size() + 1, 1))), "due mask has");
}

// Odczyty po trzech slotach: a = 10, 20, 30, zrodlo stoi na 40.
TEST_F(DataModelSlotContract, reads_by_offset_and_forward_index_fall_back_to_null) {
  ASSERT_RDB_OK(model->processZeroStep());
  for (int slot = 1; slot <= 3; ++slot)
    ASSERT_RDB_OK(model->processRows(due({"src", "a"}), slot));

  EXPECT_EQ(rdbtest::ok(model->fetchForward("a", 1))->getItemVT(0), std::optional<rdb::descFldVT>(20));
  EXPECT_FALSE(rdbtest::ok(model->fetchForward("a", 3))->getItemVT(0).has_value());
  EXPECT_FALSE(rdbtest::ok(model->fetchForward("a", -1))->getItemVT(0).has_value());
  EXPECT_EQ(rdbtest::ok(model->fetchForward("src", 3))->getItemVT(0), std::optional<rdb::descFldVT>(40));
  EXPECT_FALSE(rdbtest::ok(model->fetchForward("src", 4))->getItemVT(0).has_value());

  EXPECT_EQ(rdbtest::ok(model->getPayload("a", 1))->getItemVT(0), std::optional<rdb::descFldVT>(20));
  EXPECT_FALSE(rdbtest::ok(model->getPayload("a", 3))->getItemVT(0).has_value());
  EXPECT_EQ(rdbtest::ok(model->getPayload("src", 2))->getItemVT(0), std::optional<rdb::descFldVT>(40));

  const rdb::descFldVT fallback = nullFallbackValue(rdb::INTEGER);
  EXPECT_EQ(rdbtest::ok(model->getRow("a", 2)), std::vector<rdb::descFldVT>{10});
  EXPECT_EQ(rdbtest::ok(model->getRow("a", 3)), std::vector<rdb::descFldVT>{fallback});
  EXPECT_EQ(rdbtest::ok(model->getRow("src", 2)), std::vector<rdb::descFldVT>{40});

  auto &a = *model->qSet.at("a")->outputPayload;
  a.getPayload()->setItemVT(0, std::nullopt);
  ASSERT_EQ(rdbtest::ok(a.write()), rdb::WriteStatus::Ok);
  EXPECT_EQ(rdbtest::ok(model->getRow("a", 0)), std::vector<rdb::descFldVT>{fallback});
  EXPECT_FALSE(rdbtest::ok(model->getPayload("a", 0))->getItemVT(0).has_value());

  EXPECT_EQ(model->getStreamCount("a"), 4U);
  EXPECT_EQ(model->streamStoredSize("a"), 4U * a.descriptor.getSizeInBytes());
}

// Strumien dolaczony w slocie T = 4 przy ogonie 0 dostaje indeks pierwszego rekordu
// n = T/Delta - 1 = 3 i emituje w tym samym slocie rekord zrodla o indeksie 3.
TEST_F(DataModelSlotContract, adhoc_stream_starts_at_the_index_of_its_first_due_slot) {
  ASSERT_RDB_OK(model->processZeroStep());
  for (int slot = 1; slot <= 3; ++slot)
    ASSERT_RDB_OK(model->processRows(due({"src", "a"}), slot));

  query late = plan.getQuery("a");
  late.id    = "late";
  plan.push_back(late);
  ASSERT_EQ(model->addQueriesToModel({"late"}), "");
  EXPECT_FALSE(model->qSet.at("late")->logicalIndexBase.has_value());

  ASSERT_RDB_OK(model->processRows(due({"src", "a", "late"}), 4));
  ASSERT_EQ(model->getStreamCount("late"), 1U);
  EXPECT_EQ(model->qSet.at("late")->logicalIndexBase, std::optional<int>(3));
  EXPECT_EQ(latest("late"), std::optional<rdb::descFldVT>(40));
  EXPECT_EQ(latest("a"), std::optional<rdb::descFldVT>(40));
}

TEST_F(DataModelSlotContract, syncDeclaredCapacities_only_grows_history) {
  auto &source            = *model->qSet.at("src")->outputPayload;
  const auto starting     = source.historyCapacity();
  plan.maxCapacity["src"] = static_cast<int>(starting) + 4;
  model->syncDeclaredCapacities();
  EXPECT_EQ(source.historyCapacity(), starting + 4);
  plan.maxCapacity["src"] = 1;
  model->syncDeclaredCapacities();
  EXPECT_EQ(source.historyCapacity(), starting + 4);
}

// Zrodlo ONESHOT z piecioma rekordami: krok zerowy czyta 10, slot k przesuwa na rekord k+1.
// Slot 5 siega za koniec pliku.
TEST_F(DataModelSlotContract, exhaustedInputStream_names_the_source_after_its_last_record) {
  ASSERT_RDB_OK(model->processZeroStep());
  for (int slot = 1; slot <= 4; ++slot) {
    ASSERT_RDB_OK(model->processRows(due({"src"}), slot));
    EXPECT_EQ(model->exhaustedInputStream(), "") << "slot " << slot;
  }
  EXPECT_EQ(sourceValue(), std::optional<rdb::descFldVT>(50));
  ASSERT_RDB_OK(model->processRows(due({"src"}), 5));
  EXPECT_EQ(model->exhaustedInputStream(), "src");
}

// Restore fixture: the null-test fixtures above delete and recreate str1/str2,
// leaving them with fewer records than pattern.txt expects.  This fixture runs
// last and writes back the canonical 3-record state that ut-dataModel-compare
// compares against.
class xschema_compare_restore : public ::testing::Test {
 protected:
  xschema_compare_restore() {
    dataArea_null.reset();
    for (const auto *f : {"str1", "str1.meta", "str1.desc", "str2", "str2.desc"})
      if (std::filesystem::exists(f)) std::filesystem::remove(f);
    coreInstance.clear();
    parserRQLFile_4Test(coreInstance, "ut_example_schema.rql");
    dataArea = rdbtest::ok(dataModel::create(coreInstance));

    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 11);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 12);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 13);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 14);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(0, 15);
    dataArea->qSet["str1"]->outputPayload->getPayload()->setItem(1, 16);
    rdbtest::ok(dataArea->qSet["str1"]->outputPayload->write());

    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 111);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());
    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 222);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());
    dataArea->qSet["str2"]->outputPayload->getPayload()->setItem(0, 333);
    rdbtest::ok(dataArea->qSet["str2"]->outputPayload->write());

    pProc = dataArea.get();
  }
  ~xschema_compare_restore() override { pProc = nullptr; }
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(xschema_compare_restore, state_restored_for_compare_test) { SUCCEED(); }

}  // namespace
