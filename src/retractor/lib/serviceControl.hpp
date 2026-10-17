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
///   Test: `ServiceControlRestart.builds_system_scope_argv`, `ServiceControlRestart.builds_user_scope_argv`.
/// - przy launchd zwracać argv {"launchctl", "kickstart", "-k", <domena>/unit}, z domeną "gui/<uid>" przy userScope i "system" w przeciwnym razie.
///   Test: `ServiceControlRestart.builds_system_scope_argv`, `ServiceControlRestart.builds_user_scope_argv`. Ograniczenie: argv launchd wymaga osobnego przebiegu na Darwinie.
/// - przekazywać nazwę jednostki dosłownie, bez normalizacji.
///   Test: `ServiceControlContract.real_exec_passes_literal_arguments_and_propagates_exit_and_signal`.
///
/// @note Wydzielone z restartService, bo to jedyne miejsce rozniace sie miedzy systemami i jedyna czesc, ktora
///       da sie sprawdzic testem bez uruchamiania menedzera uslug. launchd nie zna "restart"; `kickstart -k`
///       zabija zadanie, jesli biegnie, i uruchamia je od nowa.
[[nodiscard]] std::vector<std::string> restartCommand(bool userScope, const std::string &unit);

/// Funkcja restartService powinna:
/// - wykonywać polecenie restartCommand(userScope, unit) przez runner, gdy jest podany, i zwracać jego wynik.
///   Test: `ServiceControlRestart.propagates_runner_exit_code`, `ServiceControlRestart.builds_system_scope_argv`, `ServiceControlRestart.builds_user_scope_argv`.
/// - bez runnera uruchamiać menedżera usług przez fork i execvp, bez powłoki, i czekać na jego zakończenie.
///   Test: `ServiceControlContract.real_exec_passes_literal_arguments_and_propagates_exit_and_signal`.
/// - zwracać kod wyjścia menedżera (0 = sukces), 127, gdy execvp się nie powiódł, a -1 przy błędzie fork albo waitpid oraz gdy proces zakończył się sygnałem.
///   Test: `ServiceControlContract.real_exec_passes_literal_arguments_and_propagates_exit_and_signal`, `ServiceControlContract.fork_and_wait_failures_return_minus_one`. Ograniczenie: bez -Wl,--wrap (Darwin) test bledow fork/waitpid jest pomijany jako niezweryfikowany.
///
/// @note runner jest wstrzykiwany przez testy (bez realnego systemd).
int restartService(bool userScope, const std::string &unit, const SystemctlRunner &runner = {});

/// Funkcja deliverQueryFile powinna:
/// - wczytywać plik źródłowy binarnie i zapisywać jego treść do pliku docelowego przez writeQueryFile, zwracając jej wynik.
///   Test: `ServiceControlContract.delivery_preserves_binary_bytes_and_rename_keeps_the_old_reader`.
/// - zwracać false z wpisem ERROR w logu, gdy pliku źródłowego nie da się otworzyć.
///   Test: `ServiceControlDeliver.fails_on_missing_source`.
bool deliverQueryFile(const std::string &source, const std::string &target);

/// Funkcja writeQueryFile powinna:
/// - zapisywać treść binarnie do pliku tymczasowego `<cel>.tmp.<pid>` w katalogu pliku docelowego, a następnie atomowo przenosić go na cel przez rename.
///   Test: `ServiceControlContract.delivery_preserves_binary_bytes_and_rename_keeps_the_old_reader`.
/// - dla pustej treści zostawiać pusty plik docelowy.
///   Test: `ServiceControlWrite.empty_content_truncates_the_target`.
/// - zwracać true po udanym rename, a false z wpisem ERROR w logu, gdy pliku tymczasowego nie da się otworzyć lub zapisać albo rename zawiedzie; po nieudanym rename usuwać plik tymczasowy.
///   Test: `ServiceControlWrite.fails_when_the_target_directory_does_not_exist`, `ServiceControlContract.rename_failure_preserves_the_target_and_removes_the_temporary_file`, `ServiceControlContract.write_failure_preserves_the_target_and_reports_the_temporary_file`. Ograniczenie: blad zapisu wymaga /dev/full; brak tego urzadzenia daje jawny skip.
///
/// @note Uzywane przez przeladowanie w locie (`xqry --reset`): plan przychodzi kanalem IPC, wiec pliku
///       zrodlowego po stronie serwera nie ma. Pusta tresc to plan zerowy - tak usluga przechodzi w tryb
///       bezczynny.
bool writeQueryFile(const std::string &content, const std::string &target);

/// Funkcja dropRefusedPlan powinna:
/// - przy pustej nazwie jednostki albo pustej ścieżce pliku zapytań nic nie robić i zwracać false.
///   Test: `ServiceControlDropRefused.clears_the_query_file_only_inside_a_service_unit`, `ServiceControlContract.refused_plan_handles_empty_paths_and_logs_both_outcomes`.
/// - w pozostałych przypadkach opróżniać plik zapytań przez writeQueryFile z pustą treścią i zwracać true, gdy się udało.
///   Test: `ServiceControlContract.refused_plan_handles_empty_paths_and_logs_both_outcomes`.
/// - wynik opróżnienia, udany albo nie, zapisywać w logu na poziomie CRITICAL.
///   Test: `ServiceControlContract.refused_plan_handles_empty_paths_and_logs_both_outcomes`.
///
/// @note Ta sama regula co po bledzie krytycznym (executorsm::cleanup): pusty plik nie pozwala, by
///       Restart=on-failure wracal co RestartSec na ten sam plan. Plik .rql operatora poza jednostka jest jego
///       wlasnoscia.
bool dropRefusedPlan(const std::string &unit, const std::string &queryFile);

}  // namespace servicecontrol
