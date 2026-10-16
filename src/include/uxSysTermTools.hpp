#pragma once

#include <string>

/// Funkcja _kbhit powinna:
/// - zwracać true, gdy na terminalu czeka naciśnięty klawisz, pozostawiając go do odczytu, a bez klawisza false, bez blokowania i z przywróceniem trybu terminala.
/// - zwracać false bez czytania wejścia, gdy ignoreAnyKey jest ustawione albo stdin nie jest terminalem.
///
/// @note Gdy stdin jest plikiem albo potokiem, getchar() zwracal pierwszy bajt WEJSCIA zamiast klawisza:
///       xqry z przekierowanym stdin konczyl sie kodem 0 bez odczytu strumienia (K6b, issue_215).
bool _kbhit(bool ignoreAnyKey);

/// Funkcja _getch powinna:
/// - zwracać kolejny znak ze standardowego wejścia (getchar()).
int _getch();

/// Funkcja fixArgcv powinna:
/// - usuwać końcowy znak powrotu karetki z każdego argumentu argv, pozostawiając pozostałe argumenty bez zmian.
///
/// @note Skrypty .sh przekazywaly argumenty z dodatkowym \r (od gcc 9.x czesc testow padala).
void fixArgcv(int argc, char **argv);

/// Funkcja setupLoggerMain powinna:
/// - w trybie usługi ustawiać domyślny logger na stderr, bez pliku i bez znacznika czasu, z flush po każdej linii, z przedrostkiem priorytetu sd-daemon "<N>" tylko przy systemd, i zwracać opis celu logu.
/// - poza trybem usługi dopisywać log do pliku <katalog tymczasowy>/<nazwa pliku z loggerFile>.log, bez obcinania, i zwracać jego ścieżkę.
/// - w trybie dual logować jednocześnie do pliku i na stderr, nigdy na stdout, który jest strumieniem danych klienta.
/// - ustawiać poziom logowania równy poziomowi kompilacji SPDLOG_ACTIVE_LEVEL.
///
/// @note Diagnostyka na stdout psula dane klienta i ginela przy `>/dev/null 2>err` (issue_217).
std::string setupLoggerMain(const std::string &loggerFile, bool dual = false, bool service = false);
