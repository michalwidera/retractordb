#include "rdb/externalacc.h"

#include <spdlog/spdlog.h>

#include <fstream>

namespace rdb {
externalAccessor::externalAccessor(std::string fileName) {}

void externalAccessor::attachDescriptor(const Descriptor descriptorParam) {
  descriptor = descriptorParam;
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(0, 0);
  auto fileDesc(filename.append(".desc"));
  // Creating .desc file
  descFile.open(fileDesc, std::ios::out);
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile << descriptor;
  assert((descFile.rdstate() & std::ofstream::failbit) == 0);
  descFile.close();
}
bool externalAccessor::read() { return true; }

Descriptor& externalAccessor::getDescriptor() { return descriptor; }

void externalAccessor::attachPayloadPtr(std::byte* payloadPtrVal) {
  SPDLOG_INFO("required: PayloadPtr [attached]");
  payloadPtr = payloadPtrVal;
}

void externalAccessor::attachPayload(rdb::payload& payloadRef) {
  SPDLOG_INFO("required: Payload [attached]");
  payloadPtr = payloadRef.get();
}
}  // namespace rdb
