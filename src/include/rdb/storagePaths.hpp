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
/// - jeśli podano katalog storageParam - sprawdzić, że istnieje, i osadzić w nim ścieżki deskryptora oraz danych;
///   każda z tych kontroli kończy się rzutem ConfigError, a nie końcem procesu (faza 1, plaster 2a),
/// - utrzymywać w jednym miejscu niezmiennik metaIndexFile() == storageFile() + ".meta" przy każdej zmianie ścieżki danych,
/// - realizować relokację pliku danych wg pola REF deskryptora (relocateFromRef()) - deskryptor może wskazać
///   inne położenie pliku danych; brak ścieżki danych po relokacji kończy się rzutem ConfigError,
/// - usuwać komplet plików magazynu (removeAllFiles()) dla magazynów dysponowalnych: plik danych, deskryptor,
///   indeks .meta oraz cień indeksu .meta.shadow,
/// - nie wykonywać żadnego innego I/O poza sprawdzeniem katalogu storageParam i kasowaniem plików.
class StoragePaths {
 public:
  /// @throws ConfigError on an empty identifier, or a storageParam that is missing or is not a directory.
  ///         Throwing here means the object never exists, so removeAllFiles() cannot run on a misconfigured
  ///         path - a rejected construction deletes nothing.
  StoragePaths(std::string_view qryID, std::string_view fileName, std::string_view storageParam);

  [[nodiscard]] const std::string &descriptorFile() const { return descriptorFile_; }
  [[nodiscard]] const std::string &storageFile() const { return storageFile_; }
  [[nodiscard]] const std::string &metaIndexFile() const { return metaIndexFile_; }

  /// @brief Relocate the data file according to the descriptor's REF field.
  /// @throws ConfigError when no path remains - a REF field with an empty name.
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
