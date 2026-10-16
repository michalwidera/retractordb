#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "error.hpp"

namespace rdb {

constexpr size_t kBitsPerByte = 8;

/// Funkcja packedByteCount powinna:
/// - zwracać liczbę bajtów potrzebną na bitCount bitów, czyli sufit z bitCount / 8.
constexpr size_t packedByteCount(size_t bitCount) { return (bitCount + (kBitsPerByte - 1)) / kBitsPerByte; }

/// Funkcja packBits powinna:
/// - ustawiać w dest bit i % 8 bajtu i / 8 dla każdego ustawionego bitu i (od najmłodszego bitu w bajcie), nie zerując pozostałych bitów.
/// - zwracać Errc::Config bez zapisu, gdy rozmiar dest różni się od packedByteCount(bits.size()).
///
/// @note Wolajacy zeruje dest: IndexRecord i ShadowOverride zapisuja bitset we wstepnie wyzerowany ogon
///       wiekszego bufora rekordu.
[[nodiscard]] Result<> packBits(const std::vector<bool> &bits, std::span<std::byte> dest);

/// Funkcja unpackBits powinna:
/// - zwracać bitCount bitów odczytanych z src w kolejności packBits.
/// - zwracać Errc::Config, gdy src ma mniej niż bitCount bitów, bez zawijania arytmetyki dla bitCount bliskiego SIZE_MAX.
[[nodiscard]] Result<std::vector<bool>> unpackBits(std::span<const std::byte> src, size_t bitCount);

}  // namespace rdb
