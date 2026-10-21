#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "fldType.hpp"  // rdb::descFld

/// Wejscie do parsera RQL (RQLParser.cpp). Naglowek jest JEDYNYM miejscem, w ktorym te
/// symbole sa deklarowane: do 2026-09-07 kazdy konsument robil to u siebie wlasnym `extern`,
/// a przy trzech przeciazeniach `parserRQLString` taka deklaracja nie tylko moze rozjechac
/// sie z definicja - moze po cichu wskazac INNE przeciazenie, bo przeciazenia nie daja
/// bledu linkowania, tylko wybor. RQLParser.cpp takze wlacza ten naglowek, wiec definicje
/// sa sprawdzane wzgledem deklaracji.

class qTree;

/// Stala `LET NAZWA = wartosc` (#440). Stala bez adnotacji (#547) ma typ swojego literalu: kazdy kontekst zamienia ja ta
/// sama konwersja, ktora stosuje do literalu tej postaci. Dziesietna nie ma tu wartosci liczbowej, bo
/// literal dziesietny ma w roznych kontekstach rozne typy (FLOAT w wyrazeniu, Rationalize w
/// rational_se) - kontekst parsuje jej tekst. Uzycie, ktorego literal tej postaci nie ma (dziesietna
/// w kontekscie calkowitym, ulamek w wyrazeniu), jest odmowa - bez niejawnej konwersji.
/// @requirements-exempt RqlConstant: Rekord tablicy stalych; logike ma parser.
struct RqlConstant {
  enum class Kind : std::uint8_t { integer, decimal, fraction };
  Kind kind{Kind::integer};
  std::string text;  ///< Prawa strona deklaracji, ze znakiem - do komunikatow i parsowania dziesietnej.
  int integer{0};    ///< Wartosc, gdy kind == integer albo type == INTEGER (#547).
  int nom{0};        ///< Licznik ze znakiem, gdy kind == fraction.
  int den{1};        ///< Mianownik, dodatni, gdy kind == fraction.
  size_t line{0};    ///< Wiersz deklaracji w pliku planu.
  bool used{false};  ///< Czy plan odwolal sie do stalej - podstawa ostrzezenia o nieuzytej.
  /// Indeks w qTree::variables, gdy nazwe zadeklarowal VAR (#531), a nie LET. Jedna tablica dla
  /// obu form, bo dziela znacznik `?` i przestrzen nazw (D5).
  std::optional<size_t> variable;
  /// Jawny typ stalej `LET NAZWA : TYP` (#547). Gdy jest, wyrazenie i kontekst strukturalny biora
  /// wartosc literalu po konwersji do TYPU (L4, L5), a nie wedlug jego pisowni; tekst zostaje dla
  /// prawej strony SET, ktora konwertuje go do typu zmiennej (O3).
  std::optional<rdb::descFld> type;
};

/// Tablica stalych i zmiennych JEDNEGO planu: nazwa (bez `?`) -> wpis. Zyje dluzej niz jedno
/// wywolanie parserRQLString, bo plan jest parsowany po jednej instrukcji (readLogicalLines).
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
/// - [rqlparser.parser-rql-string.12~1] rozwiązywać `?NAZWA` w `rational_se` (interwał DECLARE, `&`, `%`, `-`, TIMEOUT) tą samą konwersją co literał całkowity, dziesiętny albo ułamek tej postaci; stałą ujemną odrzucać, a w TIMEOUT `-?NAZWA` negować wartość i odrzucać wynik ujemny.
/// - [rqlparser.parser-rql-string.13~1] rozwiązywać `'-'? ?NAZWA` w wyrażeniu SELECT i RULE WHEN jak literał `'-'? DECIMAL` (INTEGER) albo `'-'? FLOAT` (FLOAT z tekstu stałej); ułamek oraz negację wychodzącą poza zakres INTEGER odrzucać.
/// - [rqlparser.parser-rql-string.14~1] dopisywać deklarację `VAR NAZWA = literał` do qTree::variables z wartością w typie literału (całkowity INTEGER, dziesiętny FLOAT, ułamek RATIONAL) i do tej samej tablicy constants co LET; ponowną deklarację nazwy w dowolnej z obu form, nazwę z `$` i wartość spoza zakresu typu zwracać jako błąd semantyczny z nazwą i pozycją.
/// - [rqlparser.parser-rql-string.15~1] odwołanie `'-'? ?NAZWA` do zmiennej VAR w wyrażeniu SELECT i RULE WHEN zamieniać na token PUSH_VAR z parą (nazwa, indeks w qTree::variables), a minus na następujący po nim NEGATE.
/// - [rqlparser.parser-rql-string.16~1] odwołanie do zmiennej VAR w każdym kontekście strukturalnym (int_const, rational_se, indeks z `$`) oraz w argumencie okna rekordowego zwracać jako błąd semantyczny z nazwą, kontekstem i pozycją `line L:C`.
/// - [rqlparser.parser-rql-string.17~1] akcję `DO SET ?NAZWA = wyrażenie` zapisywać w regule jako SET z indeksem zmiennej VAR celu, programem prawej strony (odczyty VAR jako PUSH_VAR), pozycją celu `line L:C` i numerem reguły w tekście planu; cel będący stałą LET albo nazwą niezadeklarowaną zwracać jako błąd semantyczny z nazwą i pozycją.
/// - [rqlparser.parser-rql-string.18~1] deklarację `VAR NAZWA : TYP = literał` z typem INTEGER, UINT, FLOAT, DOUBLE albo RATIONAL dopisywać do qTree::variables z literałem przekonwertowanym do TYPU (#546, H3): do INTEGER i UINT tylko dokładnie, do RATIONAL dokładnie z tekstu literału, do FLOAT i DOUBLE z zaokrągleniem; literał bez wartości w TYPIE zwracać jako błąd semantyczny z nazwą, typem i pozycją, a typ spoza tej piątki jako błąd składni.
/// - [rqlparser.parser-rql-string.19~1] prawą stronę `DO SET` będącą jednym literałem całkowitym albo dziesiętnym, także ujemnym, albo jedną stałą LET zapisywać jako PUSH_VAL w typie zmiennej celu, po tej samej konwersji co literał deklaracji z jawnym typem (#546, O3), a literał bez wartości w tym typie zwracać jako błąd semantyczny z celem, typem i pozycją; każdą inną prawą stronę zostawiać bez konwersji.
/// - [rqlparser.parser-rql-string.20~1] deklarację `LET NAZWA : TYP = literał` z typem INTEGER, UINT, FLOAT, DOUBLE albo RATIONAL dopisywać do tablicy constants z TYPEM i konwertować literał jak deklarację VAR z jawnym typem (#547, L3); literał bez wartości w TYPIE zwracać jako błąd semantyczny z nazwą stałej, typem i pozycją.
/// - [rqlparser.parser-rql-string.21~1] odwołanie `'-'? ?NAZWA` do stałej z jawnym typem w wyrażeniu SELECT i RULE WHEN zamieniać na PUSH_VAL w tym typie, liczony z tekstu stałej, a przy minusie z tekstu zanegowanego (#547, L4); negację bez wartości w typie zwracać jako błąd semantyczny z nazwą, typem i pozycją.
/// - [rqlparser.parser-rql-string.22~1] stałą z jawnym typem przyjmować w int_const i indeksie z `$` tylko jako INTEGER, a w rational_se tylko jako INTEGER albo RATIONAL, z wartością po konwersji z jej deklaracji (#547, L5); każdy inny typ zwracać jako błąd semantyczny z nazwą, typem, kontekstem i pozycją `line L:C`.
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

/// Funkcja planVariableNames powinna:
/// - [rqlparser.plan-variable-names.1~1] zwracać tablicę nazw z wpisem dla każdej zmiennej VAR z plan.variables: nazwa -> indeks w plan.variables i wiersz deklaracji.
/// - [rqlparser.plan-variable-names.2~1] nie zawierać stałych LET, bo plan ich nie niesie - kanał ad hoc nie widzi stałych planu (#440 O1).
///
/// @note Tablica dla kanalu ad hoc (#531, D7): zapytanie dolaczane do dzialajacego planu czyta jego
///       zmienne i pisze je regula SET, a deklaracja o tej samej nazwie jest kolizja z wierszem planu.
RqlConstants planVariableNames(const qTree &plan);

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
