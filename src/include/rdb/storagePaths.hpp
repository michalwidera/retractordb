#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "descriptor.hpp"
#include "error.hpp"

namespace rdb {

/// @brief Wyliczanie i utrzymywanie w spójności ścieżek plików magazynu: deskryptora (.desc),
///        pliku danych oraz indeksu metadanych (.meta).
///
/// Obiekt klasy StoragePaths powinien:
/// - przy konstrukcji walidować niepuste qryID i fileName; ścieżka deskryptora to qryID + ".desc",
///   Test: `StoragePathsTest.basic_paths_without_storage_dir`, `StoragePathsTest.empty_identifiers_are_rejected`.
/// - jeśli podano katalog storageParam - sprawdzić, że istnieje, i osadzić w nim ścieżki deskryptora oraz danych;
///   każda z tych kontroli kończy się błędem Errc::Config zwróconym z make(), a nie końcem procesu,
///   Test: `StoragePathsTest.storage_dir_prefixes_paths`, `StoragePathsTest.missing_storage_directory_is_rejected`, `StoragePathsTest.storage_param_pointing_at_a_file_is_rejected`.
/// - utrzymywać w jednym miejscu niezmiennik metaIndexFile() == storageFile() + ".meta" przy każdej zmianie ścieżki danych,
///   Test: `StoragePathsTest.basic_paths_without_storage_dir`, `StoragePathsTest.relocate_from_ref_moves_data_and_meta`.
/// - realizować relokację pliku danych wg pola REF deskryptora (relocateFromRef()) - deskryptor może wskazać
///   inne położenie pliku danych; brak ścieżki danych po relokacji wraca jako Errc::Config,
///   Test: `StoragePathsTest.relocate_from_ref_moves_data_and_meta`, `StoragePathsTest.relocate_to_an_empty_ref_is_rejected`.
/// - przy relokacji ustalać, czy REF wyprowadza plik danych poza katalogi dozwolone przez operatora
///   (katalog magazynu i allowRefDirs()) - decyzję, co z tym zrobić, podejmuje storage, bo tylko on wie,
///   czy REF pochodzi od wołającego, czy wyłącznie z wczytanego pliku .desc,
///   Test: `StoragePathsTest.ref_leaving_allowed_dirs_is_detected`.
/// - usuwać komplet plików magazynu (removeAllFiles()) dla magazynów dysponowalnych: plik danych, deskryptor,
///   indeks .meta oraz cień indeksu .meta.shadow; na żądanie z zachowaniem pliku danych i jego indeksów,
///   Test: `StoragePathsTest.remove_all_files_deletes_whole_set`, `StoragePathsTest.remove_all_files_can_keep_data_files`.
/// - nie wykonywać żadnego innego I/O poza sprawdzeniem katalogu storageParam, rozwiązaniem ścieżki REF
///   i kasowaniem plików.
///   Nietestowalne: ograniczenie odpowiedzialnosci modulu; kompletnego braku innych operacji I/O nie dowodzi test funkcjonalny, sprawdzane przegladem storagePaths.cc.
class StoragePaths {
 public:
  /// Errc::Config dla pustego identyfikatora albo storageParam, ktory nie istnieje lub nie jest katalogiem.
  /// Odmowa oznacza, ze obiekt nie powstaje, wiec removeAllFiles() nie moze zadzialac na zle
  /// skonfigurowanej sciezce - odrzucona konstrukcja niczego nie kasuje.
  [[nodiscard]] static Result<StoragePaths> make(std::string_view qryID, std::string_view fileName,
                                                 std::string_view storageParam);

  [[nodiscard]] const std::string &descriptorFile() const { return descriptorFile_; }
  [[nodiscard]] const std::string &storageFile() const { return storageFile_; }
  [[nodiscard]] const std::string &metaIndexFile() const { return metaIndexFile_; }

  /// @brief Dodatkowe katalogi, do których REF może przenieść plik danych (`storage.ref_dirs`).
  ///        Wołać przed relocateFromRef().
  void allowRefDirs(std::vector<std::string> dirs) { refDirs_ = std::move(dirs); }

  /// @brief Relocate the data file according to the descriptor's REF field.
  /// @return Errc::Config when no path remains - a REF field with an empty name.
  [[nodiscard]] Result<> relocateFromRef(const Descriptor &descriptor);

  /// @brief Czy REF przeniósł plik danych poza katalog magazynu i poza katalogi z allowRefDirs().
  [[nodiscard]] bool refLeavesAllowedDirs() const { return refLeavesAllowedDirs_; }

  /// @brief Remove data, descriptor, meta index and meta shadow files (for disposable storages).
  /// @param keepDataFiles true - usuwa tylko deskryptor, a plik danych, .meta i .meta.shadow zostają.
  void removeAllFiles(bool keepDataFiles = false) const;

 private:
  StoragePaths() = default;               ///< wylacznie przez make() - obiekt istnieje tylko z poprawna konfiguracja
  void setStorageFile(std::string file);  ///< utrzymuje niezmiennik metaIndexFile_ == storageFile_ + ".meta"

  std::string storageDir_;  ///< katalog z :STORAGE bez końcowego ukośnika; pusty = katalog roboczy procesu
  std::vector<std::string> refDirs_;
  bool refLeavesAllowedDirs_ = false;
  std::string descriptorFile_;
  std::string storageFile_;
  std::string metaIndexFile_;
};

/// Funkcja descriptorRef powinna:
/// - zwracać nazwę pierwszego pola REF deskryptora, a gdy go nie ma - pusty napis.
///   Test: `StoragePathsTest.relocate_from_ref_moves_data_and_meta`, `StoragePathsTest.relocate_without_ref_keeps_paths`.
[[nodiscard]] std::string descriptorRef(const Descriptor &descriptor);

}  // namespace rdb
