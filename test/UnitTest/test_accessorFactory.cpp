// Scenariusze użycia fabryki akcesorów (accessorFactory)
//
// Fabryka skupia w jednym miejscu odwzorowanie nazwy typu magazynu na konkretną
// implementację FileInterface oraz dobór wariantu indeksu metadanych null
// (inertny/cień indeksu/bazowy) - wydzielone z klasy storage, która zna odtąd
// wyłącznie abstrakcyjny FileInterface.

#include <gtest/gtest.h>

#include <sys/stat.h>  // S_IF*

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
  EXPECT_TRUE(rdb::isDeclaredType("BINFILE"));
  EXPECT_TRUE(rdb::isDeclaredType("DEVICE"));
  EXPECT_TRUE(rdb::isDeclaredType("TEXTSOURCE"));
  EXPECT_FALSE(rdb::isDeclaredType("DEFAULT"));
  EXPECT_FALSE(rdb::isDeclaredType("POSIX"));
}

// Profile zapisywalne to dokladnie te, ktore przyjmuje `STORAGE` w SELECT i `:SUBSTRAT` (#346).
TEST(AccessorFactoryTest, writable_types) {
  for (const auto *type : {"DEFAULT", "DIRECT", "MEMORY", "POSIX", "POSIXSHD", "GENERIC"})
    EXPECT_TRUE(rdb::isWritableType(type)) << type;
  for (const auto *type : {"BINFILE", "DEVICE", "TEXTSOURCE", "memory", "FOO", ""})
    EXPECT_FALSE(rdb::isWritableType(type)) << type;
}

// Rodzaj pliku zrodla deklarowanego (#346): plik zwykly dla BINFILE i TEXTSOURCE, urzadzenie
// znakowe albo FIFO dla DEVICE. Komunikat mowi slowem RQL - TEXTFILE, nie nazwa typu magazynu.
TEST(AccessorFactoryTest, source_kind_mismatch_by_file_type) {
  EXPECT_EQ(rdb::sourceKindMismatch("BINFILE", "p", S_IFREG), "");
  EXPECT_EQ(rdb::sourceKindMismatch("TEXTSOURCE", "p", S_IFREG), "");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "p", S_IFCHR), "");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "p", S_IFIFO), "");

  EXPECT_EQ(rdb::sourceKindMismatch("BINFILE", "p", S_IFIFO), "BINFILE 'p' is a FIFO, not a regular file");
  EXPECT_EQ(rdb::sourceKindMismatch("TEXTSOURCE", "p", S_IFCHR), "TEXTFILE 'p' is a character device, not a regular file");
  EXPECT_EQ(rdb::sourceKindMismatch("BINFILE", "p", S_IFDIR), "BINFILE 'p' is a directory, not a regular file");
  EXPECT_EQ(rdb::sourceKindMismatch("TEXTSOURCE", "p", S_IFSOCK), "TEXTFILE 'p' is a socket, not a regular file");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "p", S_IFREG), "DEVICE 'p' is a regular file, not a character device or FIFO");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "p", S_IFBLK), "DEVICE 'p' is a block device, not a character device or FIFO");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "p", S_IFDIR), "DEVICE 'p' is a directory, not a character device or FIFO");

  // Brak sciezki nie jest odmowa - akcesor ostrzega i daje NULL jak przed #346.
  EXPECT_EQ(rdb::sourceKindMismatch("BINFILE", "af_missing_kind.bin"), "");
  EXPECT_EQ(rdb::sourceKindMismatch("DEVICE", "af_missing_kind.dev"), "");
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
// Źródła deklarowane: BINFILE/DEVICE/TEXTSOURCE są tylko do odczytu, bez cienia danych.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, maps_declared_sources) {
  auto desc = testDescriptor();

  auto binary = rdb::makeAccessor("BINFILE", "af_missing.bin", desc, true, -1);
  EXPECT_NE(dynamic_cast<rdb::binaryDeviceRO *>(binary.get()), nullptr);
  EXPECT_FALSE(binary->hasShadow());

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
