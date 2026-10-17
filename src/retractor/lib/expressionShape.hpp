#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <string_view>

#include "fldType.hpp"  // rdb::descFld, rdb::rField
#include "token.hpp"    // token, std::list

/// @file
/// Ksztalt wyniku programu ONP - JEDEN analizator typu dla calego kompilatora.
///
/// Do 2026-09-11 na pytanie „jakiego typu jest to pole" odpowiadalo piec niezaleznych
/// od siebie regul lokalnych, kazda dopisana przy okazji innej awarii:
///
///  * `RQLParser::exitExpression` zaczynal od `INTEGER` i rozpoznawal `FLOAT`/`DOUBLE`
///    wylacznie wtedy, gdy rzutowanie bylo OSTATNIM tokenem programu;
///  * `compiler::inferStringFieldTypes()` osobnym przebiegiem podnosil pole do `STRING`;
///  * `compiler::resolveWindowAggregates()` ustalal typ redukcji okiennej i mial wyjatek
///    na koncowe `to_integer`;
///  * `compiler::propagateCopiedFieldShapes()` przenosil ksztalt, ale WYLACZNIE pol
///    okiennych;
///  * `compiler::expandSchemaWildcards()` wpisywal `SELECT *` na sztywno `INTEGER`.
///
/// Zadna z nich nie znala wyniku posredniego, wiec `to_float('2.5')*2` konczylo sie polem
/// `INTEGER`, `SELECT source[0]` nad `DOUBLE` gubilo typ producenta, a
/// `to_integer(AVG(x:10))+1` wracalo do `RATIONAL`. Pozycja 16 w
/// `paper-arXiv/debs/done/requested.md` opisuje wszystkie trzy granice.
///
/// Ten modul zastepuje tamte reguly jednym przejsciem po stosie typow. Zasada jest jedna
/// i brzmi: **analizator odtwarza to, co robi `expressionEvaluator`** - nie kolejnosc enum,
/// nie intuicje, tylko rzeczywista arytmetyka wariantu. Tam, gdzie ewaluator promuje
/// (`uint8_t + uint8_t` daje `int`), promuje i analizator; tam, gdzie zachowuje typ
/// (`neg`, `Abs`, funkcje matematyczne przez `callFun`), zachowuje i analizator.
///
/// Analizator NIE zalezy od upraszczania wyrazen: `simplifyExpression()` jest z zalozenia
/// zachowawcze typowo (regula C odmawia usuniecia elementu neutralnego o innej
/// reprezentacji niz podwyrazenie - patrz `dropNeutralOperand`), wiec deskryptor jest
/// ten sam przy `RDB_OPT_SIMPLIFY_EXPRESSIONS` wlaczonym i wylaczonym.

/// Ksztalt wartosci: typ, dlugosc JEDNEGO elementu w bajtach i krotnosc.
///
/// Dla typow liczbowych `rarray` wynosi zawsze 1 - wartoscia wyrazenia jest jedna liczba,
/// nawet gdy operand byl elementem tablicy. Dla `STRING` obowiazuje zapis deskryptora
/// `rlen = 1`, `rarray = szerokosc w bajtach`.
/// @requirements-exempt exprShape: Opis ksztaltu wartosci; sameAs porownuje wylacznie pola rekordu.
struct exprShape {
  rdb::descFld rtype = rdb::INTEGER;
  int rlen           = static_cast<int>(sizeof(int));
  int rarray         = 1;

  [[nodiscard]] bool sameAs(const exprShape &other) const {
    return rtype == other.rtype && rlen == other.rlen && rarray == other.rarray;
  }
};

/// Wynik analizy programu.
///
///  * `resolved`  - ksztalt znany;
///  * `unknown`   - program siega po cos, czego na tym etapie nie da sie rozstrzygnac
///                  (nierozwiazane `PUSH_ID1/2/3`, token spoza zestawu ewaluatora,
///                  nieznana grupa okna). Wolajacy ZOSTAWIA pole nietkniete;
///  * `illTyped`  - program nie ma wartosci: operand tekstowy pod operatorem liczbowym,
///                  pusty stos, wiecej niz jedna wartosc na koncu. Wolajacy takze zostawia
///                  pole nietkniete - blad ma polecic w WYKONANIU, dokladnie tam, gdzie
///                  lecial dotad.
///
///  * `rejected` - program jest skladniowo poprawny i typ da sie policzyc, ale kombinacja
///                  funkcji i typu argumentu NIE JEST ZAIMPLEMENTOWANA i policzylaby zla
///                  wartosc. Wolajacy ZATRZYMUJE kompilacje i podaje `reason` uzytkownikowi.
///                  Dzisiejsze przypadki to `Sqrt`, `sin`, `cos` i `exp` nad `RATIONAL` -
///                  patrz opis przy `rejectedIrrationalOverExact()` w expressionShape.cpp,
///                  gdzie stoi, dlaczego kazda z tych nazw trafila tam z innego powodu.
///
/// Rozroznienie `unknown` od `illTyped` nie zmienia decyzji kompilatora; istnieje po to,
/// zeby test mogl odroznic „jeszcze nie wiadomo" od „to sie nie policzy". `rejected` jest
/// inne od obu: to JEDYNY status, ktory konczy kompilacje bledem.
enum class exprShapeStatus : std::uint8_t { resolved, unknown, illTyped, rejected };

/// @requirements-exempt exprShapeResult: Wynik analizy ksztaltu; metody udostepniaja tylko zapisany status.
struct exprShapeResult {
  exprShapeStatus status = exprShapeStatus::unknown;
  exprShape shape;
  /// Wypelniony WYLACZNIE dla `rejected` - komunikat dla kanalu `Check result:`.
  std::string reason;

  [[nodiscard]] bool resolved() const { return status == exprShapeStatus::resolved; }
  [[nodiscard]] bool rejected() const { return status == exprShapeStatus::rejected; }
};

/// Ksztalt pola zrodlowego, po nazwie strumienia i PLASKIM indeksie slotu.
///
/// Wolajacy odpowiada za sprowadzenie wpisu deskryptora do ksztaltu POJEDYNCZEGO slotu:
/// `INTEGER[24]` czytane przez `PUSH_ID` daje jedna liczbe, wiec `rarray = 1`. `STRING[N]`
/// jest jednym slotem i zachowuje `rarray = N`. `std::nullopt` znaczy „nie wiadomo" i daje
/// wynik `unknown`.
using exprFieldShapeFn = std::function<std::optional<exprShape>(const std::string &streamId, int flatIndex)>;

/// Ksztalt wyniku grupy okna rekordowego, po jej indeksie w `query::windowGroups`.
/// Pusty `std::function` znaczy „plan bez okien" i kazdy token `WINDOW_*` daje `unknown`.
using exprWindowShapeFn = std::function<std::optional<exprShape>(int groupIndex)>;

/// Funkcja fieldLengthOfType powinna:
/// - [expressionshape.field-length-of-type.1~1] zwracać długość jednego elementu pola w bajtach: 1 dla BYTE i STRING, sizeof(int) dla INTEGER, sizeof(unsigned) dla UINT, sizeof(boost::rational<int>) dla RATIONAL, sizeof(float) dla FLOAT i sizeof(double) dla DOUBLE.
/// - [expressionshape.field-length-of-type.2~1] dla każdego innego typu zwracać sizeof(int).
///
/// @note Jedna definicja dla calego drzewa.
int fieldLengthOfType(rdb::descFld type);

/// Funkcja numericShape powinna:
/// - [expressionshape.numeric-shape.1~1] zwracać kształt o podanym typie, długości fieldLengthOfType(type) i krotności rarray = 1.
exprShape numericShape(rdb::descFld type);

/// Funkcja normalizedOperandType powinna:
/// - [expressionshape.normalized-operand-type.1~1] zwracać typ o wyższym indeksie wariantu z dwóch podanych, tak jak `normalize()` z `expressionEvaluator`.
///
/// @note To NIE jest typ wyniku operacji arytmetycznej - dla `BYTE` operator C++ promuje jeszcze do `int`.
///       Patrz `arithmeticValueType()`.
rdb::descFld normalizedOperandType(rdb::descFld left, rdb::descFld right);

/// Funkcja arithmeticValueType powinna:
/// - [expressionshape.arithmetic-value-type.1~1] zwracać typ wartości operatora arytmetycznego (`+`, `-`, `*`, `/`, `^`): normalizedOperandType(left, right), z wyjątkiem pary BYTE i BYTE, która daje INTEGER.
///
/// Wyjatek dla `BYTE op BYTE`: `uint8_t + uint8_t` w C++ promuje sie do `int` i wlasnie `int`
/// laduje w wariancie (`expressionEvaluator::operator+` i pozostale). Ta sama promocja stoi za
/// `exactPower()`, wiec `bajt ^ k` dla `k >= 1` takze jest `INTEGER`.
rdb::descFld arithmeticValueType(rdb::descFld left, rdb::descFld right);

/// Funkcja functionResultType powinna:
/// - [expressionshape.function-result-type.1~1] rozpoznawać nazwę funkcji bez względu na wielkość liter.
/// - [expressionshape.function-result-type.2~1] dla `to_integer`, `to_float`, `to_double` i `to_string` zwracać odpowiednio INTEGER, FLOAT, DOUBLE i STRING, niezależnie od argumentu.
/// - [expressionshape.function-result-type.3~1] dla `isnull`, `IsZero`, `IsNonZero` i `Length` zwracać INTEGER, niezależnie od argumentu.
/// - [expressionshape.function-result-type.4~1] dla `sin`, `cos` i `exp` zwracać DOUBLE, także nad argumentem całkowitym i RATIONAL.
/// - [expressionshape.function-result-type.5~1] dla `Abs`, `null2zero`, `Floor`, `Ceil`, `Sqrt`, `round`, `trunc`, `tan`, `log` i `log2` zwracać typ argumentu, a nullopt, gdy typ argumentu jest nieznany.
/// - [expressionshape.function-result-type.6~1] dla nazwy spoza tej tabeli zwracać nullopt.
///
/// @note Jedna polityka dla calego drzewa. `callFun` liczy funkcje matematyczne w `double` i rzutuje z powrotem
///       na typ wejscia, wiec `Ceil` nad `DOUBLE` daje `DOUBLE`, a nie `INTEGER`. `sin`, `cos`, `exp` licza kat
///       w RADIANACH, `NULL` przepuszczaja, a wynik niefinitywny (`exp(1000)`) daja jako `NULL`. Argument
///       `RATIONAL` funkcji o niewymiernej przeciwdziedzinie odrzuca inferExpressionShape, nie ta funkcja.
std::optional<rdb::descFld> functionResultType(std::string_view name, std::optional<rdb::descFld> argumentType);

/// Funkcja isExactType powinna:
/// - [expressionshape.is-exact-type.1~1] zwracać true wyłącznie dla typów o arytmetyce dokładnej i łącznej: BYTE, INTEGER, UINT i RATIONAL.
///
/// @note Calkowite pozostaja laczne takze w arytmetyce modulo 2^n. FLOAT i DOUBLE sa poza zbiorem swiadomie:
///       reasocjacja zmienia tam liczbe zaokraglen - dla float x = 2^24 mamy (x+1)+1 == x, ale x+2 == x+2,
///       wiec przepisanie w exprSimplify zmieniloby wynik.
bool isExactType(rdb::descFld type);

/// Funkcja inferExpressionShape powinna:
/// - [expressionshape.infer-expression-shape.1~1] dla pustego programu zwracać unknown.
/// - [expressionshape.infer-expression-shape.2~1] wykonywać program ONP na stosie kształtów jak `expressionEvaluator::eval()` na stosie wartości, zdejmując operandy przed rozpoznaniem operacji i zwracając illTyped, gdy na stosie brakuje operandów.
/// - [expressionshape.infer-expression-shape.3~1] dla literału napisowego `PUSH_VAL` kłaść STRING o szerokości równej długości napisu, dla literału liczbowego numericShape jego typu, a dla wartości o typie powyżej DOUBLE zwracać unknown.
/// - [expressionshape.infer-expression-shape.4~1] dla `PUSH_ID` kłaść kształt z shapeOfField(strumień, płaski indeks slotu), a zwracać unknown, gdy token nie niesie pary albo shapeOfField jest pusty lub zwraca nullopt.
/// - [expressionshape.infer-expression-shape.5~1] dla nierozwiązanych odwołań `PUSH_ID1`..`PUSH_ID5` i `PUSH_IDX` oraz dla każdego tokenu spoza zestawu ewaluatora zwracać unknown.
/// - [expressionshape.infer-expression-shape.6~1] dla `WINDOW_MIN`, `WINDOW_MAX`, `WINDOW_AVG` i `WINDOW_SUM` kłaść kształt z shapeOfWindow(indeks grupy), a zwracać unknown, gdy token nie niesie liczby int albo shapeOfWindow jest pusty lub zwraca nullopt.
/// - [expressionshape.infer-expression-shape.7~1] dla `+` z operandem STRING kłaść STRING o sumie szerokości operandów tekstowych, przy czym operand liczbowy wnosi zero.
/// - [expressionshape.infer-expression-shape.8~1] dla `+`, `-`, `*`, `/` i `^` nad typami liczbowymi kłaść numericShape(arithmeticValueType), a dla operandu o typie powyżej DOUBLE (także napisu pod `-`, `*`, `/`, `^`) zwracać illTyped.
/// - [expressionshape.infer-expression-shape.9~1] dla porównań kłaść INTEGER, gdy typ znormalizowany operandów to STRING, w przeciwnym razie numericShape typu znormalizowanego bez promocji BYTE, a dla typu znormalizowanego INTPAIR albo IDXPAIR zwracać illTyped.
/// - [expressionshape.infer-expression-shape.10~1] dla `AND` i `OR` kłaść INTEGER przy lewym operandzie STRING, a w przeciwnym razie kształt lewego operandu, zwracając illTyped dla operandu o typie powyżej STRING.
/// - [expressionshape.infer-expression-shape.11~1] dla jednoargumentowego `-` zachowywać kształt operandu liczbowego, odrzucać (rejected) operand STRING, a dla każdego innego typu powyżej DOUBLE zwracać illTyped.
/// - [expressionshape.infer-expression-shape.12~1] dla `~` zachowywać kształt operandu BYTE i UINT, odrzucać (rejected) z nazwą typu operand INTEGER, RATIONAL, FLOAT, DOUBLE i STRING, a dla pozostałych typów zwracać illTyped.
/// - [expressionshape.infer-expression-shape.13~1] dla `NOT` kłaść INTEGER nad STRING, a w przeciwnym razie kształt operandu, zwracając illTyped dla typu powyżej STRING.
/// - [expressionshape.infer-expression-shape.14~1] dla `to_string` kłaść STRING o szerokości zadeklarowanej w `CALL2`, a bez niej kToStringDefaultWidth; dla `null2zero` zachowywać kształt operandu, także napisu i jego szerokość.
/// - [expressionshape.infer-expression-shape.15~1] odrzucać (rejected) z komunikatem dla kanału `Check result:` funkcje `Sqrt`, `sin`, `cos`, `exp`, `tan`, `log` i `log2` nad RATIONAL, nazywając funkcję jej kanoniczną pisownią i wskazując `to_double`.
/// - [expressionshape.infer-expression-shape.16~1] dla pozostałych funkcji kłaść numericShape(functionResultType), zwracając unknown dla nazwy spoza tabeli, a illTyped dla `sin`, `cos`, `exp` nad typem powyżej DOUBLE oraz dla funkcji, której wynik byłby STRING.
/// - [expressionshape.infer-expression-shape.17~1] po wykonaniu programu zwracać illTyped, gdy na stosie nie została dokładnie jedna wartość, a w przeciwnym razie resolved z jej kształtem, przy czym napis o zerowej szerokości dostaje szerokość kToStringDefaultWidth.
///
/// @param program program pola albo warunku, w ONP
/// @param shapeOfField ksztalt pola zrodlowego dla `PUSH_ID`
/// @param shapeOfWindow ksztalt wyniku grupy okna dla `WINDOW_*`; moze byc pusty
///
/// @note Jednoargumentowy `-` nad napisem to odmowa planu, a nie illTyped, bo wyjatek w slocie konczy serwer
///       (#328); `~` tylko dla BYTE i UINT to decyzja wlasciciela (#328). Typ `AND`/`OR` zalezy w wykonaniu od
///       WARTOSCI (lewy operand, chyba ze NULL), ale operatory logiczne zyja tylko w warunku RULE i nie
///       opisuja deskryptora. Napis zerowej szerokosci dostaje szerokosc domyslna, zeby nie powstalo `STRING[0]`.
exprShapeResult inferExpressionShape(const std::list<token> &program, const exprFieldShapeFn &shapeOfField,
                                     const exprWindowShapeFn &shapeOfWindow);
