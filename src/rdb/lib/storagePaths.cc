#include "rdb/storagePaths.hpp"

#include <cstdio>  // ::remove
#include <filesystem>
#include <ranges>

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

  if (!std::filesystem::is_directory(storageParam)) {
    FatalError("storage: path '{}' is not a directory", storageParam);
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
