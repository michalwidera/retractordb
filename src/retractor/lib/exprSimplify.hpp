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

/// @brief Uproszczenia algebraiczne wyrażenia w ONP, w miejscu.
///
/// Zwija stałe i kanonizuje wyrażenia, żeby plan wykonania był krótszy już po kompilacji —
/// program pola liczony jest raz na interwał, więc każdy usunięty token jest oszczędnością
/// powtarzaną przez cały czas życia strumienia.
///
/// Reguły:
///  * A — zwijanie stałych: `1+1` → `2`, `'a'+'b'` → `'ab'`, `Sqrt(4)` → `2`;
///  * B — reasocjacja ogona stałych: `E+c1+c2` → `E+(c1+c2)` (także `-`, `*` oraz stała
///        po lewej: `c1-E-c2` → `(c1-c2)-E`);
///  * C — elementy neutralne: `E+0`, `E-0`, `E*1`, `E/1` → `E`;
///  * D — powtórzony czynnik jako potęga: `a*a` → `a^2`, `a*a*a` → `a^3`, i tak dalej dla
///        dowolnie długiego łańcucha oraz dla dowolnego powtórzonego podwyrażenia
///        (`(x+1)*(x+1)` → `(x+1)^2`). WYŁĄCZNIE przy `aggressive_expr_optimization=ON`.
///
/// Świadomie NIE ma tu `E*0` → `0`: `NULL*0` daje NULL, więc pochłanianie złamałoby 3VL.
///
/// Regułę D warto czytać jako oszczędność ODCZYTÓW, nie mnożeń: `a*a` czyta payload dwa razy,
/// `a^2` raz, a mnożeń jest tyle samo. Wolno ją zastosować wyłącznie dla typów o arytmetyce
/// DOKŁADNEJ (BYTE, INTEGER, UINT, RATIONAL), bo tylko tam `^` jest z definicji tym samym, co
/// zapisany wprost iloczyn — expressionEvaluator liczy taką potęgę tym samym operator*, którego
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
/// R3 nie przepisała ich ani razu — inaczej pomiar współdzielenia podplanu przestaje być
/// odizolowany od upraszczania wyrażeń. Włączenie reguły zapala `ninja test_gate`.
///
/// Przełącznik jest CELOWO poza rodziną `RDB_OPT_*`: tamte pięć tworzy macierz ablacyjną i
/// trafia do `xretractor --build-info`, którego ZBIÓR kluczy korpus H9 sprawdza dokładnie.
///
/// Zwijanie liczy PRODUKCYJNY ewaluator (expressionEvaluator::eval bez payloadu), więc nie
/// może się rozjechać z wykonaniem — promocje typów, 3VL i dzielenie przez zero są z definicji
/// te same. Wyrażenie, którego ewaluator nie umie policzyć (np. `'a'-'b'`), zostaje nietknięte
/// i błąd leci w wykonaniu tak jak dotąd.
///
/// Reguły B, C i D wymagają znajomości typu podwyrażenia i odmawiają przepisania dla
/// FLOAT/DOUBLE — reasocjacja zmienia tam zaokrąglenie. Typ podaje `typeOfField`, a jego
/// indeks jest PŁASKI (`a INTEGER[4]` zajmuje cztery indeksy) — patrz
/// compiler::simplifyFieldExpressions(). Program z tokenem spoza zestawu ewaluatora
/// (PUSH_STREAM, COUNT, PUSH_IDX...) nie jest ruszany w ogóle.
///
/// @param program wyrażenie w ONP; podmieniane tylko wtedy, gdy jakaś reguła zadziałała
/// @param typeOfField odwzorowanie PUSH_ID → typ pola źródłowego
/// @return liczba zastosowanych przepisań (0 = program nietknięty)
std::size_t simplifyExpression(std::list<token> &program, const fieldTypeLookup &typeOfField);

/// Szerokość pola `STRING` bez zadeklarowanego `N` — `to_string(expr)` bez dwukropka.
constexpr int kToStringDefaultWidth = 32;

/// Kształt pola źródłowego: typ i szerokość napisu w bajtach (`rlen * rarray`).
/// Dla pól nietekstowych szerokość jest bez znaczenia i nie jest czytana.
struct fieldShape {
  rdb::descFld type;
  int width;
};

/// Kształt pola źródłowego, po nazwie strumienia i PŁASKIM indeksie pola — jak
/// `fieldTypeLookup`, tylko z szerokością. std::nullopt oznacza „nie wiadomo".
using fieldShapeLookup = std::function<std::optional<fieldShape>(const std::string &, int)>;

/// @brief Szerokość pola wyjściowego, gdy wynikiem wyrażenia jest napis.
///
/// Odpowiada na jedno pytanie: czy wartość, która ZOSTAJE NA STOSIE po wykonaniu programu,
/// jest napisem, i jak szeroka. Do 2026-08-30 pytanie to było zadawane inaczej — przez skan
/// programu do PIERWSZEGO literału tekstowego — więc wyrażenie liczbowe z literałem gdziekolwiek
/// w środku lądowało w polu `STRING` (`to_integer('42')+k`, pozycja 12 w usecases/requested.md).
///
/// Wnioskowany jest WYŁĄCZNIE napis. Typy liczbowe zostają przy regule, którą stosuje
/// `RQLParser::exitExpression`: `INTEGER`, chyba że ostatnim tokenem jest `to_float`
/// albo `to_double`. Rozszerzenie na propagację `FLOAT`/`DOUBLE` byłoby zmianą znaczenia
/// istniejących planów — `Ceil(x)` nad `DOUBLE` daje dziś pole `INTEGER` i tego wymaga
/// test integracyjny `fncall_runtime_case`.
///
/// Reguły stosu: `to_string` → napis zadeklarowanej szerokości (albo kToStringDefaultWidth),
/// literał tekstowy → napis swojej długości, `PUSH_ID` → napis, gdy pole źródłowe jest
/// `STRING`, `ADD` dwóch napisów → konkatenacja (suma szerokości). Wszystko pozostałe jest
/// liczbą. Token spoza zestawu ewaluatora (PUSH_STREAM, COUNT, PUSH_TSCAN...) przerywa
/// wnioskowanie — tak samo jak w simplifyExpression().
///
/// @param program wyrażenie w ONP
/// @param shapeOfField odwzorowanie PUSH_ID → kształt pola źródłowego; przy parsowaniu,
///        gdzie schematów obcych strumieni jeszcze nie ma, wolno zwracać zawsze nullopt
/// @return szerokość pola `STRING`, albo nullopt gdy wynik nie jest napisem lub nie wiadomo
std::optional<int> inferStringWidth(const std::list<token> &program, const fieldShapeLookup &shapeOfField);
