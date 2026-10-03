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

#include <boost/rational.hpp>

#include "appConfig.hpp"

bool rtCheckAndPrint();
bool rtActivate(int priority = appcfg::kDefaultSchedulingRtPriority);

/// Termin slotu w milisekundach od kotwicy, z czasu logicznego slotu w sekundach (ułamek ms
/// obcięty). Liczony w 64 bitach: `rational<int> * 1000` po cichu przepełnia licznik, gdy oś
/// przekroczy 2^31 ms (~24,9 dnia), a przy mianowniku niepodzielnym przez 1000 dużo wcześniej
/// (1/360 s: po ~2,8 dnia). Sen względny znosił to, bo liczył tylko różnice terminów; sen
/// absolutny potrzebuje całej wartości.
long rtSlotDeadlineMs(const boost::rational<int> &slotSeconds);

/// Sen do terminu `anchor + interval_ms` na CLOCK_MONOTONIC. Pętla slotów śpi tak w każdym
/// trybie taktowanym, nie tylko z --realtime: termin liczony od stałej kotwicy sprawia, że
/// czas pracy slotu nie przesuwa kolejnych terminów. Termin, który już minął, nie usypia.
///
/// Zwraca `false`, gdy sygnał przerwał sen przed terminem. Ponowne wywołanie z tymi samymi
/// argumentami czeka do TEGO SAMEGO terminu - nie wyznacza nowego okresu.
bool rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

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
