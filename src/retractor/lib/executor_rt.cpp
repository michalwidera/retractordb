#include "executor_rt.hpp"

#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cerrno>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

// Pozycje bitów w CapEff (linux/capability.h) — wartości standardu POSIX.1e.
// Bity w /proc/self/status CapEff odpowiadają numerom capability z <sys/capability.h>.
constexpr int kCapSysNiceBit = 23;  // CAP_SYS_NICE  — wymagane do SCHED_FIFO
constexpr int kCapIpcLockBit = 14;  // CAP_IPC_LOCK  — wymagane do mlockall

// Stałe konwersji czasu — używane przy obliczaniu timespec dla clock_nanosleep.
constexpr long kNsPerMs              = 1'000'000L;      // nanosekundy na milisekundę
constexpr long kNsPerSec             = 1'000'000'000L;  // nanosekundy na sekundę
constexpr size_t kCapEffPrefixLength = 7;
constexpr int kRtSchedulerPriority   = 50;

static std::string rtReadFile(const char *path) {
  std::ifstream f(path);
  if (!f) return {};
  std::string v;
  std::getline(f, v);
  return v;
}

static uint64_t rtEffectiveCapabilities() {
  std::ifstream f("/proc/self/status");
  std::string line;
  while (std::getline(f, line)) {
    if (line.starts_with("CapEff:")) {
      uint64_t caps = 0;
      std::sscanf(line.c_str() + kCapEffPrefixLength, "%" SCNx64, &caps);
      return caps;
    }
  }
  return 0;
}

bool rtCheckAndPrint() {
  const uint64_t caps   = rtEffectiveCapabilities();
  const bool isRoot     = (geteuid() == 0);
  const bool hasSysNice = isRoot || (((caps >> kCapSysNiceBit) & 1U) != 0U);  // CAP_SYS_NICE
  const bool hasIpcLock = isRoot || (((caps >> kCapIpcLockBit) & 1U) != 0U);  // CAP_IPC_LOCK

  const std::string rtKernelVal = rtReadFile("/sys/kernel/realtime");
  const bool hasRTKernel        = (rtKernelVal == "1");

  const std::string rtThrottleVal = rtReadFile("/proc/sys/kernel/sched_rt_runtime_us");
  const bool rtThrottleOff        = (rtThrottleVal == "-1");

  struct rlimit memlockRl{};
  getrlimit(RLIMIT_MEMLOCK, &memlockRl);
  const bool memlockUnlimited = (memlockRl.rlim_cur == RLIM_INFINITY);

  const int curPolicy = sched_getscheduler(0);
  const char *policyName;
  if (curPolicy == SCHED_FIFO)
    policyName = "SCHED_FIFO";
  else if (curPolicy == SCHED_RR)
    policyName = "SCHED_RR";
  else if (curPolicy == SCHED_OTHER)
    policyName = "SCHED_OTHER";
  else
    policyName = "unknown";

  auto ok  = [](bool v) { return v ? "[OK]  " : "[FAIL]"; };
  auto rec = [](bool v) { return v ? "[OK]  " : "[WARN]"; };

  std::cout << "\n=== RT requirements check ===\n";
  std::cout << ok(hasSysNice) << " CAP_SYS_NICE / root        — required for SCHED_FIFO\n";
  std::cout << ok(hasIpcLock) << " CAP_IPC_LOCK / root        — required for mlockall\n";
  std::cout << rec(hasRTKernel)
            << " PREEMPT_RT kernel          — /sys/kernel/realtime=" << (rtKernelVal.empty() ? "missing" : rtKernelVal) << "\n";
  std::cout << rec(rtThrottleOff)
            << " RT throttling disabled     — sched_rt_runtime_us=" << (rtThrottleVal.empty() ? "missing" : rtThrottleVal)
            << (rtThrottleOff ? "" : "  (set to -1 to disable throttling)") << "\n";
  std::cout << rec(memlockUnlimited) << " RLIMIT_MEMLOCK unlimited   — cur="
            << (memlockRl.rlim_cur == RLIM_INFINITY ? "unlimited" : std::to_string(memlockRl.rlim_cur) + " bytes") << "\n";
  std::cout << "      Current scheduler      — " << policyName << "\n";
  std::cout << "=============================\n\n";

  const bool critical = hasSysNice && hasIpcLock;
  if (!critical)
    std::cout << "ERROR: Missing critical capabilities. Run as root or grant CAP_SYS_NICE+CAP_IPC_LOCK.\n"
              << "       e.g.: sudo setcap cap_sys_nice,cap_ipc_lock+ep xretractor\n\n";
  if (!hasRTKernel)
    std::cout << "WARN:  Standard kernel detected. Install PREEMPT_RT patch for minimal jitter.\n"
              << "       e.g.: apt install linux-image-rt-amd64  (Debian/Ubuntu)\n\n";
  if (!rtThrottleOff) std::cout << "WARN:  RT throttling active. Disable: echo -1 > /proc/sys/kernel/sched_rt_runtime_us\n\n";

  return critical;
}

bool rtActivate(int priority) {
  bool ok = true;
  // Polityka mlockall (sledztwo ~40 ms, JOURNAL.md 2026-07-18, Fazy 2/3):
  // synchroniczna populacja stron NOWYCH mapowan pod MCL_FUTURE kosztowala ~25 ms
  // przy mmapie segmentu kolejki IPC w watku FIFO (badanie mlock-variant), a pelne
  // MCL_ONFAULT przenosilo koszt zimnego page cache binarki w srodek biegu
  // (badanie engine-shadow-fix, rep1: seria ~20 ms przy pierwszym uruchomieniu po
  // instalacji). Tryb domyslny rozdziela wiec polityki dwoma wywolaniami:
  // ISTNIEJACE mapowania (binarka, sterta) populowane i blokowane od razu -- ten
  // koszt siedzi przed kotwica osi czasu, wiec nie obciaza slotow -- a NOWE
  // mapowania (segmenty kolejek klientow) blokowane leniwie przy dotknieciu.
  // RDB_MLOCKALL pozostaje jako przelacznik diagnostyczny:
  //   onfault (domyslnie) -- mlockall(MCL_CURRENT) + mlockall(MCL_FUTURE|MCL_ONFAULT)
  //   populate            -- dawne MCL_CURRENT|MCL_FUTURE (do pomiarow porownawczych)
  //   off                 -- bez mlockall (WYLACZNIE diagnostycznie; nie-RT-safe)
  const char *mlockEnv = std::getenv("RDB_MLOCKALL");
  std::string_view mlockMode(mlockEnv != nullptr ? mlockEnv : "onfault");
  if (mlockMode == "onfault") {
    if (mlockall(MCL_CURRENT) != 0 || mlockall(MCL_FUTURE | MCL_ONFAULT) != 0) {
      SPDLOG_WARN("mlockall failed: {}", strerror(errno));
      ok = false;
    }
  } else if (mlockMode != "off" && mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    SPDLOG_WARN("mlockall failed: {}", strerror(errno));
    ok = false;
  }
  if (mlockMode != "onfault") SPDLOG_WARN("RDB_MLOCKALL={} (diagnostic mode)", mlockMode);
  struct sched_param sp{};
  sp.sched_priority = priority;
  if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
    SPDLOG_WARN("SCHED_FIFO failed: {}", strerror(errno));
    ok = false;
  }
  return ok;
}

bool rtKeepThreadOffRtCpus(pthread_t handle) {
  // Dlaczego to istnieje. Wątek komunikacyjny (`commandProcessorLoop`) powstaje
  // PRZED `rtActivate`, a `sched_setscheduler(0, …)` dotyczy wyłącznie wątku
  // wołającego — wątek komunikacyjny zostaje więc SCHED_OTHER. Gdy operator
  // przypina CAŁY proces do jednego rdzenia (`taskset -c 3`, zwykle rdzeń
  // izolowany przez `isolcpus`), oba wątki lądują na tym samym rdzeniu. Dopóki
  // pętla przetwarzania mieści się w slocie, wątek RT oddaje rdzeń na czas snu
  // i wszystko działa. Gdy jednak obciążenie przekroczy 100 % slotu, wątek RT
  // jest bez przerwy runnable i wątek komunikacyjny NIE JEST SZEREGOWANY WCALE:
  // klient nie może się zarejestrować, a serwer wygląda na zawieszony.
  //
  // Dławienie RT tego nie ratuje: kolejka RT przypiętego rdzenia pożycza
  // niewykorzystany budżet z pozostałych rdzeni, na których nie ma zadań RT,
  // więc `sched_rt_runtime_us` faktycznie nie odbiera czasu (zmierzone: duty
  // 212 %, klient bez odpowiedzi przez pełne 3 s budżetu).
  //
  // Naprawa: wątek pomocniczy dostaje dopełnienie maski wątku RT. Gdy wątek RT
  // nie jest przypięty, dopełnienie jest puste i nie robimy nic — planista sam
  // rozłoży wątki i zagłodzenia nie ma.
  cpu_set_t rtCpus;
  CPU_ZERO(&rtCpus);
  if (sched_getaffinity(0, sizeof(rtCpus), &rtCpus) != 0) {
    std::cout << "[WARN] RT: could not read RT thread affinity: " << strerror(errno) << "\n";
    return false;
  }

  const long online = sysconf(_SC_NPROCESSORS_ONLN);
  if (online <= 0) {
    std::cout << "[WARN] RT: could not determine the number of online cores\n";
    return false;
  }

  cpu_set_t auxCpus;
  CPU_ZERO(&auxCpus);
  for (long cpu = 0; cpu < online && cpu < CPU_SETSIZE; ++cpu)
    if (!CPU_ISSET(static_cast<int>(cpu), &rtCpus)) CPU_SET(static_cast<int>(cpu), &auxCpus);

  if (CPU_COUNT(&auxCpus) == 0) {
    // Wątek RT widzi wszystkie rdzenie — nie ma dokąd przenieść, i nie trzeba.
    return false;
  }

  if (const int rc = pthread_setaffinity_np(handle, sizeof(auxCpus), &auxCpus); rc != 0) {
    std::cout << "[WARN] RT: could not move the comms thread off the RT cores: " << strerror(rc) << "\n";
    return false;
  }

  // Komunikat na stdout, nie przez spdlog: w Release SPDLOG_ACTIVE_LEVEL to
  // SPDLOG_LEVEL_ERROR, więc SPDLOG_WARN/INFO znikają na etapie kompilacji, a
  // zmiana powinowactwa musi być widoczna w logu przebiegu pomiarowego.
  std::cout << "[INFO] RT: comms thread moved off the RT cores (auxiliary cores: " << CPU_COUNT(&auxCpus) << ")\n";
  return true;
}

void rtAbsoluteSleep(const struct timespec &anchor, long interval_ms) {
  long ns           = interval_ms * kNsPerMs;
  struct timespec t = anchor;
  t.tv_sec += ns / kNsPerSec;
  t.tv_nsec += ns % kNsPerSec;
  if (t.tv_nsec >= kNsPerSec) {
    t.tv_sec++;
    t.tv_nsec -= kNsPerSec;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
}
