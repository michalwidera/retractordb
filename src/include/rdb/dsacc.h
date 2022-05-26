#ifndef STORAGE_RDB_INCLUDE_DACC_H_
#define STORAGE_RDB_INCLUDE_DACC_H_

#include <cstddef>  // std::byte
#include <memory>   // std::unique_ptr
#include <string>

#include "desc.h"
#include "faccfs.h"
#include "faccposix.h"
#include "faccposixprm.h"
#include "fainterface.h"

namespace rdb {
/**
 * @brief This object purpose is to access data via descriptor
 */
template <class T = std::byte, class K = rdb::posixPrmBinaryFileAccessor<T>>
class DataStorageAccessor {
  std::unique_ptr<K> pAccessor;

  Descriptor descriptor;

  bool reverse;

  bool removeOnExit;

  size_t recordsCount;

  std::string filename;

 public:
  DataStorageAccessor() = delete;

  ~DataStorageAccessor();

  /**
   * @brief Accessor method - get ref to descriptor
   *
   * @return Descriptor& reference to descriptor inner object
   */
  Descriptor &getDescriptor();

  /**
   * @brief Set the Reverse value
   *
   * @param value reverse iterator status
   */
  void setReverse(bool value);

  /**
   * @brief Set if files should remain after program exits
   * @param value remove on exit mode
   */
  void setRemoveOnExit(bool value);

  /**
   * @brief Get the Records Count object - how many payloads are in file
   *
   * @return const size_t count of records/payloads
   */
  const size_t getRecordsCount();

  /**
   * @brief Construct a new Data Accessor object and create descriptor file
   *
   * @param descriptor Definition of binary schema
   * @param fileName Storage file
   * @param reverse type of Get/Set operations index - from head or from tail
   */
  DataStorageAccessor(const Descriptor descriptor, std::string fileName,
                      bool reverse = false);

  /**
   * @brief Open existing Data Accessor object and check descriptor file
   *
   * @param fileName Storage file
   * @param reverse type of Get/Set operations index - from head or from tail
   */
  DataStorageAccessor(std::string fileName, bool reverse = false);

  /**
   * @brief Reads data package from storage
   *
   * @param inBuffer pointer to area where package will be fetched
   * @param recordIndex location from beginging of the storage [unit: Records]
   * @return success status - true eq. success
   */
  bool Get(T *inBuffer, const size_t recordIndex);

  /**
   * @brief Sends record to the storage
   *
   * @param outBuffer pointer to area when record is stored
   * @param recordIndex location from begining of the storage [unit: Records]
   * @return success status- true eq. success
   */
  bool Put(const T *outBuffer,
           const size_t recordIndex = std::numeric_limits<size_t>::max());
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_