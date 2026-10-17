#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "error.hpp"

namespace rdb {

constexpr size_t kBitsPerByte = 8;

/// Funkcja packedByteCount powinna:
/// - [bitsetcodec.packed-byte-count.1~1] zwracać liczbę bajtów potrzebną na bitCount bitów, czyli sufit z bitCount / 8.
constexpr size_t packedByteCount(size_t bitCount) { return (bitCount + (kBitsPerByte - 1)) / kBitsPerByte; }

/// Funkcja packBits powinna:
/// - [bitsetcodec.pack-bits.1~1] ustawiać w dest bit i % 8 bajtu i / 8 dla każdego ustawionego bitu i (od najmłodszego bitu w bajcie), nie zerując pozostałych bitów.
/// - [bitsetcodec.pack-bits.2~1] zwracać Errc::Config bez zapisu, gdy rozmiar dest różni się od packedByteCount(bits.size()).
///
/// @note Wolajacy zeruje dest: IndexRecord i ShadowOverride zapisuja bitset we wstepnie wyzerowany ogon
///       wiekszego bufora rekordu.
[[nodiscard]] Result<> packBits(const std::vector<bool> &bits, std::span<std::byte> dest);

/// Funkcja unpackBits powinna:
/// - [bitsetcodec.unpack-bits.1~1] zwracać bitCount bitów odczytanych z src w kolejności packBits.
/// - [bitsetcodec.unpack-bits.2~1] zwracać Errc::Config, gdy src ma mniej niż bitCount bitów, bez zawijania arytmetyki dla bitCount bliskiego SIZE_MAX.
[[nodiscard]] Result<std::vector<bool>> unpackBits(std::span<const std::byte> src, size_t bitCount);

}  // namespace rdb
