#ifndef STORAGE_RDB_INCLUDE_FACCTXTSRC_H_
#define STORAGE_RDB_INCLUDE_FACCTXTSRC_H_

#include <fstream>

#include "descriptor.h"
#include "fainterface.h"
#include "payload.h"

namespace rdb {
/**
 * @brief Object that implements data source interface via fstream
 */
template <typename T>
class textSrouceAccessor : public FileAccessorInterface<T> {
  std::string fileNameStr;

  Descriptor descriptor;  // filled by fctrl method

  std::unique_ptr<rdb::payload> payload;

  std::fstream myFile;

 public:
  ~textSrouceAccessor();

  textSrouceAccessor(std::string fileName);

  int read(T *ptrData, const size_t size, const size_t position) override;
  int write(const T *ptrData, const size_t size, const size_t position = std::numeric_limits<size_t>::max()) override;
  int fctrl(void *ptrData, const size_t size) override;
  std::string fileName() override;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FACCTXTSRC_H_