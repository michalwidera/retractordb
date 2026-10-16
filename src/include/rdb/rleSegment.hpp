#pragma once

#include <cstddef>
#include <vector>

#include "indexRecord.hpp"

namespace rdb {

/// Funkcja splitSegment powinna:
/// - dzielić segment RLE w pozycji offset na 1 do 3 części w kolejności: prefiks offset rekordów z bitsetem segmentu (tylko dla offset > 0), jeden rekord z newBitset i sufiks pozostałych rekordów z bitsetem segmentu (tylko gdy jakieś zostały).
/// - oznaczać wszystkie części jako niebędące przerwą.
///
/// @note Uzywana, gdy jeden rekord segmentu wielorekordowego dostaje poprawiony wzorzec NULL.
std::vector<IndexRecord> splitSegment(const IndexRecord &seg, size_t offset, const std::vector<bool> &newBitset);

/// Funkcja sumNonGapRecords powinna:
/// - zwracać sumę recordCount wpisów niebędących przerwą; znaczniki przerwy nie zużywają indeksów rekordów.
size_t sumNonGapRecords(const std::vector<IndexRecord> &entries);

}  // namespace rdb
