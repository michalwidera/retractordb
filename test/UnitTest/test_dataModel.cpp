#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "rdb/fainterface.hpp"
#include "rdb/payload.hpp"
#include "rdb/probe.hpp"  // sonda E4 (liczy tylko w buildzie RDB_BENCH_PROBE)
#include "rdb/storage.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
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
