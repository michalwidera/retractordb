#include "rdb/sourceBuffer.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>   // EILSEQ
#include <cstdlib>  // EXIT_SUCCESS
#include <vector>

namespace rdb {

void SourceBuffer::attach(const Descriptor &descriptor) { chamber_ = std::make_unique<rdb::payload>(descriptor); }

void SourceBuffer::setCapacity(const int capacity) { circularBuffer_.set_capacity(capacity == 0 ? 1 : capacity); }

Result<> SourceBuffer::readCurrent(FileInterface &source, payload &out) {
  //
  // THIS IS ONLY ONE PLACE WHERE DATA ARE READ FROM SOURCE
  //
  std::vector<bool> nullBitset;
  auto result = source.read(chamber_->span().data(), nullBitset, 0);
  // Blad TRESCI (plik nie zgadza sie z deklaracja) konczy odczyt, zanim rekord byl kompletny -
  // komora i bitset sa w polowie, wiec nie wolno ich wydac dalej.
  if (result == EILSEQ) return fail(Errc::Config, source.inputError());
  chamber_->setNullBitset(nullBitset);

  // Źródła deklarowane przy błędzie same zwracają dane wyzerowane i wzorzec all-null.
  if (result != EXIT_SUCCESS) {
    SPDLOG_WARN("Read failure from declared source {}. Returning null row.", source.name());
  }

  out = *chamber_;
  return {};
}

void SourceBuffer::fire(payload &out) {
  RDB_ASSERT(circularBuffer_.capacity() != 0, "SourceBuffer::fire: circular buffer capacity is zero - attach() not called");
  out = *chamber_;
  circularBuffer_.push_front(out);  // only one place when buffer is feed.
}

}  // namespace rdb
