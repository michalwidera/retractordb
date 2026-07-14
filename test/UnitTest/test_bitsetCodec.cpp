// Testy wspólnego kodeka bitsetu (bitsetCodec.hpp)
//
// packBits/unpackBits to jedno źródło prawdy formatu pakowania wzorców null,
// współdzielone przez IndexRecord (indeks .meta) i metaShadow::ShadowOverride
// (cień indeksu .meta.shadow). Kluczowy przypadek: bitset dłuższy niż 8 bitów
// (deskryptor z >8 polami) — pakowanie przekracza granicę bajtu, czego testy
// serializacji rekordów nie ćwiczą wprost.

#include <gtest/gtest.h>

#include "rdb/bitsetCodec.hpp"

namespace {

std::vector<bool> roundTrip(const std::vector<bool> &bits) {
  std::vector<std::byte> buf(rdb::packedByteCount(bits.size()), std::byte{0});
  rdb::packBits(bits, buf);
  return rdb::unpackBits(buf, bits.size());
}

}  // namespace

TEST(BitsetCodecTest, packedByteCount_zaokragla_w_gore) {
  EXPECT_EQ(rdb::packedByteCount(0), 0U);
  EXPECT_EQ(rdb::packedByteCount(1), 1U);
  EXPECT_EQ(rdb::packedByteCount(8), 1U);
  EXPECT_EQ(rdb::packedByteCount(9), 2U);
  EXPECT_EQ(rdb::packedByteCount(16), 2U);
  EXPECT_EQ(rdb::packedByteCount(17), 3U);
}

TEST(BitsetCodecTest, roundtrip_w_obrebie_jednego_bajtu) {
  const std::vector<bool> bits = {true, false, true, true, false};
  EXPECT_EQ(roundTrip(bits), bits);
}

TEST(BitsetCodecTest, roundtrip_przekracza_granice_bajtu) {
  // 13 bitów = 2 bajty; wzorzec asymetryczny wykrywa pomylenie kolejności bitów/bajtów.
  const std::vector<bool> bits = {true,  false, false, true,  true, false, true, false,  // bajt 0
                                  false, true,  true,  false, true};                     // bajt 1
  EXPECT_EQ(roundTrip(bits), bits);
}

TEST(BitsetCodecTest, roundtrip_skrajne_wzorce) {
  EXPECT_EQ(roundTrip(std::vector<bool>(17, true)), std::vector<bool>(17, true));
  EXPECT_EQ(roundTrip(std::vector<bool>(17, false)), std::vector<bool>(17, false));
  EXPECT_EQ(roundTrip(std::vector<bool>{}), std::vector<bool>{});
}

TEST(BitsetCodecTest, packBits_ustawia_tylko_jedynki) {
  // Kontrakt: bufor zerowany przez wywołującego; packBits nie dotyka bitów 0
  // (tak IndexRecord i ShadowOverride pakują bitset do wyzerowanego ogona bufora rekordu).
  const std::vector<bool> bits = {false, true, false, false, false, false, false, false, true};
  std::vector<std::byte> buf(rdb::packedByteCount(bits.size()), std::byte{0});
  rdb::packBits(bits, buf);
  EXPECT_EQ(std::to_integer<uint8_t>(buf[0]), 0b00000010);
  EXPECT_EQ(std::to_integer<uint8_t>(buf[1]), 0b00000001);
}
