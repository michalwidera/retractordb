#pragma once

#include <any>
#include <boost/rational.hpp>

#include "fldType.hpp"

inline constexpr double kDefaultRationalizeDiff    = 1E-6;
inline constexpr int kDefaultRationalizeIterations = 11;

/// Obiekt klasy cast powinien:
/// - [converttypes.cast.1~1] obsługiwać specjalizacje dla descFldVT i std::any; dla docelowego NULLTYPE zwracać std::monostate.
/// - [converttypes.cast.2~1] dla wejściowego std::monostate zwracać typowaną wartość nullFallbackValue zamiast znacznika NULL.
/// - [converttypes.cast.3~1] przy konwersji liczb do BYTE, INTEGER i UINT sprawdzać zakres po obcięciu części ułamkowej ku zeru;
///   wartość poza zakresem, NaN i nieskończoność zamieniać na std::monostate.
///   Zakres testow: `cast_variant.double_at_integer_bounds_is_exact`, `cast_variant.fraction_past_integer_bounds_truncates_into_range`,
///   `cast_variant.int_outside_byte_range_is_null`, `cast_variant.rational_to_byte_uses_the_integer_part`,
///   `cast_variant.negative_rational_to_uint_is_null`, `cast_variant.double_nan_to_integer_is_null`,
///   `cast_variant.double_infinity_to_integer_is_null`, `cast_any.double_above_integer_range_is_null`.
/// - [converttypes.cast.4~1] przy konwersji FLOAT/DOUBLE do RATIONAL zwracać NULL dla wartości nieskończonych i NaN oraz trunc(abs(x)) >= 2^31,
///   a przy konwersji UINT do RATIONAL dla wartości ponad INT_MAX; pozostałe wartości przybliżać przez Rationalize.
///   Zakres testow: `cast_variant.double_nan_to_rational_is_null`, `cast_variant.double_infinity_to_rational_is_null`,
///   `cast_variant.finite_double_outside_rational_range_is_null`, `cast_variant.uint_above_int_max_to_rational_is_null`,
///   `cast_variant.negative_double_to_rational`, `cast_any.finite_float_outside_rational_range_is_null`.
/// - [converttypes.cast.5~1] odmawiać konwersji skalaru do INTPAIR oraz pary INTPAIR/IDXPAIR do skalaru liczbowego przez NULL;
///   konwersja do IDXPAIR wartości innej niż wejściowy std::monostate również daje NULL.
///   Zakres testow: `cast_variant.scalar_to_intpair_is_null`, `cast_any.scalar_to_intpair_is_null`,
///   `cast_variant.pair_to_scalar_is_null`, `cast_any.pair_to_scalar_is_null`,
///   `cast_contract.invalid_rational_pairs_and_idxpair_targets_are_null`.
/// - [converttypes.cast.6~1] dla pustego std::any i typu spoza descFldVT zwracać NULL na obsługiwanych ścieżkach konwersji do danych.
/// - [converttypes.cast.7~1] konwertować STRING do typów skalarnych przez from_chars: prefiks int dla BYTE/INTEGER/UINT, prefiks double
///   dla FLOAT/DOUBLE; odrzucać błąd parsowania lub zakresu bez wymogu odczytania całego napisu.
/// - [converttypes.cast.8~1] czytać STRING do RATIONAL/INTPAIR przez strumień z separatorem '/' lub ',' i wartościami początkowymi 0, 1;
///   błąd strumienia nie daje sam w sobie NULL. Para do RATIONAL daje NULL dla mianownika 0 lub INT_MIN oraz
///   licznika INT_MIN przy ujemnym mianowniku; poprawna para jest normalizowana.
///   Zakres testow: `cast_contract.string_prefixes_and_pair_parsing_match_both_interfaces`,
///   `cast_contract.invalid_rational_pairs_and_idxpair_targets_are_null`.
/// - [converttypes.cast.9~1] konwertować IDXPAIR do RATIONAL przez drugi element z mianownikiem 1, a do INTPAIR przez atoi pierwszego
///   elementu z zachowaniem drugiego; konwersja INTPAIR do INTPAIR zachowuje parę.
/// - [converttypes.cast.10~1] formatować liczby, RATIONAL i pary jako STRING, rozdzielając pary przecinkiem, a ułamek ukośnikiem.
///   Zakres testow: `cast_variant.int_to_string`, `cast_variant.double_to_string`, `cast_variant.rational_to_string`,
///   `cast_variant.intpair_to_string`, `cast_variant.idxpair_to_string`, `cast_any.rational_to_string`.
template <typename T>
struct cast {
  T operator()(const T &inVar, rdb::descFld reqType);
};

/// Funkcja any_to_variant_cast powinna:
/// - [converttypes.any-to-variant-cast.1~1] dla pustego std::any oraz std::any z std::monostate zwracać std::monostate (NULL).
/// - [converttypes.any-to-variant-cast.2~1] dla std::any z wartością typu uint8_t, int, unsigned, boost::rational<int>, float, double albo std::string zwracać tę samą wartość jako alternatywę rdb::descFldVT tego typu.
/// - [converttypes.any-to-variant-cast.3~1] traktować std::any z typem spoza tej listy, także z parą INTPAIR lub IDXPAIR, jako naruszenie niezmiennika i kończyć proces przez rdb::fatal z nazwą typu w komunikacie.
rdb::descFldVT any_to_variant_cast(std::any a);

/// Funkcja nullFallbackValue powinna:
/// - [converttypes.null-fallback-value.1~1] zwracać dla typu pola wartość zastępującą NULL: zero w alternatywie tego typu dla BYTE, INTEGER, UINT, FLOAT i DOUBLE, 0/1 dla RATIONAL, parę (0, 0) dla INTPAIR, parę ("", 0) dla IDXPAIR, pusty napis dla STRING i std::monostate dla NULLTYPE.
/// - [converttypes.null-fallback-value.2~1] dla pól metadeskryptora TYPE, REF, RETENTION i RETMEMORY zwracać uint8_t(0).
rdb::descFldVT nullFallbackValue(rdb::descFld type);

/// Funkcja Rationalize powinna:
/// - [converttypes.rationalize.1~1] zwracać konwergent rozwinięcia modułu inValue w ułamek łańcuchowy, ze znakiem inValue.
/// - [converttypes.rationalize.2~1] kończyć rozwinięcie, gdy reszta ułamkowa spadnie poniżej DIFF, albo po uwzględnieniu ttl_const + 1 wyrazów.
/// - [converttypes.rationalize.3~1] zwracać ostatni konwergent, którego licznik i mianownik mieszczą się w int, gdy następny by się w nim nie zmieścił.
/// - [converttypes.rationalize.4~1] zwracać 0/1 dla NaN, nieskończoności i wartości, których moduł obcięty do części całkowitej wynosi co najmniej 2^31.
/// - [converttypes.rationalize.5~1] nie zwracać NULL: o NULL dla wartości bez reprezentacji w RATIONAL decyduje ścieżka konwersji cast.
///
/// @param DIFF prog reszty ulamkowej konczacy rozwiniecie
/// @param ttl_const limit wyrazow rozwiniecia ponad pierwszy
boost::rational<int> Rationalize(double inValue, double DIFF = kDefaultRationalizeDiff,
                                 int ttl_const = kDefaultRationalizeIterations);
