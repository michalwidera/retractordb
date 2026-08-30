#include "rdb/storagePaths.hpp"

#include <cstdio>  // ::remove
#include <filesystem>
#include <ranges>
#include <string>
#include <system_error>  // std::error_code

#include "fatalError.hpp"
#include "rdb/storageShadow.hpp"

namespace rdb {

StoragePaths::StoragePaths(const std::string_view qryID, const std::string_view fileName, const std::string_view storageParam) {
  if (qryID.empty()) FatalError("storage: qryID must not be empty");
  if (fileName.empty()) FatalError("storage: fileName must not be empty");

  descriptorFile_ = std::string(qryID) + ".desc";
  setStorageFile(std::string(fileName));

  if (storageParam.empty()) {
    return;  // no change
  }

  // Katalog wskazany przez :STORAGE musi ISTNIEC — dyrektywa go nie tworzy. Rozroznienie
  // "nie ma" od "jest, ale nie katalogiem" jest tu istotne: pierwszy przypadek to zwykle
  // zapomniane `mkdir`, drugi to kolizja nazw. Komunikat podaje sciezke BEZWZGLEDNA, bo
  // sciezka wzgledna rozwiazuje sie wzgledem katalogu roboczego procesu, a ten przy
  // uruchomieniu z ctest albo z serwisu nie jest tym, o ktorym mysli autor zapytania.
  // Koncowy ukosnik trzeba sciac PRZED sprawdzeniem istnienia: `exists("temp/")` dla
  // zwyklego pliku `temp` daje falsz, bo ukosnik zada katalogu — bez tego galaz o kolizji
  // nazw bylaby nieosiagalna i kazdy przypadek raportowalby "nie istnieje".
  std::string dirName(storageParam);
  while (dirName.size() > 1 && dirName.back() == std::filesystem::path::preferred_separator)
    dirName.pop_back();

  if (!std::filesystem::exists(dirName)) {
    std::error_code absError;
    const auto full = std::filesystem::absolute(dirName, absError);
    FatalError(
        "storage: directory '{}' from the STORAGE directive does not exist ({}); "
        "RetractorDB does not create it — run 'mkdir -p {}' first",
        dirName, absError ? std::string("path could not be resolved") : full.string(), dirName);
  }

  if (!std::filesystem::is_directory(dirName)) {
    FatalError("storage: path '{}' from the STORAGE directive exists but is not a directory", dirName);
  }

  descriptorFile_ = std::filesystem::path(storageParam) / std::filesystem::path(descriptorFile_);
  setStorageFile(std::filesystem::path(storageParam) / std::filesystem::path(storageFile_));
}

void StoragePaths::setStorageFile(std::string file) {
  storageFile_   = std::move(file);
  metaIndexFile_ = storageFile_ + ".meta";
}

void StoragePaths::relocateFromRef(const Descriptor &descriptor) {
  auto it = std::ranges::find_if(descriptor,  //
                                 [](const auto &item) { return item.rtype == rdb::REF; });

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    setStorageFile((*it).rname);
  }

  // if storage object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_.empty()) {
    FatalError("storage: storage file not set in descriptor (missing REF field or :STORAGE directive)");
  }
}

void StoragePaths::removeAllFiles() const {
  if (!storageFile_.empty()) (void)::remove(storageFile_.c_str());
  if (std::filesystem::exists(descriptorFile_)) ::remove(descriptorFile_.c_str());
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_)) ::remove(metaIndexFile_.c_str());
  const std::string metaShadowFile = storageShadow::metaShadowFilePath(metaIndexFile_);
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaShadowFile)) ::remove(metaShadowFile.c_str());
}

}  // namespace rdb
