#pragma once

// Wpis procesu nieczytelny dla JEDNEGO pid-u - obraz, jaki widzi instancja innego
// uzytkownika, gdy /proc jest zamontowany z hidepid=2 (ProtectProc=invisible) albo
// sysctl odmawia EPERM: wpisu procesu nie ma, a jadro proces zna. Bez roota i bez
// zmiany konfiguracji hosta: podmieniamy odpowiedz jadra na pytanie o istnienie procesu
// (kill(pid, 0) na procfs, sysctl KERN_PROC_PID na BSD) - patrz osPlatform.cpp.
//
// Test, ktory dolacza ten naglowek, potrzebuje -Wl,--wrap=kill (procfs) albo
// -Wl,--wrap=sysctl (BSD) na linii konsolidacji - test/UnitTest/CMakeLists.txt.
// Naglowek definiuje symbole, wiec dolacza go dokladnie jeden plik binarki testu.

#include <sys/types.h>

#include <atomic>
#include <cerrno>
#include <cstddef>

#include "platformConfig.h"
#include "syscallWrap.hpp"

#if RDB_HAS_SYSCTL_KERN_PROC
#include <sys/sysctl.h>
#endif

/// pid, ktorego wpis udajemy nieczytelnym; 0 = bez podmiany. Atomowy, bo czyta go
/// takze watek roszczenia w tescie zamka magistrali.
inline std::atomic<pid_t> unreadablePid{0};

#if RDB_HAS_PROCFS

extern "C" int __real_kill(pid_t pid, int sig);

extern "C" int __wrap_kill(pid_t pid, int sig) {
  if (sig == 0 && pid > 0 && pid == unreadablePid.load()) {
    errno = EPERM;
    return -1;
  }
  return __real_kill(pid, sig);
}

RDB_WRAP_SYSCALL(int, kill, (pid_t pid, int sig), (pid, sig));

#elif RDB_HAS_SYSCTL_KERN_PROC

extern "C" int __real_sysctl(int *name, u_int namelen, void *oldp, std::size_t *oldlenp, void *newp, std::size_t newlen);

extern "C" int __wrap_sysctl(int *name, u_int namelen, void *oldp, std::size_t *oldlenp, void *newp, std::size_t newlen) {
  const pid_t target = unreadablePid.load();
  if (target > 0 && namelen == 4 && name[0] == CTL_KERN && name[1] == KERN_PROC && name[2] == KERN_PROC_PID &&
      name[3] == target) {
    errno = EPERM;
    return -1;
  }
  return __real_sysctl(name, namelen, oldp, oldlenp, newp, newlen);
}

RDB_WRAP_SYSCALL(int, sysctl, (int *name, u_int namelen, void *oldp, std::size_t *oldlenp, void *newp, std::size_t newlen),
                 (name, namelen, oldp, oldlenp, newp, newlen));

#endif
