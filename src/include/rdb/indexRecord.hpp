#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "error.hpp"

namespace rdb {

/// @brief Single entry in a meta index – a null bit-set pattern and count
///        of consecutive records sharing that pattern.
/// Obiekt klasy IndexRecord powinien:
/// - [indexrecord.index-record.1~1] przechowywać wzorzec null jako jeden bit na pole deskryptora, liczbę kolejnych rekordów o tym wzorcu oraz znacznik przerwy transmisji.
/// - [indexrecord.index-record.2~1] przez serialize zapisywać wpis jako bajt znacznika przerwy (1 dla przerwy, 0 w przeciwnym razie), recordCount i długość bitsetu jako size_t w porządku bajtów maszyny, a następnie bitset upakowany od najmłodszego bitu w ceil(n/8) bajtach z zerowymi bitami dopełnienia.
/// - [indexrecord.index-record.3~1] przez deserialize odtwarzać wpis zapisany przez serialize, traktując każdy niezerowy bajt znacznika jako przerwę.
/// - [indexrecord.index-record.4~1] zwracać z deserialize Errc::IO z kodem EIO, gdy ramka jest za krótka na nagłówek albo deklarowana długość bitsetu przekracza pozostałe bajty, porównując długość przed zaokrągleniem do bajtów, tak aby skrajna długość nie powodowała odczytu poza buforem.
/// - [indexrecord.index-record.5~1] zachowywać format wpisu zgodny z istniejącymi plikami .meta i z formatem wpisów metaShadow::ShadowOverride.
struct IndexRecord {
  std::vector<bool> nullBitset;                            ///< one bit per descriptor field (true = null)
  size_t recordCount{0};                                   ///< number of consecutive records with this pattern
  bool isGap{false};                                       ///< true if this entry represents a transmission gap
  [[nodiscard]] std::vector<std::byte> serialize() const;  ///< serialize the entry to a vector of bytes
  /// Odtwarza wpis; blad IO opisuje uszkodzona ramke lub niepoprawna dlugosc bitsetu.
  [[nodiscard]] static Result<IndexRecord> deserialize(std::span<const std::byte> data);
};

}  // namespace rdb
