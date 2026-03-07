#pragma once

#include <fstream>

#include "descriptor.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
/**
 * @brief Object that implements READ ONLY data source interface via fstream
 *
 * Type: TEXTSOURCE
 */
class textSourceAccessorRO : public FileAccessorInterface {
  std::string filename_;
  const ssize_t recSize_;

  Descriptor descriptor_;

  std::unique_ptr<rdb::payload> payload_;

  std::fstream myFile_;

  size_t readCount_ = 0;

  bool loopToBeginningIfEOF_ = true;

 public:
  textSourceAccessorRO(const std::string_view fileName,    //
                       const ssize_t recSize,              //
                       const rdb::Descriptor &descriptor,  //
                       bool loopToBeginningIfEOF);

  ~textSourceAccessorRO() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override {
    return EXIT_FAILURE;
  }

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
