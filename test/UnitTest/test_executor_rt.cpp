#include <gtest/gtest.h>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "platformConfig.h"
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
  // Wiersze wspolne dla kazdej platformy: ramka, limit blokowania pamieci,
  // wzmianka o dlawieniu RT i biezaca polityka szeregowania.
  EXPECT_NE(out.find("RT requirements check"), std::string::npos);
  EXPECT_NE(out.find("RT throttling"), std::string::npos);
  EXPECT_NE(out.find("RLIMIT_MEMLOCK"), std::string::npos);
  EXPECT_NE(out.find("Current scheduler"), std::string::npos);

#if RDB_HAS_PROCFS
  // Capabilities POSIX.1e i latka PREEMPT_RT sa pojeciami jadra Linuksa. Wiersz
  // "CAP_SYS_NICE" na jadrze bez capabilities bylby nieprawda w formacie raportu
  // zgodnosci, wiec tam raport wypisuje inne pozycje - patrz executor_rt.cpp.
  EXPECT_NE(out.find("CAP_SYS_NICE"), std::string::npos);
  EXPECT_NE(out.find("CAP_IPC_LOCK"), std::string::npos);
  EXPECT_NE(out.find("PREEMPT_RT"), std::string::npos);
#else
  EXPECT_NE(out.find("SCHED_FIFO available"), std::string::npos);
  EXPECT_NE(out.find("Real-time kernel"), std::string::npos);
#endif
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

#if RDB_HAS_SCHED_SETSCHEDULER

TEST(ExecutorRtActivateTest, WithoutRootReturnsFalse) {
  if (geteuid() == 0) {
    GTEST_SKIP() << "Running as root; cannot test unprivileged path";
  }
  const bool result = rtActivate();
#if RDB_HAS_MLOCKALL
  // rtActivate moze wlaczyc MCL_FUTURE przed odmowa SCHED_FIFO. Zwolnij te
  // blokade, aby limit memlock nie uniemozliwil nastepnym testom tworzenia watkow.
  EXPECT_EQ(munlockall(), 0);
#endif
  EXPECT_FALSE(result);
}

#else

// Na jadrach, w ktorych polityka szeregowania jest wlasnoscia WATKU, a nie calego
// procesu, podniesienie do SCHED_FIFO nie jest zastrzezone dla roota tak jak na
// Linuksie - wiec "bez roota ma sie nie udac" nie jest tu zdaniem prawdziwym
// i nie ma czego asercjonowac. Sprawdzalne jest natomiast, ze wynik NIE KLAMIE:
// gdy rtActivate melduje sukces, wolajacy watek naprawde biegnie pod SCHED_FIFO.
TEST(ExecutorRtActivateTest, SuccessImpliesRealTimePolicyOnCallingThread) {
  int policyBefore = SCHED_OTHER;
  struct sched_param paramBefore{};
  ASSERT_EQ(pthread_getschedparam(pthread_self(), &policyBefore, &paramBefore), 0);

  const bool activated = rtActivate();

  int policyAfter = SCHED_OTHER;
  struct sched_param paramAfter{};
  ASSERT_EQ(pthread_getschedparam(pthread_self(), &policyAfter, &paramAfter), 0);

  // Polityke przywracamy ZAWSZE i przed asercja: gdyby test zostawil watek
  // testowy pod SCHED_FIFO, kazdy nastepny test w tym binarium biegl by z
  // priorytetem czasu rzeczywistego.
  pthread_setschedparam(pthread_self(), policyBefore, &paramBefore);

  if (activated) EXPECT_EQ(policyAfter, SCHED_FIFO);
}

#endif

// --- rtKeepThreadOffRtCpus (issue_217, badanie W8) ---
//
// Wątek komunikacyjny silnika jest SCHED_OTHER i dzieli rdzeń z wątkiem
// SCHED_FIFO, gdy operator przypina cały proces (`taskset -c 3`). Powyżej 100 %
// obciążenia slotu wątek RT nigdy nie oddaje rdzenia i wątek komunikacyjny nie
// jest szeregowany wcale - klient nie zdąży się zarejestrować.
//
// Samego zagłodzenia nie da się odtworzyć w teście jednostkowym bez CAP_SYS_NICE
// i bez ryzyka zawieszenia rdzenia biegaczowi testów, więc testowany jest
// mechanizm, który mu zapobiega: rozdział rdzeni.

#if RDB_HAS_SCHED_AFFINITY

TEST(ExecutorRtAffinityTest, MovesThreadOffPinnedRtCore) {
  if (sysconf(_SC_NPROCESSORS_ONLN) < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch rdzeni online";

  cpu_set_t original;
  CPU_ZERO(&original);
  ASSERT_EQ(sched_getaffinity(0, sizeof(original), &original), 0);

  // Udajemy wątek RT przypięty do jednego rdzenia - to konfiguracja kampanii.
  cpu_set_t pinned;
  CPU_ZERO(&pinned);
  CPU_SET(0, &pinned);
  ASSERT_EQ(sched_setaffinity(0, sizeof(pinned), &pinned), 0);

  std::atomic<bool> stop{false};
  std::thread aux([&stop] {
    while (!stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });

  // Rozdzial rdzeni raportuje sie operatorowi na stdout - asercje sa na masce, nie na wydruku.
  testing::internal::CaptureStdout();
  const bool moved = rtKeepThreadOffRtCpus(aux.native_handle());
  testing::internal::GetCapturedStdout();

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

// Bez przypięcia nie ma zagłodzenia i nie ma czego naprawiać - funkcja musi
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

#else

// Bez masek powinowactwa nie ma czego przestawiac i nie ma tez zaglodzenia,
// przed ktorym tamten mechanizm broni: rtActivate podnosi wtedy do SCHED_FIFO
// sam watek wolajacy, wiec watek komunikacyjny zostaje przy polityce domyslnej.
// Kontrakt, ktory MUSI obowiazywac takze tutaj: funkcja melduje false, czyli
// "niczego nie zmieniono", zamiast udawac, ze watek zostal przeniesiony.
TEST(ExecutorRtAffinityTest, ReportsNoMoveWhereAffinityIsUnavailable) {
  std::atomic<bool> stop{false};
  std::thread aux([&stop] {
    while (!stop)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });

  testing::internal::CaptureStdout();
  const bool moved      = rtKeepThreadOffRtCpus(aux.native_handle());
  const std::string out = testing::internal::GetCapturedStdout();

  stop = true;
  aux.join();

  EXPECT_FALSE(moved);
  // Komunikat wypisywany jest RAZ na proces, wiec przy drugim wywolaniu w tym
  // samym binarium bedzie pusty - stad asercja tylko o tresci, gdy cos padlo.
  if (!out.empty()) EXPECT_NE(out.find("CPU affinity is not available"), std::string::npos);
}

#endif

}  // namespace
