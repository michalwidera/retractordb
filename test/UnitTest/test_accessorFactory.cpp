// Scenariusze użycia fabryki akcesorów (accessorFactory)
//
// Fabryka skupia w jednym miejscu odwzorowanie nazwy typu magazynu na konkretną
// implementację FileInterface oraz dobór wariantu indeksu metadanych null
// (inertny/cień indeksu/bazowy) — wydzielone z klasy storage, która zna odtąd
// wyłącznie abstrakcyjny FileInterface.

#include <gtest/gtest.h>

#include <filesystem>

#include "rdb/accessorFactory.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccbindev.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/faccposixshd.hpp"
#include "rdb/facctxtsrc.hpp"
#include "rdb/storageShadow.hpp"

namespace {
rdb::Descriptor testDescriptor() { return {rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER)}; }
}  // namespace

TEST(AccessorFactoryTest, declared_types) {
  EXPECT_TRUE(rdb::isDeclaredType("DEVICE"));
  EXPECT_TRUE(rdb::isDeclaredType("TEXTSOURCE"));
  EXPECT_FALSE(rdb::isDeclaredType("DEFAULT"));
  EXPECT_FALSE(rdb::isDeclaredType("POSIX"));
}

// ---------------------------------------------------------------------------
// Odwzorowanie typu magazynu na konkretną klasę akcesora; DEFAULT/POSIXSHD
// utrzymują plik cienia danych (hasShadow()), pozostałe nie.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, maps_type_to_accessor_class) {
  auto desc = testDescriptor();

  auto posix = rdb::makeAccessor("POSIX", "af_posix.bin", desc, false, -1);
  EXPECT_NE(dynamic_cast<rdb::posixBinaryFile *>(posix.get()), nullptr);
  EXPECT_FALSE(posix->hasShadow());

  auto posixShd = rdb::makeAccessor("POSIXSHD", "af_posixshd.bin", desc, false, -1);
  EXPECT_NE(dynamic_cast<rdb::posixBinaryFileWithShadow *>(posixShd.get()), nullptr);
  EXPECT_TRUE(posixShd->hasShadow());

  auto generic = rdb::makeAccessor("GENERIC", "af_generic.bin", desc, false, -1);
  EXPECT_NE(dynamic_cast<rdb::genericBinaryFile *>(generic.get()), nullptr);
  EXPECT_FALSE(generic->hasShadow());

  auto byDefault = rdb::makeAccessor("DEFAULT", "af_default.bin", desc, false, -1);
  EXPECT_TRUE(byDefault->hasShadow());

  auto direct = rdb::makeAccessor("DIRECT", "af_direct.bin", desc, false, -1);
  EXPECT_FALSE(direct->hasShadow());
}

// ---------------------------------------------------------------------------
// Źródła deklarowane: DEVICE/TEXTSOURCE są tylko do odczytu, bez cienia danych.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, maps_declared_sources) {
  auto desc = testDescriptor();

  auto device = rdb::makeAccessor("DEVICE", "af_missing.dev", desc, true, -1);
  EXPECT_NE(dynamic_cast<rdb::binaryDeviceRO *>(device.get()), nullptr);
  EXPECT_FALSE(device->hasShadow());

  auto text = rdb::makeAccessor("TEXTSOURCE", "af_missing.txt", desc, true, -1);
  EXPECT_NE(dynamic_cast<rdb::textSourceRO *>(text.get()), nullptr);
  EXPECT_FALSE(text->hasShadow());
}

// ---------------------------------------------------------------------------
// Dobór wariantu indeksu metadanych: inertny dla deklarowanych (bez pliku),
// storageShadow przy cieniu danych, bazowy metaData w pozostałych przypadkach.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, meta_index_variant_selection) {
  const auto desc = testDescriptor();

  auto inert = rdb::makeMetaIndex(true, false, desc, "af_decl.meta");
  EXPECT_EQ(dynamic_cast<rdb::storageShadow *>(inert.get()), nullptr);
  EXPECT_TRUE(inert->isEmpty());
  EXPECT_FALSE(std::filesystem::exists("af_decl.meta"));  // wariant inertny nie dotyka pliku

  auto shadowed = rdb::makeMetaIndex(false, true, desc, "af_shd.meta");
  EXPECT_NE(dynamic_cast<rdb::storageShadow *>(shadowed.get()), nullptr);

  auto plain = rdb::makeMetaIndex(false, false, desc, "af_plain.meta");
  EXPECT_EQ(dynamic_cast<rdb::storageShadow *>(plain.get()), nullptr);
}
