#include <gtest/gtest.h>

#include <unistd.h>
#include <ctime>
#include <iostream>
#include <sstream>

#include "retractor/lib/executor_rt.hpp"

// ctest -R '^ut-test_executor_rt' -V

namespace {

// --- rtAbsoluteSleep ---

TEST(ExecutorRtSleepTest, PastAnchorReturnsImmediately) {
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);
  anchor.tv_sec -= 100;

  struct timespec before{};
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &before);
  rtAbsoluteSleep(anchor, 0);
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_LT(elapsed_ms, 500);  // luźny próg: valgrind + obciążone CI dają jitter rzędu dziesiątek ms
}

TEST(ExecutorRtSleepTest, SmallIntervalCompletesInTime) {
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);

  struct timespec before = anchor;
  rtAbsoluteSleep(anchor, 10);
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_GE(elapsed_ms, 8);
  EXPECT_LT(elapsed_ms, 500);
}

TEST(ExecutorRtSleepTest, NanosecondCarryOverDoesNotHang) {
  // anchor.tv_nsec near 1s boundary + 200ms interval → carry triggered
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);
  anchor.tv_sec -= 100;
  anchor.tv_nsec = 900'000'000L;

  struct timespec before{};
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &before);
  rtAbsoluteSleep(anchor, 200);
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_LT(elapsed_ms, 500);  // luźny próg: valgrind + obciążone CI dają jitter rzędu dziesiątek ms
}

TEST(ExecutorRtSleepTest, LargeIntervalNoCarryPastAnchor) {
  // anchor in the past, large interval that would normally overflow tv_nsec
  // but anchor is old enough that result is still in the past
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);
  anchor.tv_sec -= 10;
  anchor.tv_nsec = 0;

  struct timespec before{};
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &before);
  rtAbsoluteSleep(anchor, 1000);  // 1s from 10s-ago anchor = still 9s in past
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_LT(elapsed_ms, 500);  // luźny próg: valgrind + obciążone CI dają jitter rzędu dziesiątek ms
}

// --- rtCheckAndPrint ---

TEST(ExecutorRtCheckTest, DoesNotCrashAndReturnsBool) {
  std::ostringstream captured;
  auto *old   = std::cout.rdbuf(captured.rdbuf());
  bool result = rtCheckAndPrint();
  std::cout.rdbuf(old);

  EXPECT_TRUE(result == true || result == false);
}

TEST(ExecutorRtCheckTest, OutputContainsAllRequiredSections) {
  std::ostringstream captured;
  auto *old = std::cout.rdbuf(captured.rdbuf());
  rtCheckAndPrint();
  std::cout.rdbuf(old);

  const auto &out = captured.str();
  EXPECT_NE(out.find("RT requirements check"), std::string::npos);
  EXPECT_NE(out.find("CAP_SYS_NICE"), std::string::npos);
  EXPECT_NE(out.find("CAP_IPC_LOCK"), std::string::npos);
  EXPECT_NE(out.find("PREEMPT_RT"), std::string::npos);
  EXPECT_NE(out.find("RT throttling"), std::string::npos);
  EXPECT_NE(out.find("RLIMIT_MEMLOCK"), std::string::npos);
  EXPECT_NE(out.find("Current scheduler"), std::string::npos);
}

TEST(ExecutorRtCheckTest, OutputUsesOkOrFailMarkers) {
  std::ostringstream captured;
  auto *old = std::cout.rdbuf(captured.rdbuf());
  rtCheckAndPrint();
  std::cout.rdbuf(old);

  const auto &out = captured.str();
  bool hasMarker  = out.find("[OK]") != std::string::npos || out.find("[FAIL]") != std::string::npos ||
                   out.find("[WARN]") != std::string::npos;
  EXPECT_TRUE(hasMarker);
}

TEST(ExecutorRtCheckTest, FalseReturnImpliesErrorInOutput) {
  std::ostringstream captured;
  auto *old   = std::cout.rdbuf(captured.rdbuf());
  bool result = rtCheckAndPrint();
  std::cout.rdbuf(old);

  if (!result) {
    EXPECT_NE(captured.str().find("ERROR"), std::string::npos);
  }
}

// --- rtActivate ---

TEST(ExecutorRtActivateTest, WithoutRootReturnsFalse) {
  if (geteuid() == 0) {
    GTEST_SKIP() << "Running as root; cannot test unprivileged path";
  }
  bool result = rtActivate();
  EXPECT_FALSE(result);
}

}  // namespace
