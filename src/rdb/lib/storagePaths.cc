#include "rdb/storagePaths.hpp"

#include <fmt/format.h>

#include <cstdio>  // ::remove
#include <filesystem>
#include <ranges>
#include <string>
#include <system_error>  // std::error_code

#include "rdb/storageShadow.hpp"

namespace rdb {

Result<StoragePaths> StoragePaths::make(const std::string_view qryID, const std::string_view fileName,
                                        const std::string_view storageParam) {
  // Odmowa PRZED zbudowaniem obiektu: storage, ktory tego nie przejdzie, nigdy nie powstaje, wiec
  // jego destruktor sie nie wykona - a to on kasuje pliki magazynu disposable. Zla konfiguracja
  // nie moze niczego usunac, i nie usuwa.
  if (qryID.empty()) return fail(Errc::Config, "storage: qryID must not be empty");
  if (fileName.empty()) return fail(Errc::Config, "storage: fileName must not be empty");

  StoragePaths paths;
  paths.descriptorFile_ = std::string(qryID) + ".desc";
  paths.setStorageFile(std::string(fileName));

  if (storageParam.empty()) {
    return paths;  // no change
  }

  // Katalog wskazany przez :STORAGE musi ISTNIEC - dyrektywa go nie tworzy. Rozroznienie
  // "nie ma" od "jest, ale nie katalogiem" jest tu istotne: pierwszy przypadek to zwykle
  // zapomniane `mkdir`, drugi to kolizja nazw. Komunikat podaje sciezke BEZWZGLEDNA, bo
  // sciezka wzgledna rozwiazuje sie wzgledem katalogu roboczego procesu, a ten przy
  // uruchomieniu z ctest albo z serwisu nie jest tym, o ktorym mysli autor zapytania.
  // Koncowy ukosnik trzeba sciac PRZED sprawdzeniem istnienia: `exists("temp/")` dla
  // zwyklego pliku `temp` daje falsz, bo ukosnik zada katalogu - bez tego galaz o kolizji
  // nazw bylaby nieosiagalna i kazdy przypadek raportowalby "nie istnieje".
  std::string dirName(storageParam);
  while (dirName.size() > 1 && dirName.back() == std::filesystem::path::preferred_separator)
    dirName.pop_back();

  // Przeciazenia z error_code: wersje rzucajace zglaszaja filesystem_error np. przy braku prawa
  // do odczytu katalogu nadrzednego. Blad stat() traktujemy jak brak katalogu - komunikat i tak
  // kaze go sprawdzic.
  std::error_code statError;
  if (!std::filesystem::exists(dirName, statError)) {
    std::error_code absError;
    const auto full = std::filesystem::absolute(dirName, absError);
    return fail(Errc::Config,
                fmt::format("storage: directory '{}' from the STORAGE directive does not exist ({}); "
                            "RetractorDB does not create it - run 'mkdir -p {}' first",
                            dirName, absError ? std::string("path could not be resolved") : full.string(), dirName));
  }

  if (!std::filesystem::is_directory(dirName, statError)) {
    return fail(Errc::Config,
                fmt::format("storage: path '{}' from the STORAGE directive exists but is not a directory", dirName));
  }

  paths.descriptorFile_ = std::filesystem::path(storageParam) / std::filesystem::path(paths.descriptorFile_);
  paths.setStorageFile(std::filesystem::path(storageParam) / std::filesystem::path(paths.storageFile_));
  return paths;
}

void StoragePaths::setStorageFile(std::string file) {
  storageFile_   = std::move(file);
  metaIndexFile_ = storageFile_ + ".meta";
}

Result<> StoragePaths::relocateFromRef(const Descriptor &descriptor) {
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
    return fail(Errc::Config, "storage: storage file not set in descriptor (missing REF field or :STORAGE directive)");
  }
  return {};
}

void StoragePaths::removeAllFiles() const {
  // Sprzatanie z destruktora: nic tu nie moze zawiesc glosniej niz "pliku nie bylo". Stad
  // przeciazenia z error_code - wersja rzucajaca w destruktorze to std::terminate.
  std::error_code ignored;
  if (!storageFile_.empty()) (void)::remove(storageFile_.c_str());
  if (std::filesystem::exists(descriptorFile_, ignored)) (void)::remove(descriptorFile_.c_str());
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_, ignored)) (void)::remove(metaIndexFile_.c_str());
  const std::string metaShadowFile = storageShadow::metaShadowFilePath(metaIndexFile_);
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaShadowFile, ignored)) (void)::remove(metaShadowFile.c_str());
}

}  // namespace rdb
