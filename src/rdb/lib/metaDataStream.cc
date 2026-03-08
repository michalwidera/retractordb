#include "rdb/metaDataStream.h"
#include <memory>

namespace rdb {

metaDataStream::metaDataStream(const Descriptor &descriptor, const std::string &metaFilePath)  //
    : descriptorRef_(std::make_shared<Descriptor>(descriptor)),
      metaFilePath_(metaFilePath) {
  createNullBitsetTemplate();

    indexFile_.rdbuf()->pubsetbuf(nullptr, 0);

    indexFile_.open(metaFilePath_, std::ios::app);  // Open for appending (creates the file if it doesn't exist)
    if (indexFile_.good()) ;
};

metaDataStream::~metaDataStream() {
    if (indexFile_.is_open()) {
        indexFile_.close();
    }
}

void metaDataStream::createNullBitsetTemplate() {
  nullBitset_.resize(descriptorRef_->size());
  std::fill(nullBitset_.begin(), nullBitset_.end(), false);
}

void metaDataStream::onRecordModified(size_t recordIndex, std::vector<bool> &nullBitset) {};

void metaDataStream::onRecordAppended(std::vector<bool> &nullBitset) {
  /*
  if (nullBitset == nullBitset_) {
      if (!entries_.empty()) {
          entries_.back().recordCount++;
      } else {
          entries_.push_back({nullBitset, 1});
      }
  } else {
      entries_.push_back({nullBitset, 1});
  }
  */
};

}  // namespace rdb
