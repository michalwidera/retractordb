#include <gtest/gtest.h>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>

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

// --- rtKeepThreadOffRtCpus (issue_217, badanie W8) ---
//
// Wątek komunikacyjny silnika jest SCHED_OTHER i dzieli rdzeń z wątkiem
// SCHED_FIFO, gdy operator przypina cały proces (`taskset -c 3`). Powyżej 100 %
// obciążenia slotu wątek RT nigdy nie oddaje rdzenia i wątek komunikacyjny nie
// jest szeregowany wcale — klient nie zdąży się zarejestrować.
//
// Samego zagłodzenia nie da się odtworzyć w teście jednostkowym bez CAP_SYS_NICE
// i bez ryzyka zawieszenia rdzenia biegaczowi testów, więc testowany jest
// mechanizm, który mu zapobiega: rozdział rdzeni.

TEST(ExecutorRtAffinityTest, MovesThreadOffPinnedRtCore) {
  if (sysconf(_SC_NPROCESSORS_ONLN) < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch rdzeni online";

  cpu_set_t original;
  CPU_ZERO(&original);
  ASSERT_EQ(sched_getaffinity(0, sizeof(original), &original), 0);

  // Udajemy wątek RT przypięty do jednego rdzenia — to konfiguracja kampanii.
  cpu_set_t pinned;
  CPU_ZERO(&pinned);
  CPU_SET(0, &pinned);
  ASSERT_EQ(sched_setaffinity(0, sizeof(pinned), &pinned), 0);

  std::atomic<bool> stop{false};
  std::thread aux([&stop] {
    while (!stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });

  const bool moved = rtKeepThreadOffRtCpus(aux.native_handle());

  cpu_set_t auxMask;
  CPU_ZERO(&auxMask);
  const int rc = pthread_getaffinity_np(aux.native_handle(), sizeof(auxMask), &auxMask);

  stop = true;
  aux.join();
  ASSERT_EQ(sched_setaffinity(0, sizeof(original), &original), 0);

  ASSERT_TRUE(moved) << "watek pomocniczy nie zostal przeniesiony poza przypiety rdzen RT";
  ASSERT_EQ(rc, 0);
  EXPECT_FALSE(CPU_ISSET(0, &auxMask)) << "watek pomocniczy nadal dzieli rdzen z watkiem RT";
  EXPECT_GT(CPU_COUNT(&auxMask), 0) << "watek pomocniczy zostal bez zadnego rdzenia";
}

// Bez przypięcia nie ma zagłodzenia i nie ma czego naprawiać — funkcja musi
// wtedy zostawić powinowactwo w spokoju, zamiast zawężać je na własną rękę.
TEST(ExecutorRtAffinityTest, LeavesUnpinnedThreadAlone) {
  const long online = sysconf(_SC_NPROCESSORS_ONLN);
  if (online < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch rdzeni online";

  cpu_set_t original;
  CPU_ZERO(&original);
  ASSERT_EQ(sched_getaffinity(0, sizeof(original), &original), 0);

  cpu_set_t all;
  CPU_ZERO(&all);
  for (long cpu = 0; cpu < online && cpu < CPU_SETSIZE; ++cpu)
    CPU_SET(static_cast<int>(cpu), &all);
  if (sched_setaffinity(0, sizeof(all), &all) != 0) GTEST_SKIP() << "brak prawa do rozszerzenia powinowactwa";

  std::atomic<bool> stop{false};
  std::thread aux([&stop] {
    while (!stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });

  const bool moved = rtKeepThreadOffRtCpus(aux.native_handle());

  stop = true;
  aux.join();
  ASSERT_EQ(sched_setaffinity(0, sizeof(original), &original), 0);

  EXPECT_FALSE(moved) << "bez przypiecia watku RT nie wolno ruszac powinowactwa watku pomocniczego";
}

}  // namespace
