#pragma once

#include <vector>

#include "fldType.hpp"
#include "query.hpp"  // windowStats
#include "rdb/error.hpp"
#include "rdb/payload.hpp"
#include "token.hpp"  // token, std::list

/// @brief Klasa opisująca proces wyznaczania wartości wyrażenia.
///
/// Obiekt klasy expressionEvaluator powinien:
/// - [expressionevaluator.expression-evaluator.1~1] przyjmować listę tokenów reprezentujących wyrażenie do wyznaczenia.
/// - [expressionevaluator.expression-evaluator.2~1] przyjmować opcjonalny wskaźnik do obiektu payload, który może być używany do pobierania wartości zmiennych w wyrażeniu.
/// - [expressionevaluator.expression-evaluator.3~1] wykonywać operatory ADD (+), SUBTRACT (-), MULTIPLY (*), DIVIDE (/) i POWER (^) dla typów liczbowych
///   BYTE, INTEGER, UINT, RATIONAL, FLOAT i DOUBLE; dla STRING dopuszczać ADD jako konkatenację,
/// - [expressionevaluator.expression-evaluator.4~1] obsługiwać warianty operandów BYTE (uint8_t), INTEGER (int), UINT (unsigned int), RATIONAL
///   (boost::rational<int>), FLOAT (float), DOUBLE (double), INTPAIR (std::pair<int, int>), IDXPAIR
///   (std::pair<std::string, int>), STRING (std::string) i NULLTYPE (std::monostate),
/// - [expressionevaluator.expression-evaluator.5~1] przed +, -, *, /, ^ i porównaniami sprowadzać operandy różnych typów do typu o wyższym indeksie
///   descFldVT przez cast<descFldVT>; kolejność indeksów jest taka jak powyższa lista typów; arytmetyka
///   +, -, *, /, ^ na dwóch BYTE daje INTEGER, na pozostałych zgodnych typach liczbowych zachowuje typ,
/// - [expressionevaluator.expression-evaluator.6~1] dla mieszanej pary INTEGER/UINT wykonywać +, -, *, / i porównania na dokładnych wartościach także
///   przy ujemnym INTEGER; wynik arytmetyczny zwracać jako UINT albo NULL, jeśli nie mieści się w UINT,
///   a porównanie jako UINT 0 lub 1; dla potęgowania takiej pary zawężać do UINT dopiero obliczony wynik,
/// - [expressionevaluator.expression-evaluator.7~1] dla BYTE, INTEGER, UINT i RATIONAL z nieujemnym całkowitym wykładnikiem liczyć potęgę jako iloczyn
///   kolejnych czynników; dla wykładnika zero zwracać jedynkę typu iloczynu (dla BYTE INTEGER, jak przy
///   wykładniku dodatnim); pozostałe
///   potęgi liczyć przez std::pow w double i konwertować do typu po normalizacji; NaN i nieskończoność dają NULL,
/// - [expressionevaluator.expression-evaluator.8~1] wykonywać CMP_EQUAL (=), CMP_NOT_EQUAL (!=), CMP_LT (<), CMP_GT (>), CMP_LE (<=) i CMP_GE (>=)
///   dla liczb oraz STRING; porównania liczb zwracają 0 lub 1 w typie po normalizacji, porównania tekstu
///   zwracają INTEGER 0 lub 1; INTPAIR i IDXPAIR w porównaniach odrzucać błędem Errc::Eval,
/// - [expressionevaluator.expression-evaluator.9~1] wykonywać NEGATE: dla INTEGER, RATIONAL, FLOAT i DOUBLE zmieniać znak, dla BYTE i UINT odwracać
///   bity; BIT_NOT (~) dopuszczać wyłącznie dla BYTE i UINT; oba operatory przepuszczają NULL,
/// - [expressionevaluator.expression-evaluator.10~2] dla INTPAIR odrzucać ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER i NEGATE błędem Errc::Eval, także z operandem innego typu lub NULL; dla IDXPAIR
///   ADD łączy napisy i dodaje drugie składowe, SUBTRACT/MULTIPLY/DIVIDE zwracają pierwszą składową
///   odpowiednio "?? -"/"?? *"/"?? /" i wykonują działanie na drugich składowych, NEGATE dodaje prefiks
///   "-" do napisu i zmienia znak drugiej składowej; POWER dla obu rodzajów par odrzucać błędem Errc::Eval,
/// - [expressionevaluator.expression-evaluator.11~1] wykonywać AND, OR i NOT w logice trójwartościowej: zero liczbowe i pusty STRING oznaczają fałsz,
///   pozostałe wartości liczbowe i niepusty STRING prawdę; pary odrzucać błędem Errc::Eval; określony
///   wynik ma typ pierwszego nie-null operandu (NOT: argumentu), a dla STRING jest INTEGER 0 lub 1,
/// - [expressionevaluator.expression-evaluator.12~1] obsługiwać CALL bez rozróżniania wielkości liter dla nazw Floor, Ceil, Sqrt, Round, Sin, Cos, Exp,
///   Tan, Log, Log2, Trunc, IsNull, Null2Zero, Abs, IsZero, IsNonZero, Length, to_integer, to_float,
///   to_double i to_string; CALL2 obsługuje wyłącznie nazwę to_string (dokladnie ta pisownia), również konwertując przez cast<descFldVT>,
/// - [expressionevaluator.expression-evaluator.13~1] dla liczbowych Floor, Ceil, Sqrt, Round, Tan, Log, Log2 i Trunc liczyć funkcję w double i konwertować
///   wynik z powrotem do typu argumentu; Sin, Cos i Exp zwracają DOUBLE albo NULL dla wyniku niefinitywnego;
///   Abs zachowuje typ liczbowy; IsZero i IsNonZero zwracają INTEGER 0 lub 1 dla liczb; Length zwraca
///   długość STRING w bajtach jako INTEGER; wymienione funkcje propagują NULL i odrzucają inne typy błędem Errc::Eval,
/// - [expressionevaluator.expression-evaluator.14~1] zwracać z IsNull INTEGER 1 dla NULL i 0 dla pozostałych wartości, a z Null2Zero INTEGER 0 dla NULL
///   i niezmieniony argument w pozostałych przypadkach; to_integer, to_float, to_double i to_string
///   konwertują przez cast<descFldVT> do INTEGER, FLOAT, DOUBLE i STRING, propagując NULL,
/// - [expressionevaluator.expression-evaluator.15~1] odkładać na stos wartość PUSH_VAL oraz odczyt pola payload dla PUSH_ID i PUSH_ID2; tokeny WINDOW_MIN,
///   WINDOW_MAX, WINDOW_AVG i WINDOW_SUM pobierają odpowiednio minValue, maxValue, avgValue i sumValue
///   z przekazanej tablicy windowValues, według indeksu grupy zapisanego w tokenie.
/// - [expressionevaluator.expression-evaluator.20~1] dla PUSH_VAR odkładać na stos wartość z przekazanej tablicy variables według indeksu z pary (nazwa, indeks) w tokenie, także NULL; brak tablicy albo indeks spoza niej zgłaszać błędem Errc::Eval.
/// - [expressionevaluator.expression-evaluator.16~1] zwalniać własną pamięć roboczą po zakończeniu eval(), również gdy wynikiem jest błąd.
/// - [expressionevaluator.expression-evaluator.17~1] zwracać wynik wyrażenia jako rdb::Result<rdb::descFldVT>, którego wartość może reprezentować różne typy danych.
/// - [expressionevaluator.expression-evaluator.18~1] zgłaszać błędem Result o kodzie Errc::Eval brak operandów, nieznane funkcje lub tokeny oraz operatory
///   nieokreślone dla typu.
/// - [expressionevaluator.expression-evaluator.19~1] dla dzielenia przez zero oraz przepełnienia wykrytego przez checkedArith zwracać std::monostate (NULL)
///   jako udany wynik Result.
///
/// @note Każde wyrażenie składa się z operatorów i operandów, reprezentowanych jako tokeny. Wykorzystuje stos do przechowywania pośrednich wyników obliczeń. Obsługuje operatory arytmetyczne, logiczne i porównania, a także negację. Umożliwia normalizację typów operandów przed wykonaniem operacji, zapewniając zgodność typów. Może być rozszerzona o dodatkowe operatory i funkcje w przyszłości.
/// @note W wyrażeniach mogą się pojawić wartości null, które powinny być obsługiwane zgodnie z zasadami logiki trójwartościowej (3VL). Porównanie wartości null z inną wartością zwraca null. Operacje logiczne z udziałem null są obsługiwane zgodnie z SQL 3VL: true OR null = true, false AND null = false, w pozostałych przypadkach wynik jest null (np. true AND null = null, false OR null = null).
/// @note Logika trójwartościowa (3VL) w SQL obsługuje wartość NULL jako trzeci stan – „nieznany” (unknown), obok prawdy (TRUE) i fałszu (FALSE). Występuje, gdy brakuje danych, a porównania z NULL (np. x = NULL) dają wynik nieznany, a nie fałsz.
class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator() = default;

  /// @param windowValues wyniki okien rekordowych tego taktu, indeksowane numerem grupy
  ///        (qTree::windowGroups). Wypełnia je dataModel::computeWindowAggregates() PRZED
  ///        wyliczeniem pól, bo okno czyta historię ŹRÓDŁA, a payload wejściowy niesie tylko
  ///        rekord bieżący. nullptr znaczy: plan bez okien.
  /// Wartosc wyrazenia RPN albo Errc::Eval, gdy programu nie da sie policzyc dla tych operandow
  /// (operator nieokreslony dla typu, brak operandu, nieznana funkcja). NULL - brak wyniku w zbiorze
  /// wartosci (dzielenie przez zero, przepelnienie) - jest wynikiem, nie bledem.
  /// @param variables wartosci zmiennych VAR planu w tym takcie, indeksowane jak qTree::variables
  ///        (#531). Trzyma je dataModel. nullptr znaczy: brak zmiennych - PUSH_VAR jest wtedy
  ///        bledem programu, wiec zwijanie stalych (exprSimplify, bez tablicy) nie moze go policzyc.
  [[nodiscard]] rdb::Result<rdb::descFldVT> eval(const std::list<token> &program, const rdb::payload *payload = nullptr,
                                                 const std::vector<windowStats> *windowValues = nullptr,
                                                 const std::vector<rdb::descFldVT> *variables = nullptr);
};
