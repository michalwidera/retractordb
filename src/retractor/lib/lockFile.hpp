#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

/// @brief Pliki blokad flock, ktore wolno kasowac.
///
/// Plik blokady, ktorego nikt nie kasuje, zostaje na dysku na zawsze - po jednym na kazda
/// nazwe instancji. Samo "skasuj plik, ktorego nikt nie trzyma" jest jednak wyscigiem: proces,
/// ktory otworzyl sciezke tuz przed skasowaniem, zajmuje potem blokade na i-wezle bez nazwy,
/// a nastepny tworzy nowy plik i zajmuje go takze - dwoch wlascicieli jednej tozsamosci.
/// Wyscig zamykaja dwie reguly, ktorych przestrzega kazdy uczestnik:
///   1. plik kasuje wylacznie ten, kto trzyma na nim blokade WYLACZNA, i robi to PRZED jej
///      zwolnieniem;
///   2. kto zajmie blokade, sprawdza, czy sciezka nadal wskazuje ten sam i-wezel, a jesli nie,
///      zaczyna od nowa.
/// Blokada zajeta na i-wezle wskazywanym przez sciezke jest wiec jedyna w swoim rodzaju,
/// a porzucony plik moze skasowac kazdy, komu uda sie zajac go wylacznie.
namespace lockfile {

enum class Result : std::uint8_t { Acquired, Busy, Error, Refused };

/// Funkcja acquire powinna:
/// - [lockfile.acquire.1~1] otwierać plik `path` bez podążania za dowiązaniem (O_RDWR przy writable, inaczej O_RDONLY), a brakujący tworzyć przez O_CREAT | O_EXCL z prawami 0644 niezależnymi od umask.
/// - [lockfile.acquire.2~1] zajmować na nim flock LOCK_EX przy exclusive, inaczej LOCK_SH, bez blokowania; przy EWOULDBLOCK i nadal podłączonej ścieżce ponawiać próbę co 1 ms do wspólnego terminu kTransientHolderGrace (500 ms od wejścia do acquire), a po jego wyczerpaniu zwracać Result::Busy.
/// - [lockfile.acquire.3~1] po zajęciu blokady sprawdzać, że ścieżka nadal wskazuje ten sam i-węzeł, a jeśli nie - zaczynać od nowa, po 100 takich trafieniach zwracając Result::Error z ESTALE.
/// - [lockfile.acquire.4~1] przy Result::Acquired zwracać deskryptor z zajętą blokadą w fd.
/// - [lockfile.acquire.5~1] zwracać Result::Refused z powodem w errno: ELOOP dla dowiązania symbolicznego, EINVAL dla obiektu niebędącego zwykłym plikiem, EPERM (tylko writable) dla pliku innego właściciela niż UID procesu, EMLINK (tylko writable) dla pliku o więcej niż jednej nazwie, EAGAIN po wyczerpaniu 100 prób open/create.
/// - [lockfile.acquire.6~1] zwracać Result::Error z errno błędu systemu dla każdej innej porażki open, fstat albo flock z wyjątkiem EINTR z flock: wtedy zamykać kandydacki deskryptor i ponawiać od otwarcia ścieżki, bez odnawiania terminu. Samo EINTR nie zwraca Error ani Busy; termin zajętości jest sprawdzany na ścieżce EWOULDBLOCK.
///   Ograniczenie weryfikacji: ponowienie po EINTR potwierdza kontrola strukturalna acquire w lockFile.cpp i proba diagnostyczna opisana w #521; brak trwalej asercji EINTR.
///
/// @note Okno 500 ms czeka na wlasciciela przejsciowego (sprzatacz, proces kasujacy plik); zywa instancja trzyma
///       blokade do konca, wiec po nim "zajete" jest prawdziwe. Termin ma takze czytelnik LOCK_SH: zatrzymany
///       posiadacz LOCK_EX nie moze zawiesic startu Bus. O_RDONLY wystarcza do flock i dziala takze na pliku,
///       do ktorego ten uzytkownik nie ma prawa zapisu.
///       Jest to okno oczekiwania na zajeta blokade, nie twardy limit czasu syscalls, planisty ani serii EINTR.
Result acquire(const std::string &path, bool exclusive, bool writable, int &fd);

/// Funkcja stillLinked powinna:
/// - [lockfile.still-linked.1~1] zwracać true wyłącznie wtedy, gdy `path` (bez podążania za dowiązaniem) jest zwykłym plikiem o tym samym urządzeniu i i-węźle co deskryptor fd.
[[nodiscard]] bool stillLinked(int fd, const std::string &path);

/// Funkcja removeAndRelease powinna:
/// - [lockfile.remove-and-release.1~1] kasować `path` tylko wtedy, gdy stillLinked(fd, path), a następnie zawsze zamykać fd, co zwalnia blokadę.
///
/// @note Wolajacy trzyma blokade wylaczna. Pliku, ktory przestal byc tym i-wezlem, nie rusza: pod ta sciezka
///       lezy wtedy cudza blokada.
void removeAndRelease(const std::string &path, int fd);

/// Funkcja isHeld powinna:
/// - [lockfile.is-held.1~1] zwracać true wyłącznie wtedy, gdy plik da się otworzyć do odczytu, a próba LOCK_SH bez czekania kończy się EWOULDBLOCK, czyli ktoś trzyma blokadę wyłączną.
/// - [lockfile.is-held.2~1] zwracać false dla pliku brakującego, odrzuconego (dowiązanie, nie zwykły plik) albo nieotwieralnego.
///
/// @note Wlasciciel startujacy w tej samej chwili trafia co najwyzej na okno wlasciciela przejsciowego.
[[nodiscard]] bool isHeld(const std::string &path);

/// Funkcja claimAbandoned powinna:
/// - [lockfile.claim-abandoned.1~1] otwierać istniejący zwykły plik do odczytu bez tworzenia i bez podążania za dowiązaniem, zajmować LOCK_EX bez czekania i zwracać deskryptor, gdy ścieżka nadal wskazuje ten i-węzeł.
/// - [lockfile.claim-abandoned.2~1] zwracać -1, gdy plik jest trzymany, zniknął, został podmieniony albo nie da się go otworzyć.
[[nodiscard]] int claimAbandoned(const std::string &path);

/// Funkcja sweep powinna:
/// - [lockfile.sweep.1~1] dla każdego pliku katalogu `dir`, którego nazwę przyjmie `accept`, zajmować porzuconą blokadę przez claimAbandoned i pomijać pliki trzymane przez żywych właścicieli.
/// - [lockfile.sweep.2~1] pomijać pliki innego właściciela niż efektywny UID procesu, także gdy proces działa jako root.
///   Zakres testow: `IpcSecurity.SweepDoesNotRemoveForeignLockOrInvokeItsCleanup`; integracja `it_ipc_security-foreign-uid` wymaga root i moze byc niewykonana.
/// - [lockfile.sweep.3~1] dla zajętego pliku wołać `removeGuarded(nazwa)` wciąż pod blokadą, a następnie removeAndRelease.
/// - [lockfile.sweep.4~1] zwracać liczbę plików, dla których wywołano removeGuarded i removeAndRelease.
/// - [lockfile.sweep.5~1] kończyć przegląd bez błędu, gdy katalogu nie da się otworzyć albo czytać.
///   Zakres testow: `LockFileSafety.SweepCallbackRunsUnderExclusiveLockAndCountsOnlyAcceptedFiles` (brak katalogu). Nietestowalne: blad odczytu w trakcie iteracji katalogu - kontrola strukturalna petli z error_code w lockFile.cpp; bez udawania bledu iteratora przez uszkadzanie katalogu hosta.
std::size_t sweep(const std::string &dir, const std::function<bool(std::string_view)> &accept,
                  const std::function<void(std::string_view)> &removeGuarded);

}  // namespace lockfile
