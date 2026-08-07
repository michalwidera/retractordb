#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <variant>

#include "config.h"
#include "rdb/fainterface.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/qTree.hpp"  // coreInstance

// ctest -R "^ut_crsMath$" -V

// https://github.com/google/googletest/blob/main/docs/index.md

using namespace CRationalStreamMath;

extern std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
extern dataModel *pProc;

qTree coreInstance;

namespace {

TEST(TimeLineUnitTest, GeneratesNearestNextTermsForSimpleDeltas) {
  set<boost::rational<int>> deltas = {
      boost::rational<int>(1, 2),
      boost::rational<int>(3, 4),
  };

  TimeLine tl(deltas);

  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 2));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(3, 4));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 1));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(3, 2));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(2, 1));
}

TEST(TimeLineUnitTest, FiltersOutNaturalMultiplesFromInputSet) {
  set<boost::rational<int>> deltas = {
      boost::rational<int>(1, 1),
      boost::rational<int>(4, 1),
      boost::rational<int>(1, 2),
      boost::rational<int>(3, 4),
  };

  TimeLine tl(deltas);

  // Expected behavior from constructor comments: {1, 4, 1/2, 3/4} -> {1/2, 3/4}.
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 2));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(3, 4));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 1));
}

TEST(TimeLineUnitTest, AwaitCheckMatchesCurrentTimeSlotAndRejectsZeroDelta) {
  set<boost::rational<int>> deltas = {
      boost::rational<int>(1, 2),
      boost::rational<int>(3, 4),
      boost::rational<int>(1, 1),
  };

  TimeLine tl(deltas);

  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 2)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(0, 1)));

  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 2));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 2)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(3, 4)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 1)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(0, 1)));

  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(3, 4));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(3, 4)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 1)));

  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 1));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 2)));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 1)));
}

TEST(TimeLineUnitTest, UpdatingIntervalsPreservesPositionAndSchedulesNewFasterRate) {
  TimeLine tl({boost::rational<int>(1, 10), boost::rational<int>(1, 5)});

  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 10));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 5));

  tl.updateTimeIntervals({boost::rational<int>(1, 20), boost::rational<int>(1, 10), boost::rational<int>(1, 5)});

  // The timeline must neither restart at 1/20 nor skip to the old next slot 3/10.
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(1, 4));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 20)));
  EXPECT_FALSE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 10)));
  EXPECT_EQ(tl.getNextTimeSlot(), boost::rational<int>(3, 10));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 20)));
  EXPECT_TRUE(tl.isThisDeltaAwaitCurrentTimeSlot(boost::rational<int>(1, 10)));
}

const int TEST_COUNT = 15;

// std::unique_ptr<dataModel> dataArea;

struct crsMathTestInit {
  crsMathTestInit() {
    assert(std::filesystem::exists("ut_crsmath.rql") && "file ut_crsmath.rql does not exist!");
    std::ifstream infl("ut_crsmath.rql");
    for (std::string line; std::getline(infl, line);)
      std::cout << line << '\n';
  }

  ~crsMathTestInit() = default;

} crsMathTestInstance_;

class crsMathTest : public ::testing::Test {
 protected:
  crsMathTest() = default;

  ~crsMathTest() override = default;

  void SetUp() override {
    auto compiled = parserRQLFile_4Test(coreInstance, "ut_crsmath.rql") == "OK";
    assert(compiled && "Query set malformed according to grammar.");

    compiler cm(coreInstance);
    std::string response = cm.compile();
    EXPECT_TRUE(response == "OK");
  }

  void TearDown() override {
    coreInstance.clear();

    // This magic is clearing all files that have .desc and are .desc - so called artifacts
    auto result = std::system("find ./*.desc | sed 's/\\.[^.]*$//' | cut -c 3- | xargs rm -f ; rm -f *.desc");
    if (result != 0) {
      SPDLOG_ERROR("Error during cleanup - {}", result);
      assert(false && "Error during cleanup");
    }
  }
};

// TEST_F(crsMathTest, Only_nine_items_in_query) { EXPECT_TRUE(coreInstance.size() == 9); }

const std::vector<std::string> allStreams = {"cx",  "s1x", "s2x", "s3x", "s4x", "s5x", "s6x",
                                             "s7x", "s8x", "cy",  "s1y", "s2y", "s9x"};

TEST_F(crsMathTest, check_if_streams_sequence_are_correct) {
  const auto colSize = 4;
  const auto *const expectedResult =
      "Dlt: { 1/1}{ 1/3}{ 2/3}{ 1/3}{ 1/1}{ 1/1}{ 1/1}{ 2/3}{ 2/3}{ 1/3}{ 1/3}{ 2/3}{ 2/3}\n"
      " 000:{  cx}{    }{    }{    }{    }{    }{    }{    }{    }{  cy}{    }{    }{    }\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {  cx}{ s1x}{    }{ s3x}{ s4x}{ s5x}{ s6x}{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {  cx}{ s1x}{ s2x}{ s3x}{ s4x}{ s5x}{ s6x}{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {  cx}{ s1x}{    }{ s3x}{ s4x}{ s5x}{ s6x}{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {  cx}{ s1x}{ s2x}{ s3x}{ s4x}{ s5x}{ s6x}{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n"
      " 333 {    }{ s1x}{    }{ s3x}{    }{    }{    }{    }{    }{  cy}{ s1y}{    }{    }\n"
      " 333 {    }{ s1x}{ s2x}{ s3x}{    }{    }{    }{ s7x}{ s8x}{  cy}{ s1y}{ s2y}{ s9x}\n";

  std::stringstream strstream;

  dataModel proc(coreInstance);
  pProc = &proc;  // This need to be set for dumpManager

  TimeLine tl(coreInstance.getAvailableTimeIntervals());
  boost::rational<int> prev_interval(0);

  strstream << std::setw(colSize) << "Dlt: ";

  // Delta presentation

  for (const auto &x : allStreams)
    strstream << "{" << std::setw(colSize) << coreInstance.getDelta(x) << "}";
  strstream << '\n';

  // Init row - process all declaration

  std::set<std::string> initSet;
  for (const auto &it : coreInstance)
    if (it.isDeclaration()) initSet.insert(it.id);

  proc.processZeroStep();

  strstream << std::setw(colSize) << " 000:";
  for (const auto &x : allStreams)
    strstream << "{" << std::setw(colSize) << (initSet.contains(x) ? x : "") << "}";

  strstream << '\n';

  // Process declarations and queries in time slots

  auto queryCounter{TEST_COUNT};
  while (queryCounter-- != 1) {
    const int msInSec = 1000;
    boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
    int period(rational_cast<int>(interval - prev_interval));  // miliseconds
    prev_interval = interval;

    strstream << std::setw(colSize) << period << " ";

    std::set<std::string> procSet;
    for (const auto &it : coreInstance)
      if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) procSet.insert(it.id);

    for (const auto &x : allStreams)
      strstream << "{" << std::setw(colSize) << (procSet.contains(x) ? x : "") << "}";

    strstream << '\n';

    proc.processRows(procSet);
  }

  if (strstream.str() != expectedResult) {
    std::cerr << "Actual:\n";
    std::cerr << strstream.str().c_str() << '\n';
    std::cerr << "Expected:\n";
    std::cerr << expectedResult << '\n';
  }
  EXPECT_TRUE(strstream.str() == expectedResult);
}

std::string print(const std::string &query_name, dataModel &proc) {
  std::stringstream coutstring;
  auto cnt = proc.getPayload(query_name)->descriptor.flatElementCount();
  for (auto value : proc.getRow(query_name, 0)) {
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
        value);
    if (--cnt) coutstring << ",";
  }  // endfor
  return coutstring.str();
}

// K24/H10a: wzorzec przesunięty razem z postacią zamkniętą ogona `@`. Wszystkie
// Strumienie planu są oknami nad deklaracją cx (F=3, Delta=1, wartości 1..9 cyklicznie),
// stemplowanymi KOŃCEM przedziału: rekord n obejmuje pozycje spłaszczone
// n*step-(|len|-1) ... n*step, a pozycja p niesie wartość (p mod 9)+1.
//
// Stąd każda komórka tabeli daje się sprawdzić rachunkiem, np.:
//   s2x = cx@(2,2), origin ceil(1/2)=1 -> rekord 1 to pozycje 1..2 = (3,2) [najnowsze pierwsze],
//   s8x = cx@(2,4), origin ceil(3/2)=2 -> rekord 2 to pozycje 1..4 = (5,4,3,2),
//   s6x = cx@(3,-3), origin 1        -> rekord 1 to pozycje 1..3 w kolejności napływu = (2,3,4).
//
// Wobec stemplowania początkiem przedziału zmieniają się DWIE rzeczy: dla step=1 (s1x, s3x)
// treść zostaje ta sama, a okno przesuwa się o slot później — to właśnie usunięta precesja;
// dla step>1 (s2x, s4x..s8x) zmienia się także zbiór próbek, bo na siatce kroku leży teraz
// koniec okna, a nie jego początek.
TEST_F(crsMathTest, check_if_streams_values_are_correct) {
  const auto colSize = 9;
  // Kolumna s9x = (s1x>1)@(2,2) przesunieta o jeden wlasny slot (dwa wiersze siatki 1/3)
  // w gore wobec stanu sprzed 2026-08-07. Wyprowadzenie, nie przepisanie z wyjscia silnika:
  // s1x = cx@(1,1) ma ogon 2 i origin 0; (s1x>1) ma origin 1 i ogon max(0, 2-1) = 1;
  // s9x ma origin ceil((1*1+1)/2) = 1 i ogon ceil((1+1)*1/2)-1 = 0. Rekord 1 obejmuje
  // pozycje 1..2, czyli s1x[0] i s1x[1]; s1x[1] jest dostepny w chwili (1+1+2)/3 = 4/3,
  // a slot 1 konczy sie w (1+1+0)*2/3 = 4/3 — dokladnie na czas, bez slotu zapasu.
  // Poprzednio (s1x>1) mialo ogon 2, wymuszony adresowaniem wzglednym w fetchBack.
  const auto *const expectedResult =
      // clang-format off
      " Dlt:|      1/1|      1/3|      2/3|      1/3|      1/1|      1/1|      1/1|      2/3|      2/3|      1/3|      1/3|      2/3|      2/3|\n"
      "Name:|       cx|      s1x|      s2x|      s3x|      s4x|      s5x|      s6x|      s7x|      s8x|       cy|      s1y|      s2y|      s9x|\n"
      " 000 |    1,2,3|         |         |         |         |         |         |         |         |        1|         |         |         |\n"
      " 333 |         |         |         |         |         |         |         |         |         |        2|        1|         |         |\n"
      " 333 |         |         |         |         |         |         |         |         |         |        3|        2|         |         |\n"
      " 333 |    4,5,6|        1|         |         |         |         |         |         |         |        4|        3|         |         |\n"
      " 333 |         |        2|         |      2,1|         |         |         |         |         |        5|        4|      2,1|      2,1|\n"
      " 333 |         |        3|         |      3,2|         |         |         |         |         |        6|        5|         |         |\n"
      " 333 |    7,8,9|        4|      3,2|      4,3|      4,3|    4,3,2|    2,3,4|    3,2,1|         |        7|        6|      4,3|      4,3|\n"
      " 333 |         |        5|         |      5,4|         |         |         |         |         |        8|        7|         |         |\n"
      " 333 |         |        6|      5,4|      6,5|         |         |         |    5,4,3|  5,4,3,2|        9|        8|      6,5|      6,5|\n"
      " 333 |    1,2,3|        7|         |      7,6|      7,6|    7,6,5|    5,6,7|         |         |        1|        9|         |         |\n"
      " 333 |         |        8|      7,6|      8,7|         |         |         |    7,6,5|  7,6,5,4|        2|        1|      8,7|      8,7|\n"
      " 333 |         |        9|         |      9,8|         |         |         |         |         |        3|        2|         |         |\n"
      " 333 |    4,5,6|        1|      9,8|      1,9|      1,9|    1,9,8|    8,9,1|    9,8,7|  9,8,7,6|        4|        3|      1,9|      1,9|\n"
      " 333 |         |        2|         |      2,1|         |         |         |         |         |        5|        4|         |         |\n"
      " 333 |         |        3|      2,1|      3,2|         |         |         |    2,1,9|  2,1,9,8|        6|        5|      3,2|      3,2|\n";
  // clang-format on

  // Q: W wyniku zmian w bieżącym branch zmieniona została wartość expectdResults.
  // Nie rozumiem dlaczego w linii 275 kolumnie 2/3 nie może wystąpić wiersz 1,2. Wyjaśnij mi to.

  // A: Która to kolumna Delta 2/3 w trzeciej kolumnie to s2x = cx@(2,2) → step=2, length=2.
  // Źródło cx: F=3 pola, Delta=1 s, pozycja spłaszczona p niesie wartość (p mod 9)+1.

  // Dlaczego „1,2" tam nie może wystąpić — dwie niezależne przyczyny
  //
  // 1. Treść: okno o takiej zawartości nie jest rekordem s2x.
  // Po przestemplowaniu (ta gałąź) rekord n obejmuje pozycje n*step-(len-1) … n*step,
  // czyli jego koniec leży na siatce kroku — streamInstance.cpp:118.
  // Dla step=2 legalne okna kończą się wyłącznie na pozycjach parzystych:
  // n	pozycje	wartości	wydruk
  // 1	1–2	2,3	3,2
  // 2	3–4	4,5	5,4
  // 3	5–6	6,7	7,6
  // 4	7–8	8,9	9,8
  // 5	9–10	1,2	2,1
  // Okno z wartościami 1 i 2 to pozycje 0–1, koniec na pozycji 1 — nieparzystej.
  // Takiego rekordu w s2x po prostu nie ma.
  // Na masterze był (windowStart = n*step → n=0 → pozycje 0,1 → 2,1 dokładnie w linii 275)
  // i to jest ta usunięta precesja: okno stemplowane początkiem przedziału wyprzedzało własny indeks logiczny.
  //
  // 2. Czas: w tym slocie s2x jeszcze niczego nie wydaje.
  // origin = ceil((0*3+2-1)/2) = 1 (SOperations.hpp:123), ogon W = ceil(3/2)-1 = 1 (SOperations.hpp:114).
  // Slot k wydaje rekord k-W; linia 275 to t=4/3, czyli slot 1 → rekord 0 → poniżej origin, więc nie ma definicji.
  // Zasada brzegu: brakujący rekord jest nieobecny, nie NULL-owany.
  // Pierwsza emisja wypada w slocie 2 (t=2, linia 277) rekordem 1 = 3,2.
  // Suma milczenia origin+ogon = 2 jest ta sama co ogon przed zmianą — przesunął się tylko adres w czasie.
  //
  // Kontrast w tym samym wierszu
  // s3x = cx@(1,2) ma step=1, więc każda pozycja leży na siatce —
  // jego okno kończące się na pozycji 1 istnieje i to jest właśnie 2,1 w linii 275.
  // Różnica między kolumnami to wyłącznie krok, nie długość okna.
  // Uzupełniająco: wartości 1 i 2 wracają w s2x w ostatnim wierszu (linia 285, 2,1), ale jako pozycje 9–10 następnego cyklu,
  // a w kolumnie 2/3 obok — s7x (len=3) ma pozycje 0–2 jako 3,2,1 w linii 277.
  // Wartość z pozycji 0 nie trafia natomiast do żadnego okna s2x
  // — to bezpośrednia konsekwencja przypięcia końca okna do parzystej siatki
  // i nieparzystego początku okna (step=2, len=2) w tym samym cyklu.

  std::stringstream strstream;

  dataModel proc(coreInstance);
  pProc = &proc;  // This need to be set for dumpManager

  TimeLine tl(coreInstance.getAvailableTimeIntervals());
  boost::rational<int> prev_interval(0);

  // Delta presentation
  strstream << std::setw(4) << " Dlt:";
  for (const auto &x : allStreams)
    strstream << "|" << std::setw(colSize) << coreInstance.getDelta(x);
  strstream << "|" << '\n';

  // Names
  strstream << std::setw(4) << "Name:";
  for (const auto &x : allStreams)
    strstream << "|" << std::setw(colSize) << x;
  strstream << "|" << '\n';

  // Init row - process all declaration

  std::set<std::string> initSet;
  for (const auto &it : coreInstance)
    if (it.isDeclaration()) initSet.insert(it.id);

  proc.processZeroStep();

  strstream << std::setw(4) << " 000 ";
  for (const auto &x : allStreams)
    strstream << "|" << std::setw(colSize) << (initSet.contains(x) ? print(x, proc) : "");

  strstream << "|" << '\n';

  // Process declarations and queries in time slots

  auto queryCounter{TEST_COUNT};
  while (queryCounter-- != 1) {
    const int msInSec = 1000;
    boost::rational<int> interval(tl.getNextTimeSlot() * msInSec /* sec->ms */);
    int period(rational_cast<int>(interval - prev_interval));  // miliseconds
    prev_interval = interval;

    strstream << std::setw(4) << period << " ";

    std::set<std::string> procSet;
    for (const auto &it : coreInstance)
      if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) procSet.insert(it.id);

    std::map<std::string, size_t> recordsBefore;
    for (const auto &x : procSet)
      recordsBefore.emplace(x, proc.qSet.at(x)->outputPayload->getRecordsCount());

    proc.processRows(procSet);

    for (const auto &x : allStreams)
      strstream << "|" << std::setw(colSize)
                << (procSet.contains(x) && proc.qSet.at(x)->outputPayload->getRecordsCount() > recordsBefore.at(x)
                        ? print(x, proc)
                        : "");
    strstream << "|" << '\n';
  }

  if (strstream.str() != expectedResult) {
    std::cerr << "Actual:\n";
    std::cerr << strstream.str().c_str() << '\n';
    std::cerr << "Expected:\n";
    std::cerr << expectedResult << '\n';
  }
  EXPECT_TRUE(strstream.str() == expectedResult);
}

}  // Namespace
