#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <string>

#include "fldType.hpp"  // rdb::descFld
#include "token.hpp"    // token, std::list

/// Typ pola źródłowego, po nazwie strumienia i indeksie pola w jego schemacie.
/// std::nullopt oznacza „typ nieznany" i blokuje wszystkie reguły, które typu wymagają.
using fieldTypeLookup = std::function<std::optional<rdb::descFld>(const std::string &, int)>;

/// Funkcja simplifyExpression powinna:
/// - [exprsimplify.simplify-expression.1~1] dla pustego programu, programu z tokenem spoza zestawu ewaluatora (np. PUSH_STREAM, COUNT, PUSH_IDX) i programu, który nie zostawia na stosie dokładnie jednej wartości, zwracać 0 i nie zmieniać programu.
/// - [exprsimplify.simplify-expression.2~1] podmieniać program wyłącznie wtedy, gdy zadziałało co najmniej jedno przepisanie, i zwracać liczbę przepisań.
/// - [exprsimplify.simplify-expression.3~1] brać typ PUSH_ID z typeOfField po nazwie strumienia i płaskim indeksie pola, a PUSH_ID2 traktować jako liść bez typu.
/// - [exprsimplify.simplify-expression.4~1] (A) zwijać podwyrażenie złożone wyłącznie ze stałych do jednej stałej policzonej produkcyjnym ewaluatorem (expressionEvaluator::eval bez payloadu), a zostawiać je, gdy ewaluator nie da wartości albo da NULL lub parę.
/// - [exprsimplify.simplify-expression.5~1] nigdy nie zwijać wywołania `to_string` (CALL i CALL2), także nad stałym argumentem; zwijać wolno tylko sam argument.
/// - [exprsimplify.simplify-expression.6~1] (B) łączyć dwie stałe ogona `(E op c1) op c2` w jedną dla `+` i `-` (`E-c1-c2` → `E-(c1+c2)`, `c1-E-c2` → `(c1-c2)-E`) oraz `*` z `*`, wyłącznie gdy obie stałe są INTEGER, E ma znany typ BYTE, INTEGER albo UINT, zwinięta stała ma reprezentację w typie operacji, a dziedzina określoności formy przepisanej zawiera się w dziedzinie wyniku pośredniego.
/// - [exprsimplify.simplify-expression.7~1] (B) łączyć ogon konkatenacji `E+'a'+'b'` → `E+'ab'` dla E typu STRING, wyłącznie ze stałymi po prawej.
/// - [exprsimplify.simplify-expression.8~1] (C) usuwać element neutralny (`E+0`, `E-0`, `E*1`, `E/1`) wyłącznie wtedy, gdy E ma znany typ o arytmetyce dokładnej, a stała ma ten sam typ co E.
/// - [exprsimplify.simplify-expression.9~1] stosować reguły B i C do stałej po lewej (`c+E`, `c*E`) tylko dla `+` i `*`, gdy E ma znany typ liczbowy, a stała jest liczbą.
/// - [exprsimplify.simplify-expression.10~1] (D) wyłącznie przy `aggressive_expr_optimization=ON` zwijać iloczyn powtórzonego niestałego podwyrażenia o typie dokładnym do potęgi: `a*a` → `a^2`, `a^k*a` → `a^(k+1)`.
/// - [exprsimplify.simplify-expression.11~1] nie stosować reguł B, C i D do `^`, porównań ani operatorów logicznych, a dla FLOAT i DOUBLE nie stosować żadnej z nich.
/// - [exprsimplify.simplify-expression.12~1] nie przepisywać `E*0` na `0`.
///
/// Zwija stałe i kanonizuje wyrażenia, żeby plan wykonania był krótszy już po kompilacji -
/// program pola liczony jest raz na interwał, więc każdy usunięty token jest oszczędnością
/// powtarzaną przez cały czas życia strumienia.
///
/// Reguły:
///  * A - zwijanie stałych: `1+1` → `2`, `'a'+'b'` → `'ab'`, `Sqrt(4)` → `2`;
///  * B - reasocjacja ogona stałych: `E+c1+c2` → `E+(c1+c2)` (także `-`, `*` oraz stała
///        po lewej: `c1-E-c2` → `(c1-c2)-E`);
///  * C - elementy neutralne: `E+0`, `E-0`, `E*1`, `E/1` → `E`;
///  * D - powtórzony czynnik jako potęga: `a*a` → `a^2`, `a*a*a` → `a^3`, i tak dalej dla
///        dowolnie długiego łańcucha oraz dla dowolnego powtórzonego podwyrażenia
///        (`(x+1)*(x+1)` → `(x+1)^2`). WYŁĄCZNIE przy `aggressive_expr_optimization=ON`.
///
/// B wymaga zachowania NULL: dziedzina określoności wyniku po przepisaniu musi zawierać się
/// w dziedzinie wyniku pośredniego. Strażnik obejmuje bazę INTEGER/UINT/BYTE ze stałymi INTEGER;
/// dla BYTE przyjmuje cały zakres INTEGER. RATIONAL i nieudowodnione promocje pozostają bez
/// reasocjacji. Konkatenacja STRING jest dopuszczona. Reguły A i C działają bez zmian.
///
/// Świadomie NIE ma tu `E*0` → `0`: `NULL*0` daje NULL, więc pochłanianie złamałoby 3VL.
///
/// Regułę D warto czytać jako oszczędność ODCZYTÓW, nie mnożeń: `a*a` czyta payload dwa razy,
/// `a^2` raz, a mnożeń jest tyle samo. Wolno ją zastosować wyłącznie dla typów o arytmetyce
/// DOKŁADNEJ (BYTE, INTEGER, UINT, RATIONAL), bo tylko tam `^` jest z definicji tym samym, co
/// zapisany wprost iloczyn - expressionEvaluator liczy taką potęgę tym samym operator*, którego
/// użyłby MULTIPLY (patrz exactPower), więc zawinięcie modulo 2^n i promocja BYTE do int
/// zostają zachowane. Dla FLOAT i DOUBLE przepisania NIE MA: `x*x` to jedno mnożenie IEEE,
/// a `x^2` idzie przez std::pow, który nie ma gwarancji poprawnego zaokrąglenia.
///
/// Na tej równości stoi też niezmiennik ablacyjny: przy RDB_OPT_SIMPLIFY_EXPRESSIONS=OFF
/// w planie zostaje `a*a` i musi policzyć dokładnie to samo, co `a^2`.
///
/// Reguła D stoi za osobnym przełącznikiem `aggressive_expr_optimization`, DOMYŚLNIE
/// WYŁĄCZONYM, i nie dlatego, że budzi wątpliwość co do wyniku. Powodem jest aparatura
/// badawcza: mierzone plany H9 stoją na normie `Sqrt(A[0]*A[0]+B[0]*B[0])` (15 z 21 plików
/// `test/research_gate/h9/rql/`), a `validate_corpus.py::require_main_r3_zero` wymaga, żeby
/// R3 nie przepisała ich ani razu - inaczej pomiar współdzielenia podplanu przestaje być
/// odizolowany od upraszczania wyrażeń. Włączenie reguły zapala `ninja test_gate`.
///
/// Przełącznik jest CELOWO poza rodziną `RDB_OPT_*`: tamte pięć tworzy macierz ablacyjną i
/// trafia do `xretractor --build-info`, którego ZBIÓR kluczy korpus H9 sprawdza dokładnie.
///
/// Zwijanie liczy PRODUKCYJNY ewaluator (expressionEvaluator::eval bez payloadu), więc nie
/// może się rozjechać z wykonaniem - promocje typów, 3VL i dzielenie przez zero są z definicji
/// te same. Wyrażenie, którego ewaluator nie umie policzyć (np. `'a'-'b'`), zostaje nietknięte
/// i błąd leci w wykonaniu tak jak dotąd.
///
/// Reguły B, C i D wymagają znajomości typu podwyrażenia i odmawiają przepisania dla
/// FLOAT/DOUBLE - reasocjacja zmienia tam zaokrąglenie. Typ podaje `typeOfField`, a jego
/// indeks jest PŁASKI (`a INTEGER[4]` zajmuje cztery indeksy) - patrz
/// compiler::simplifyFieldExpressions().
///
/// @note `to_string` nie zwija sie, bo token niesie DEKLARACJE szerokosci pola: zastapienie programu
///       literalem kasowalo ja i `SELECT to_string(42:16)` zwezalo sie z 16 na 2 przy drugiej kompilacji
///       zywego planu (executorsm::getAdHoc).
std::size_t simplifyExpression(std::list<token> &program, const fieldTypeLookup &typeOfField);

/// Szerokość pola `STRING` bez zadeklarowanego `N` - `to_string(expr)` bez dwukropka.
constexpr int kToStringDefaultWidth = 32;

/// Kształt pola źródłowego: typ i szerokość napisu w bajtach (`rlen * rarray`).
/// Dla pól nietekstowych szerokość jest bez znaczenia i nie jest czytana.
/// @requirements-exempt fieldShape: Opis typu i szerokosci pola przekazywany do analizy wyrazenia.
struct fieldShape {
  rdb::descFld type;
  int width;
};

/// Kształt pola źródłowego, po nazwie strumienia i PŁASKIM indeksie pola - jak
/// `fieldTypeLookup`, tylko z szerokością. std::nullopt oznacza „nie wiadomo".
using fieldShapeLookup = std::function<std::optional<fieldShape>(const std::string &, int)>;

/// Funkcja inferStringWidth powinna:
/// - [exprsimplify.infer-string-width.1~1] dla pustego programu, tokenu spoza zestawu ewaluatora (np. PUSH_STREAM, COUNT, PUSH_TSCAN) i operatora bez operandów na stosie zwracać nullopt.
/// - [exprsimplify.infer-string-width.2~1] traktować literał tekstowy jako napis swojej długości, a każdy inny literał jako liczbę.
/// - [exprsimplify.infer-string-width.3~1] traktować PUSH_ID jako napis o szerokości z shapeOfField, gdy pole źródłowe jest STRING, a każde inne odwołanie, także nierozwiązane PUSH_ID1..PUSH_ID5 i PUSH_IDX oraz pole o nieznanym kształcie, jako liczbę.
/// - [exprsimplify.infer-string-width.4~1] traktować `to_string` jako napis o szerokości zadeklarowanej w CALL2, a bez niej kToStringDefaultWidth; każdą inną funkcję i każdy operator jednoargumentowy jako liczbę.
/// - [exprsimplify.infer-string-width.5~1] traktować `+` z co najmniej jednym operandem tekstowym jako napis o sumie szerokości operandów, w której liczba wnosi 0, nasyconej na INT_MAX; pozostałe operatory dwuargumentowe jako liczbę.
/// - [exprsimplify.infer-string-width.6~1] zwracać szerokość, gdy na stosie zostaje dokładnie jedna wartość i jest napisem, przy szerokości 0 - kToStringDefaultWidth; w przeciwnym razie nullopt.
///
/// Odpowiada na jedno pytanie: czy wartość, która ZOSTAJE NA STOSIE po wykonaniu programu,
/// jest napisem, i jak szeroka. Do 2026-08-30 pytanie to było zadawane inaczej - przez skan
/// programu do PIERWSZEGO literału tekstowego - więc wyrażenie liczbowe z literałem gdziekolwiek
/// w środku lądowało w polu `STRING` (`to_integer('42')+k`, pozycja 12 w paper-arXiv/debs/done/requested.md).
///
/// Wnioskowany jest WYŁĄCZNIE napis, i to tylko na użytek `RQLParser::exitExpression`,
/// któremu wystarczy wartość początkowa pola: publiczny kształt - łącznie z szerokością
/// napisu - ustala `compiler::inferFieldShapes()` z całego programu ONP, przez
/// `inferExpressionShape()` (expressionShape.hpp). Ta druga funkcja stosuje dla napisów
/// dokładnie te same reguły stosu co ta, a dodatkowo wyprowadza typy liczbowe.
///
/// @param shapeOfField odwzorowanie PUSH_ID → kształt pola źródłowego; przy parsowaniu,
///        gdzie schematów obcych strumieni jeszcze nie ma, wolno zwracać zawsze nullopt
std::optional<int> inferStringWidth(const std::list<token> &program, const fieldShapeLookup &shapeOfField);
