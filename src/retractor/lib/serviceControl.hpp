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

// Restart jednostki systemd. userScope => "systemctl --user restart", inaczej "systemctl restart".
// Zwraca kod wyjścia systemctl (0 = sukces, -1 = błąd fork/exec). runner pusty => realny fork/exec.
int restartService(bool userScope, const std::string &unit, const SystemctlRunner &runner = {});

// Atomowo nadpisuje plik docelowy treścią pliku źródłowego (zapis do temp w katalogu docelowym
// + rename). Zwraca false przy błędzie odczytu/zapisu/rename.
bool deliverQueryFile(const std::string &source, const std::string &target);

}  // namespace servicecontrol
