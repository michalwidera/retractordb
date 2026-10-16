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
/// - przyjmować listę tokenów reprezentujących wyrażenie do wyznaczenia.
///   Test: `xExpressionEval.long_program`.
/// - przyjmować opcjonalny wskaźnik do obiektu payload, który może być używany do pobierania wartości zmiennych w wyrażeniu.
///   Test: `xExpressionEval.push_id_reads_value_from_payload`, `xExpressionEval.push_id_null_payload_throws`.
/// - wykonywać operatory ADD (+), SUBTRACT (-), MULTIPLY (*), DIVIDE (/) i POWER (^) dla typów liczbowych
///   BYTE, INTEGER, UINT, RATIONAL, FLOAT i DOUBLE; dla STRING dopuszczać ADD jako konkatenację,
///   Test: `xExpressionEval.numeric_arithmetic_and_comparisons_cover_all_promotions`, `xExpressionEval.add_string_int`, `xExpressionEval.add_string_float_converts_via_to_string`.
/// - obsługiwać warianty operandów BYTE (uint8_t), INTEGER (int), UINT (unsigned int), RATIONAL
///   (boost::rational<int>), FLOAT (float), DOUBLE (double), INTPAIR (std::pair<int, int>), IDXPAIR
///   (std::pair<std::string, int>), STRING (std::string) i NULLTYPE (std::monostate),
///   Test: `xExpressionEval.numeric_arithmetic_and_comparisons_cover_all_promotions`, `xExpressionEval.pair_arithmetic_and_refusals_preserve_the_contract`, `xExpressionEval.add_string_int`.
///   Test: `xExpressionEval.add_null_int_propagates_null`.
/// - przed +, -, *, /, ^ i porównaniami sprowadzać operandy różnych typów do typu o wyższym indeksie
///   descFldVT przez cast<descFldVT>; kolejność indeksów jest taka jak powyższa lista typów; arytmetyka
///   +, -, *, /, ^ na dwóch BYTE daje INTEGER, na pozostałych zgodnych typach liczbowych zachowuje typ,
///   Test: `xExpressionEval.numeric_arithmetic_and_comparisons_cover_all_promotions`, `xExpressionEval.add_rational_string`, `xExpressionEval.float_add_int_stays_float_at_2p24`.
/// - dla mieszanej pary INTEGER/UINT wykonywać +, -, *, / i porównania na dokładnych wartościach także
///   przy ujemnym INTEGER; wynik arytmetyczny zwracać jako UINT albo NULL, jeśli nie mieści się w UINT,
///   a porównanie jako UINT 0 lub 1; dla potęgowania takiej pary zawężać do UINT dopiero obliczony wynik,
///   Test: `xExpressionEval.uint_with_negative_int_is_exact_when_result_fits`, `xExpressionEval.uint_with_negative_int_is_null_when_result_does_not_fit`, `xExpressionEval.comparison_of_uint_with_negative_int_is_exact`.
///   Test: `xExpressionEval.power_of_uint_with_negative_int_is_exact_when_result_fits`.
/// - dla BYTE, INTEGER, UINT i RATIONAL z nieujemnym całkowitym wykładnikiem liczyć potęgę jako iloczyn
///   kolejnych czynników; dla wykładnika zero zwracać jedynkę typu iloczynu (dla BYTE INTEGER, jak przy
///   wykładniku dodatnim); pozostałe
///   potęgi liczyć przez std::pow w double i konwertować do typu po normalizacji; NaN i nieskończoność dają NULL,
///   Test: `xExpressionEval.pow_on_exact_types_matches_multiplication`, `xExpressionEval.exact_power_zero_exponent_is_one_of_the_product_type`, `xExpressionEval.pow_negative_exponent_truncates_on_integer_base`.
///   Test: `xExpressionEval.pow_non_finite_result_gives_null`, `xExpressionEval.negated_byte_power_with_zero_exponent_is_minus_one`.
/// - wykonywać CMP_EQUAL (=), CMP_NOT_EQUAL (!=), CMP_LT (<), CMP_GT (>), CMP_LE (<=) i CMP_GE (>=)
///   dla liczb oraz STRING; porównania liczb zwracają 0 lub 1 w typie po normalizacji, porównania tekstu
///   zwracają INTEGER 0 lub 1; INTPAIR i IDXPAIR w porównaniach odrzucać błędem Errc::Eval,
///   Test: `xExpressionEval.numeric_arithmetic_and_comparisons_cover_all_promotions`, `xExpressionEval.cmp_equal_null_propagates_null`, `xExpressionEval.cmp_not_equal_string_equal`.
///   Test: `xExpressionEval.cmp_lt_string_true`, `xExpressionEval.cmp_gt_string_true`, `xExpressionEval.cmp_le_string_true_equal`.
///   Test: `xExpressionEval.cmp_ge_string_true_equal`, `xExpressionEval.pair_arithmetic_and_refusals_preserve_the_contract`.
/// - wykonywać NEGATE: dla INTEGER, RATIONAL, FLOAT i DOUBLE zmieniać znak, dla BYTE i UINT odwracać
///   bity; BIT_NOT (~) dopuszczać wyłącznie dla BYTE i UINT; oba operatory przepuszczają NULL,
///   Test: `xExpressionEval.minus_and_bitwise_not_agree_on_byte_and_uint`, `xExpressionEval.bitwise_not_outside_byte_and_uint_throws`, `xExpressionEval.unary_minus_stays_arithmetic_for_signed_types`.
///   Test: `xExpressionEval.neg_null_propagates_null`.
/// - dla INTPAIR wykonywać +, -, *, / po obu składowych i NEGATE przez zmianę obu znaków; dla IDXPAIR
///   ADD łączy napisy i dodaje drugie składowe, SUBTRACT/MULTIPLY/DIVIDE zwracają pierwszą składową
///   odpowiednio "?? -"/"?? *"/"?? /" i wykonują działanie na drugich składowych, NEGATE dodaje prefiks
///   "-" do napisu i zmienia znak drugiej składowej; POWER dla obu rodzajów par odrzucać błędem Errc::Eval,
///   Test: `xExpressionEval.pair_arithmetic_and_refusals_preserve_the_contract`, `xExpressionEval.divide_intpair_by_zero_component_yields_null`, `xExpressionEval.divide_idxpair_by_zero_index_yields_null`.
/// - wykonywać AND, OR i NOT w logice trójwartościowej: zero liczbowe i pusty STRING oznaczają fałsz,
///   pozostałe wartości liczbowe i niepusty STRING prawdę; pary odrzucać błędem Errc::Eval; określony
///   wynik ma typ pierwszego nie-null operandu (NOT: argumentu), a dla STRING jest INTEGER 0 lub 1,
///   Test: `xExpressionEval.three_valued_truth_tables_cover_both_operand_orders`, `xExpressionEval.logic_result_uses_the_first_non_null_operand_type`, `xExpressionEval.not_null_is_null`.
///   Test: `xExpressionEval.and_string_int_uses_string_truthiness`, `xExpressionEval.pair_arithmetic_and_refusals_preserve_the_contract`.
/// - obsługiwać CALL bez rozróżniania wielkości liter dla nazw Floor, Ceil, Sqrt, Round, Sin, Cos, Exp,
///   Tan, Log, Log2, Trunc, IsNull, Null2Zero, Abs, IsZero, IsNonZero, Length, to_integer, to_float,
///   to_double i to_string; CALL2 obsługuje wyłącznie nazwę to_string (dokladnie ta pisownia), również konwertując przez cast<descFldVT>,
///   Test: `xExpressionEval.call_function_name_as_grammar_writes_it`, `xExpressionEval.scalar_math_functions_preserve_numeric_types_and_reject_pairs`, `xExpressionEval.real_functions_return_double_for_exact_arguments`.
///   Test: `xExpressionEval.call_iszero_and_isnonzero_return_integer`, `xExpressionEval.call_length_counts_value_not_field_width`, `xExpressionEval.call2_accepts_only_the_canonical_to_string_name`.
///   Test: `xExpressionEval.call_to_integer_from_string`, `xExpressionEval.call_to_float_from_int`, `xExpressionEval.call_to_double_from_int`.
///   Test: `xExpressionEval.call_to_string_from_int`, `xExpressionEval.isnull_returns_1_for_null`, `xExpressionEval.null2zero_replaces_null_with_zero`.
/// - dla liczbowych Floor, Ceil, Sqrt, Round, Tan, Log, Log2 i Trunc liczyć funkcję w double i konwertować
///   wynik z powrotem do typu argumentu; Sin, Cos i Exp zwracają DOUBLE albo NULL dla wyniku niefinitywnego;
///   Abs zachowuje typ liczbowy; IsZero i IsNonZero zwracają INTEGER 0 lub 1 dla liczb; Length zwraca
///   długość STRING w bajtach jako INTEGER; wymienione funkcje propagują NULL i odrzucają inne typy błędem Errc::Eval,
///   Test: `xExpressionEval.scalar_math_functions_preserve_numeric_types_and_reject_pairs`, `xExpressionEval.call_floor_function_rational`, `xExpressionEval.call_round_function_double`.
///   Test: `xExpressionEval.real_functions_return_double_for_exact_arguments`, `xExpressionEval.real_functions_propagate_null_and_reject_text`, `xExpressionEval.exp_overflow_returns_null`.
///   Test: `xExpressionEval.call_iszero_and_isnonzero_return_integer`, `xExpressionEval.call_length_counts_value_not_field_width`, `xExpressionEval.call_length_rejects_numeric_operand`.
///   Test: `xExpressionEval.new_functions_propagate_null`.
/// - zwracać z IsNull INTEGER 1 dla NULL i 0 dla pozostałych wartości, a z Null2Zero INTEGER 0 dla NULL
///   i niezmieniony argument w pozostałych przypadkach; to_integer, to_float, to_double i to_string
///   konwertują przez cast<descFldVT> do INTEGER, FLOAT, DOUBLE i STRING, propagując NULL,
///   Test: `xExpressionEval.isnull_returns_1_for_null`, `xExpressionEval.isnull_returns_0_for_non_null`, `xExpressionEval.null2zero_replaces_null_with_zero`.
///   Test: `xExpressionEval.null2zero_passes_a_value_through_unchanged`, `xExpressionEval.call_to_integer_from_string`, `xExpressionEval.call_to_float_from_string`.
///   Test: `xExpressionEval.call_to_double_from_string`, `xExpressionEval.call_to_string_from_int`, `xExpressionEval.call_to_integer_null_propagates_null`.
///   Test: `xExpressionEval.call_to_float_null_propagates_null`, `xExpressionEval.call_to_double_null_propagates_null`, `xExpressionEval.call_to_string_null_propagates_null`.
/// - odkładać na stos wartość PUSH_VAL oraz odczyt pola payload dla PUSH_ID i PUSH_ID2; tokeny WINDOW_MIN,
///   WINDOW_MAX, WINDOW_AVG i WINDOW_SUM pobierają odpowiednio minValue, maxValue, avgValue i sumValue
///   z przekazanej tablicy windowValues, według indeksu grupy zapisanego w tokenie.
///   Test: `xExpressionEval.add_int_int`, `xExpressionEval.push_id_reads_value_from_payload`, `xExpressionEval.push_id2_reads_value_from_payload`.
///   Test: `xExpressionEval.window_tokens_read_the_selected_group_and_reject_invalid_indices`.
/// - zwalniać własną pamięć roboczą po zakończeniu eval(), również gdy wynikiem jest błąd.
///   Test: `xExpressionEval.deep_stack_is_released_after_success_and_error`.
/// - zwracać wynik wyrażenia jako rdb::Result<rdb::descFldVT>, którego wartość może reprezentować różne typy danych.
///   Test: `xExpressionEval.numeric_arithmetic_and_comparisons_cover_all_promotions`, `xExpressionEval.malformed_stack_throws`.
/// - zgłaszać błędem Result o kodzie Errc::Eval brak operandów, nieznane funkcje lub tokeny oraz operatory
///   nieokreślone dla typu.
///   Test: `xExpressionEval.malformed_stack_throws`, `xExpressionEval.empty_program_throws`, `xExpressionEval.call_unknown_function_keeps_author_spelling`.
///   Test: `xExpressionEval.deep_stack_is_released_after_success_and_error`, `xExpressionEval.pair_arithmetic_and_refusals_preserve_the_contract`.
/// - dla dzielenia przez zero oraz przepełnienia wykrytego przez checkedArith zwracać std::monostate (NULL)
///   jako udany wynik Result.
///   Test: `xExpressionEval.divide_by_zero_yields_null_for_every_numeric_type`, `xExpressionEval.int_overflow_returns_null`, `xExpressionEval.uint_arithmetic_at_the_bound_is_exact`.
///   Test: `xExpressionEval.uint_add_and_mul_overflow_is_null`, `xExpressionEval.rational_overflow_returns_null`.
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
  [[nodiscard]] rdb::Result<rdb::descFldVT> eval(const std::list<token> &program, rdb::payload *payload = nullptr,
                                                 const std::vector<windowStats> *windowValues = nullptr);
};
