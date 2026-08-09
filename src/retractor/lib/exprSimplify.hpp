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
///  * C — elementy neutralne: `E+0`, `E-0`, `E*1`, `E/1` → `E`.
///
/// Świadomie NIE ma tu `E*0` → `0`: `NULL*0` daje NULL, więc pochłanianie złamałoby 3VL.
///
/// Zwijanie liczy PRODUKCYJNY ewaluator (expressionEvaluator::eval bez payloadu), więc nie
/// może się rozjechać z wykonaniem — promocje typów, 3VL i dzielenie przez zero są z definicji
/// te same. Wyrażenie, którego ewaluator nie umie policzyć (np. `'a'-'b'`), zostaje nietknięte
/// i błąd leci w wykonaniu tak jak dotąd.
///
/// Reguły B i C wymagają znajomości typu podwyrażenia i odmawiają przepisania dla FLOAT/DOUBLE
/// — reasocjacja zmienia tam zaokrąglenie. Program z tokenem spoza zestawu ewaluatora
/// (PUSH_STREAM, COUNT, PUSH_IDX...) nie jest ruszany w ogóle.
///
/// @param program wyrażenie w ONP; podmieniane tylko wtedy, gdy jakaś reguła zadziałała
/// @param typeOfField odwzorowanie PUSH_ID → typ pola źródłowego
/// @return liczba zastosowanych przepisań (0 = program nietknięty)
std::size_t simplifyExpression(std::list<token> &program, const fieldTypeLookup &typeOfField);
