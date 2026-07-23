#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <vector>

#include "SOperations.hpp"

// ctest -R '^ut-soperations' -V

TEST(xSOperations, hash_operations) {
  std::vector<std::vector<int>> testdata{
      {0, 1, 0},  // B[0]
      {1, 1, 1},  // B[1]
      {2, 0, 0},  //      A[0]
      {3, 1, 2},  // B[2]
      {4, 1, 3},  // B[3]
      {5, 0, 1},  //      A[1]
      {6, 1, 4},  // B[4]
      {7, 1, 5},  // B[5]
      {8, 0, 2},  //      A[2]
      {9, 1, 6},  // B[6]
  };

  auto deltaA = boost::rational<int>(1, 1);
  auto deltaB = boost::rational<int>(1, 2);

  int retPos{0};
  for (int i = 0; i < 10; i++) {
    auto leftOrRight = Hash(deltaA, deltaB, i, retPos);
    EXPECT_TRUE(testdata[i][1] == leftOrRight ? 1 : 0);
    EXPECT_TRUE(testdata[i][2] == retPos);
  }
}

// Wzorzec wyprowadzony z definicji przeplotu (def:interleave):
// z = deltaB/(deltaA+deltaB) = 2/3 dla pary (1/16, 1/8) — wzorzec B,A,A,...
// Stary kod używał delta_c zamiast z w retPos, co było maskowane w teście
// wyżej przez deltaA == 1 (wtedy delta_c == z liczbowo).
TEST(xSOperations, hash_operations_nonunit_delta) {
  std::vector<std::vector<int>> testdata{
      {0, 1, 0},  // B[0]
      {1, 0, 0},  //      A[0]
      {2, 0, 1},  //      A[1]
      {3, 1, 1},  // B[1]
      {4, 0, 2},  //      A[2]
      {5, 0, 3},  //      A[3]
      {6, 1, 2},  // B[2]
      {7, 0, 4},  //      A[4]
      {8, 0, 5},  //      A[5]
      {9, 1, 3},  // B[3]
  };

  auto deltaA = boost::rational<int>(1, 16);
  auto deltaB = boost::rational<int>(1, 8);

  int retPos{0};
  for (int i = 0; i < 10; i++) {
    auto leftOrRight = Hash(deltaA, deltaB, i, retPos);
    EXPECT_EQ(testdata[i][1], leftOrRight ? 1 : 0) << "i=" << i;
    EXPECT_EQ(testdata[i][2], retPos) << "i=" << i;
  }
}

// Równe delty (perfect shuffle): z = 1/2 — wzorzec B,A,B,A,...
TEST(xSOperations, hash_operations_equal_delta) {
  auto delta = boost::rational<int>(1, 360);

  int retPos{0};
  for (int i = 0; i < 20; i++) {
    auto leftOrRight = Hash(delta, delta, i, retPos);
    EXPECT_EQ(i % 2 == 0, leftOrRight) << "i=" << i;
    EXPECT_EQ(i / 2, retPos) << "i=" << i;
  }
}

// Dopasowanie przesunięć przeplotu:
// (A > i) # (B > k) == (A # B) > (i + k), gdy i*deltaA == k*deltaB.
// Test sprawdza niezależnie od kompilatora zarówno prefiks opóźnienia, jak i
// późniejszy wybór tej samej gałęzi oraz tego samego indeksu źródła.
TEST(xSOperations, hash_matched_time_moves) {
  struct TestCase {
    boost::rational<int> deltaA;
    boost::rational<int> deltaB;
    int shiftA;
    int shiftB;
  };
  const std::vector<TestCase> cases{
      {{1, 10}, {1, 5}, 2, 1},
      {{1, 10}, {1, 10}, 3, 3},
      {{3, 10}, {1, 10}, 1, 3},
  };

  for (const auto &[deltaA, deltaB, shiftA, shiftB] : cases) {
    ASSERT_EQ(deltaA * shiftA, deltaB * shiftB);
    const int outputShift = shiftA + shiftB;

    for (int n = 0; n < 200; ++n) {
      int shiftedSourceIndex = 0;
      const bool takeB       = Hash(deltaA, deltaB, n, shiftedSourceIndex);
      const int sourceShift  = takeB ? shiftB : shiftA;

      if (n < outputShift) {
        EXPECT_LT(shiftedSourceIndex, sourceShift) << "n=" << n;
        continue;
      }

      int originalSourceIndex  = 0;
      const bool originalTakeB = Hash(deltaA, deltaB, n - outputShift, originalSourceIndex);
      EXPECT_EQ(takeB, originalTakeB) << "n=" << n;
      EXPECT_EQ(shiftedSourceIndex - sourceShift, originalSourceIndex) << "n=" << n;
    }
  }
}

TEST(xSOperations, divmod_operations) {
  std::vector<std::vector<int>> testdata{
      {0, 2, 0},    //
      {1, 5, 1},    //
      {2, 8, 3},    //
      {3, 11, 4},   //
      {4, 14, 6},   //
      {5, 17, 7},   //
      {6, 20, 9},   //
      {7, 23, 10},  //
      {8, 26, 12},  //
      {9, 29, 13},  //
  };

  auto deltaA = boost::rational<int>(1, 1);
  auto deltaB = boost::rational<int>(1, 2);

  for (int i = 0; i < 10; i++) {
    auto retPosDiv = Div(deltaA, deltaB, i);
    auto retPosMod = Mod(deltaA, deltaB, i);
    EXPECT_TRUE(testdata[i][1] == retPosDiv);
    EXPECT_TRUE(testdata[i][2] == retPosMod);
  }
}

// Spójność rozplotu z przeplotem (cor:exact): pozycja z Div/Mod musi
// wskazywać w strumieniu przeplecionym element WŁAŚCIWEJ składowej o tym
// samym indeksie — Θ (Div) trafia w gałąź A, ~Θ (Mod) w gałąź B.
TEST(xSOperations, divmod_inverts_hash) {
  std::vector<std::pair<boost::rational<int>, boost::rational<int>>> pairs{
      {{1, 1}, {1, 2}},      //
      {{1, 16}, {1, 8}},     //
      {{1, 360}, {1, 360}},  //
      {{3, 10}, {1, 10}},    //
  };

  for (const auto &[deltaA, deltaB] : pairs) {
    for (int i = 0; i < 200; i++) {
      int retPos{0};
      EXPECT_FALSE(Hash(deltaA, deltaB, Div(deltaA, deltaB, i), retPos));
      EXPECT_EQ(i, retPos);
      EXPECT_TRUE(Hash(deltaA, deltaB, Mod(deltaA, deltaB, i), retPos));
      EXPECT_EQ(i, retPos);
    }
  }
}
