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
// operator[]
// ============================================================

TEST(qTree, bracket_operator_returns_correct_query) {
  qTree qt;
  qt.push_back(makeQuery("x"));
  EXPECT_EQ(qt["x"].id, "x");
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
  // A zależy od B — po sortowaniu B musi być przed A
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
  qt.dumpCore();
}

// ============================================================
// getAvailableTimeIntervals()
// ============================================================

TEST(qTree, getAvailableTimeIntervals_returns_unique_intervals) {
  qTree qt;
  qt.push_back(makeQuery("s1", 1, 2));
  qt.push_back(makeQuery("s2", 1, 1));
  qt.push_back(makeQuery("s3", 1, 2));  // duplikat — zbiór nie powtarza

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
