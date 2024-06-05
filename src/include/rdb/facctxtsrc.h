#ifndef STORAGE_RDB_INCLUDE_FACCTXTSRC_H_
#define STORAGE_RDB_INCLUDE_FACCTXTSRC_H_

#include <fstream>

#include "descriptor.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
/**
 * @brief Object that implements data source interface via fstream
 *
 * Type: TEXTSOURCE
 */
template <typename T>
class textSourceAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

  Descriptor descriptor;

  std::unique_ptr<rdb::payload> payload;

  std::fstream myFile;

 public:
  ~textSourceAccessor();

  explicit textSourceAccessor(const std::string &fileName, const rdb::Descriptor &descriptor);

  ssize_t read(T *ptrData, const size_t size, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;

  std::string fileName() override;
  size_t count() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCTXTSRC_H_