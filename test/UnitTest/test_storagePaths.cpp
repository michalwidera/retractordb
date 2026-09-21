// Scenariusze użycia klasy StoragePaths
//
// StoragePaths to wydzielona z klasy storage logika wyliczania ścieżek plików
// magazynu (deskryptor .desc, plik danych, indeks .meta) wraz z relokacją wg
// pola REF deskryptora i kasowaniem kompletu plików magazynów dysponowalnych.
// Niezmiennik metaIndexFile == storageFile + ".meta" jest utrzymywany w jednym miejscu.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "rdb/descriptor.hpp"
#include "rdb/exceptions.hpp"
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
// Faza 1, plaster 2a: zla konfiguracja jest wyjatkiem, nie koncem procesu.
//
// Zadnego z tych przypadkow nie dalo sie wczesniej przetestowac - kazdy konczyl
// binarke testow przez std::exit, wiec caly zestaw szedl z nim. To jest dokladnie
// ten powod, dla ktorego sciezki bledow w tej warstwie byly nieprzetestowane.
// ---------------------------------------------------------------------------
TEST(StoragePathsTest, empty_identifiers_are_rejected) {
  EXPECT_THROW((void)rdb::StoragePaths("", "data", ""), rdb::ConfigError);
  EXPECT_THROW((void)rdb::StoragePaths("qry", "", ""), rdb::ConfigError);
}

TEST(StoragePathsTest, missing_storage_directory_is_rejected) {
  const std::filesystem::path absent{"storage_paths_absent_dir"};
  std::filesystem::remove_all(absent);

  EXPECT_THROW((void)rdb::StoragePaths("qry", "data", absent.string()), rdb::ConfigError);
}

// Sciezka z koncowym ukosnikiem trafia w GALAZ KOLIZJI NAZW, nie w "nie istnieje":
// konstruktor scina ukosnik przed sprawdzeniem, bo exists("plik/") jest falszem dla
// zwyklego pliku i bez sciecia ta galaz bylaby nieosiagalna.
TEST(StoragePathsTest, storage_param_pointing_at_a_file_is_rejected) {
  const std::filesystem::path file{"storage_paths_not_a_dir"};
  std::filesystem::remove_all(file);
  { std::ofstream(file) << "x"; }

  EXPECT_THROW((void)rdb::StoragePaths("qry", "data", file.string()), rdb::ConfigError);
  EXPECT_THROW((void)rdb::StoragePaths("qry", "data", file.string() + "/"), rdb::ConfigError);

  std::filesystem::remove(file);
}

// Pusta sciezka po relokacji jest osiagalna DOKLADNIE jedna droga: pole REF o pustej
// nazwie. Konstruktor odrzuca puste fileName, wiec storageFile_ nie moze byc puste
// wczesniej - dopiero relocateFromRef() przepisuje je z deskryptora i dopiero wtedy
// moze zniknac. Bez tej obserwacji sciezki nie da sie przetestowac inaczej niz przez
// prywatny setStorageFile().
TEST(StoragePathsTest, relocate_to_an_empty_ref_is_rejected) {
  rdb::StoragePaths paths("qry", "data", "");
  ASSERT_EQ(paths.storageFile(), "data");

  const rdb::Descriptor emptyRef{rdb::Descriptor("", 0, 0, rdb::REF)};
  EXPECT_THROW(paths.relocateFromRef(emptyRef), rdb::ConfigError);
}

// Wyjatek z konstruktora znaczy, ze obiekt NIE POWSTAL - wiec nie ma destruktora,
// ktory mialby cokolwiek posprzatac. Dla StoragePaths to istotne, bo removeAllFiles()
// kasuje komplet plikow magazynu: zla konfiguracja nie moze skasowac cudzych danych.
TEST(StoragePathsTest, a_rejected_construction_removes_nothing) {
  const std::filesystem::path victim{"storage_paths_victim.desc"};
  { std::ofstream(victim) << "keep me"; }

  EXPECT_THROW((void)rdb::StoragePaths("", "storage_paths_victim", ""), rdb::ConfigError);
  EXPECT_TRUE(std::filesystem::exists(victim));

  std::filesystem::remove(victim);
}
