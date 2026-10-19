#pragma once

#include <cstddef>
#include <iosfwd>
#include <map>
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

/// Stala `LET NAZWA = wartosc` (#440). Wartosc liczbowa trzyma tylko stala calkowita, bo tylko taka
/// ma dzis odbiorcow (konteksty calkowite, etap A); dziesietna i ulamek sa przyjmowane w deklaracji,
/// a ich uzycie w kontekscie calkowitym jest odmowa - bez niejawnego obcinania.
/// @requirements-exempt RqlConstant: Rekord tablicy stalych; logike ma parser.
struct RqlConstant {
  enum class Kind { integer, decimal, fraction };
  Kind kind{Kind::integer};
  std::string text;  ///< Prawa strona deklaracji, ze znakiem - do komunikatow.
  int integer{0};    ///< Wartosc, gdy kind == integer.
  size_t line{0};    ///< Wiersz deklaracji w pliku planu.
  bool used{false};  ///< Czy plan odwolal sie do stalej - podstawa ostrzezenia o nieuzytej.
};

/// Tablica stalych JEDNEGO planu: nazwa (bez `?`) -> stala. Zyje dluzej niz jedno wywolanie
/// parserRQLString, bo plan jest parsowany po jednej instrukcji (readLogicalLines).
using RqlConstants = std::map<std::string, RqlConstant>;

/// Funkcja parserRQLString powinna:
/// - [rqlparser.parser-rql-string.1~1] parsować porcję tekstu RQL `inlet` i dopisywać węzły jej instrukcji do coreInstance.
/// - [rqlparser.parser-rql-string.2~1] zwracać krotkę {status, pierwsze słowo kluczowe porcji wielkimi literami, nazwa strumienia pierwszej instrukcji SELECT, DECLARE albo RULE}.
/// - [rqlparser.parser-rql-string.3~1] przy powodzeniu zwracać status "OK"; porcję bez instrukcji, także pusty tekst, traktować jako błąd składni (reguła `prog` wymaga co najmniej jednej instrukcji), bo pusty plan rozpoznaje wcześniej parsePlanText.
/// - [rqlparser.parser-rql-string.4~1] przy błędzie leksera lub składni zwracać treść błędu jako status, słowo "UNRECOGNIZED" i pustą nazwę strumienia.
/// - [rqlparser.parser-rql-string.5~1] przy błędzie semantycznym zwracać jego treść jako status razem ze słowem kluczowym i nazwą strumienia instrukcji.
/// - [rqlparser.parser-rql-string.6~1] czyścić statementKeywords i zapisywać do niego słowa kluczowe wszystkich instrukcji porcji, wielkimi literami.
/// - [rqlparser.parser-rql-string.7~1] liczyć wiersze w komunikatach od firstLine i podawać sourceFile w logu błędów składni; przeciążenia bez firstLine przyjmują 1, a przeciążenie bez statementKeywords odrzuca zebrane słowa.
/// - [rqlparser.parser-rql-string.8~1] nie przepuszczać żadnego wyjątku: złamany niezmiennik listenera i każdy wyjątek runtime ANTLR zwracać statusem z przedrostkiem kInternalCompilerError i słowem "UNRECOGNIZED".
///   Test sprawdza kopie tej jednostki wkompilowana w ut_planSource, z ANTLRInputStream podmienionym makrem na strumien
///   rzucajacy wyjatek; biblioteka zostaje bez haka. Zmiana tworzenia strumienia nie wycisza testu: przy innym typie
///   strumienia nic nie rzuca i status wraca "OK" (czerwony), przy nazwie kwalifikowanej podmiana sie nie kompiluje.
/// - [rqlparser.parser-rql-string.9~1] po błędzie nie cofać zmian już naniesionych w coreInstance.
/// - [rqlparser.parser-rql-string.10~1] dopisywać deklaracje LET porcji do tablicy constants i rozwiązywać z niej odwołania `?NAZWA`, oznaczając stałą jako użytą; przeciążenia bez tej tablicy parsują z pustą tablicą lokalną, więc stała nie wychodzi poza porcję.
/// - [rqlparser.parser-rql-string.11~1] odwołanie do stałej nieznanej, niecałkowitej albo o wartości łamiącej ograniczenia kontekstu oraz ponowną deklarację LET i nazwę z `$` zwracać jako błąd semantyczny z nazwą stałej i pozycją `line L:C`.
///
/// @note firstLine to numer wiersza porcji w pliku zrodlowym: podaja go wolajacy, ktory tnie plik na
///       instrukcje (parsePlanText, parserRQLFile_4Test). Zmieniony po bledzie coreInstance odrzuca
///       wolajacy (caly plan w launcherze, kopie planu w executorsm::getAdHoc). Plik ma jedyna w
///       retractorcore wyspe wyjatkow (RDB_NOEXCEPT_CHECK), bo ANTLR przerywa parsowanie rzutem.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords, size_t firstLine,
                                                                  std::string_view sourceFile, RqlConstants &constants);

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords, size_t firstLine,
                                                                  std::string_view sourceFile = {});

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords);

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet);

/// Funkcja readLogicalLines powinna:
/// - [rqlparser.read-logical-lines.1~1] pomijać wiersze puste lub złożone ze spacji, tabulacji i CR oraz wiersze, których pierwszy niebiały znak to `#`.
/// - [rqlparser.read-logical-lines.2~1] zamykać instrukcję na każdym pozostałym wierszu, który nie kończy się znakiem `\`.
/// - [rqlparser.read-logical-lines.3~1] wiersz kończący się `\` doklejać do instrukcji bez tego znaku, ze spacją w jego miejscu, i czytać dalej.
/// - [rqlparser.read-logical-lines.4~1] zwracać pary (treść instrukcji, numer wiersza pliku od 1, na którym instrukcja się zaczyna) w kolejności pliku.
/// - [rqlparser.read-logical-lines.5~1] pomijać bez komunikatu instrukcję, której ostatni wiersz przed końcem strumienia kończy się `\`.
///
/// @note Komentarz `#` obsluguje ta funkcja, nie lekser: lekser zna `#` tylko jako operator przeplotu, a do
///       2026-08-29 jego regula `'# '` zjadala `FROM a # b` do `FROM a`. Komentarz konczacy wiersz to `//`.
///       Numer wiersza pliku jest kotwica komunikatow skladni - bez niego blad wskazywal wiersz 1 instrukcji.
std::vector<std::pair<std::string, size_t>> readLogicalLines(std::istream &file);

/// Funkcja parserRQLFile_4Test powinna:
/// - [rqlparser.parser-rql-file-4-test.1~1] dla pliku, którego nie da się otworzyć, logować błąd i zwracać "Unable to open file.".
/// - [rqlparser.parser-rql-file-4-test.2~1] parsować kolejno instrukcje z readLogicalLines przez parserRQLString z numerem pierwszego wiersza i nazwą pliku.
/// - [rqlparser.parser-rql-file-4-test.5~1] parsować wszystkie instrukcje pliku z jedną tablicą stałych LET.
/// - [rqlparser.parser-rql-file-4-test.3~1] przy pierwszej nieudanej instrukcji logować ją i zwracać jej status.
/// - [rqlparser.parser-rql-file-4-test.4~1] zwracać "OK" po udanym sparsowaniu wszystkich instrukcji, a "Empty file." dla pliku bez instrukcji.
///
/// @note Tylko dla testow jednostkowych: droga produkcyjna to parsePlanText (planSource.hpp), ktora
///       dodatkowo zapamietuje wiersze zrodlowe planu.
std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
