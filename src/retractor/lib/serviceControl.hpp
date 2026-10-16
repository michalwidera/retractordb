#pragma once

#include <functional>
#include <string>
#include <vector>

/// @brief Sterowanie jednostką systemd (restart) oraz atomowe dostarczenie pliku zapytań.
/// Używane przez ścieżkę E3: nowa instancja wykrywa działający serwis, nadpisuje jego plik
/// zapytań i zleca restart, by serwis załadował nowy zestaw (zachowując konfigurację jednostki).

namespace servicecontrol {

// Wykonanie polecenia systemctl: argv -> kod wyjścia. Wstrzykiwalne dla testów (bez realnego systemd).
using SystemctlRunner = std::function<int(const std::vector<std::string> &)>;

/// Funkcja restartCommand powinna:
/// - przy systemd zwracać argv {"systemctl", "--user", "restart", unit}, z "--user" wyłącznie przy userScope.
/// - przy launchd zwracać argv {"launchctl", "kickstart", "-k", <domena>/unit}, z domeną "gui/<uid>" przy userScope i "system" w przeciwnym razie.
/// - przekazywać nazwę jednostki dosłownie, bez normalizacji.
///
/// @note Wydzielone z restartService, bo to jedyne miejsce rozniace sie miedzy systemami i jedyna czesc, ktora
///       da sie sprawdzic testem bez uruchamiania menedzera uslug. launchd nie zna "restart"; `kickstart -k`
///       zabija zadanie, jesli biegnie, i uruchamia je od nowa.
[[nodiscard]] std::vector<std::string> restartCommand(bool userScope, const std::string &unit);

/// Funkcja restartService powinna:
/// - wykonywać polecenie restartCommand(userScope, unit) przez runner, gdy jest podany, i zwracać jego wynik.
/// - bez runnera uruchamiać menedżera usług przez fork i execvp, bez powłoki, i czekać na jego zakończenie.
/// - zwracać kod wyjścia menedżera (0 = sukces), 127, gdy execvp się nie powiódł, a -1 przy błędzie fork albo waitpid oraz gdy proces zakończył się sygnałem.
///
/// @note runner jest wstrzykiwany przez testy (bez realnego systemd).
int restartService(bool userScope, const std::string &unit, const SystemctlRunner &runner = {});

/// Funkcja deliverQueryFile powinna:
/// - wczytywać plik źródłowy binarnie i zapisywać jego treść do pliku docelowego przez writeQueryFile, zwracając jej wynik.
/// - zwracać false z wpisem ERROR w logu, gdy pliku źródłowego nie da się otworzyć.
bool deliverQueryFile(const std::string &source, const std::string &target);

/// Funkcja writeQueryFile powinna:
/// - zapisywać treść binarnie do pliku tymczasowego `<cel>.tmp.<pid>` w katalogu pliku docelowego, a następnie atomowo przenosić go na cel przez rename.
/// - dla pustej treści zostawiać pusty plik docelowy.
/// - zwracać true po udanym rename, a false z wpisem ERROR w logu, gdy pliku tymczasowego nie da się otworzyć lub zapisać albo rename zawiedzie; po nieudanym rename usuwać plik tymczasowy.
///
/// @note Uzywane przez przeladowanie w locie (`xqry --reset`): plan przychodzi kanalem IPC, wiec pliku
///       zrodlowego po stronie serwera nie ma. Pusta tresc to plan zerowy - tak usluga przechodzi w tryb
///       bezczynny.
bool writeQueryFile(const std::string &content, const std::string &target);

/// Funkcja dropRefusedPlan powinna:
/// - przy pustej nazwie jednostki albo pustej ścieżce pliku zapytań nic nie robić i zwracać false.
/// - w pozostałych przypadkach opróżniać plik zapytań przez writeQueryFile z pustą treścią i zwracać true, gdy się udało.
/// - wynik opróżnienia, udany albo nie, zapisywać w logu na poziomie CRITICAL.
///
/// @note Ta sama regula co po bledzie krytycznym (executorsm::cleanup): pusty plik nie pozwala, by
///       Restart=on-failure wracal co RestartSec na ten sam plan. Plik .rql operatora poza jednostka jest jego
///       wlasnoscia.
bool dropRefusedPlan(const std::string &unit, const std::string &queryFile);

}  // namespace servicecontrol
