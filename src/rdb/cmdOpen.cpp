#include "cmdOpen.hpp"

#include <iostream>
#include <sstream>

#include "rdb/descriptor.hpp"

bool executeOpen(std::unique_ptr<rdb::storage>& dacc, std::string& file, const std::string& storageParam,
                 const std::string& storagePolicy, payloadStatusType& payloadStatus, const Colors& colors) {
  std::cin >> file;
  if (file.find('{') != std::string::npos) {
    std::cout << colors.RED << "unrecognized or missing file:" << file << "\n" << colors.RESET;
    return false;
  }
  auto oldPos = file.find(".old");
  if (oldPos != std::string::npos) {
    dacc = std::make_unique<rdb::storage>(file.substr(0, oldPos), file, storageParam, storagePolicy);
  } else {
    dacc = std::make_unique<rdb::storage>(file, file, storageParam, storagePolicy);
  }
  if (dacc->descriptorFileExist()) {
    dacc->attachDescriptor();
  } else {
    std::string sschema;
    std::string object;
    do {
      std::cin >> object;
      sschema.append(object);
      sschema.append(" ");
    } while (object.find("}") == std::string::npos);
    std::stringstream scheamStringStream(sschema);
    rdb::Descriptor desc;
    scheamStringStream >> desc;
    dacc->attachDescriptor(&desc);
  }
  payloadStatus = clean;
  dacc->setDisposable(false);
  if (dacc->isDeclared()) dacc->setCapacity(1);
  return true;
}
