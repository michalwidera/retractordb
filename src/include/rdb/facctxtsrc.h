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

  Descriptor descriptor;  // filled by fctrl method

  std::unique_ptr<rdb::payload> payload;

  std::fstream myFile;

 public:
  ~textSourceAccessor();

  explicit textSourceAccessor(const std::string &fileName);

  ssize_t read(T *ptrData, const size_t size, const size_t position) override;
  ssize_t write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  ssize_t fctrl(void *ptrData, const size_t size) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCTXTSRC_H_