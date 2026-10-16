#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/// Wejscie do parsera RQL (RQLParser.cpp). Naglowek jest JEDYNYM miejscem, w ktorym te
/// symbole sa deklarowane: do 2026-09-07 kazdy konsument robil to u siebie wlasnym `extern`,
/// a przy trzech przeciazeniach `parserRQLString` taka deklaracja nie tylko moze rozjechac
/// sie z definicja - moze po cichu wskazac INNE przeciazenie, bo przeciazenia nie daja
/// bledu linkowania, tylko wybor. RQLParser.cpp takze wlacza ten naglowek, wiec definicje
/// sa sprawdzane wzgledem deklaracji.

class qTree;

/// Funkcja parserRQLString powinna:
/// - parsować porcję tekstu RQL `inlet` i dopisywać węzły jej instrukcji do coreInstance.
/// - zwracać krotkę {status, pierwsze słowo kluczowe porcji wielkimi literami, nazwa strumienia pierwszej instrukcji SELECT, DECLARE albo RULE}.
/// - przy powodzeniu zwracać status "OK", a dla porcji bez instrukcji słowo kluczowe "UNRECOGNIZED".
/// - przy błędzie leksera lub składni zwracać treść błędu jako status, słowo "UNRECOGNIZED" i pustą nazwę strumienia.
/// - przy błędzie semantycznym zwracać jego treść jako status razem ze słowem kluczowym i nazwą strumienia instrukcji.
/// - czyścić statementKeywords i zapisywać do niego słowa kluczowe wszystkich instrukcji porcji, wielkimi literami.
/// - liczyć wiersze w komunikatach od firstLine i podawać sourceFile w logu błędów składni; przeciążenia bez firstLine przyjmują 1, a przeciążenie bez statementKeywords odrzuca zebrane słowa.
/// - nie przepuszczać żadnego wyjątku: złamany niezmiennik listenera i każdy wyjątek runtime ANTLR zwracać statusem z przedrostkiem kInternalCompilerError i słowem "UNRECOGNIZED".
/// - po błędzie nie cofać zmian już naniesionych w coreInstance.
///
/// @note firstLine to numer wiersza porcji w pliku zrodlowym: podaja go wolajacy, ktory tnie plik na
///       instrukcje (parsePlanText, parserRQLFile_4Test). Zmieniony po bledzie coreInstance odrzuca
///       wolajacy (caly plan w launcherze, kopie planu w executorsm::getAdHoc). Plik ma jedyna w
///       retractorcore wyspe wyjatkow (RDB_NOEXCEPT_CHECK), bo ANTLR przerywa parsowanie rzutem.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords, size_t firstLine,
                                                                  std::string_view sourceFile = {});

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords);

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet);

/// Funkcja readLogicalLines powinna:
/// - pomijać wiersze puste lub złożone ze spacji, tabulacji i CR oraz wiersze, których pierwszy niebiały znak to `#`.
/// - zamykać instrukcję na każdym pozostałym wierszu, który nie kończy się znakiem `\`.
/// - wiersz kończący się `\` doklejać do instrukcji bez tego znaku, ze spacją w jego miejscu, i czytać dalej.
/// - zwracać pary (treść instrukcji, numer wiersza pliku od 1, na którym instrukcja się zaczyna) w kolejności pliku.
/// - pomijać bez komunikatu instrukcję, której ostatni wiersz przed końcem strumienia kończy się `\`.
///
/// @note Komentarz `#` obsluguje ta funkcja, nie lekser: lekser zna `#` tylko jako operator przeplotu, a do
///       2026-08-29 jego regula `'# '` zjadala `FROM a # b` do `FROM a`. Komentarz konczacy wiersz to `//`.
///       Numer wiersza pliku jest kotwica komunikatow skladni - bez niego blad wskazywal wiersz 1 instrukcji.
std::vector<std::pair<std::string, size_t>> readLogicalLines(std::istream &file);

/// Funkcja parserRQLFile_4Test powinna:
/// - dla pliku, którego nie da się otworzyć, logować błąd i zwracać "Unable to open file.".
/// - parsować kolejno instrukcje z readLogicalLines przez parserRQLString z numerem pierwszego wiersza i nazwą pliku.
/// - przy pierwszej nieudanej instrukcji logować ją i zwracać jej status.
/// - zwracać "OK" po udanym sparsowaniu wszystkich instrukcji, a "Empty file." dla pliku bez instrukcji.
///
/// @note Tylko dla testow jednostkowych: droga produkcyjna to parsePlanText (planSource.hpp), ktora
///       dodatkowo zapamietuje wiersze zrodlowe planu.
std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
