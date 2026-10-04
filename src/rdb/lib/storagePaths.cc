#include "rdb/storagePaths.hpp"

#include <algorithm>
#include <cstdio>  // ::remove
#include <filesystem>
#include <ranges>
#include <string>
#include <system_error>  // std::error_code

#include "fatalError.hpp"
#include "rdb/storageShadow.hpp"

namespace rdb {

namespace {

// Zawarcie sprawdzane po rozwiazaniu dowiazan (weakly_canonical) i po komponentach sciezki, nie po
// prefiksie napisu: `/srv/rdb2` nie lezy w `/srv/rdb`. Dowiazanie w katalogu magazynu wskazujace na
// zewnatrz wychodzi wiec poza katalog. Blad rozwiazania sciezki to "poza" - niepewnosc nie moze
// poszerzac uprawnien. Sciezka wzgledna rozwiazuje sie wzgledem katalogu roboczego, tak jak
// w open() akcesora.
bool liesWithin(const std::filesystem::path &file, const std::string &dir) {
  std::error_code ec;
  const auto base = dir.empty() ? std::filesystem::current_path(ec) : std::filesystem::absolute(dir, ec);
  if (ec) return false;
  const auto resolvedDir = std::filesystem::weakly_canonical(base, ec);
  if (ec) return false;
  const auto resolvedFile = std::filesystem::weakly_canonical(std::filesystem::absolute(file, ec), ec);
  if (ec) return false;
  const auto [dirEnd, fileRest] = std::ranges::mismatch(resolvedDir, resolvedFile);
  return dirEnd == resolvedDir.end() && fileRest != resolvedFile.end();
}

}  // namespace

std::string descriptorRef(const Descriptor &descriptor) {
  const auto it = std::ranges::find_if(descriptor, [](const auto &item) { return item.rtype == rdb::REF; });
  return (it == descriptor.end()) ? std::string{} : it->rname;
}

StoragePaths::StoragePaths(const std::string_view qryID, const std::string_view fileName, const std::string_view storageParam) {
  if (qryID.empty()) FatalError("storage: qryID must not be empty");
  if (fileName.empty()) FatalError("storage: fileName must not be empty");

  descriptorFile_ = std::string(qryID) + ".desc";
  setStorageFile(std::string(fileName));

  if (storageParam.empty()) {
    return;  // no change
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

  if (!std::filesystem::exists(dirName)) {
    std::error_code absError;
    const auto full = std::filesystem::absolute(dirName, absError);
    FatalError(
        "storage: directory '{}' from the STORAGE directive does not exist ({}); "
        "RetractorDB does not create it - run 'mkdir -p {}' first",
        dirName, absError ? std::string("path could not be resolved") : full.string(), dirName);
  }

  if (!std::filesystem::is_directory(dirName)) {
    FatalError("storage: path '{}' from the STORAGE directive exists but is not a directory", dirName);
  }

  storageDir_ = dirName;

  descriptorFile_ = std::filesystem::path(storageParam) / std::filesystem::path(descriptorFile_);
  setStorageFile(std::filesystem::path(storageParam) / std::filesystem::path(storageFile_));
}

void StoragePaths::setStorageFile(std::string file) {
  storageFile_   = std::move(file);
  metaIndexFile_ = storageFile_ + ".meta";
}

// Czym REF jest: sciezka pliku danych, rozwiazywana wzgledem katalogu roboczego procesu, nie
// katalogu magazynu. Silnik zapisuje w nim sciezke z DECLARE ... BINFILE/TEXTFILE/DEVICE, czyli
// polozenie zewnetrznego zrodla - `/dev/urandom`, `../rec205/...` - wiec REF legalnie wychodzi poza
// katalog magazynu i tego tu nie zabraniamy.
//
// Czym REF nie jest: zgoda na zapis ani usuwanie w dowolnym miejscu (#278, S-06). Tu zapada tylko
// ustalenie, czy plik danych wyszedl poza katalog magazynu i poza `storage.ref_dirs`. Odmowe zapisu
// i ochrone przed usunieciem stosuje storage (attachStorage, ~storage) - wylacznie dla REF wzietego
// z wczytanego `.desc`, bo REF podany przez wolajacego (plan, schemat w xtrdb) jest decyzja operatora.
//
// Ustalenie zapada przed pierwszym otwarciem pliku. Dowiazanie pod nazwa pliku magazynu - takze
// bez REF - zatrzymuje dopiero O_NOFOLLOW przy samym otwarciu (openStorageFile(), #374), wiec
// podmiana pliku na dowiazanie po tym sprawdzeniu tez nic nie daje. Zostaje podmiana katalogu
// posredniego sciezki REF miedzy sprawdzeniem a open(): wymaga zapisu do katalogu nadrzednego,
// a jej zamkniecie (deskryptor katalogu, przejscie po komponentach) jest odlozone.
void StoragePaths::relocateFromRef(const Descriptor &descriptor) {
  auto it = std::ranges::find_if(descriptor,  //
                                 [](const auto &item) { return item.rtype == rdb::REF; });

  // Descriptor changes storageFile location
  if (it != descriptor.end()) {
    setStorageFile((*it).rname);
    refLeavesAllowedDirs_ = !liesWithin(storageFile_, storageDir_) &&
                            std::ranges::none_of(refDirs_, [&](const auto &dir) { return liesWithin(storageFile_, dir); });
  }

  // if storage object was created with default storage as ""
  // and there is no specified storage as REF in descriptor - we should
  // stop immediately.
  if (storageFile_.empty()) {
    FatalError("storage: storage file not set in descriptor (missing REF field or :STORAGE directive)");
  }
}

void StoragePaths::removeAllFiles(const bool keepDataFiles) const {
  if (std::filesystem::exists(descriptorFile_)) ::remove(descriptorFile_.c_str());
  if (keepDataFiles) return;
  if (!storageFile_.empty()) (void)::remove(storageFile_.c_str());
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaIndexFile_)) ::remove(metaIndexFile_.c_str());
  const std::string metaShadowFile = storageShadow::metaShadowFilePath(metaIndexFile_);
  if (!metaIndexFile_.empty() && std::filesystem::exists(metaShadowFile)) ::remove(metaShadowFile.c_str());
}

}  // namespace rdb
