#include "osPlatform.hpp"

#include <unistd.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include "platformConfig.h"

#if RDB_HAS_PROCFS
#include <fstream>
#include <sstream>
#endif

#if RDB_HAS_SYSCTL_KERN_PROC
#include <sys/types.h>

#include <sys/sysctl.h>

#include <cstddef>
#endif

namespace osplat {

namespace {

#if RDB_HAS_PROCFS

/// Pole 22 (`starttime`) i pole 3 (`state`) z /proc/<pid>/stat w JEDNYM odczycie.
///
/// Pole 2 (`comm`) jest w nawiasach i moze zawierac spacje oraz nawiasy, wiec
/// parsowanie zaczyna sie od OSTATNIEGO ')' w linii. Za nim stoi pole 3, a
/// starttime jest polem 22 - czyli dziewietnastym tokenem za stanem.
constexpr int kProcStatStartTimeField = 22;

ProcessSnapshot inspectViaProcFs(std::int32_t pid) {
  ProcessSnapshot retVal;

  std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
  if (!stat.is_open()) return retVal;

  std::string line;
  if (!std::getline(stat, line)) return retVal;

  const auto lastParen = line.rfind(')');
  if (lastParen == std::string::npos) return retVal;

  std::istringstream fields(line.substr(lastParen + 1));
  std::string token;
  char state = '\0';
  for (int index = 3; index < kProcStatStartTimeField; ++index) {
    if (!(fields >> token)) return retVal;
    if (index == 3 && !token.empty()) state = token.front();
  }

  std::uint64_t startTime{0};
  if (!(fields >> startTime)) return retVal;

  retVal.found     = true;
  retVal.zombie    = (state == 'Z');
  retVal.startTime = startTime;
  return retVal;
}

#endif  // RDB_HAS_PROCFS

#if RDB_HAS_SYSCTL_KERN_PROC

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000ULL;

/// Odpowiednik odczytu /proc/<pid>/stat na jadrach BSD-owych: jedno wywolanie
/// sysctl po wpis procesu w tablicy jadra.
///
/// Dwa zachowania, ktore odrozniaja to od Linuksa i sa tu istotne:
///   - dla nieistniejacego PID-u sysctl KONCZY SIE SUKCESEM i zwraca len == 0,
///     wiec sam kod powrotu nie wystarcza do rozstrzygniecia "nie ma procesu";
///   - `p_starttime` jest czasem SCIENNYM (timeval), a nie taktami od startu
///     jadra. Wartosc i tak jest porownywana wylacznie na rownosc, wiec jednostka
///     nie ma znaczenia - liczy sie, ze jest stala dla jednej inkarnacji PID-u.
ProcessSnapshot inspectViaSysctl(std::int32_t pid) {
  ProcessSnapshot retVal;

  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(pid)};
  struct kinfo_proc entry{};
  std::size_t length = sizeof(entry);

  if (sysctl(mib, 4, &entry, &length, nullptr, 0) != 0) return retVal;
  if (length == 0) return retVal;  // PID nieznany jadru

  retVal.found     = true;
  retVal.zombie    = (entry.kp_proc.p_stat == SZOMB);
  retVal.startTime = static_cast<std::uint64_t>(entry.kp_proc.p_starttime.tv_sec) * kMicrosecondsPerSecond +
                     static_cast<std::uint64_t>(entry.kp_proc.p_starttime.tv_usec);

  // Znacznik rowny zeru znaczylby "nie ustalono" - patrz kontrakt ProcessSnapshot.
  // Proces uruchomiony dokladnie w epoce jest niemozliwy, ale gdyby jadro oddalo
  // same zera, lepiej zglosic brak pomiaru niz wartosc, ktora zawsze sie zgadza.
  if (retVal.startTime == 0) retVal.found = false;
  return retVal;
}

#endif  // RDB_HAS_SYSCTL_KERN_PROC

#if RDB_HAS_SYSTEMD

/// Nazwa jednostki systemd z /proc/self/cgroup. systemd umieszcza jednostke w
/// sciezce cgroup typu ".../system.slice/xretractor.service" lub (dla --user)
/// ".../user.slice/user@1000.service/.../xretractor.service".
ServiceIdentity detectSystemd() {
  ServiceIdentity retVal;

  std::ifstream cgroup("/proc/self/cgroup");
  if (!cgroup.is_open()) return retVal;

  std::string line;
  while (std::getline(cgroup, line)) {
    // Format: "hierarchy:controllers:path" (v2: "0::/...path"). Interesuje nas ostatnie pole.
    const auto lastColon = line.rfind(':');
    if (lastColon == std::string::npos) continue;
    std::string_view path(line);
    path.remove_prefix(lastColon + 1);

    // Zakres user, gdy sciezka cgroup biegnie przez user.slice / user@<uid>.service.
    const bool userScope = path.find("/user.slice") != std::string_view::npos || path.find("/user@") != std::string_view::npos;

    // Ostatni (najglebszy) segment sciezki konczacy sie na ".service" jest nazwa naszego unitu;
    // pomijamy user@<uid>.service, ktory jest menedzerem sesji, nie nasza jednostka.
    std::string_view scan = path;
    while (!scan.empty()) {
      const auto slash     = scan.rfind('/');
      std::string_view seg = (slash == std::string_view::npos) ? scan : scan.substr(slash + 1);
      if (seg.ends_with(".service") && !seg.starts_with("user@")) {
        retVal.unit      = std::string(seg);
        retVal.userScope = userScope;
        return retVal;
      }
      if (slash == std::string_view::npos) break;
      scan = scan.substr(0, slash);
    }
  }
  return retVal;
}

#endif  // RDB_HAS_SYSTEMD

#if RDB_HAS_LAUNCHD

/// Tozsamosc zadania launchd.
///
/// launchd nie udostepnia niczego w rodzaju /proc/self/cgroup: nie ma pliku,
/// z ktorego proces odczytalby swoja etykiete. Udostepnia za to SRODOWISKO -
/// zadaniu, ktore sam uruchomil, wstawia XPC_SERVICE_NAME rowne jego etykiecie
/// (`Label` z pliku .plist).
///
/// Sama ta zmienna NIE WYSTARCZA i to jest tu najwazniejsze. Srodowisko jest
/// DZIEDZICZONE, a launchd uruchamia takze aplikacje z interfejsem graficznym -
/// wiec kazdy program wywolany z terminala odpalonego z Findera widzi
/// XPC_SERVICE_NAME swojego dziadka (postaci "application.<id>.<...>"). Sam
/// odczyt zmiennej meldowalby wtedy tryb uslugowy dla zwyklego uruchomienia
/// z powloki, czyli dokladnie odwrotnie niz trzeba.
///
/// Stad trzy warunki naraz:
///   1. zmienna jest ustawiona i rozna od "0" (tyle daje launchd zadaniom bez etykiety),
///   2. nie zaczyna sie od "application." - to znacznik aplikacji GUI, nie uslugi,
///   3. rodzicem procesu jest launchd (PID 1) - zadanie LaunchDaemon/LaunchAgent jest
///      jego BEZPOSREDNIM potomkiem, a proces z powloki nigdy nie jest.
/// Warunek 3 jest tym, ktory odcina dziedziczenie: bez niego punkty 1-2 przepuszczaja
/// kazdy program uruchomiony z aplikacji GUI.
///
/// Zakres ustalamy z efektywnego UID-a, i jest to HEURYSTYKA, nie odczyt:
/// LaunchDaemons startuja z domeny systemowej (root), LaunchAgents z domeny
/// uzytkownika. Daemon z jawnym `UserName` innym niz root zostanie wiec opisany
/// jako uzytkownika. Cena pomylki jest ograniczona - zakres wybiera wylacznie
/// domene dla `launchctl` (system/<label> kontra gui/<uid>/<label>).
ServiceIdentity detectLaunchd() {
  ServiceIdentity retVal;

  const char *label = std::getenv("XPC_SERVICE_NAME");
  if (label == nullptr) return retVal;

  const std::string_view name(label);
  if (name.empty() || name == "0") return retVal;
  if (name.starts_with("application.")) return retVal;
  if (getppid() != 1) return retVal;

  retVal.unit      = std::string(name);
  retVal.userScope = (geteuid() != 0);
  return retVal;
}

#endif  // RDB_HAS_LAUNCHD

}  // namespace

ProcessSnapshot inspectProcess(std::int32_t pid) {
  if (pid <= 0) return {};
#if RDB_HAS_PROCFS
  return inspectViaProcFs(pid);
#elif RDB_HAS_SYSCTL_KERN_PROC
  return inspectViaSysctl(pid);
#else
#error "Brak drogi odczytu wpisu procesu - patrz platformConfig.h"
#endif
}

ServiceIdentity detectServiceIdentity() {
#if RDB_HAS_SYSTEMD
  return detectSystemd();
#elif RDB_HAS_LAUNCHD
  return detectLaunchd();
#else
  return {};  // system bez menedzera uslug, ktory znamy: kazdy proces jest zwykly
#endif
}

std::string sharedMemoryBackingPath() {
#if RDB_OS_DARWIN
  // Boost.Interprocess NIE uzywa tu shm_open. macOS ustawia
  // _POSIX_SHARED_MEMORY_OBJECTS na wartosc ujemna, wiec
  // BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS nie jest zdefiniowane i
  // segmenty powstaja jako ZWYKLE PLIKI w katalogu roboczym biblioteki
  // ("/tmp/boost_interprocess/<znacznik startu jadra>"). Mierzymy wiec wolumin
  // pod /tmp, a nie /dev/shm, ktorego na tym systemie nie ma w ogole.
  //
  // Zwracany jest sam "/tmp", a nie pelna sciezka katalogu Boosta: katalog
  // powstaje dopiero przy pierwszym obiekcie IPC, a statvfs opisuje WOLUMIN,
  // wiec nadrzedna sciezka na tym samym woluminie daje ten sam wynik i jest
  // odporna na zmiane ukladu katalogow w Booscie.
  return "/tmp";
#else
  // tmpfs spod shm_open. Sciezka jest szczegolem implementacji libc, wiec sluzy
  // wylacznie za droge ZAPASOWA - pomiar glowny idzie przez fstatvfs na
  // deskryptorze realnie utworzonego obiektu (patrz shmbudget::space).
  return "/dev/shm";
#endif
}

}  // namespace osplat
