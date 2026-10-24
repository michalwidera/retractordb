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

/// Funkcja rtCheckAndPrint powinna:
/// - [executorrt.rt-check-and-print.1~1] na Linuksie wypisywać na stdout raport: CAP_SYS_NICE i CAP_IPC_LOCK (z CapEff albo root) jako wymagania krytyczne [OK]/[FAIL], a PREEMPT_RT (/sys/kernel/realtime == 1), wyłączone dławienie RT (sched_rt_runtime_us == -1) i nieograniczony RLIMIT_MEMLOCK jako zalecenia [OK]/[WARN], z bieżącą polityką szeregowania wątku wołającego i podpowiedziami naprawy.
///   Ograniczenie: raport rzeczywistej konfiguracji hosta; nie wymusza wszystkich wartosci CapEff, /sys i limitow jadra.
/// - [executorrt.rt-check-and-print.2~1] na Linuksie zwracać true wyłącznie przy obu capabilities krytycznych.
///   Ograniczenie: korelacja werdyktu z wierszami; parser wszystkich masek capabilities nie ma wstrzyknietej macierzy.
/// - [executorrt.rt-check-and-print.3~1] bez /proc wypisywać dostępność SCHED_FIFO z zakresem priorytetów, uprzywilejowanie procesu, brak odpowiednika PREEMPT_RT i dławienia RT, RLIMIT_MEMLOCK, bieżącą politykę i ostrzeżenie o czasie rzeczywistym best-effort.
///   Ograniczenie: galaz bez /proc wymaga osobnego wykonania na takiej platformie.
/// - [executorrt.rt-check-and-print.4~1] bez /proc zwracać true wyłącznie wtedy, gdy jądro oferuje SCHED_FIFO.
///   Ograniczenie: galaz bez /proc nie wykonuje sie na Linuksie.
///
/// @note Poza Linuksem wiersze maja celowo inne nazwy: wiersz "CAP_SYS_NICE" na systemie bez capabilities
///       bylby nieprawda w formacie raportu zgodnosci.
bool rtCheckAndPrint();

/// Funkcja rtActivate powinna:
/// - [executorrt.rt-activate.1~1] blokować pamięć według RDB_MLOCKALL: "onfault" (domyślnie) - mlockall(MCL_CURRENT), a dopiero po jego powodzeniu MCL_FUTURE | MCL_ONFAULT (bez MCL_ONFAULT samo MCL_CURRENT); "off" - bez blokowania; każda inna wartość - MCL_CURRENT | MCL_FUTURE.
/// - [executorrt.rt-activate.2~1] dla wartości RDB_MLOCKALL innej niż "onfault" logować ostrzeżenie o trybie diagnostycznym.
/// - [executorrt.rt-activate.3~1] traktować ENOSYS z mlockall i brak mlockall na platformie jako powodzenie, a inny błąd mlockall logować ostrzeżeniem i zwracać false.
/// - [executorrt.rt-activate.4~1] ustawiać wątkowi wołającemu (nie całemu procesowi) politykę SCHED_FIFO z priorytetem sprowadzonym do zakresu przyjmowanego przez jądro, z ostrzeżeniem przy zmianie wartości.
/// - [executorrt.rt-activate.5~1] porażkę ustawienia SCHED_FIFO albo brak takiej możliwości logować ostrzeżeniem i zwracać false.
/// - [executorrt.rt-activate.6~1] zwracać true, gdy blokowanie pamięci i SCHED_FIFO się powiodły.
///
/// @note Sledztwo ~40 ms (JOURNAL.md 2026-07-18): MCL_FUTURE z populacja kosztowalo ~25 ms na mmapie kolejki
///       IPC w watku FIFO, a pelne MCL_ONFAULT przenosilo koszt zimnego page cache binarki w srodek biegu.
///       MCL_FUTURE tylko po udanym MCL_CURRENT: inaczej przy niskim RLIMIT_MEMLOCK nowe mapowania i
///       pthread_create koncza sie EAGAIN (2026-09-19). ENOSYS to brak funkcji w jadrze (Darwin), nie brak
///       uprawnien. Zakresy SCHED_FIFO: Linux 1..99, jadra BSD-owe zwykle 15..47.
bool rtActivate(int priority = appcfg::kDefaultSchedulingRtPriority);

/// Funkcja rtSlotDeadlineMs powinna:
/// - [executorrt.rt-slot-deadline-ms.1~1] zwracać termin slotu w milisekundach od kotwicy: licznik slotSeconds razy 1000 podzielony całkowicie przez mianownik, w typie long, z obciętym ułamkiem ms.
///
/// @note `rational<int> * 1000` po cichu przepelnia licznik, gdy os przekroczy 2^31 ms (~24,9 dnia), a przy
///       mianowniku niepodzielnym przez 1000 duzo wczesniej (1/360 s: po ~2,8 dnia). Sen wzgledny znosil to,
///       bo liczyl tylko roznice terminow; sen absolutny potrzebuje calej wartosci.
long rtSlotDeadlineMs(const boost::rational<int> &slotSeconds);

/// Funkcja rtAbsoluteSleep powinna:
/// - [executorrt.rt-absolute-sleep.1~1] spać do terminu anchor + interval_ms na zegarze CLOCK_MONOTONIC, a przy terminie, który już minął, wracać od razu.
/// - [executorrt.rt-absolute-sleep.2~1] zwracać false wyłącznie wtedy, gdy sygnał przerwał sen przed terminem, a w każdym innym przypadku true.
/// - [executorrt.rt-absolute-sleep.3~1] przy ponownym wywołaniu z tymi samymi argumentami czekać do tego samego terminu, bez wyznaczania nowego okresu.
/// - [executorrt.rt-absolute-sleep.4~1] bez clock_nanosleep (Darwin) przeliczać czas pozostały do terminu według CLOCK_MONOTONIC na oś mach_absolute_time i spać mach_wait_until, a bez obu mechanizmów spać względnie o pozostały czas.
///   Nietestowalne: kontrola strukturalna galezi RDB_HAS_CLOCK_NANOSLEEP/RDB_HAS_MACH_TIME_H w executor_rt.cpp; testy snu wykonuja tylko galaz aktywna na danej platformie, wymagany osobny Darwin.
///
/// @note Petla slotow spi tak w kazdym trybie taktowanym, nie tylko z --realtime: termin od stalej kotwicy
///       sprawia, ze czas pracy slotu nie przesuwa kolejnych terminow. Na Darwinie CLOCK_MONOTONIC tyka takze
///       w uspieniu, a mach_absolute_time nie - termin przeniesiony wprost zawieszal ut_executor_rt.
bool rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

/// Funkcja rtKeepThreadOffRtCpus powinna:
/// - [executorrt.rt-keep-thread-off-rt-cpus.1~1] przy maskach powinowactwa ustawiać wątkowi handle dopełnienie maski powinowactwa wątku wołającego w obrębie rdzeni online, wypisywać na stdout komunikat [INFO] z liczbą rdzeni pomocniczych i zwracać true.
/// - [executorrt.rt-keep-thread-off-rt-cpus.2~1] nic nie zmieniać i zwracać false, gdy dopełnienie jest puste, bo wątek wołający widzi wszystkie rdzenie.
/// - [executorrt.rt-keep-thread-off-rt-cpus.3~2] przy błędzie odczytu maski, liczby rdzeni online albo ustawienia powinowactwa innym niż EINVAL wypisywać [WARN] na stdout i zwracać false.
/// - [executorrt.rt-keep-thread-off-rt-cpus.4~1] na jądrach bez masek powinowactwa zawsze zwracać false, wypisując na stdout jeden komunikat [INFO] na proces.
///   Ograniczenie: ta galaz jest skompilowana wylacznie bez masek powinowactwa; wykonanie linuksowe jej nie weryfikuje.
/// - [executorrt.rt-keep-thread-off-rt-cpus.5~1] traktować EINVAL z ustawienia powinowactwa (cpuset nie dopuszcza żadnego rdzenia dopełnienia) jak puste dopełnienie: nic nie wypisywać i zwracać false.
///
/// @note Wolac po `rtActivate`, z uchwytem watku, ktory MUSI byc szeregowany mimo obciazenia watku RT. Watek
///       komunikacyjny powstaje przed rtActivate i zostaje SCHED_OTHER; przy `taskset -c 3` i obciazeniu ponad
///       100 % slotu nie bylby szeregowany wcale, a dlawienie RT tego nie ratuje (zmierzone: duty 212 %).
///       Komunikaty ida na stdout, bo w Release SPDLOG_WARN/INFO sa wycinane w kompilacji.
bool rtKeepThreadOffRtCpus(pthread_t handle);
