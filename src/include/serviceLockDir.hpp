#pragma once

#include <filesystem>
#include <string_view>

namespace ipc {

/// Funkcja serviceLockDir powinna:
/// - [constants.service-lock-dir.1~1] zwracac skonfigurowany katalog blokad instancji (paths.lock_dir), a gdy jest pusty - katalog tymczasowy procesu z std::filesystem::temp_directory_path.
///
/// @note Inaczej niz kMachineLockDir: ta blokada rozdziela instancje w obrebie jednego katalogu.
inline std::filesystem::path serviceLockDir(std::string_view configuredDir) {
  if (configuredDir.empty()) return std::filesystem::temp_directory_path();
  return {configuredDir};
}

}  // namespace ipc
