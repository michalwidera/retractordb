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
    ASSERT_TRUE(testdata[i][1] == leftOrRight ? 1 : 0);
    ASSERT_TRUE(testdata[i][2] == retPos);
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
    std::cout << "{" << i << ", " << retPosDiv << ", " << retPosMod << "},  //" << std::endl;
    ASSERT_TRUE(testdata[i][1] == retPosDiv);
    ASSERT_TRUE(testdata[i][2] == retPosMod);
  }
}
