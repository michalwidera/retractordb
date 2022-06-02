#include "dbstream.h"

#include <algorithm>           // for max
#include <cassert>             // for assert
#include <cstring>             // for memcpy, NULL
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <iostream>            // for basic_ostream::operator<<, operator<<

#include "rdb/dataset.h"

rdb::dataSet database;

using namespace boost;

long streamStoredSize(std::string filename) {
  return database.streamStoredSize(filename);
}

void setStorageLocation(std::string location) {
  database.setStoragePath(location);
}

dbStream::dbStream(std::string streamName, std::list<field> schema)
    : streamName(streamName),
      schema(schema),
      mpReadNr(-1),
      mpLenNr(-1),
      frameSize(schema.size() * sizeof(number)),
      pRawData(new char[frameSize]) {
  rdb::Descriptor data;
  for (auto const &i : schema) data | rdb::Descriptor(i.fieldName, i.fieldType);
  database.DefBlock(streamName, data);
  mpRead.resize(schema.size());
}

void dbStream::store() {
  char *p(pRawData.get());
  int cnt(0);
  for (auto &str : schema) {
    number fieldData;
    memcpy(p, &fieldData, sizeof(number));
    p += sizeof(number);
    cnt++;
  }
  database.PutBlock(streamName, pRawData.get());
  mpLenNr = -1;
  mpReadNr = -1;
}

number dbStream::readData(int offset, int keyval, bool reverse) {
  if (offset < 0) {
    abort();
  }
  assert(offset >= 0);
  int len = database.GetLen(streamName);
  // if (mpReadNr == offset && mpLenNr == len) return mpRead[keyval];
  if (offset > len || len == 0) {
    const number fake = boost::rational<int>(999, 1);
    for (auto i = 0; i < schema.size(); i++) mpRead[i] = fake;
    return fake;
  }
  assert(len != 0);
  bool success;
  if (reverse)
    success = database.GetBlock(streamName, offset, pRawData.get());
  else {
    database.reverse(streamName, true);
    success = database.GetBlock(streamName, offset, pRawData.get());
    database.reverse(streamName, false);
  }
  if (!success) {
    abort();
  }
  mpRead.clear();
  char *p(pRawData.get());
  for (int i = 0; i < schema.size(); i++) {
    number fieldDataDest;
    memcpy(&fieldDataDest, p, sizeof(number));
    mpRead[i] = fieldDataDest;
    p += sizeof(number);
  }
  mpReadNr = offset; /* position */
  mpLenNr = len;
  return mpRead[keyval];
}