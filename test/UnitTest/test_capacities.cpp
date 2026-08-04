// Tabelaryczna regresja pojemności źródła deklarowanego dla różnicy strumieni.
// Uzupełnia scenariusz integracyjny k24_capacity o ułamkowe fazy i różne szerokości
// rekordu. Oczekiwania pochodzą z niezależnego przeglądu jednego okresu fazowego,
// bez wywoływania SubtractStartupLatency() ani computeRequiredCapacities().

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include <boost/rational.hpp>

#include "retractor/lib/compiler.hpp"
#include "retractor/lib/qTree.hpp"

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

namespace {

using ratio = boost::rational<int>;

int floorOf(const ratio &value) {
  const int numerator   = value.numerator();
  const int denominator = value.denominator();
  const int quotient    = numerator / denominator;
  return numerator < 0 && numerator % denominator != 0 ? quotient - 1 : quotient;
}

int ceilOf(const ratio &value) { return -floorOf(-value); }

std::string ratioText(const ratio &value) {
  return std::to_string(value.numerator()) + "/" + std::to_string(value.denominator());
}

std::string declareSource(int width, const ratio &delta) {
  std::string rql = "DECLARE ";
  for (int field = 0; field < width; ++field)
    rql += (field == 0 ? "" : ", ") + std::string("f") + std::to_string(field) + " INTEGER";
  return rql + " STREAM src, " + ratioText(delta) + " FILE 'source.txt'\n";
}

// W chwili emisji rekordu n konsumenta źródło ma na czole
// floor((n+1+Wout)*ratio) rekordów z modelu czasu. Potrzebny jest rekord
// ceil(n*ratio), a deklaracja ma jeszcze dwa rekordy prefetchu realizacyjnego.
// Reszty n*ratio powtarzają się po mianowniku zredukowanego ilorazu.
int eventCapacity(const ratio &intervalRatio, int outputTail) {
  constexpr int declarationPrefetch = 2;
  int required                      = 1;
  for (int n = 0; n < intervalRatio.denominator(); ++n) {
    const int sourceCount = floorOf(ratio(n + 1 + outputTail) * intervalRatio);
    const int sourceIndex = ceilOf(ratio(n) * intervalRatio);
    required              = std::max(required, sourceCount - sourceIndex + declarationPrefetch);
  }
  return required;
}

struct capacityCase {
  ratio intervalRatio;
  int sourceWidth;
  int expectedTail;
  int expectedCapacity;
};

const std::vector<capacityCase> cases{
    {.intervalRatio = ratio(1), .sourceWidth = 1, .expectedTail = 1, .expectedCapacity = 4},
    {.intervalRatio = ratio(3, 2), .sourceWidth = 2, .expectedTail = 1, .expectedCapacity = 5},
    {.intervalRatio = ratio(2), .sourceWidth = 3, .expectedTail = 1, .expectedCapacity = 6},
    {.intervalRatio = ratio(7, 3), .sourceWidth = 4, .expectedTail = 1, .expectedCapacity = 6},
    {.intervalRatio = ratio(3), .sourceWidth = 1, .expectedTail = 1, .expectedCapacity = 8},
    {.intervalRatio = ratio(4), .sourceWidth = 2, .expectedTail = 1, .expectedCapacity = 10},
    {.intervalRatio = ratio(5), .sourceWidth = 4, .expectedTail = 1, .expectedCapacity = 12},
};

}  // namespace

TEST(capacities, subtract_declared_matches_event_table) {
  const ratio sourceDelta(1, 100);

  for (const auto &testCase : cases) {
    const ratio targetDelta = sourceDelta * testCase.intervalRatio;
    const std::string rql =
        declareSource(testCase.sourceWidth, sourceDelta) + "SELECT * STREAM result FROM src-" + ratioText(targetDelta) + "\n";

    qTree instance;
    auto [parseResult, keyword, name] = parserRQLString(instance, rql);
    ASSERT_EQ(parseResult, "OK") << rql;

    compiler compilerInstance(instance);
    ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

    const auto &result = instance.getQuery("result");
    EXPECT_EQ(result.rInterval, targetDelta) << rql;
    EXPECT_EQ(result.startupLatency, testCase.expectedTail) << rql;
    EXPECT_EQ(eventCapacity(testCase.intervalRatio, testCase.expectedTail), testCase.expectedCapacity) << rql;
    EXPECT_EQ(instance.maxCapacity.at("src"), testCase.expectedCapacity) << rql;
  }
}
