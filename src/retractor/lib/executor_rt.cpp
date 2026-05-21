#ifdef __linux__

#include "executor_rt.hpp"

#include <spdlog/spdlog.h>

#include <cinttypes>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

static std::string rtReadFile(const char* path) {
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
    if (line.rfind("CapEff:", 0) == 0) {
      uint64_t caps = 0;
      std::sscanf(line.c_str() + 7, "%" SCNx64, &caps);
      return caps;
    }
  }
  return 0;
}

bool rtCheckAndPrint() {
  const uint64_t caps    = rtEffectiveCapabilities();
  const bool isRoot      = (geteuid() == 0);
  const bool hasSysNice  = isRoot || ((caps >> 23) & 1u);  // CAP_SYS_NICE
  const bool hasIpcLock  = isRoot || ((caps >> 14) & 1u);  // CAP_IPC_LOCK

  const std::string rtKernelVal   = rtReadFile("/sys/kernel/realtime");
  const bool hasRTKernel          = (rtKernelVal == "1");

  const std::string rtThrottleVal = rtReadFile("/proc/sys/kernel/sched_rt_runtime_us");
  const bool rtThrottleOff        = (rtThrottleVal == "-1");

  struct rlimit memlockRl{};
  getrlimit(RLIMIT_MEMLOCK, &memlockRl);
  const bool memlockUnlimited = (memlockRl.rlim_cur == RLIM_INFINITY);

  const int curPolicy      = sched_getscheduler(0);
  const char* policyName   = (curPolicy == SCHED_FIFO)    ? "SCHED_FIFO"
                             : (curPolicy == SCHED_RR)    ? "SCHED_RR"
                             : (curPolicy == SCHED_OTHER) ? "SCHED_OTHER"
                                                          : "unknown";

  auto ok  = [](bool v) { return v ? "[OK]  " : "[FAIL]"; };
  auto rec = [](bool v) { return v ? "[OK]  " : "[WARN]"; };

  std::cout << "\n=== RT requirements check ===\n";
  std::cout << ok(hasSysNice)        << " CAP_SYS_NICE / root        — required for SCHED_FIFO\n";
  std::cout << ok(hasIpcLock)        << " CAP_IPC_LOCK / root        — required for mlockall\n";
  std::cout << rec(hasRTKernel)      << " PREEMPT_RT kernel          — /sys/kernel/realtime="
            << (rtKernelVal.empty() ? "missing" : rtKernelVal) << "\n";
  std::cout << rec(rtThrottleOff)    << " RT throttling disabled     — sched_rt_runtime_us="
            << (rtThrottleVal.empty() ? "missing" : rtThrottleVal)
            << (rtThrottleOff ? "" : "  (set to -1 to disable throttling)") << "\n";
  std::cout << rec(memlockUnlimited) << " RLIMIT_MEMLOCK unlimited   — cur="
            << (memlockRl.rlim_cur == RLIM_INFINITY ? "unlimited"
                                                    : std::to_string(memlockRl.rlim_cur) + " bytes") << "\n";
  std::cout << "      Current scheduler      — " << policyName << "\n";
  std::cout << "=============================\n\n";

  const bool critical = hasSysNice && hasIpcLock;
  if (!critical)
    std::cout << "ERROR: Missing critical capabilities. Run as root or grant CAP_SYS_NICE+CAP_IPC_LOCK.\n"
              << "       e.g.: sudo setcap cap_sys_nice,cap_ipc_lock+ep xretractor\n\n";
  if (!hasRTKernel)
    std::cout << "WARN:  Standard kernel detected. Install PREEMPT_RT patch for minimal jitter.\n"
              << "       e.g.: apt install linux-image-rt-amd64  (Debian/Ubuntu)\n\n";
  if (!rtThrottleOff)
    std::cout << "WARN:  RT throttling active. Disable: echo -1 > /proc/sys/kernel/sched_rt_runtime_us\n\n";

  return critical;
}

bool rtActivate() {
  bool ok = true;
  if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    SPDLOG_WARN("mlockall failed: {}", strerror(errno));
    ok = false;
  }
  struct sched_param sp{};
  sp.sched_priority = 50;
  if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
    SPDLOG_WARN("SCHED_FIFO failed: {}", strerror(errno));
    ok = false;
  }
  return ok;
}

void rtAbsoluteSleep(const struct timespec& anchor, long interval_ms) {
  long ns       = interval_ms * 1'000'000L;
  struct timespec t = anchor;
  t.tv_sec  += ns / 1'000'000'000L;
  t.tv_nsec += ns % 1'000'000'000L;
  if (t.tv_nsec >= 1'000'000'000L) {
    t.tv_sec++;
    t.tv_nsec -= 1'000'000'000L;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
}

#endif  // __linux__
