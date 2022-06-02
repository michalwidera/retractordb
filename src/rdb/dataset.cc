#include "rdb/dataset.h"

#include <cstddef>
#include <iostream>  // }{

#include "rdb/desc.h"
#include "rdb/dsacc.h"

namespace rdb {
void dataSet::setStoragePath(std::string pathParam) { path = pathParam; }

long dataSet::streamStoredSize(std::string filename) {
  return data[path + filename]->getRecordsCount();
}

long dataSet::GetLen(std::string filename) {
  return data[path + filename]->getRecordsCount() * recordSize;
}

void dataSet::DefBlock(std::string filename, rdb::Descriptor desc) {
  data[path + filename] = std::unique_ptr<rdb::DataStorageAccessor<std::byte>>(
      new rdb::DataStorageAccessor(desc, path + filename, false));
  recordSize = desc.GetSize();
  if (filename == "source") {  // BUG LOGGING
    std::cerr << "}{" << filename << std::endl << desc << std::endl;
  }
}

void dataSet::PutBlock(std::string filename, char* pRawData) {
  data[path + filename]->Put(reinterpret_cast<std::byte*>(pRawData));
}

bool dataSet::GetBlock(std::string filename, int offset, char* pRawData) {
  bool success = data[path + filename]->Get(
      reinterpret_cast<std::byte*>(pRawData), offset);
  if (filename == "source")
    std::cerr << "3. " << (int)*pRawData << " " << offset << std::endl;
  return success;
}

void dataSet::reverse(std::string filename, bool val) {
  data[path + filename]->setReverse(val);
}
}  // namespace rdb
