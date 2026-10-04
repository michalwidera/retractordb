#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "descriptor.hpp"

namespace rdb {

/// @brief Wyliczanie i utrzymywanie w spójności ścieżek plików magazynu: deskryptora (.desc),
///        pliku danych oraz indeksu metadanych (.meta).
///
/// Obiekt klasy StoragePaths powinien:
/// - przy konstrukcji walidować niepuste qryID i fileName; ścieżka deskryptora to qryID + ".desc",
/// - jeśli podano katalog storageParam - sprawdzić, że istnieje, i osadzić w nim ścieżki deskryptora oraz danych,
/// - utrzymywać w jednym miejscu niezmiennik metaIndexFile() == storageFile() + ".meta" przy każdej zmianie ścieżki danych,
/// - realizować relokację pliku danych wg pola REF deskryptora (relocateFromRef()) - deskryptor może wskazać
///   inne położenie pliku danych; brak ścieżki danych po relokacji kończy się przez FatalError,
/// - przy relokacji ustalać, czy REF wyprowadza plik danych poza katalogi dozwolone przez operatora
///   (katalog magazynu i allowRefDirs()) - decyzję, co z tym zrobić, podejmuje storage, bo tylko on wie,
///   czy REF pochodzi od wołającego, czy wyłącznie z wczytanego pliku .desc,
/// - usuwać komplet plików magazynu (removeAllFiles()) dla magazynów dysponowalnych: plik danych, deskryptor,
///   indeks .meta oraz cień indeksu .meta.shadow; na żądanie z zachowaniem pliku danych i jego indeksów,
/// - nie wykonywać żadnego innego I/O poza sprawdzeniem katalogu storageParam, rozwiązaniem ścieżki REF
///   i kasowaniem plików.
class StoragePaths {
 public:
  StoragePaths(std::string_view qryID, std::string_view fileName, std::string_view storageParam);

  [[nodiscard]] const std::string &descriptorFile() const { return descriptorFile_; }
  [[nodiscard]] const std::string &storageFile() const { return storageFile_; }
  [[nodiscard]] const std::string &metaIndexFile() const { return metaIndexFile_; }

  /// @brief Dodatkowe katalogi, do których REF może przenieść plik danych (`storage.ref_dirs`).
  ///        Wołać przed relocateFromRef().
  void allowRefDirs(std::vector<std::string> dirs) { refDirs_ = std::move(dirs); }

  /// @brief Relocate the data file according to the descriptor's REF field; FatalError when no path remains.
  void relocateFromRef(const Descriptor &descriptor);

  /// @brief Czy REF przeniósł plik danych poza katalog magazynu i poza katalogi z allowRefDirs().
  [[nodiscard]] bool refLeavesAllowedDirs() const { return refLeavesAllowedDirs_; }

  /// @brief Remove data, descriptor, meta index and meta shadow files (for disposable storages).
  /// @param keepDataFiles true - usuwa tylko deskryptor, a plik danych, .meta i .meta.shadow zostają.
  void removeAllFiles(bool keepDataFiles = false) const;

 private:
  void setStorageFile(std::string file);  ///< utrzymuje niezmiennik metaIndexFile_ == storageFile_ + ".meta"

  std::string storageDir_;  ///< katalog z :STORAGE bez końcowego ukośnika; pusty = katalog roboczy procesu
  std::vector<std::string> refDirs_;
  bool refLeavesAllowedDirs_ = false;
  std::string descriptorFile_;
  std::string storageFile_;
  std::string metaIndexFile_;
};

/// @brief Wartość pola REF deskryptora; pusta, gdy go nie ma.
[[nodiscard]] std::string descriptorRef(const Descriptor &descriptor);

}  // namespace rdb
