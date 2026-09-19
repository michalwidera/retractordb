#include "executor_rt.hpp"

#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

#include "platformConfig.h"

#if RDB_HAS_PROCFS
#include <fstream>
#endif

#if !RDB_HAS_CLOCK_NANOSLEEP && RDB_HAS_MACH_TIME_H
#include <mach/mach_time.h>
#endif

// Stałe konwersji czasu - używane przy obliczaniu timespec dla snu absolutnego.
constexpr long kNsPerMs  = 1'000'000L;      // nanosekundy na milisekundę
constexpr long kNsPerSec = 1'000'000'000L;  // nanosekundy na sekundę

namespace {

/// Wypis zgodnosci: dwa poziomy istotnosci, ten sam szeroki na szesc znakow prefiks.
const char *ok(bool v) { return v ? "[OK]  " : "[FAIL]"; }
const char *rec(bool v) { return v ? "[OK]  " : "[WARN]"; }

/// Priorytet SCHED_FIFO sprowadzony do zakresu, ktory ten planista naprawde przyjmuje.
///
/// Zakresy roznia sie miedzy jadrami: Linux daje SCHED_FIFO 1..99, jadra BSD-owe
/// (w tym Darwin) zwykle 15..47. Domyslne 50 z appcfg mieści sie w pierwszym i NIE
/// mieści w drugim, gdzie dawalo EINVAL - czyli ciche zejscie do SCHED_OTHER na
/// kazdym uruchomieniu. Ograniczenie zakresem jest bezczynne tam, gdzie wartosc i tak
/// jest poprawna, wiec na Linuksie nie zmienia niczego.
int clampRtPriority(int priority) {
  const int lowest  = sched_get_priority_min(SCHED_FIFO);
  const int highest = sched_get_priority_max(SCHED_FIFO);
  if (lowest < 0 || highest < 0) return priority;
  const int clamped = std::clamp(priority, lowest, highest);
  if (clamped != priority)
    SPDLOG_WARN("SCHED_FIFO priority {} out of range [{}, {}] on this kernel, using {}", priority, lowest, highest, clamped);
  return clamped;
}

/// Nazwa biezacej polityki szeregowania. Na Linuksie pytamy o CALY proces
/// (sched_getscheduler), gdzie indziej o WOLAJACY WATEK (pthread_getschedparam) -
/// bo tam polityka jest wlasnoscia watku i procesowego odpowiednika po prostu nie ma.
const char *currentSchedulerName() {
#if RDB_HAS_SCHED_SETSCHEDULER
  const int policy = sched_getscheduler(0);
#else
  int policy = SCHED_OTHER;
  struct sched_param sp{};
  if (pthread_getschedparam(pthread_self(), &policy, &sp) != 0) return "unknown";
#endif
  if (policy == SCHED_FIFO) return "SCHED_FIFO";
  if (policy == SCHED_RR) return "SCHED_RR";
  if (policy == SCHED_OTHER) return "SCHED_OTHER";
  return "unknown";
}

std::string memlockLimitText(const struct rlimit &limit) {
  return limit.rlim_cur == RLIM_INFINITY ? std::string("unlimited") : std::to_string(limit.rlim_cur) + " bytes";
}

#if RDB_HAS_PROCFS

// Pozycje bitów w CapEff (linux/capability.h) - wartości standardu POSIX.1e.
// Bity w /proc/self/status CapEff odpowiadają numerom capability z <sys/capability.h>.
constexpr int kCapSysNiceBit         = 23;  // CAP_SYS_NICE  - wymagane do SCHED_FIFO
constexpr int kCapIpcLockBit         = 14;  // CAP_IPC_LOCK  - wymagane do mlockall
constexpr size_t kCapEffPrefixLength = 7;

std::string rtReadFile(const char *path) {
  std::ifstream f(path);
  if (!f) return {};
  std::string v;
  std::getline(f, v);
  return v;
}

uint64_t rtEffectiveCapabilities() {
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

/// Wypis zgodnosci dla jadra Linuksa: capabilities, PREEMPT_RT, dlawienie RT.
bool checkLinux() {
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

  std::cout << "\n=== RT requirements check ===\n";
  std::cout << ok(hasSysNice) << " CAP_SYS_NICE / root        - required for SCHED_FIFO\n";
  std::cout << ok(hasIpcLock) << " CAP_IPC_LOCK / root        - required for mlockall\n";
  std::cout << rec(hasRTKernel)
            << " PREEMPT_RT kernel          - /sys/kernel/realtime=" << (rtKernelVal.empty() ? "missing" : rtKernelVal) << "\n";
  std::cout << rec(rtThrottleOff)
            << " RT throttling disabled     - sched_rt_runtime_us=" << (rtThrottleVal.empty() ? "missing" : rtThrottleVal)
            << (rtThrottleOff ? "" : "  (set to -1 to disable throttling)") << "\n";
  std::cout << rec(memlockUnlimited) << " RLIMIT_MEMLOCK unlimited   - cur=" << memlockLimitText(memlockRl) << "\n";
  std::cout << "      Current scheduler      - " << currentSchedulerName() << "\n";
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

#else

/// Wypis zgodnosci dla jader bez capabilities POSIX.1e i bez /proc.
///
/// Nazwy wierszy sa tu INNE niz na Linuksie i to jest celowe: wiersz
/// "CAP_SYS_NICE" na systemie, ktory nie ma capabilities, bylby nieprawda
/// podana w formacie raportu zgodnosci. Zachowana jest za to ramka i te wiersze,
/// ktore maja sens wszedzie - limit blokowania pamieci i biezaca polityka.
///
/// Co realnie daje sie tu sprawdzic:
///   - polityke czasu rzeczywistego dla WATKU (pthread_setschedparam) - jest zawsze,
///     ale wysokie priorytety wymagaja uprawnien, wiec sprawdzamy realny zakres;
///   - RLIMIT_MEMLOCK - istnieje tak samo jak na Linuksie;
///   - blokowanie CALEJ przestrzeni adresowej - patrz rtActivate, bywa niezaimplementowane.
/// Czego sprawdzic sie nie da, bo nie istnieje: jadra PREEMPT_RT i dlawienia RT.
bool checkGeneric() {
  struct rlimit memlockRl{};
  getrlimit(RLIMIT_MEMLOCK, &memlockRl);
  const bool memlockUnlimited = (memlockRl.rlim_cur == RLIM_INFINITY);

  const int lowest      = sched_get_priority_min(SCHED_FIFO);
  const int highest     = sched_get_priority_max(SCHED_FIFO);
  const bool rtPolicyOk = (lowest >= 0 && highest >= lowest);

  std::cout << "\n=== RT requirements check ===\n";
  std::cout << ok(rtPolicyOk) << " SCHED_FIFO available       - thread RT policy, priority range " << lowest << ".." << highest
            << "\n";
  std::cout << rec(geteuid() == 0) << " Privileged process         - high RT priorities usually need root\n";
  std::cout << rec(false) << " Real-time kernel           - not available: this kernel has no PREEMPT_RT equivalent\n";
  std::cout << rec(false) << " RT throttling disabled     - not applicable: this kernel does not throttle RT tasks\n";
  std::cout << rec(memlockUnlimited) << " RLIMIT_MEMLOCK unlimited   - cur=" << memlockLimitText(memlockRl) << "\n";
  std::cout << "      Current scheduler      - " << currentSchedulerName() << "\n";
  std::cout << "=============================\n\n";

  if (!rtPolicyOk) std::cout << "ERROR: SCHED_FIFO is not offered by this kernel; the RT loop cannot be prioritised.\n\n";
  std::cout << "WARN:  Best-effort real time only. This kernel gives no deadline guarantee comparable to PREEMPT_RT.\n\n";

  return rtPolicyOk;
}

#endif  // RDB_HAS_PROCFS

}  // namespace

bool rtCheckAndPrint() {
#if RDB_HAS_PROCFS
  return checkLinux();
#else
  return checkGeneric();
#endif
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

#if RDB_HAS_MLOCKALL
  // Niepowodzenie z ENOSYS nie jest bledem tej instalacji, tylko brakiem funkcji
  // w jadrze: mlockall istnieje wtedy jako symbol (wiec probe kompilacyjne je widzi),
  // ale nie robi nic i zglasza "nie zaimplementowano". Darwin zachowuje sie dokladnie
  // tak. Liczenie tego jako bledu sprowadzaloby rtActivate do false na kazdym
  // uruchomieniu, a wolajacy odczytalby to jako brak uprawnien, ktorych nie brakuje.
  const auto lockAll = [&ok](int flags) {
    if (mlockall(flags) == 0) return;
    if (errno == ENOSYS) {
      SPDLOG_DEBUG("mlockall not implemented by this kernel; continuing without locked pages");
      return;
    }
    SPDLOG_WARN("mlockall failed: {}", strerror(errno));
    ok = false;
  };

  if (mlockMode == "onfault") {
#if RDB_HAS_MCL_ONFAULT
    lockAll(MCL_CURRENT);
    lockAll(MCL_FUTURE | MCL_ONFAULT);
#else
    // Bez MCL_ONFAULT zostaje samo MCL_CURRENT: blokujemy to, co juz jest odwzorowane,
    // i NIE wlaczamy MCL_FUTURE, bo to wlasnie ono kosztowalo ~25 ms na mmapie kolejki.
    lockAll(MCL_CURRENT);
#endif
  } else if (mlockMode != "off") {
    lockAll(MCL_CURRENT | MCL_FUTURE);
  }
#else
  if (mlockMode != "off") SPDLOG_DEBUG("mlockall unavailable on this platform; continuing without locked pages");
#endif

  if (mlockMode != "onfault") SPDLOG_WARN("RDB_MLOCKALL={} (diagnostic mode)", mlockMode);

  const int effective = clampRtPriority(priority);
  struct sched_param sp{};
  sp.sched_priority = effective;

#if RDB_HAS_SCHED_SETSCHEDULER
  if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
    SPDLOG_WARN("SCHED_FIFO failed: {}", strerror(errno));
    ok = false;
  }
#else
  // Odpowiednik dla jader, w ktorych polityka szeregowania jest wlasnoscia WATKU,
  // a nie procesu (m.in. Darwin: sched_setscheduler tam nie istnieje). Roznica jest
  // realna i warto ja znac: obejmuje wylacznie watek wolajacy, wiec watek komunikacyjny
  // zostaje przy polityce domyslnej -- co akurat jest tu pozadane, bo dokladnie temu
  // sluzy rtKeepThreadOffRtCpus na Linuksie.
  if (const int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp); rc != 0) {
    SPDLOG_WARN("SCHED_FIFO failed: {}", strerror(rc));
    ok = false;
  }
#endif
  return ok;
}

bool rtKeepThreadOffRtCpus([[maybe_unused]] pthread_t handle) {
#if RDB_HAS_SCHED_AFFINITY
  // Dlaczego to istnieje. Wątek komunikacyjny (`commandProcessorLoop`) powstaje
  // PRZED `rtActivate`, a `sched_setscheduler(0, …)` dotyczy wyłącznie wątku
  // wołającego - wątek komunikacyjny zostaje więc SCHED_OTHER. Gdy operator
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
  // nie jest przypięty, dopełnienie jest puste i nie robimy nic - planista sam
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
    // Wątek RT widzi wszystkie rdzenie - nie ma dokąd przenieść, i nie trzeba.
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
#else
  // Ten system nie ma masek powinowactwa procesora.
  //
  // Najblizszym odpowiednikiem na Darwinie jest thread_policy_set z
  // THREAD_AFFINITY_POLICY, ale to NIE jest przypiecie do rdzenia, tylko podpowiedz
  // dla planisty, ktore watki dzielic po tym samym L2; na Apple Silicon jadro zwraca
  // z niej KERN_NOT_SUPPORTED. Wolanie jej tutaj wygladaloby jak przeniesienie watku
  // i nie bylo by nim, wiec funkcja mowi wprost, ze nic nie zrobila.
  //
  // Zagłodzenia, przed ktorym broni galaz linuksowa, na tym systemie zreszta nie ma:
  // rtActivate podnosi do SCHED_FIFO sam WATEK wolajacy (nie ma tam odpowiednika
  // sched_setscheduler dla calego procesu), wiec watek komunikacyjny i tak zostaje
  // przy polityce domyslnej i jest szeregowany.
  //
  // Komunikat na stdout i tylko RAZ - z tego samego powodu co w galezi wyzej:
  // w Release SPDLOG_WARN jest wycinany w kompilacji, a log przebiegu pomiarowego
  // ma nieść informację, że powinowactwo nie zostało ustawione.
  static bool announced = false;
  if (!announced) {
    announced = true;
    std::cout << "[INFO] RT: CPU affinity is not available on this platform; comms thread left to the scheduler\n";
  }
  return false;
#endif
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

#if RDB_HAS_CLOCK_NANOSLEEP
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
#elif RDB_HAS_MACH_TIME_H
  // Sen ABSOLUTNY bez clock_nanosleep, na jadrach Macha.
  //
  // PULAPKA, przez ktora to juz raz zawislo: mach_wait_until czeka na osi
  // mach_absolute_time, a kotwica przyszla z clock_gettime(CLOCK_MONOTONIC) - i na
  // Darwinie to NIE jest ta sama os. CLOCK_MONOTONIC tyka takze wtedy, gdy maszyna
  // spi (odpowiednik mach_continuous_time), a mach_absolute_time staje na czas
  // uspienia. Roznica miedzy nimi to suma wszystkich uspien od startu systemu -
  // na laptopie godziny. Przeliczony CLOCK_MONOTONIC podstawiony wprost pod
  // mach_wait_until dawal wiec termin oddalony o te godziny i sen nie konczyl sie
  // nigdy (test ut_executor_rt wisial).
  //
  // Liczymy zatem POZOSTALY CZAS wzgledem tego samego zegara, z ktorego pochodzi
  // kotwica, i dopiero ten odcinek przenosimy na os Macha. Wlasnosc, dla ktorej ta
  // funkcja istnieje, zostaje zachowana: kotwica jest stala miedzy slotami, wiec
  // blad kazdego snu liczy sie od niej na nowo i NIE KUMULUJE sie - inaczej niz
  // przy naiwnym "spij interval_ms od teraz".
  static mach_timebase_info_data_t timebase = [] {
    mach_timebase_info_data_t info{};
    mach_timebase_info(&info);
    return info;
  }();
  if (timebase.numer == 0 || timebase.denom == 0) return;

  struct timespec now{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  const std::int64_t remainingNs = (static_cast<std::int64_t>(t.tv_sec) - static_cast<std::int64_t>(now.tv_sec)) * kNsPerSec +
                                   (static_cast<std::int64_t>(t.tv_nsec) - static_cast<std::int64_t>(now.tv_nsec));
  if (remainingNs <= 0) return;  // termin juz minal

  // Mnozenie rozbite na iloraz i reszte, zeby nie przepelnic 64 bitow; wynik jest
  // identyczny jak remainingNs * denom / numer.
  const std::uint64_t left  = static_cast<std::uint64_t>(remainingNs);
  const std::uint64_t numer = timebase.numer;
  const std::uint64_t denom = timebase.denom;
  const std::uint64_t ticks = (left / numer) * denom + ((left % numer) * denom) / numer;
  mach_wait_until(mach_absolute_time() + ticks);
#else
  // Ostatnia droga: sen WZGLEDNY o pozostaly czas. Rozni sie od dwoch powyzszych
  // tym, ze miedzy odczytem zegara a zasnieciem moze wypasc wywlaszczenie i ten
  // kawalek czasu przepada - czyli dryf, ktoremu sen absolutny wlasnie zapobiega.
  // Zostaje jako zabezpieczenie na jadro, ktore nie ma ani jednego, ani drugiego.
  struct timespec now{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  struct timespec delta{.tv_sec = t.tv_sec - now.tv_sec, .tv_nsec = t.tv_nsec - now.tv_nsec};
  if (delta.tv_nsec < 0) {
    delta.tv_sec -= 1;
    delta.tv_nsec += kNsPerSec;
  }
  if (delta.tv_sec < 0) return;  // termin juz minal
  nanosleep(&delta, nullptr);
#endif
}
