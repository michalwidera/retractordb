// Scenariusze użycia fabryki akcesorów (accessorFactory)
//
// Fabryka skupia w jednym miejscu odwzorowanie nazwy typu magazynu na konkretną
// implementację FileInterface oraz dobór wariantu indeksu metadanych null
// (inertny/cień indeksu/bazowy) - wydzielone z klasy storage, która zna odtąd
// wyłącznie abstrakcyjny FileInterface.

#include <gtest/gtest.h>

#include <sys/stat.h>  // S_IF*

#include <filesystem>
#include <string>

#include "rdb/accessorFactory.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccbindev.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/faccposixshd.hpp"
#include "rdb/facctxtsrc.hpp"
#include "rdb/storageShadow.hpp"
#include "rdbResult.hpp"

namespace {
rdb::Descriptor testDescriptor() { return {rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER)}; }
}  // namespace

TEST(AccessorFactoryTest, declared_types) {
  // covers: accessorfactory.fabryki.2~1, accessorfactory.is-declared-type.1~1
  EXPECT_TRUE(rdb::isDeclaredType("BINFILE"));
  EXPECT_TRUE(rdb::isDeclaredType("DEVICE"));
  EXPECT_TRUE(rdb::isDeclaredType("TEXTSOURCE"));
  EXPECT_FALSE(rdb::isDeclaredType("DEFAULT"));
  EXPECT_FALSE(rdb::isDeclaredType("POSIX"));
}

// Profile zapisywalne to dokladnie te, ktore przyjmuje `STORAGE` w SELECT i `:SUBSTRAT` (#346).
TEST(AccessorFactoryTest, writable_types) {
  // covers: accessorfactory.is-writable-type.1~1
  for (const auto *type : {"DEFAULT", "DIRECT", "MEMORY", "POSIX", "POSIXSHD", "GENERIC"})
    EXPECT_TRUE(rdb::isWritableType(type)) << type;
  for (const auto *type : {"BINFILE", "DEVICE", "TEXTSOURCE", "memory", "FOO", ""})
    EXPECT_FALSE(rdb::isWritableType(type)) << type;
}

// Rodzaj pliku zrodla deklarowanego (#346): plik zwykly dla BINFILE i TEXTSOURCE, urzadzenie
// znakowe albo FIFO dla DEVICE. Komunikat mowi slowem RQL - TEXTFILE, nie nazwa typu magazynu.
TEST(AccessorFactoryTest, source_kind_mismatch_by_file_type) {
  // covers: accessorfactory.source-kind-mismatch.1~1, accessorfactory.source-kind-mismatch.2~1
  // covers: accessorfactory.source-kind-mismatch.3~1
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
  // covers: accessorfactory.fabryki.1~1, accessorfactory.make-accessor.1~1, faccposix.posix-binary-file.1~1
  // covers: faccposixshd.posix-binary-file-with-shadow.8~1, faccposixshd.posix-binary-file-with-shadow.13~1, storage.storage.3~1
  // covers: faccfs.generic-binary-file.2~1
  auto desc = testDescriptor();

  auto posix = rdbtest::ok(rdb::makeAccessor("POSIX", "af_posix.bin", desc, false, -1));
  EXPECT_NE(dynamic_cast<rdb::posixBinaryFile *>(posix.get()), nullptr);
  EXPECT_FALSE(posix->hasShadow());

  auto posixShd = rdbtest::ok(rdb::makeAccessor("POSIXSHD", "af_posixshd.bin", desc, false, -1));
  EXPECT_NE(dynamic_cast<rdb::posixBinaryFileWithShadow *>(posixShd.get()), nullptr);
  EXPECT_TRUE(posixShd->hasShadow());

  auto generic = rdbtest::ok(rdb::makeAccessor("GENERIC", "af_generic.bin", desc, false, -1));
  EXPECT_NE(dynamic_cast<rdb::genericBinaryFile *>(generic.get()), nullptr);
  EXPECT_FALSE(generic->hasShadow());

  auto byDefault = rdbtest::ok(rdb::makeAccessor("DEFAULT", "af_default.bin", desc, false, -1));
  EXPECT_TRUE(byDefault->hasShadow());

  auto direct = rdbtest::ok(rdb::makeAccessor("DIRECT", "af_direct.bin", desc, false, -1));
  EXPECT_FALSE(direct->hasShadow());
}

// ---------------------------------------------------------------------------
// Źródła deklarowane: BINFILE/DEVICE/TEXTSOURCE są tylko do odczytu, bez cienia danych.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, maps_declared_sources) {
  // covers: accessorfactory.fabryki.1~1, accessorfactory.make-accessor.1~1, storage.storage.3~1
  auto desc = testDescriptor();

  auto binary = rdbtest::ok(rdb::makeAccessor("BINFILE", "af_missing.bin", desc, true, -1));
  EXPECT_NE(dynamic_cast<rdb::binaryDeviceRO *>(binary.get()), nullptr);
  EXPECT_FALSE(binary->hasShadow());

  auto device = rdbtest::ok(rdb::makeAccessor("DEVICE", "af_missing.dev", desc, true, -1));
  EXPECT_NE(dynamic_cast<rdb::binaryDeviceRO *>(device.get()), nullptr);
  EXPECT_FALSE(device->hasShadow());

  auto text = rdbtest::ok(rdb::makeAccessor("TEXTSOURCE", "af_missing.txt", desc, true, -1));
  EXPECT_NE(dynamic_cast<rdb::textSourceRO *>(text.get()), nullptr);
  EXPECT_FALSE(text->hasShadow());
}

// ---------------------------------------------------------------------------
// Dobór wariantu indeksu metadanych: inertny dla deklarowanych (bez pliku),
// storageShadow przy cieniu danych, bazowy metaData w pozostałych przypadkach.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, meta_index_variant_selection) {
  // covers: accessorfactory.fabryki.3~1, accessorfactory.make-meta-index.1~1, accessorfactory.make-meta-index.2~1
  // covers: fainterface.file-interface.7~1, storage.storage.3~1, storage.storage.6~1, storageshadow.storage-shadow.1~1
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

// ---------------------------------------------------------------------------
// Faza 1, plaster 2a: nieznany typ magazynu jest bledem (dzis Errc::Config), nie koncem procesu.
//
// To byla jedyna sciezka w tej fabryce osiagalna poprawnym wywolaniem API i JEDYNA
// nieoslonieta przez straz w wiazaniu Pythona - wiazanie nie zna listy typow, bo lista
// jest wlasnie tutaj. rdb.Storage(storage_type="NONSENSE") zabijal jadro notatnika.
// ---------------------------------------------------------------------------
TEST(AccessorFactoryTest, unsupported_storage_type_is_rejected) {
  // covers: accessorfactory.fabryki.1~1, accessorfactory.make-accessor.4~1
  auto desc = testDescriptor();

  EXPECT_RDB_ERROR(rdb::makeAccessor("NONSENSE", "af_unsupported.bin", desc, false, -1), rdb::Errc::Config);
  // Malymi literami tez nie - porownanie jest dokladne, a nie bez wzgledu na wielkosc.
  EXPECT_RDB_ERROR(rdb::makeAccessor("default", "af_unsupported.bin", desc, false, -1), rdb::Errc::Config);
}

TEST(AccessorFactoryTest, empty_arguments_are_rejected) {
  // covers: accessorfactory.fabryki.1~1, accessorfactory.make-accessor.4~1
  auto desc = testDescriptor();

  EXPECT_RDB_ERROR(rdb::makeAccessor("DEFAULT", "", desc, false, -1), rdb::Errc::Config);
  EXPECT_RDB_ERROR(rdb::makeAccessor("", "af_empty_type.bin", desc, false, -1), rdb::Errc::Config);
}

// Komunikat ma nazywac to, co wolno bylo podac. Bez tego jedyna droga do listy typow
// prowadzi przez czytanie zrodla fabryki.
TEST(AccessorFactoryTest, unsupported_type_message_names_the_accepted_values) {
  auto desc = testDescriptor();

  const auto made = rdb::makeAccessor("NONSENSE", "af_unsupported.bin", desc, false, -1);
  ASSERT_RDB_ERROR(made, rdb::Errc::Config);
  const std::string &message = made.error().message();
  EXPECT_NE(message.find("NONSENSE"), std::string::npos) << message;
  EXPECT_NE(message.find("TEXTSOURCE"), std::string::npos) << message;
}
