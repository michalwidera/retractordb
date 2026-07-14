#pragma once

#include <string>
#include <string_view>

#include "descriptor.hpp"

namespace rdb {

/// @brief Wyliczanie i utrzymywanie w spójności ścieżek plików magazynu: deskryptora (.desc),
///        pliku danych oraz indeksu metadanych (.meta).
///
/// Obiekt klasy StoragePaths powinien:
/// - przy konstrukcji walidować niepuste qryID i fileName; ścieżka deskryptora to qryID + ".desc",
/// - jeśli podano katalog storageParam — sprawdzić, że istnieje, i osadzić w nim ścieżki deskryptora oraz danych,
/// - utrzymywać w jednym miejscu niezmiennik metaIndexFile() == storageFile() + ".meta" przy każdej zmianie ścieżki danych,
/// - realizować relokację pliku danych wg pola REF deskryptora (relocateFromRef()) — deskryptor może wskazać
///   inne położenie pliku danych; brak ścieżki danych po relokacji kończy się przez FatalError,
/// - usuwać komplet plików magazynu (removeAllFiles()) dla magazynów dysponowalnych: plik danych, deskryptor,
///   indeks .meta oraz cień indeksu .meta.shadow,
/// - nie wykonywać żadnego innego I/O poza sprawdzeniem katalogu storageParam i kasowaniem plików.
class StoragePaths {
 public:
  StoragePaths(std::string_view qryID, std::string_view fileName, std::string_view storageParam);

  [[nodiscard]] const std::string &descriptorFile() const { return descriptorFile_; }
  [[nodiscard]] const std::string &storageFile() const { return storageFile_; }
  [[nodiscard]] const std::string &metaIndexFile() const { return metaIndexFile_; }

  /// @brief Relocate the data file according to the descriptor's REF field; FatalError when no path remains.
  void relocateFromRef(const Descriptor &descriptor);

  /// @brief Remove data, descriptor, meta index and meta shadow files (for disposable storages).
  void removeAllFiles() const;

 private:
  void setStorageFile(std::string file);  ///< utrzymuje niezmiennik metaIndexFile_ == storageFile_ + ".meta"

  std::string descriptorFile_;
  std::string storageFile_;
  std::string metaIndexFile_;
};

}  // namespace rdb
