#pragma once

#include <cstdint>
#include <optional>
#include <string>

/// @brief Warstwa systemowa: te trzy pytania, na ktore Linux i Darwin odpowiadaja
///        ROZNYMI interfejsami jadra, a nie roznymi flagami tej samej funkcji.
///
/// Zakres jest celowo waski. Wiekszosc kodu silnika jest czystym POSIX-em i dziala
/// na obu systemach bez posrednika; tutaj trafia wylacznie to, czego POSIX nie
/// standaryzuje wcale:
///
///   1. Zywotnosc obcego procesu   - Linux: /proc/<pid>/stat, Darwin: sysctl KERN_PROC_PID.
///   2. Tozsamosc w menedzerze uslug - Linux: /proc/self/cgroup (systemd), Darwin: launchd.
///   3. Magazyn obiektow IPC        - Linux: /dev/shm, Darwin: katalog Boost.Interprocess.
///
/// Wybor galezi robi platformConfig.h (generowany z prob kompilacyjnych), a nie
/// nazwa systemu wpisana w kod - patrz komentarz w cmake/PlatformChecks.cmake.
namespace osplat {

/// Migawka wpisu procesu w tablicy procesow jadra.
///
/// Trzy stany, ktorych nie wolno ze soba mieszac:
///   - `found == true`: wpis odczytany; `zombie` i `startTime` sa wtedy wiarygodne.
///     `zombie` to proces zakonczony, ktorego rodzic jeszcze nie zebral przez wait().
///   - `found == false`, `unreadable == false`: jadro POTWIERDZILO, ze tego PID-u nie
///     ma (Linux: kill(pid, 0) == ESRCH, Darwin: sysctl bez wpisu). Tylko to jest "martwy".
///   - `unreadable == true`: wpisu nie dalo sie odczytac albo sparsowac, a jadro nie
///     potwierdzilo braku procesu - typowo hidepid / ProtectProc na /proc albo EPERM
///     z sysctl dla procesu innego uzytkownika. Znaczy "nie wiem", NIE "martwy";
///     `found`, `zombie` i `startTime` sa wtedy puste.
/// @requirements-exempt ProcessSnapshot: Dane migawki procesu; odczyt i interpretacje realizuja funkcje warstwy systemowej.
struct ProcessSnapshot {
  bool found{false};
  bool zombie{false};
  bool unreadable{false};
  /// Znacznik uruchomienia TEJ inkarnacji PID-u. Wartosc jest nieprzezroczysta i
  /// porownywana wylacznie na rownosc - jednostka rozni sie miedzy systemami
  /// (Linux: takty od startu jadra, Darwin: mikrosekundy epoki). Zero znaczy
  /// "nie ustalono".
  std::uint64_t startTime{0};
};

/// Funkcja inspectProcess powinna:
/// - dla pid <= 0 zwracać pustą migawkę (found == false, unreadable == false).
///   Test: `RequirementContracts.catalogue_fallback_values_and_process_snapshot`.
///   Test: `OsPlatform.InvalidPidAndConfirmedAbsenceHaveEmptySnapshots`.
/// - odczytywać stan i czas startu z tego samego wpisu procesu (Linux: /proc/<pid>/task/<pid>/stat, Darwin: sysctl KERN_PROC_PID) i dla odczytanego wpisu ustawiać found, zombie i niezerowy startTime.
///   Test: `RequirementContracts.catalogue_fallback_values_and_process_snapshot`.
///   Test: `OsPlatform.ExitedChildRemainsAZombieUntilReaped`. Ograniczenie: stan i startTime pochodza z jednego wpisu, bez gwarancji pojedynczego wywolania read przez iostream; bez procfs (Darwin) test jest pomijany jako niezweryfikowany.
/// - zgłaszać brak procesu (found == false, unreadable == false) wyłącznie wtedy, gdy jądro go potwierdziło: kill(pid, 0) z ESRCH na Linuksie, ESRCH albo pusty wynik sysctl na Darwinie.
///   Test: `OsPlatform.InvalidPidAndConfirmedAbsenceHaveEmptySnapshots`.
/// - w każdym innym przypadku nieudanego odczytu, także przy ukrytym /proc (hidepid) i EPERM, ustawiać wyłącznie unreadable == true.
///   Test: `OsPlatform.PermissionDenialIsUnknownRatherThanDead`. Ograniczenie: wymuszono EPERM po nieudanym odczycie; uszkodzonej tresci /proc nie wstrzykuje sie do implementacji.
///
/// @note Na Linuksie czytany jest wpis lidera grupy watkow, bo QEMU user-mode syntetyzuje /proc/<wlasny pid>/stat
///       z innym starttime niz widzi obserwator.
[[nodiscard]] ProcessSnapshot inspectProcess(std::int32_t pid);

/// Tozsamosc procesu w menedzerze uslug systemu.
///
/// `unit == nullopt` znaczy "zwykly proces": na Linuksie proces poza jednostka
/// systemd, na Darwinie proces poza zadaniem launchd. Nazwa jednostki jest
/// przekazywana menedzerowi doslownie (systemctl restart <unit> / launchctl
/// kickstart), wiec nie jest normalizowana.
/// @requirements-exempt ServiceIdentity: Dane identyfikujace jednostke uslugi i jej zakres.
struct ServiceIdentity {
  std::optional<std::string> unit;
  /// Zakres uzytkownika: systemd --user (user.slice) albo launchd LaunchAgent.
  /// Rozstrzyga, czy menedzera wola sie z przelacznikiem zakresu uzytkownika.
  bool userScope{false};
};

/// Funkcja detectServiceIdentity powinna:
/// - na systemie z systemd zwracać jako unit najgłębszy segment ścieżki cgroup z /proc/self/cgroup kończący się na ".service", z pominięciem user@<uid>.service, a userScope ustawiać, gdy ścieżka biegnie przez /user.slice albo /user@.
///   Nietestowalne: kontrola strukturalna detectSystemd w src/common/osPlatform.cpp i st_stage12_contracts (filtry .service, user@ i userScope); bez wymuszenia wariantow cgroup w dzialajacej jednostce.
/// - na systemie z launchd zwracać jako unit wartość XPC_SERVICE_NAME tylko wtedy, gdy jest niepusta, różna od "0", nie zaczyna się od "application." i rodzicem procesu jest launchd (PID 1), a userScope ustawiać dla efektywnego UID różnego od 0.
///   Nietestowalne: kontrola strukturalna detectLaunchd w src/common/osPlatform.cpp i st_stage12_contracts (etykieta, PPID i UID); wymaga osobnego wykonania na Darwinie.
/// - w każdym innym przypadku, także na systemie bez znanego menedżera usług, zwracać unit == nullopt (zwykły proces).
///   Nietestowalne: kontrola strukturalna detectServiceIdentity w st_stage12_contracts i pustych wynikow detectSystemd/detectLaunchd; system bez znanego menedzera nie jest tu wykonywany.
///
/// @note Na Darwinie samo XPC_SERVICE_NAME nie wystarcza: srodowisko jest dziedziczone, wiec program z terminala
///       uruchomionego z Findera widzi etykiete aplikacji GUI. Zakres z UID-a to heurystyka - wybiera tylko
///       domene dla `launchctl`.
[[nodiscard]] ServiceIdentity detectServiceIdentity();

/// Funkcja sharedMemoryBackingPath powinna:
/// - zwracać ścieżkę systemu plików, który niesie obiekty pamięci dzielonej procesu: "/dev/shm" na Linuksie i "/tmp" na Darwinie.
///   Test: `RequirementContracts.catalogue_fallback_values_and_process_snapshot`.
///
/// @note To, co trzeba zmierzyc, pytajac "ile zostalo miejsca na kolejki IPC". Na Darwinie Boost.Interprocess nie
///       uzywa shm_open (`_POSIX_SHARED_MEMORY_OBJECTS` jest ujemne), wiec segmenty sa zwyklymi plikami pod /tmp,
///       a /dev/shm nie istnieje. Na Linuksie sciezka jest tylko droga zapasowa - pomiar glowny idzie przez
///       fstatvfs na deskryptorze utworzonego obiektu (shmbudget::space).
[[nodiscard]] std::string sharedMemoryBackingPath();

}  // namespace osplat
