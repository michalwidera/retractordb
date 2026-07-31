#pragma once

#ifdef __linux__

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
/// i funkcja nie robi nic — bez przypięcia planista i tak rozłoży wątki.
bool rtKeepThreadOffRtCpus(pthread_t handle);

#endif  // __linux__
