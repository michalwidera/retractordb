#include "rdb/descriptorIO.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>

#include "fatalError.hpp"

extern std::string parserDESCString(rdb::Descriptor &desc, std::string_view inlet);

namespace rdb {

std::string tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor) {
  std::ifstream file(descriptorFile);
  if (!file) return "cannot open descriptor file: " + descriptorFile;

  std::ostringstream content;
  content << file.rdbuf();
  if (file.bad()) return "cannot read descriptor file: " + descriptorFile;

  Descriptor parsed;
  if (const std::string result = parserDESCString(parsed, content.str()); result != "OK")
    return "descriptor parse failed in '" + descriptorFile + "': " + result;
  if (parsed.getSizeInBytes() == 0) return "storage: empty descriptor file: " + descriptorFile;
  descriptor = std::move(parsed);
  return {};
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
  FatalError("storage: descriptor schema mismatch - remove data files and restart");
}

}  // namespace rdb
