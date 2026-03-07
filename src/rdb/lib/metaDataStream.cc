#include "rdb/metaDataStream.h"
#include <memory>

namespace rdb {

metaDataStream::metaDataStream(const Descriptor &descriptor)  //
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)) {};

void metaDataStream::onRecordModified(size_t recordIndex, std::vector<bool> &nullBitset) {};

void metaDataStream::onRecordAppended(std::vector<bool> &nullBitset) {};

}  // namespace rdb
