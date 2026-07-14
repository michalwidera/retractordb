#include "rdb/descriptorIO.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>

#include "fatalError.hpp"

namespace rdb {

Descriptor loadDescriptorFile(const std::string &descriptorFile) {
  Descriptor descriptor;

  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(descriptorFile, std::ios::in);  // Open existing descriptor
  if (myFile.good()) myFile >> descriptor;
  myFile.close();

  if (descriptor.getSizeInBytes() == 0) {
    SPDLOG_ERROR("Empty descriptor in file.");
    std::cerr << "Error, empty descriptor file:" << descriptorFile << '\n';
    FatalError("storage: empty descriptor file");
  }
  return descriptor;
}

void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor) {
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(nullptr, 0);
  descFile.open(descriptorFile, std::ios::out);
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to open descriptor file for writing: {}", descriptorFile);
  }
  descFile << descriptor;
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to write descriptor file: {}", descriptorFile);
  }
  descFile.close();
}

void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile) {
  if (provided == existing) return;

  SPDLOG_ERROR("Descriptors do not match.");
  std::cerr << "Error in data descriptor file: " << descriptorFile << '\n';
  std::cerr << "Provided Descriptor:\n" << provided << "\nExisting Descriptor:\n" << existing << '\n';
  FatalError("storage: descriptor schema mismatch — remove data files and restart");
}

}  // namespace rdb
