#include <gtest/gtest.h>

#include <boost/rational.hpp>
#include <set>
#include <stdexcept>
#include <string>

#include "retractor/lib/qTree.hpp"
#include "retractor/lib/token.hpp"

// ctest -R '^ut_qTree' -V

using rational = boost::rational<int>;

static query makeQuery(const std::string &id, int num = 1, int den = 1) { return query(rational(num, den), id); }

// ============================================================
// exists()
// ============================================================

TEST(qTree, exists_returns_true_when_query_present) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  EXPECT_TRUE(qt.exists("alpha"));
}

TEST(qTree, exists_returns_false_when_query_absent) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  EXPECT_FALSE(qt.exists("beta"));
}

TEST(qTree, exists_returns_false_on_empty_tree) {
  qTree qt;
  EXPECT_FALSE(qt.exists("anything"));
}

// ============================================================
// getQuery()
// ============================================================

TEST(qTree, getQuery_returns_correct_query) {
  qTree qt;
  qt.push_back(makeQuery("s1"));
  qt.push_back(makeQuery("s2"));
  EXPECT_EQ(qt.getQuery("s1").id, "s1");
  EXPECT_EQ(qt.getQuery("s2").id, "s2");
}

TEST(qTree, getQuery_throws_logic_error_when_not_found) {
  qTree qt;
  qt.push_back(makeQuery("s1"));
  EXPECT_THROW(qt.getQuery("missing"), std::logic_error);
}

TEST(qTree, getQuery_fatals_on_empty_name) {
  qTree qt;
  EXPECT_DEATH({ qt.getQuery(""); }, "query name is empty");
}

// ============================================================
// getQuery() - blizniacze nazwy
// ============================================================

// Dwa wezly o TEJ SAMEJ nazwie sa stanem DOZWOLONYM planu, nie defektem:
// validateSubstratNameUniqueness pyta o rownosc programu, nie o unikalnosc nazwy, a przy
// RDB_OPT_DEDUP_SUBSTRATES=OFF oba dochodza do wykonania. Kompilator stoi na tym, ze
// wyszukanie po nazwie wskazuje PIERWSZE wystapienie - i to jest jedyne miejsce, ktore
// tej semantyki pilnuje. Kazda proba zastapienia skanu czyms szybszym (indeks, zapamietana
// pozycja) musi ja odtworzyc, inaczej wyszukanie zwraca inny wezel niz kompilator zalozyl.

TEST(qTree, getQuery_returns_first_of_twin_names) {
  qTree qt;
  qt.push_back(makeQuery("x", 1, 2));  // pierwszy
  qt.push_back(makeQuery("x", 1, 3));  // blizniak o rownej nazwie

  // Zdanie "pierwsze wystapienie" jest twierdzeniem, a nie tautologia, dopiero gdy oba
  // wezly sa rozroznialne. Bez tych dwoch asercji test przeszedlby takze wtedy, gdyby
  // wyszukanie zwracalo drugi wezel.
  ASSERT_NE(&qt[0], &qt[1]);
  ASSERT_NE(qt[0].rInterval, qt[1].rInterval);

  EXPECT_EQ(&qt.getQuery("x"), &qt[0]);
  EXPECT_EQ(qt.getQuery("x").rInterval, rational(1, 2));

  // operator[](nazwa) i getDelta() przechodza przez getQuery - ta sama semantyka.
  EXPECT_EQ(&qt["x"], &qt[0]);
  EXPECT_EQ(qt.getDelta("x"), rational(1, 2));
}

// Kontrprzyklad z #272. Podpowiedz pozycyjna uzbrojona na ukladzie [a, x, x'] potwierdza sie
// na sprawdzeniu "czy [1].id == x", a mimo to po skasowaniu 'a' pozycja 1 trzyma juz blizniaka.
// Uklad powstaje w factorMatchedHashTimeMoves (compiler.cpp:2185 push_back, :2211 erase), a
// compile() biegnie na ZYWYM drzewie przy imporcie ad-hoc (executorsmAdHoc.cpp:234), wiec nie
// jest hipotetyczny.
TEST(qTree, getQuery_keeps_first_occurrence_after_plan_shift) {
  qTree qt;
  qt.push_back(makeQuery("a", 1, 1));
  qt.push_back(makeQuery("x", 1, 2));  // pierwszy
  qt.push_back(makeQuery("x", 1, 3));  // blizniak

  static_cast<void>(qt.getQuery("x"));  // uzbrojenie ewentualnej podpowiedzi
  qt.erase(qt.begin());                 // [x, x'] - pozycje przesuniete o jeden

  ASSERT_EQ(qt.size(), 2u);
  ASSERT_NE(qt[0].rInterval, qt[1].rInterval);

  EXPECT_EQ(&qt.getQuery("x"), &qt[0]);
  EXPECT_EQ(qt.getQuery("x").rInterval, rational(1, 2));
}

// exists() nie rozstrzyga, ktory wezel - ale ma odpowiadac TAK na nazwe blizniacza.
TEST(qTree, exists_true_for_twin_names) {
  qTree qt;
  qt.push_back(makeQuery("x", 1, 2));
  qt.push_back(makeQuery("x", 1, 3));
  EXPECT_TRUE(qt.exists("x"));
}

// ============================================================
// operator[]
// ============================================================

TEST(qTree, bracket_operator_returns_correct_query) {
  qTree qt;
  qt.push_back(makeQuery("x"));
  EXPECT_EQ(qt["x"].id, "x");
}

// Obie postacie indeksu MUSZA byc osiagalne naraz. Bez `using std::vector<query>::operator[]`
// w qTree.hpp wersja pozycyjna jest ukryta przez wersje po nazwie i ten test nie kompiluje sie -
// to jest cala jego tresc, wiec nie wolno go usunac jako "oczywistego".
TEST(qTree, bracket_operator_takes_position_and_name) {
  qTree qt;
  qt.push_back(makeQuery("pierwszy"));
  qt.push_back(makeQuery("drugi"));

  const std::size_t second = 1;
  EXPECT_EQ(qt[0].id, "pierwszy");     // literal calkowity -> pozycja
  EXPECT_EQ(qt[second].id, "drugi");   // zmienna calkowita -> pozycja
  EXPECT_EQ(qt["drugi"].id, "drugi");  // literal napisowy  -> nazwa
  EXPECT_EQ(qt[std::string("pierwszy")].id, "pierwszy");
}

// ============================================================
// getDelta()
// ============================================================

TEST(qTree, getDelta_returns_rInterval) {
  qTree qt;
  qt.push_back(makeQuery("q", 3, 4));
  EXPECT_EQ(qt.getDelta("q"), rational(3, 4));
}

// ============================================================
// sort()
// ============================================================

TEST(qTree, sort_orders_by_rInterval_ascending) {
  qTree qt;
  qt.push_back(makeQuery("slow", 3, 1));
  qt.push_back(makeQuery("fast", 1, 2));
  qt.push_back(makeQuery("mid", 1, 1));

  qt.sort();

  EXPECT_EQ(qt.at(0).id, "fast");
  EXPECT_EQ(qt.at(1).id, "mid");
  EXPECT_EQ(qt.at(2).id, "slow");
}

// ============================================================
// topologicalSort()
// ============================================================

TEST(qTree, topologicalSort_single_node_unchanged) {
  qTree qt;
  qt.push_back(makeQuery("only"));
  qt.topologicalSort();
  ASSERT_EQ(qt.size(), 1u);
  EXPECT_EQ(qt.at(0).id, "only");
}

TEST(qTree, topologicalSort_places_dependency_before_dependent) {
  // A zależy od B - po sortowaniu B musi być przed A
  qTree qt;

  query a = makeQuery("A");
  a.lProgram.push_back(token(PUSH_STREAM, std::string("B")));
  qt.push_back(a);

  qt.push_back(makeQuery("B"));

  qt.topologicalSort();

  ASSERT_EQ(qt.size(), 2u);
  EXPECT_EQ(qt.at(0).id, "B");
  EXPECT_EQ(qt.at(1).id, "A");
}

TEST(qTree, topologicalSort_chain_three_nodes) {
  // C zależy od B, B zależy od A. Oryginalna kolejność: C B A. Oczekiwana: A B C.
  qTree qt;

  query c = makeQuery("C");
  c.lProgram.push_back(token(PUSH_STREAM, std::string("B")));
  qt.push_back(c);

  query b = makeQuery("B");
  b.lProgram.push_back(token(PUSH_STREAM, std::string("A")));
  qt.push_back(b);

  qt.push_back(makeQuery("A"));

  qt.topologicalSort();

  ASSERT_EQ(qt.size(), 3u);
  EXPECT_EQ(qt.at(0).id, "A");
  EXPECT_EQ(qt.at(1).id, "B");
  EXPECT_EQ(qt.at(2).id, "C");
}

// ============================================================
// dumpCore()
// ============================================================

TEST(qTree, dumpCore_does_not_crash) {
  qTree qt;
  qt.push_back(makeQuery("s1", 1, 2));
  qt.push_back(makeQuery("s2", 1, 1));
  qt.maxCapacity["s1"] = 10;
  qt.maxCapacity["s2"] = 5;

  // dumpCore() pisze tabele na stdout - to jego zadanie, nie hałas do logu testu.
  testing::internal::CaptureStdout();
  qt.dumpCore();
  testing::internal::GetCapturedStdout();
}

// ============================================================
// getAvailableTimeIntervals()
// ============================================================

TEST(qTree, getAvailableTimeIntervals_returns_unique_intervals) {
  qTree qt;
  qt.push_back(makeQuery("s1", 1, 2));
  qt.push_back(makeQuery("s2", 1, 1));
  qt.push_back(makeQuery("s3", 1, 2));  // duplikat - zbiór nie powtarza

  auto intervals = qt.getAvailableTimeIntervals();

  EXPECT_EQ(intervals.size(), 2u);
  EXPECT_TRUE(intervals.count(rational(1, 2)));
  EXPECT_TRUE(intervals.count(rational(1, 1)));
}

TEST(qTree, getAvailableTimeIntervals_skips_compiler_directives) {
  qTree qt;
  qt.push_back(makeQuery("s1", 1, 1));

  query directive = makeQuery(":STORAGE", 1, 1);
  qt.push_back(directive);

  auto intervals = qt.getAvailableTimeIntervals();
  EXPECT_EQ(intervals.size(), 1u);
}

TEST(qTree, getAvailableTimeIntervals_fatals_when_rInterval_is_zero) {
  qTree qt;
  qt.push_back(makeQuery("bad", 0, 1));
  EXPECT_DEATH({ qt.getAvailableTimeIntervals(); }, "rInterval is zero");
}

// ============================================================
// planRevision()
// ============================================================
//
// Rewizja opisuje KSZTAŁT planu: długość, kolejność węzłów i nazwę na każdej pozycji. Trzyma
// ją ten, kto buduje strukturę równoległą do planu (dataModel::handles_) i musi wiedzieć,
// kiedy ją przebudować. Testy poniżej pilnują trzech rzeczy: że numer bierze KAŻDA operacja
// zmieniająca kształt, że NIE bierze go zmiana treści węzła, i że dwie drogi kopiowania
// całego drzewa nie potrafią podstawić numeru, który obserwator uzna za swój.

TEST(qTree, planRevision_changes_on_push_back_copy) {
  qTree qt;
  const auto before = qt.planRevision();
  const query node  = makeQuery("alpha");
  qt.push_back(node);
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_push_back_move) {
  qTree qt;
  const auto before = qt.planRevision();
  qt.push_back(makeQuery("alpha"));
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_pop_back) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  const auto before = qt.planRevision();
  qt.pop_back();
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_erase_single) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  qt.push_back(makeQuery("beta"));
  const auto before = qt.planRevision();
  qt.erase(qt.begin());
  EXPECT_NE(qt.planRevision(), before);
  EXPECT_EQ(qt.at(0).id, "beta");
}

TEST(qTree, planRevision_changes_on_erase_range) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  qt.push_back(makeQuery("beta"));
  const auto before = qt.planRevision();
  qt.erase(qt.begin(), qt.end());
  EXPECT_NE(qt.planRevision(), before);
  EXPECT_TRUE(qt.empty());
}

TEST(qTree, planRevision_changes_on_clear) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  const auto before = qt.planRevision();
  qt.clear();
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_replaceAll) {
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  const auto before = qt.planRevision();
  std::vector<query> nodes{makeQuery("beta")};
  qt.replaceAll(std::move(nodes));
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_sort_even_when_order_already_correct) {
  // sort() jest operacją na kształcie z definicji, a nie z wyniku: kolejność po niej jest
  // nowym stanem także wtedy, gdy wyszła taka sama. Numer bierze zawsze - inaczej trzeba by
  // porównywać plan przed i po, czyli robić dokładnie tę pracę, której numer ma oszczędzić.
  qTree qt;
  qt.push_back(makeQuery("fast", 1, 2));
  qt.sort();
  const auto before = qt.planRevision();
  qt.sort();
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_changes_on_topologicalSort) {
  qTree qt;
  query a = makeQuery("A");
  a.lProgram.push_back(token(PUSH_STREAM, std::string("B")));
  qt.push_back(a);
  qt.push_back(makeQuery("B"));

  const auto before = qt.planRevision();
  qt.topologicalSort();
  EXPECT_NE(qt.planRevision(), before);
}

TEST(qTree, planRevision_unchanged_by_node_content) {
  // Granica niezmiennika: at() i operator[] wydają query&, a zmiana pola w miejscu kształtu
  // nie rusza. Dla isOneShot tak ma być - tak właśnie robi executorsm::run w trybie --until-eof.
  qTree qt;
  qt.push_back(makeQuery("alpha"));
  const auto before = qt.planRevision();

  qt.at(0).isOneShot      = true;
  qt["alpha"].rInterval   = rational(1, 4);
  qt.maxCapacity["alpha"] = 7;

  EXPECT_EQ(qt.planRevision(), before);
}

TEST(qTree, planRevision_differs_between_freshly_built_trees) {
  // Numery wydaje jeden dozownik na proces, więc puste drzewo powstałe przy przeładowaniu
  // planu nie może trafić w numer, który ktoś zapamiętał dla innego drzewa.
  qTree first;
  qTree second;
  EXPECT_NE(first.planRevision(), second.planRevision());
}

TEST(qTree, planRevision_copy_carries_source_number_until_first_mutation) {
  // Kopia planu w kanale ad-hoc (executorsm::getAdHoc) niesie numer źródła, bo w tej chwili
  // ma dokładnie jego kształt. Pierwsza mutacja kopii bierze własny numer.
  qTree live;
  live.push_back(makeQuery("alpha"));

  qTree copy = live;
  EXPECT_EQ(copy.planRevision(), live.planRevision());

  copy.push_back(makeQuery("beta"));
  EXPECT_NE(copy.planRevision(), live.planRevision());
  EXPECT_EQ(live.size(), 1u);
}

TEST(qTree, planRevision_assignment_cannot_restore_an_observed_number) {
  // Przeładowanie planu: `*coreInstancePtr = qTree{}`. Obserwator pamięta numer sprzed
  // przypisania, a po nim MUSI zobaczyć inny - inaczej jego struktura równoległa do planu
  // opisywałaby plan, którego już nie ma.
  qTree live;
  live.push_back(makeQuery("alpha"));
  const auto observed = live.planRevision();

  live = qTree{};

  EXPECT_NE(live.planRevision(), observed);
  EXPECT_TRUE(live.empty());
}

TEST(qTree, planRevision_never_matches_the_value_reserved_for_no_observation) {
  // Zero znaczy "jeszcze nic nie obserwowane" u tego, kto numer zapamiętuje, więc żadne
  // żywe drzewo nie ma prawa go nosić.
  qTree qt;
  EXPECT_NE(qt.planRevision(), 0u);
  qt.push_back(makeQuery("alpha"));
  EXPECT_NE(qt.planRevision(), 0u);
}
