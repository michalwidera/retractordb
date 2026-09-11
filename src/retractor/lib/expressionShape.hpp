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
/// Ksztalt wyniku programu ONP — JEDEN analizator typu dla calego kompilatora.
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
/// `paper-arXiv/usecases/requested.md` opisuje wszystkie trzy granice.
///
/// Ten modul zastepuje tamte reguly jednym przejsciem po stosie typow. Zasada jest jedna
/// i brzmi: **analizator odtwarza to, co robi `expressionEvaluator`** — nie kolejnosc enum,
/// nie intuicje, tylko rzeczywista arytmetyka wariantu. Tam, gdzie ewaluator promuje
/// (`uint8_t + uint8_t` daje `int`), promuje i analizator; tam, gdzie zachowuje typ
/// (`neg`, `Abs`, funkcje matematyczne przez `callFun`), zachowuje i analizator.
///
/// Analizator NIE zalezy od upraszczania wyrazen: `simplifyExpression()` jest z zalozenia
/// zachowawcze typowo (regula C odmawia usuniecia elementu neutralnego o innej
/// reprezentacji niz podwyrazenie — patrz `dropNeutralOperand`), wiec deskryptor jest
/// ten sam przy `RDB_OPT_SIMPLIFY_EXPRESSIONS` wlaczonym i wylaczonym.

/// Ksztalt wartosci: typ, dlugosc JEDNEGO elementu w bajtach i krotnosc.
///
/// Dla typow liczbowych `rarray` wynosi zawsze 1 — wartoscia wyrazenia jest jedna liczba,
/// nawet gdy operand byl elementem tablicy. Dla `STRING` obowiazuje zapis deskryptora
/// `rlen = 1`, `rarray = szerokosc w bajtach`.
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
///  * `resolved`  — ksztalt znany;
///  * `unknown`   — program siega po cos, czego na tym etapie nie da sie rozstrzygnac
///                  (nierozwiazane `PUSH_ID1/2/3`, token spoza zestawu ewaluatora,
///                  nieznana grupa okna). Wolajacy ZOSTAWIA pole nietkniete;
///  * `illTyped`  — program nie ma wartosci: operand tekstowy pod operatorem liczbowym,
///                  pusty stos, wiecej niz jedna wartosc na koncu. Wolajacy takze zostawia
///                  pole nietkniete — blad ma polecic w WYKONANIU, dokladnie tam, gdzie
///                  lecial dotad.
///
///  * `rejected` — program jest skladniowo poprawny i typ da sie policzyc, ale kombinacja
///                  funkcji i typu argumentu NIE JEST ZAIMPLEMENTOWANA i policzylaby zla
///                  wartosc. Wolajacy ZATRZYMUJE kompilacje i podaje `reason` uzytkownikowi.
///                  Jedyny dzisiejszy przypadek to `Sqrt` nad `RATIONAL` — patrz TODO przy
///                  `rejectedIrrationalOverExact()` w expressionShape.cpp.
///
/// Rozroznienie `unknown` od `illTyped` nie zmienia decyzji kompilatora; istnieje po to,
/// zeby test mogl odroznic „jeszcze nie wiadomo" od „to sie nie policzy". `rejected` jest
/// inne od obu: to JEDYNY status, ktory konczy kompilacje bledem.
enum class exprShapeStatus : std::uint8_t { resolved, unknown, illTyped, rejected };

struct exprShapeResult {
  exprShapeStatus status = exprShapeStatus::unknown;
  exprShape shape;
  /// Wypelniony WYLACZNIE dla `rejected` — komunikat dla kanalu `Check result:`.
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

/// Dlugosc jednego elementu pola danego typu, w bajtach — jedna definicja dla calego drzewa.
int fieldLengthOfType(rdb::descFld type);

/// Ksztalt pola liczbowego danego typu (`rarray = 1`).
exprShape numericShape(rdb::descFld type);

/// Typ operandow po `normalize()` z `expressionEvaluator`: wygrywa WYZSZY indeks wariantu.
///
/// To NIE jest typ wyniku operacji arytmetycznej — dla `BYTE` operator C++ promuje jeszcze
/// do `int`. Patrz `arithmeticValueType()`.
rdb::descFld normalizedOperandType(rdb::descFld left, rdb::descFld right);

/// Typ WARTOSCI operatora arytmetycznego (`+`, `-`, `*`, `/`, `^`).
///
/// Rowny `normalizedOperandType()` z jednym wyjatkiem: `BYTE op BYTE` daje `INTEGER`, bo
/// `uint8_t + uint8_t` w C++ promuje sie do `int` i wlasnie `int` laduje w wariancie
/// (`expressionEvaluator::operator+` i pozostale). Ta sama promocja stoi za `exactPower()`,
/// wiec `bajt ^ k` dla `k >= 1` takze jest `INTEGER`.
rdb::descFld arithmeticValueType(rdb::descFld left, rdb::descFld right);

/// Typ wyniku funkcji skalarnej, po nazwie (bez wzgledu na wielkosc liter) i typie argumentu.
///
/// Jedna polityka dla calego drzewa:
///  * `isnull`, `IsZero`, `IsNonZero`, `Length` — zawsze `INTEGER`;
///  * `to_integer`, `to_float`, `to_double`, `to_string` — typ docelowy, takze wewnatrz
///    wiekszego wyrazenia;
///  * `Abs`, `null2zero` oraz funkcje matematyczne liczone przez `callFun`
///    (`Sqrt`, `Ceil`, `Floor`, `round`, `trunc`, `sin`, `cos`, `tan`, `log`, `log2`) —
///    typ ARGUMENTU. `callFun` liczy w `double` i rzutuje z powrotem na typ wejscia, wiec
///    `Ceil` nad `DOUBLE` daje `DOUBLE`, a nie `INTEGER`.
///
/// `std::nullopt` dla nazwy spoza tabeli albo dla nieznanego typu argumentu tam, gdzie typ
/// wyniku od niego zalezy.
std::optional<rdb::descFld> functionResultType(std::string_view name, std::optional<rdb::descFld> argumentType);

/// Typy o arytmetyce DOKLADNEJ i lacznej — `BYTE`, `INTEGER`, `UINT`, `RATIONAL`.
bool isExactType(rdb::descFld type);

/// @brief Ksztalt wyniku programu ONP — typ, dlugosc elementu i krotnosc.
///
/// Wykonuje program na stosie KSZTALTOW, tak samo jak `expressionEvaluator::eval()`
/// wykonuje go na stosie WARTOSCI, i obsluguje ten sam zestaw tokenow: `PUSH_VAL`,
/// `PUSH_ID`, operatory jedno- i dwuargumentowe, `CALL`/`CALL2` oraz rozwiazane agregaty
/// okienne `WINDOW_*`.
///
/// @param program program pola albo warunku, w ONP
/// @param shapeOfField ksztalt pola zrodlowego dla `PUSH_ID`
/// @param shapeOfWindow ksztalt wyniku grupy okna dla `WINDOW_*`; moze byc pusty
exprShapeResult inferExpressionShape(const std::list<token> &program, const exprFieldShapeFn &shapeOfField,
                                     const exprWindowShapeFn &shapeOfWindow);
