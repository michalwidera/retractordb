#include <gtest/gtest.h>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <ctime>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logCapture.hpp"
#include "platformConfig.h"
#include "retractor/lib/executor_rt.hpp"
#include "syscallWrap.hpp"

#if RDB_HAS_SCHED_SETSCHEDULER && RDB_HAS_MLOCKALL
namespace {
bool mockActivation = false;
std::vector<int> lockCalls;
int lockError = 0, scheduleError = 0, scheduledPid = -1, scheduledPolicy = -1, scheduledPriority = -1;
}  // namespace
extern "C" int __real_mlockall(int);
extern "C" int __real_sched_setscheduler(pid_t, int, const struct sched_param *);
extern "C" int __wrap_mlockall(int flags) {
  if (!mockActivation) return __real_mlockall(flags);
  lockCalls.push_back(flags);
  errno = lockError;
  return lockError ? -1 : 0;
}
extern "C" int __wrap_sched_setscheduler(pid_t pid, int policy, const struct sched_param *parameter) {
  if (!mockActivation) return __real_sched_setscheduler(pid, policy, parameter);
  scheduledPid      = pid;
  scheduledPolicy   = policy;
  scheduledPriority = parameter->sched_priority;
  errno             = scheduleError;
  return scheduleError ? -1 : 0;
}
RDB_WRAP_SYSCALL(int, mlockall, (int flags), (flags));
RDB_WRAP_SYSCALL(int, sched_setscheduler, (pid_t pid, int policy, const struct sched_param *parameter),
                 (pid, policy, parameter));
#endif

#if RDB_HAS_SCHED_AFFINITY
namespace {
bool mockAffinity     = false;
int affinityReadError = 0, affinityWriteError = 0, affinityWrites = 0;
long affinityOnline = 3;
cpu_set_t callingMask{}, writtenMask{};
pthread_t writtenThread{};
}  // namespace
extern "C" int __real_sched_getaffinity(pid_t, size_t, cpu_set_t *);
extern "C" long __real_sysconf(int);
extern "C" int __real_pthread_setaffinity_np(pthread_t, size_t, const cpu_set_t *);
extern "C" int __wrap_sched_getaffinity(pid_t pid, size_t size, cpu_set_t *mask) {
  if (!mockAffinity) return __real_sched_getaffinity(pid, size, mask);
  *mask = callingMask;
  errno = affinityReadError;
  return affinityReadError ? -1 : 0;
}
extern "C" long __wrap_sysconf(int key) {
  return mockAffinity && key == _SC_NPROCESSORS_ONLN ? affinityOnline : __real_sysconf(key);
}
extern "C" int __wrap_pthread_setaffinity_np(pthread_t thread, size_t size, const cpu_set_t *mask) {
  if (!mockAffinity) return __real_pthread_setaffinity_np(thread, size, mask);
  ++affinityWrites;
  writtenThread = thread;
  writtenMask   = *mask;
  return affinityWriteError;
}
RDB_WRAP_SYSCALL(int, sched_getaffinity, (pid_t pid, size_t size, cpu_set_t *mask), (pid, size, mask));
RDB_WRAP_SYSCALL(long, sysconf, (int key), (key));
RDB_WRAP_SYSCALL(int, pthread_setaffinity_np, (pthread_t thread, size_t size, const cpu_set_t *mask), (thread, size, mask));
#endif

// ctest -R '^ut_executor_rt' -V

namespace {

#if RDB_HAS_SCHED_SETSCHEDULER && RDB_HAS_MLOCKALL
TEST(ExecutorRtActivateTest, MockedCallsCheckMemoryPolicyErrnosAndThreadPriority) {
  // covers: executorrt.rt-activate.1~1, executorrt.rt-activate.2~1, executorrt.rt-activate.3~1, executorrt.rt-activate.4~1
  // covers: executorrt.rt-activate.5~1, executorrt.rt-activate.6~1
  struct Restore {
    std::optional<std::string> previous =
        std::getenv("RDB_MLOCKALL") ? std::optional<std::string>{std::getenv("RDB_MLOCKALL")} : std::nullopt;
    ~Restore() {
      mockActivation = false;
      if (previous)
        setenv("RDB_MLOCKALL", previous->c_str(), 1);
      else
        unsetenv("RDB_MLOCKALL");
    }
  } restore;
  mockActivation = true;
  for (const std::string mode : {"onfault", "populate", "off"}) {
    SCOPED_TRACE(mode);
    ASSERT_EQ(setenv("RDB_MLOCKALL", mode.c_str(), 1), 0);
    lockCalls.clear();
    lockError = scheduleError = 0;
    LogCapture log;
    EXPECT_TRUE(rtActivate(0));
    EXPECT_EQ(scheduledPid, 0);
    EXPECT_EQ(scheduledPolicy, SCHED_FIFO);
    EXPECT_EQ(scheduledPriority, sched_get_priority_min(SCHED_FIFO));
    if (mode == "off")
      EXPECT_TRUE(lockCalls.empty());
    else if (mode == "populate")
      EXPECT_EQ(lockCalls, (std::vector<int>{MCL_CURRENT | MCL_FUTURE}));
    else {
#if RDB_HAS_MCL_ONFAULT
      EXPECT_EQ(lockCalls, (std::vector<int>{MCL_CURRENT, MCL_FUTURE | MCL_ONFAULT}));
#else
      EXPECT_EQ(lockCalls, (std::vector<int>{MCL_CURRENT}));
#endif
    }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
    if (mode != "onfault") EXPECT_TRUE(log.text().contains("diagnostic mode"));
#endif
    EXPECT_TRUE(rtActivate(1000));
    EXPECT_EQ(scheduledPriority, sched_get_priority_max(SCHED_FIFO));
  }
  ASSERT_EQ(setenv("RDB_MLOCKALL", "onfault", 1), 0);
  lockCalls.clear();
  lockError = EPERM;
  EXPECT_FALSE(rtActivate());
  EXPECT_EQ(lockCalls, (std::vector<int>{MCL_CURRENT}));
  lockError = ENOSYS;
  EXPECT_TRUE(rtActivate());
  lockError     = 0;
  scheduleError = EPERM;
  EXPECT_FALSE(rtActivate());
  scheduleError = 0;
}
#endif

// --- rtAbsoluteSleep ---

TEST(ExecutorRtSleepTest, PastAnchorReturnsImmediately) {
  // covers: executorrt.rt-absolute-sleep.1~1
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);
  anchor.tv_sec -= 100;

  struct timespec before{};
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &before);
  EXPECT_TRUE(rtAbsoluteSleep(anchor, 0));
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_LT(elapsed_ms, 500);  // luźny próg: valgrind + obciążone CI dają jitter rzędu dziesiątek ms
}

TEST(ExecutorRtSleepTest, SmallIntervalCompletesInTime) {
  // covers: executorrt.rt-absolute-sleep.1~1
  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);

  struct timespec before = anchor;
  EXPECT_TRUE(rtAbsoluteSleep(anchor, 10));
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &after);

  long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000 + (after.tv_nsec - before.tv_nsec) / 1'000'000;
  EXPECT_GE(elapsed_ms, 8);
  EXPECT_LT(elapsed_ms, 500);
}

TEST(ExecutorRtSleepTest, NanosecondCarryOverDoesNotHang) {
  // covers: executorrt.rt-absolute-sleep.1~1
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

void ignoreWakeSignal(int /*signum*/) {}

// #43: przerwany sen jest zglaszany, a ponowienie czeka do TEGO SAMEGO terminu. Obsluga sygnalu
// przez signal(), jak handleSignal w launcherze (SA_RESTART nie wznawia clock_nanosleep).
// Sygnaly przychodza przez pierwsze 300 ms, wiec watek zdazy zasnac nawet pod valgrindem.
// Ponowienie liczace nowy okres skonczyloby sie najwczesniej po 300 + 600 ms.
TEST(ExecutorRtSleepTest, InterruptReportsEarlyWakeAndRetryKeepsDeadline) {
  // covers: executorrt.rt-absolute-sleep.2~1, executorrt.rt-absolute-sleep.3~1
  constexpr long kDeadlineMs = 600;
  const auto previous        = std::signal(SIGUSR1, ignoreWakeSignal);
  ASSERT_NE(previous, SIG_ERR);

  struct timespec anchor{};
  clock_gettime(CLOCK_MONOTONIC, &anchor);
  const pthread_t sleeper = pthread_self();
  std::thread interrupter([sleeper] {
    for (int i = 0; i < 10; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
      pthread_kill(sleeper, SIGUSR1);
    }
  });

  int earlyWakes = 0;
  while (!rtAbsoluteSleep(anchor, kDeadlineMs))
    ++earlyWakes;
  struct timespec after{};
  clock_gettime(CLOCK_MONOTONIC, &after);
  interrupter.join();
  std::signal(SIGUSR1, previous);

  const long sinceAnchorMs = (after.tv_sec - anchor.tv_sec) * 1000 + (after.tv_nsec - anchor.tv_nsec) / 1'000'000;
  EXPECT_GT(earlyWakes, 0);
  EXPECT_GE(sinceAnchorMs, kDeadlineMs);
  EXPECT_LT(sinceAnchorMs, kDeadlineMs + 200);
}

// #43: termin w ms liczony w 64 bitach. Dawne `rational<int> * 1000` przepelnialo licznik po cichu:
// przy 100 Hz w slocie 214748365 (~24,9 dnia) termin wychodzil ujemny, przy 360 Hz juz po ~2,8 dnia,
// a sen absolutny do terminu z przeszlosci nie spi wcale.
TEST(ExecutorRtSleepTest, SlotDeadlineMsSurvivesLongAxis) {
  // covers: executorrt.rt-slot-deadline-ms.1~1
  EXPECT_EQ(rtSlotDeadlineMs(boost::rational<int>(1, 3)), 333);
  EXPECT_EQ(rtSlotDeadlineMs(boost::rational<int>(2, 3)), 666);
  EXPECT_EQ(rtSlotDeadlineMs(boost::rational<int>(214748365, 100)), 2147483650L);
  EXPECT_EQ(rtSlotDeadlineMs(boost::rational<int>(85899346, 360)), 238609294L);
  EXPECT_EQ(rtSlotDeadlineMs(boost::rational<int>(2147483647, 1)), 2147483647000L);
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
  // covers: executorrt.rt-check-and-print.1~1, executorrt.rt-check-and-print.3~1
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

TEST(ExecutorRtCheckTest, ReturnValueAgreesWithCriticalRowsAndWarningsStayAdvisory) {
  // covers: executorrt.rt-check-and-print.1~1, executorrt.rt-check-and-print.2~1, executorrt.rt-check-and-print.3~1
  // covers: executorrt.rt-check-and-print.4~1
  testing::internal::CaptureStdout();
  const bool result = rtCheckAndPrint();
  const auto output = testing::internal::GetCapturedStdout();
#if RDB_HAS_PROCFS
  const auto nice = output.find(" CAP_SYS_NICE / root");
  const auto lock = output.find(" CAP_IPC_LOCK / root");
  ASSERT_NE(nice, std::string::npos);
  ASSERT_NE(lock, std::string::npos);
  const bool both = output.substr(nice - 6, 6) == "[OK]  " && output.substr(lock - 6, 6) == "[OK]  ";
  EXPECT_EQ(result, both);
  EXPECT_EQ(output.contains("ERROR: Missing critical capabilities"), !both);
  EXPECT_EQ(output.contains("setcap cap_sys_nice,cap_ipc_lock+ep"), !both);
  const auto preempt = output.find(" PREEMPT_RT kernel");
  ASSERT_NE(preempt, std::string::npos);
  EXPECT_EQ(output.contains("Install PREEMPT_RT"), output.substr(preempt - 6, 6) == "[WARN]");
#else
  EXPECT_TRUE(output.contains("Best-effort real time only"));
  EXPECT_FALSE(output.contains("CAP_SYS_NICE"));
  EXPECT_EQ(result, (RDB_HAS_SCHED_SETSCHEDULER || RDB_HAS_PTHREAD_SCHEDPARAM) && sched_get_priority_min(SCHED_FIFO) >= 0 &&
                        sched_get_priority_max(SCHED_FIFO) >= sched_get_priority_min(SCHED_FIFO));
#endif
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

#elif RDB_HAS_PTHREAD_SCHEDPARAM

// Na jadrach bez sched_setscheduler (m.in. Darwin) SCHED_FIFO ustawia sie przez
// pthread_setschedparam, a podniesienie do niego nie jest zastrzezone dla roota
// tak jak na Linuksie - wiec "bez roota ma sie nie udac" nie jest tu zdaniem prawdziwym
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
  ASSERT_EQ(pthread_setschedparam(pthread_self(), policyBefore, &paramBefore), 0);

  if (!activated) GTEST_SKIP() << "jadro odmowilo rzeczywistej aktywacji RT; kontrakt sprawdza ut_executor_rt_pthread";
  EXPECT_EQ(policyAfter, SCHED_FIFO);
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

TEST(ExecutorRtAffinityTest, MockedComplementAndEveryRefusalAreReported) {
  // covers: executorrt.rt-keep-thread-off-rt-cpus.1~1, executorrt.rt-keep-thread-off-rt-cpus.2~1
  // covers: executorrt.rt-keep-thread-off-rt-cpus.3~2, executorrt.rt-keep-thread-off-rt-cpus.5~1
  struct Restore {
    ~Restore() { mockAffinity = false; }
  } restore;
  mockAffinity = true;
  for (int scenario = 0; scenario < 6; ++scenario) {
    SCOPED_TRACE(scenario);
    affinityReadError  = scenario == 1 ? EPERM : 0;
    affinityWriteError = scenario == 3 ? EPERM : scenario == 5 ? EINVAL : 0;
    affinityOnline     = scenario == 2 ? 0 : 3;
    affinityWrites     = 0;
    CPU_ZERO(&callingMask);
    CPU_SET(0, &callingMask);
    if (scenario == 4) {
      CPU_SET(1, &callingMask);
      CPU_SET(2, &callingMask);
    }
    const auto thread = pthread_self();
    testing::internal::CaptureStdout();
    const bool moved  = rtKeepThreadOffRtCpus(thread);
    const auto output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(moved, scenario == 0);
    EXPECT_EQ(affinityWrites, scenario == 0 || scenario == 3 || scenario == 5 ? 1 : 0);
    if (scenario == 0) {
      EXPECT_TRUE(pthread_equal(writtenThread, thread));
      EXPECT_EQ(CPU_COUNT(&writtenMask), 2);
      EXPECT_FALSE(CPU_ISSET(0, &writtenMask));
      EXPECT_TRUE(CPU_ISSET(1, &writtenMask));
      EXPECT_TRUE(CPU_ISSET(2, &writtenMask));
      EXPECT_TRUE(output.contains("[INFO]"));
      EXPECT_TRUE(output.contains("auxiliary cores: 2"));
    } else if (scenario == 4 || scenario == 5) {
      EXPECT_TRUE(output.empty()) << output;
    } else {
      EXPECT_TRUE(output.contains("[WARN]"));
      const char *reason = scenario == 1 ? "read RT thread affinity" : scenario == 2 ? "online cores" : "move the comms thread";
      EXPECT_TRUE(output.contains(reason)) << output;
    }
  }
}

TEST(ExecutorRtAffinityTest, MovesThreadOffPinnedRtCore) {
  // covers: executorrt.rt-keep-thread-off-rt-cpus.1~1
  cpu_set_t original;
  CPU_ZERO(&original);
  ASSERT_EQ(sched_getaffinity(0, sizeof(original), &original), 0);
  if (CPU_COUNT(&original) < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch dozwolonych rdzeni";
  int rtCpu = 0;
  while (!CPU_ISSET(rtCpu, &original))
    ++rtCpu;

  // Udajemy wątek RT przypięty do jednego rdzenia - to konfiguracja kampanii.
  cpu_set_t pinned;
  CPU_ZERO(&pinned);
  CPU_SET(rtCpu, &pinned);
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
  EXPECT_FALSE(CPU_ISSET(rtCpu, &auxMask)) << "watek pomocniczy nadal dzieli rdzen z watkiem RT";
  EXPECT_GT(CPU_COUNT(&auxMask), 0) << "watek pomocniczy zostal bez zadnego rdzenia";
}

// Bez przypięcia nie ma zagłodzenia i nie ma czego naprawiać - funkcja musi
// wtedy zostawić powinowactwo w spokoju, zamiast zawężać je na własną rękę.
TEST(ExecutorRtAffinityTest, LeavesUnpinnedThreadAlone) {
  // covers: executorrt.rt-keep-thread-off-rt-cpus.2~1
  const long online = sysconf(_SC_NPROCESSORS_ONLN);
  if (online < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch rdzeni online";

  cpu_set_t original;
  CPU_ZERO(&original);
  ASSERT_EQ(sched_getaffinity(0, sizeof(original), &original), 0);
  if (CPU_COUNT(&original) < 2) GTEST_SKIP() << "test wymaga co najmniej dwoch dozwolonych rdzeni";

  cpu_set_t all;
  CPU_ZERO(&all);
  for (long cpu = 0; cpu < online && cpu < CPU_SETSIZE; ++cpu)
    CPU_SET(static_cast<int>(cpu), &all);
  if (sched_setaffinity(0, sizeof(all), &all) != 0) GTEST_SKIP() << "brak prawa do rozszerzenia powinowactwa";
  // Cpuset przycina maske po cichu i zwraca sukces; watek RT bylby wtedy przypiety,
  // a test przechodzilby przez odmowe przeniesienia zamiast przez brak przypiecia.
  cpu_set_t widened;
  CPU_ZERO(&widened);
  ASSERT_EQ(sched_getaffinity(0, sizeof(widened), &widened), 0);
  if (!CPU_EQUAL(&widened, &all)) {
    ASSERT_EQ(sched_setaffinity(0, sizeof(original), &original), 0);
    GTEST_SKIP() << "powinowactwa nie da sie rozszerzyc na wszystkie rdzenie online";
  }

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
// przed ktorym tamten mechanizm broni: watku RT i watku komunikacyjnego nie da
// sie przypiac do tych samych rdzeni.
// Kontrakt, ktory MUSI obowiazywac takze tutaj: funkcja melduje false, czyli
// "niczego nie zmieniono", zamiast udawac, ze watek zostal przeniesiony.
TEST(ExecutorRtAffinityTest, ReportsNoMoveWhereAffinityIsUnavailable) {
  // covers: executorrt.rt-keep-thread-off-rt-cpus.4~1
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
  testing::internal::CaptureStdout();
  EXPECT_FALSE(rtKeepThreadOffRtCpus(pthread_self()));
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

#endif

}  // namespace
