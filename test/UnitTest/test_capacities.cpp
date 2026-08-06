// Tabelaryczna regresja pojemności źródeł deklarowanych dla różnicy i sumy
// strumieni. Uzupełnia scenariusz integracyjny k24_capacity o ułamkowe fazy
// i różne szerokości rekordu. Oczekiwania pochodzą z niezależnego przeglądu
// jednego okresu fazowego, bez wywoływania funkcji silnika liczących ogon
// ani computeRequiredCapacities().

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

std::string declareSource(const std::string &id, int width, const ratio &delta) {
  std::string rql = "DECLARE ";
  for (int field = 0; field < width; ++field)
    rql += (field == 0 ? "" : ", ") + std::string("f") + std::to_string(field) + " INTEGER";
  return rql + " STREAM " + id + ", " + ratioText(delta) + " FILE '" + id + ".txt'\n";
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

struct subtractCapacityCase {
  ratio intervalRatio;
  int sourceWidth;
  int expectedTail;
  int expectedCapacity;
};

const std::vector<subtractCapacityCase> subtractCases{
    {.intervalRatio = ratio(1), .sourceWidth = 1, .expectedTail = 1, .expectedCapacity = 4},
    {.intervalRatio = ratio(3, 2), .sourceWidth = 2, .expectedTail = 1, .expectedCapacity = 5},
    {.intervalRatio = ratio(2), .sourceWidth = 3, .expectedTail = 1, .expectedCapacity = 6},
    {.intervalRatio = ratio(7, 3), .sourceWidth = 4, .expectedTail = 1, .expectedCapacity = 6},
    {.intervalRatio = ratio(3), .sourceWidth = 1, .expectedTail = 1, .expectedCapacity = 8},
    {.intervalRatio = ratio(4), .sourceWidth = 2, .expectedTail = 1, .expectedCapacity = 10},
    {.intervalRatio = ratio(5), .sourceWidth = 4, .expectedTail = 1, .expectedCapacity = 12},
};

// ADD wybiera rekord floor(n*Dout/Dsrc) każdej składowej. Ogon jest najmniejszą
// liczbą slotów wyjścia, dla której chwila emisji nie poprzedza dostępności tego
// rekordu. Przegląd mianownika ilorazu obejmuje wszystkie fazy Beatty'ego.
int addSourceTail(const ratio &outputToSourceRatio) {
  int required = 0;
  for (int n = 0; n < outputToSourceRatio.denominator(); ++n) {
    const int sourceIndex = floorOf(ratio(n) * outputToSourceRatio);
    const ratio deficit   = ratio(sourceIndex + 1) / outputToSourceRatio - ratio(n + 1);
    required              = std::max(required, ceilOf(deficit));
  }
  return required;
}

int addEventCapacity(const ratio &outputToSourceRatio, int outputTail) {
  constexpr int declarationPrefetch = 2;
  int required                      = 1;
  for (int n = 0; n < outputToSourceRatio.denominator(); ++n) {
    const int sourceCount = floorOf(ratio(n + 1 + outputTail) * outputToSourceRatio);
    const int sourceIndex = floorOf(ratio(n) * outputToSourceRatio);
    required              = std::max(required, sourceCount - sourceIndex + declarationPrefetch);
  }
  return required;
}

struct addCapacityCase {
  ratio leftDelta;
  ratio rightDelta;
  int leftWidth;
  int rightWidth;
  int expectedTail;
  int expectedLeftCapacity;
  int expectedRightCapacity;
};

const std::vector<addCapacityCase> addCases{
    {.leftDelta             = ratio(1, 100),
     .rightDelta            = ratio(1, 100),
     .leftWidth             = 1,
     .rightWidth            = 2,
     .expectedTail          = 0,
     .expectedLeftCapacity  = 3,
     .expectedRightCapacity = 3},
    {.leftDelta             = ratio(1, 100),
     .rightDelta            = ratio(3, 200),
     .leftWidth             = 2,
     .rightWidth            = 3,
     .expectedTail          = 1,
     .expectedLeftCapacity  = 4,
     .expectedRightCapacity = 4},
    {.leftDelta             = ratio(1, 100),
     .rightDelta            = ratio(3, 100),
     .leftWidth             = 3,
     .rightWidth            = 1,
     .expectedTail          = 2,
     .expectedLeftCapacity  = 5,
     .expectedRightCapacity = 3},
    {.leftDelta             = ratio(1, 50),
     .rightDelta            = ratio(3, 100),
     .leftWidth             = 4,
     .rightWidth            = 2,
     .expectedTail          = 1,
     .expectedLeftCapacity  = 4,
     .expectedRightCapacity = 4},
    {.leftDelta             = ratio(3, 100),
     .rightDelta            = ratio(1, 100),
     .leftWidth             = 1,
     .rightWidth            = 4,
     .expectedTail          = 2,
     .expectedLeftCapacity  = 3,
     .expectedRightCapacity = 5},
};

}  // namespace

TEST(capacities, subtract_declared_matches_event_table) {
  const ratio sourceDelta(1, 100);

  for (const auto &testCase : subtractCases) {
    const ratio targetDelta = sourceDelta * testCase.intervalRatio;
    const std::string rql   = declareSource("src", testCase.sourceWidth, sourceDelta) + "SELECT * STREAM result FROM src-" +
                            ratioText(targetDelta) + "\n";

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

TEST(capacities, add_declared_matches_event_table) {
  for (const auto &testCase : addCases) {
    const ratio outputDelta = std::min(testCase.leftDelta, testCase.rightDelta);
    const ratio leftRatio   = outputDelta / testCase.leftDelta;
    const ratio rightRatio  = outputDelta / testCase.rightDelta;
    const std::string rql   = declareSource("left", testCase.leftWidth, testCase.leftDelta) +
                            declareSource("right", testCase.rightWidth, testCase.rightDelta) +
                            "SELECT * STREAM result FROM left+right\n";

    qTree instance;
    auto [parseResult, keyword, name] = parserRQLString(instance, rql);
    ASSERT_EQ(parseResult, "OK") << rql;

    compiler compilerInstance(instance);
    ASSERT_EQ(compilerInstance.compile(), "OK") << rql;

    const auto &result = instance.getQuery("result");
    EXPECT_EQ(result.rInterval, outputDelta) << rql;
    EXPECT_EQ(std::max(addSourceTail(leftRatio), addSourceTail(rightRatio)), testCase.expectedTail) << rql;
    EXPECT_EQ(result.startupLatency, testCase.expectedTail) << rql;
    EXPECT_EQ(addEventCapacity(leftRatio, testCase.expectedTail), testCase.expectedLeftCapacity) << rql;
    EXPECT_EQ(addEventCapacity(rightRatio, testCase.expectedTail), testCase.expectedRightCapacity) << rql;
    EXPECT_EQ(instance.maxCapacity.at("left"), testCase.expectedLeftCapacity) << rql;
    EXPECT_EQ(instance.maxCapacity.at("right"), testCase.expectedRightCapacity) << rql;
  }
}
