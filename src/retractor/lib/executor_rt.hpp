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
/// - na Linuksie wypisywać na stdout raport: CAP_SYS_NICE i CAP_IPC_LOCK (z CapEff albo root) jako wymagania krytyczne [OK]/[FAIL], a PREEMPT_RT (/sys/kernel/realtime == 1), wyłączone dławienie RT (sched_rt_runtime_us == -1) i nieograniczony RLIMIT_MEMLOCK jako zalecenia [OK]/[WARN], z bieżącą polityką szeregowania wątku wołającego i podpowiedziami naprawy.
/// - na Linuksie zwracać true wyłącznie przy obu capabilities krytycznych.
/// - bez /proc wypisywać dostępność SCHED_FIFO z zakresem priorytetów, uprzywilejowanie procesu, brak odpowiednika PREEMPT_RT i dławienia RT, RLIMIT_MEMLOCK, bieżącą politykę i ostrzeżenie o czasie rzeczywistym best-effort.
/// - bez /proc zwracać true wyłącznie wtedy, gdy jądro oferuje SCHED_FIFO.
///
/// @note Poza Linuksem wiersze maja celowo inne nazwy: wiersz "CAP_SYS_NICE" na systemie bez capabilities
///       bylby nieprawda w formacie raportu zgodnosci.
bool rtCheckAndPrint();

/// Funkcja rtActivate powinna:
/// - blokować pamięć według RDB_MLOCKALL: "onfault" (domyślnie) - mlockall(MCL_CURRENT), a dopiero po jego powodzeniu MCL_FUTURE | MCL_ONFAULT (bez MCL_ONFAULT samo MCL_CURRENT); "off" - bez blokowania; każda inna wartość - MCL_CURRENT | MCL_FUTURE.
/// - dla wartości RDB_MLOCKALL innej niż "onfault" logować ostrzeżenie o trybie diagnostycznym.
/// - traktować ENOSYS z mlockall i brak mlockall na platformie jako powodzenie, a inny błąd mlockall logować ostrzeżeniem i zwracać false.
/// - ustawiać wątkowi wołającemu (nie całemu procesowi) politykę SCHED_FIFO z priorytetem sprowadzonym do zakresu przyjmowanego przez jądro, z ostrzeżeniem przy zmianie wartości.
/// - porażkę ustawienia SCHED_FIFO albo brak takiej możliwości logować ostrzeżeniem i zwracać false.
/// - zwracać true, gdy blokowanie pamięci i SCHED_FIFO się powiodły.
///
/// @note Sledztwo ~40 ms (JOURNAL.md 2026-07-18): MCL_FUTURE z populacja kosztowalo ~25 ms na mmapie kolejki
///       IPC w watku FIFO, a pelne MCL_ONFAULT przenosilo koszt zimnego page cache binarki w srodek biegu.
///       MCL_FUTURE tylko po udanym MCL_CURRENT: inaczej przy niskim RLIMIT_MEMLOCK nowe mapowania i
///       pthread_create koncza sie EAGAIN (2026-09-19). ENOSYS to brak funkcji w jadrze (Darwin), nie brak
///       uprawnien. Zakresy SCHED_FIFO: Linux 1..99, jadra BSD-owe zwykle 15..47.
bool rtActivate(int priority = appcfg::kDefaultSchedulingRtPriority);

/// Funkcja rtSlotDeadlineMs powinna:
/// - zwracać termin slotu w milisekundach od kotwicy: licznik slotSeconds razy 1000 podzielony całkowicie przez mianownik, w typie long, z obciętym ułamkiem ms.
///
/// @note `rational<int> * 1000` po cichu przepelnia licznik, gdy os przekroczy 2^31 ms (~24,9 dnia), a przy
///       mianowniku niepodzielnym przez 1000 duzo wczesniej (1/360 s: po ~2,8 dnia). Sen wzgledny znosil to,
///       bo liczyl tylko roznice terminow; sen absolutny potrzebuje calej wartosci.
long rtSlotDeadlineMs(const boost::rational<int> &slotSeconds);

/// Funkcja rtAbsoluteSleep powinna:
/// - spać do terminu anchor + interval_ms na zegarze CLOCK_MONOTONIC, a przy terminie, który już minął, wracać od razu.
/// - zwracać false wyłącznie wtedy, gdy sygnał przerwał sen przed terminem, a w każdym innym przypadku true.
/// - przy ponownym wywołaniu z tymi samymi argumentami czekać do tego samego terminu, bez wyznaczania nowego okresu.
/// - bez clock_nanosleep (Darwin) przeliczać czas pozostały do terminu według CLOCK_MONOTONIC na oś mach_absolute_time i spać mach_wait_until, a bez obu mechanizmów spać względnie o pozostały czas.
///
/// @note Petla slotow spi tak w kazdym trybie taktowanym, nie tylko z --realtime: termin od stalej kotwicy
///       sprawia, ze czas pracy slotu nie przesuwa kolejnych terminow. Na Darwinie CLOCK_MONOTONIC tyka takze
///       w uspieniu, a mach_absolute_time nie - termin przeniesiony wprost zawieszal ut_executor_rt.
bool rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

/// Funkcja rtKeepThreadOffRtCpus powinna:
/// - przy maskach powinowactwa ustawiać wątkowi handle dopełnienie maski powinowactwa wątku wołającego w obrębie rdzeni online, wypisywać na stdout komunikat [INFO] z liczbą rdzeni pomocniczych i zwracać true.
/// - nic nie zmieniać i zwracać false, gdy dopełnienie jest puste, bo wątek wołający widzi wszystkie rdzenie.
/// - przy błędzie odczytu maski, liczby rdzeni online albo ustawienia powinowactwa wypisywać [WARN] na stdout i zwracać false.
/// - na jądrach bez masek powinowactwa zawsze zwracać false, wypisując na stdout jeden komunikat [INFO] na proces.
///
/// @note Wolac po `rtActivate`, z uchwytem watku, ktory MUSI byc szeregowany mimo obciazenia watku RT. Watek
///       komunikacyjny powstaje przed rtActivate i zostaje SCHED_OTHER; przy `taskset -c 3` i obciazeniu ponad
///       100 % slotu nie bylby szeregowany wcale, a dlawienie RT tego nie ratuje (zmierzone: duty 212 %).
///       Komunikaty ida na stdout, bo w Release SPDLOG_WARN/INFO sa wycinane w kompilacji.
bool rtKeepThreadOffRtCpus(pthread_t handle);
