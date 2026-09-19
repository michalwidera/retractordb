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
/// `found == false` znaczy "jadro nie zna tego PID-u" i jest stanem ROZNYM od
/// `zombie`: pierwszy przypadek to proces, po ktorym nie ma juz sladu, drugi to
/// proces zakonczony, ktorego rodzic jeszcze nie zebral przez wait().
struct ProcessSnapshot {
  bool found{false};
  bool zombie{false};
  /// Znacznik uruchomienia TEJ inkarnacji PID-u. Wartosc jest nieprzezroczysta i
  /// porownywana wylacznie na rownosc - jednostka rozni sie miedzy systemami
  /// (Linux: takty od startu jadra, Darwin: mikrosekundy epoki). Zero znaczy
  /// "nie ustalono".
  std::uint64_t startTime{0};
};

/// Odczyt wpisu procesu. Jedno wywolanie jadra na obu platformach.
[[nodiscard]] ProcessSnapshot inspectProcess(std::int32_t pid);

/// Tozsamosc procesu w menedzerze uslug systemu.
///
/// `unit == nullopt` znaczy "zwykly proces": na Linuksie proces poza jednostka
/// systemd, na Darwinie proces poza zadaniem launchd. Nazwa jednostki jest
/// przekazywana menedzerowi doslownie (systemctl restart <unit> / launchctl
/// kickstart), wiec nie jest normalizowana.
struct ServiceIdentity {
  std::optional<std::string> unit;
  /// Zakres uzytkownika: systemd --user (user.slice) albo launchd LaunchAgent.
  /// Rozstrzyga, czy menedzera wola sie z przelacznikiem zakresu uzytkownika.
  bool userScope{false};
};

/// Ustala tozsamosc WLASNEGO procesu w menedzerze uslug.
[[nodiscard]] ServiceIdentity detectServiceIdentity();

/// Sciezka systemu plikow, ktory NAPRAWDE niesie obiekty pamieci dzielonej tego
/// procesu - to, co trzeba zmierzyc, pytajac "ile zostalo miejsca na kolejki IPC".
///
/// Na Linuksie jest to tmpfs spod shm_open (zwykle /dev/shm). Na Darwinie
/// Boost.Interprocess nie uzywa shm_open w ogole: `_POSIX_SHARED_MEMORY_OBJECTS`
/// jest tam ujemne, wiec segmenty sa ZWYKLYMI PLIKAMI w katalogu roboczym
/// biblioteki pod $TMPDIR. Pomiar /dev/shm nie mialby tam sensu - tej sciezki po
/// prostu nie ma - a pomiar $TMPDIR opisuje wlasciwy wolumin.
///
/// Napis pusty znaczy "nie ustalono"; wolajacy ma wtedy przepuscic operacje, a nie
/// odmowic (patrz kontrakt shmbudget::Space).
[[nodiscard]] std::string sharedMemoryBackingPath();

}  // namespace osplat
