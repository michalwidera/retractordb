#pragma once

#include <any>
#include <boost/rational.hpp>

#include "fldType.hpp"

inline constexpr double kDefaultRationalizeDiff    = 1E-6;
inline constexpr int kDefaultRationalizeIterations = 11;

template <typename T>
struct cast {
  /// @brief Convert input value to requested descriptor type.
  /// @param inVar source value (std::variant or std::any specializations are used in this project)
  /// @param reqType target rdb field type
  /// @return converted value represented as type T, or std::monostate (NULL) when the value has
  ///         no representation in the requested type: a floating-point value outside an integer
  ///         type's range (NaN and infinity included), an integer or rational value outside the
  ///         range of a narrower integer type (a negative value to UINT, UINT above INT_MAX to
  ///         INTEGER or RATIONAL, a value outside 0..255 to BYTE), any scalar to INTPAIR (a pair is
  ///         two independent numbers, not a fraction), a pair (INTPAIR, IDXPAIR) to BYTE, INTEGER,
  ///         UINT, FLOAT or DOUBLE, a std::any holding a type outside descFldVT, or a string that
  ///         does not parse
  T operator()(const T &inVar, rdb::descFld reqType);
};

/// Funkcja any_to_variant_cast powinna:
/// - dla pustego std::any oraz std::any z std::monostate zwracać std::monostate (NULL).
/// - dla std::any z wartością typu uint8_t, int, unsigned, boost::rational<int>, float, double albo std::string zwracać tę samą wartość jako alternatywę rdb::descFldVT tego typu.
/// - traktować std::any z typem spoza tej listy, także z parą INTPAIR lub IDXPAIR, jako naruszenie niezmiennika i kończyć proces przez rdb::fatal z nazwą typu w komunikacie.
rdb::descFldVT any_to_variant_cast(std::any a);

/// Funkcja nullFallbackValue powinna:
/// - zwracać dla typu pola wartość zastępującą NULL: zero w alternatywie tego typu dla BYTE, INTEGER, UINT, FLOAT i DOUBLE, 0/1 dla RATIONAL, parę (0, 0) dla INTPAIR, parę ("", 0) dla IDXPAIR, pusty napis dla STRING i std::monostate dla NULLTYPE.
/// - dla pól metadeskryptora TYPE, REF, RETENTION i RETMEMORY zwracać uint8_t(0).
rdb::descFldVT nullFallbackValue(rdb::descFld type);

/// Funkcja Rationalize powinna:
/// - zwracać konwergent rozwinięcia modułu inValue w ułamek łańcuchowy, ze znakiem inValue.
/// - kończyć rozwinięcie, gdy reszta ułamkowa spadnie poniżej DIFF, albo po uwzględnieniu ttl_const + 1 wyrazów.
/// - zwracać ostatni konwergent, którego licznik i mianownik mieszczą się w int, gdy następny by się w nim nie zmieścił.
/// - zwracać 0/1 dla NaN, nieskończoności i wartości, których moduł obcięty do części całkowitej wynosi co najmniej 2^31.
/// - nie zwracać NULL: o NULL dla wartości bez reprezentacji w RATIONAL decyduje ścieżka konwersji cast.
///
/// @param DIFF prog reszty ulamkowej konczacy rozwiniecie
/// @param ttl_const limit wyrazow rozwiniecia ponad pierwszy
boost::rational<int> Rationalize(double inValue, double DIFF = kDefaultRationalizeDiff,
                                 int ttl_const = kDefaultRationalizeIterations);
