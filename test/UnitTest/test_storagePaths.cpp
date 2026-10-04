// Scenariusze użycia klasy StoragePaths
//
// StoragePaths to wydzielona z klasy storage logika wyliczania ścieżek plików
// magazynu (deskryptor .desc, plik danych, indeks .meta) wraz z relokacją wg
// pola REF deskryptora i kasowaniem kompletu plików magazynów dysponowalnych.
// Niezmiennik metaIndexFile == storageFile + ".meta" jest utrzymywany w jednym miejscu.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "rdb/descriptor.hpp"
#include "rdb/storagePaths.hpp"

// ---------------------------------------------------------------------------
// Konstrukcja bez katalogu storageParam: deskryptor to qryID + ".desc",
// indeks metadanych to plik danych + ".meta".
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, basic_paths_without_storage_dir) {
  rdb::StoragePaths paths("qry1", "data1", "");

  EXPECT_EQ(paths.descriptorFile(), "qry1.desc");
  EXPECT_EQ(paths.storageFile(), "data1");
  EXPECT_EQ(paths.metaIndexFile(), "data1.meta");
}

// ---------------------------------------------------------------------------
// Konstrukcja z katalogiem storageParam: ścieżki deskryptora i danych są
// osadzane w katalogu, niezmiennik .meta podąża za plikiem danych.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, storage_dir_prefixes_paths) {
  const std::filesystem::path dir{"storage_paths_dir"};
  std::filesystem::create_directory(dir);

  rdb::StoragePaths paths("qry1", "data1", dir.string());

  EXPECT_EQ(paths.descriptorFile(), (dir / "qry1.desc").string());
  EXPECT_EQ(paths.storageFile(), (dir / "data1").string());
  EXPECT_EQ(paths.metaIndexFile(), (dir / "data1").string() + ".meta");

  std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------------------
// Relokacja wg pola REF: deskryptor wskazuje inne położenie pliku danych,
// indeks .meta przenosi się razem z nim.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, relocate_from_ref_moves_data_and_meta) {
  rdb::StoragePaths paths("qry1", "data1", "");

  auto desc = rdb::Descriptor("other.bin", 0, 0, rdb::REF) +  //
              rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  paths.relocateFromRef(desc);

  EXPECT_EQ(paths.storageFile(), "other.bin");
  EXPECT_EQ(paths.metaIndexFile(), "other.bin.meta");
  EXPECT_EQ(paths.descriptorFile(), "qry1.desc");  // deskryptor zostaje na miejscu
}

// ---------------------------------------------------------------------------
// Deskryptor bez pola REF: ścieżki pozostają bez zmian.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, relocate_without_ref_keeps_paths) {
  rdb::StoragePaths paths("qry1", "data1", "");

  auto desc = rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER);
  paths.relocateFromRef(desc);

  EXPECT_EQ(paths.storageFile(), "data1");
  EXPECT_EQ(paths.metaIndexFile(), "data1.meta");
}

// ---------------------------------------------------------------------------
// removeAllFiles: kasuje plik danych, deskryptor, indeks .meta oraz cień
// indeksu .meta.shadow - porządkowanie magazynów dysponowalnych.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, remove_all_files_deletes_whole_set) {
  rdb::StoragePaths paths("qry_rm", "data_rm", "");

  for (const auto &file : {std::string("data_rm"), std::string("qry_rm.desc"),  //
                           std::string("data_rm.meta"), std::string("data_rm.meta.shadow")}) {
    std::ofstream out(file);
    out << "x";
  }

  paths.removeAllFiles();

  EXPECT_FALSE(std::filesystem::exists("data_rm"));
  EXPECT_FALSE(std::filesystem::exists("qry_rm.desc"));
  EXPECT_FALSE(std::filesystem::exists("data_rm.meta"));
  EXPECT_FALSE(std::filesystem::exists("data_rm.meta.shadow"));
}

// ---------------------------------------------------------------------------
// removeAllFiles(true): plik danych spoza dozwolonych katalogów nie należy do
// instancji - znika sam deskryptor, plik danych i jego indeksy zostają (#278).
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, remove_all_files_can_keep_data_files) {
  rdb::StoragePaths paths("qry_keep", "data_keep", "");

  for (const auto &file : {std::string("data_keep"), std::string("qry_keep.desc"),  //
                           std::string("data_keep.meta"), std::string("data_keep.meta.shadow")}) {
    std::ofstream out(file);
    out << "x";
  }

  paths.removeAllFiles(true);

  EXPECT_FALSE(std::filesystem::exists("qry_keep.desc"));
  EXPECT_TRUE(std::filesystem::exists("data_keep"));
  EXPECT_TRUE(std::filesystem::exists("data_keep.meta"));
  EXPECT_TRUE(std::filesystem::exists("data_keep.meta.shadow"));

  for (const auto *file : {"data_keep", "data_keep.meta", "data_keep.meta.shadow"})
    std::filesystem::remove(file);
}

// ---------------------------------------------------------------------------
// Zawarcie REF w katalogu magazynu (#278): liczy się ścieżka po rozwiązaniu
// `..` i dowiązań, porównywana po komponentach, a katalogi z allowRefDirs()
// poszerzają zakres. Ścieżka względna REF rozwiązuje się względem katalogu
// roboczego, tak jak w open() akcesora.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, ref_leaving_allowed_dirs_is_detected) {
  namespace fs       = std::filesystem;
  const fs::path dir = "storage_paths_ref";
  fs::remove_all(dir);
  fs::create_directories(dir / "store" / "sub");
  fs::create_directories(dir / "store2");
  fs::create_directories(dir / "archive");
  fs::create_directory_symlink(fs::absolute(dir / "archive"), dir / "store" / "link");

  const auto leaves = [&](const std::string &ref, const std::vector<std::string> &refDirs) {
    rdb::StoragePaths paths("qry1", "data1", (dir / "store").string());
    paths.allowRefDirs(refDirs);
    paths.relocateFromRef(rdb::Descriptor(ref, 0, 0, rdb::REF) + rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER));
    return paths.refLeavesAllowedDirs();
  };
  const std::string store = (dir / "store").string();

  EXPECT_FALSE(leaves(store + "/sub/data.bin", {}));
  EXPECT_FALSE(leaves(store + "/sub/../data.bin", {}));
  EXPECT_TRUE(leaves(store + "/../outside.bin", {}));
  EXPECT_TRUE(leaves(store + "/../../../../etc/rdb", {}));
  EXPECT_TRUE(leaves((dir / "store2" / "data.bin").string(), {})) << "store2 nie lezy w store mimo wspolnego prefiksu";
  EXPECT_TRUE(leaves(store + "/link/data.bin", {})) << "dowiazanie wyprowadza poza katalog magazynu";
  EXPECT_TRUE(leaves(store, {})) << "sam katalog nie jest plikiem w katalogu";

  const std::string archive = fs::absolute(dir / "archive").string();
  EXPECT_FALSE(leaves((dir / "archive" / "data.bin").string(), {archive}));
  EXPECT_FALSE(leaves(store + "/link/data.bin", {archive + "/"}));
  EXPECT_TRUE(leaves((dir / "store2" / "data.bin").string(), {archive}));

  fs::remove_all(dir);
}

// Bez pola REF ścieżka pochodzi od wołającego - nie ma czego ograniczać.
TEST(StoragePathsTest, no_ref_never_leaves_allowed_dirs) {
  rdb::StoragePaths paths("qry1", "../data1", "");
  paths.relocateFromRef(rdb::Descriptor("a", sizeof(int), 1, rdb::INTEGER));
  EXPECT_FALSE(paths.refLeavesAllowedDirs());
}
