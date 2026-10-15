#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "rdb/error.hpp"

namespace rdb::embed {

/// Wlasnosc katalogu osadzonego jest trwala; zajecie rozstrzyga flock na i-wezle katalogu.
/// Nie tworzy blokad instancji xretractor ani obiektow magistrali.
class StorageGuard {
 public:
  [[nodiscard]] static Result<std::unique_ptr<StorageGuard>> create(std::string_view directory, bool claim);
  ~StorageGuard();
  StorageGuard(const StorageGuard &)            = delete;
  StorageGuard &operator=(const StorageGuard &) = delete;

  [[nodiscard]] const std::string &directory() const noexcept { return directory_; }
  [[nodiscard]] bool ownsDirectory() const noexcept { return fd_ >= 0; }
  /// Plik artefaktu musi zostac wewnatrz katalogu, takze po rozwiazaniu dowiazan.
  [[nodiscard]] Result<> checkPath(const std::string &path) const;

 private:
  StorageGuard() = default;
  [[nodiscard]] Result<> initialize(std::string_view directory, bool claim);
  std::string directory_;
  int fd_{-1};
};

/// Kontrola blokad serwisu po rozstrzygnieciu argumentu i pliku konfiguracji (#450).
/// @requirements-exempt OfflinePolicy: Parametry kontroli dostepu offline; kontrole wykonuje checkOffline.
struct OfflinePolicy {
  std::string lockDir;  ///< pusty => TMPDIR
  /// Jak xtrdb: odmowe daje tylko trzymany flock; wpis nieczytelny i brak katalogu nie.
  bool lenient{true};
};

/// Plik konfiguracji czytany tylko, gdy podany. Argument lockDir wygrywa nad paths.lock_dir;
/// lagodny jest katalog wymieniony w embedded.lenient_lock_dirs, a bez tego klucza katalog
/// tymczasowy systemu (TMPDIR, bez niej /tmp).
[[nodiscard]] Result<OfflinePolicy> offlinePolicy(std::string_view lockDir, std::string_view configPath);

/// Tylko obserwacja trzymanych blokad. Katalog musi odpowiadac paths.lock_dir serwisu.
/// Nie wyklucza pozniejszego startu xretractor.
[[nodiscard]] Result<> checkOffline(const OfflinePolicy &policy);
[[nodiscard]] Result<std::string> temporaryStorageDirectory();
/// Sprzatanie tymczasowego katalogu nie usuwa magazynu zajetego przez Storage.
void removeTemporaryStorage(const std::string &directory) noexcept;

}  // namespace rdb::embed
