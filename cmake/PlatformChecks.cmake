# Wykrywanie mozliwosci platformy przez PROBE, nie przez nazwe systemu.
#
# Apple zaleca wprost (Porting UNIX/Linux Applications to macOS, rozdzial
# "Compiling Your Code in OS X"): nie rozgalezaj kodu na `#ifdef __APPLE__`,
# tylko sprawdzaj OBECNOSC funkcji. Powod jest praktyczny - `clock_nanosleep`
# albo `PTHREAD_MUTEX_ROBUST` moga sie na Darwinie pojawic w dowolnym wydaniu, a
# warunek na nazwe systemu nigdy tego nie zauwazy. Probe zauwaza przy pierwszej
# rekonfiguracji.
#
# Wynik trafia do generated/platformConfig.h (src/platformConfig.h.in) jako
# rodzina stalych RDB_HAS_*. Kod uzywa `#if RDB_HAS_X`, nigdy `#ifdef
# __linux__`.
#
# Wyjatki od reguly probe, swiadome: /proc i sysctl KERN_PROC nie sa funkcjami
# biblioteki, tylko kontraktami jadra, wiec dla nich zostaje RDB_OS_LINUX /
# RDB_OS_DARWIN ustawiane nizej z CMAKE_SYSTEM_NAME.

include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)

# Probe kompilacyjne musza widziec rozszerzenia POSIX-owe glibc
# (sched_getaffinity, PTHREAD_MUTEX_ROBUST, MCL_ONFAULT). Bez _GNU_SOURCE czesc
# z nich jest ukryta i probe wyszedlby FALSZYWIE ujemny na Linuksie.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
endif()
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

# --- czas ---------------------------------------------------------------
check_cxx_symbol_exists(clock_nanosleep "ctime" RDB_HAS_CLOCK_NANOSLEEP)
check_include_file_cxx("mach/mach_time.h" RDB_HAS_MACH_TIME_H)

# --- szeregowanie czasu rzeczywistego -----------------------------------
check_cxx_symbol_exists(sched_setscheduler "sched.h" RDB_HAS_SCHED_SETSCHEDULER)
check_cxx_symbol_exists(sched_getaffinity "sched.h" RDB_HAS_SCHED_AFFINITY)
check_cxx_symbol_exists(mlockall "sys/mman.h" RDB_HAS_MLOCKALL)
check_cxx_source_compiles("#include <sys/mman.h>
   int main() { return MCL_ONFAULT; }" RDB_HAS_MCL_ONFAULT)
check_cxx_source_compiles(
  "#include <pthread.h>
   int main() {
     pthread_attr_t attr;
     struct sched_param sp{};
     sp.sched_priority = 1;
     return pthread_attr_init(&attr) + pthread_attr_setschedparam(&attr, &sp) + SCHED_FIFO;
   }"
  RDB_HAS_PTHREAD_SCHEDPARAM)

# --- muteks miedzyprocesowy ---------------------------------------------
# Sprawdzane jako para: sam PTHREAD_MUTEX_ROBUST bez pthread_mutex_consistent
# nie da sie uzyc, a Darwin nie ma zadnego z nich.
check_cxx_source_compiles(
  "#include <pthread.h>
   int main() {
     pthread_mutexattr_t a;
     pthread_mutexattr_init(&a);
     pthread_mutexattr_setrobust(&a, PTHREAD_MUTEX_ROBUST);
     pthread_mutex_t m;
     pthread_mutex_init(&m, &a);
     return pthread_mutex_consistent(&m);
   }"
  RDB_HAS_ROBUST_MUTEX)

# --- procesy ------------------------------------------------------------
check_cxx_source_compiles(
  "#include <sys/types.h>
   #include <sys/sysctl.h>
   int main() {
     int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, 1};
     struct kinfo_proc kp;
     size_t len = sizeof(kp);
     return sysctl(mib, 4, &kp, &len, nullptr, 0);
   }"
  RDB_HAS_SYSCTL_KERN_PROC)

# --- konsolidator -------------------------------------------------------
# --wrap przekierowuje odwolania do symbolu na __wrap_<symbol>, a
# __real_<symbol> na oryginal. Uzywaja tego trzy testy jednostkowe do
# wstrzykiwania EINTR w pread i write. Opcja jest rozszerzeniem GNU ld/lld; ld64
# Apple'a odrzuca ja bledem "unknown option", wiec te trzy binaria trzeba tam
# zlozyc inaczej - patrz test/UnitTest/syscallWrap.hpp.
set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--wrap=rdb_wrap_probe")
check_cxx_source_compiles(
  "int rdb_wrap_probe() { return 0; }\nint main() { return rdb_wrap_probe(); }"
  RDB_HAS_LD_WRAP)
unset(CMAKE_REQUIRED_LINK_OPTIONS)

# --- rozne --------------------------------------------------------------
check_cxx_symbol_exists(pipe2 "unistd.h" RDB_HAS_PIPE2)
check_cxx_symbol_exists(statvfs "sys/statvfs.h" RDB_HAS_STATVFS)

unset(CMAKE_REQUIRED_DEFINITIONS)
unset(CMAKE_REQUIRED_LIBRARIES)

# --- narzedzia zewnetrzne wolane w czasie dzialania ---------------------
# boost::stacktrace z zapleczem ADDR2LINE uruchamia ten program przy kazdym
# zbieraniu sladu. Bez niego zaplecze musi byc inne - patrz
# src/rdb/lib/payload.cc.
find_program(PROG_ADDR2LINE addr2line)
if(PROG_ADDR2LINE)
  set(RDB_HAS_ADDR2LINE 1)
else()
  set(RDB_HAS_ADDR2LINE 0)
endif()
message(STATUS "RDB_HAS_ADDR2LINE=${RDB_HAS_ADDR2LINE}")

# --- kontrakty jadra, nie funkcje biblioteki ----------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(RDB_OS_LINUX 1)
  set(RDB_OS_DARWIN 0)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(RDB_OS_LINUX 0)
  set(RDB_OS_DARWIN 1)
else()
  set(RDB_OS_LINUX 0)
  set(RDB_OS_DARWIN 0)
  message(
    WARNING
      "Nieznany system '${CMAKE_SYSTEM_NAME}': zywotnosc procesow i tozsamosc uslugi beda dzialac w trybie zdegradowanym."
  )
endif()

# systemd istnieje wylacznie na Linuksie; launchd wylacznie na Darwinie.
set(RDB_HAS_SYSTEMD ${RDB_OS_LINUX})
set(RDB_HAS_LAUNCHD ${RDB_OS_DARWIN})

# /proc jest interfejsem jadra Linuksa. Darwin ma na to sysctl (probe wyzej).
set(RDB_HAS_PROCFS ${RDB_OS_LINUX})

foreach(
  _rdb_feature
  RDB_HAS_CLOCK_NANOSLEEP
  RDB_HAS_MACH_TIME_H
  RDB_HAS_SCHED_SETSCHEDULER
  RDB_HAS_SCHED_AFFINITY
  RDB_HAS_MLOCKALL
  RDB_HAS_MCL_ONFAULT
  RDB_HAS_PTHREAD_SCHEDPARAM
  RDB_HAS_ROBUST_MUTEX
  RDB_HAS_SYSCTL_KERN_PROC
  RDB_HAS_LD_WRAP
  RDB_HAS_PIPE2
  RDB_HAS_STATVFS)
  if(${_rdb_feature})
    set(${_rdb_feature} 1)
  else()
    set(${_rdb_feature} 0)
  endif()
  message(STATUS "${_rdb_feature}=${${_rdb_feature}}")
endforeach()
