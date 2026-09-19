#pragma once

// Moduł czasu rzeczywistego: SCHED_FIFO, blokowanie stron, powinowactwo CPU, sen absolutny.
//
// Interfejs jest wspólny dla wszystkich platform, implementacja nie: każda z czterech
// funkcji ma w executor_rt.cpp gałąź dobieraną przez RDB_HAS_* z platformConfig.h.
// Tam, gdzie jądro nie ma odpowiednika (maski powinowactwa poza Linuksem, PREEMPT_RT),
// funkcja mówi wprost, że nic nie zrobiła, zamiast udawać sukces - patrz komentarze
// przy poszczególnych gałęziach.

#include <pthread.h>

#include <ctime>

#include "appConfig.hpp"

bool rtCheckAndPrint();
bool rtActivate(int priority = appcfg::kDefaultSchedulingRtPriority);
void rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

/// Przenosi wątek pomocniczy poza rdzenie, na których pracuje wątek czasu
/// rzeczywistego (czyli poza maskę powinowactwa WOŁAJĄCEGO wątku).
///
/// Wołać po `rtActivate`, z uchwytem wątku, który MUSI być szeregowany mimo
/// obciążenia wątku RT. Zwraca `true`, gdy powinowactwo zostało zmienione.
/// Gdy wątek RT nie jest przypięty do podzbioru rdzeni, dopełnienie jest puste
/// i funkcja nie robi nic - bez przypięcia planista i tak rozłoży wątki.
///
/// Na jądrach bez masek powinowactwa zwraca `false` zawsze: nie ma tam czego
/// przestawić, a zagłodzenia też nie ma, bo `rtActivate` podnosi wtedy priorytet
/// samego wątku wołającego, nie całego procesu.
bool rtKeepThreadOffRtCpus(pthread_t handle);
