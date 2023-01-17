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
template <class K = rdb::posixPrmBinaryFileAccessor<std::byte>>
class storageAccessor {
  std::unique_ptr<K> accessor;

  Descriptor descriptor;

  bool reverse = false;

  bool removeOnExit;

  size_t recordsCount;

  std::string filename;

 public:
  storageAccessor() = delete;

  ~storageAccessor();

  std::unique_ptr<std::byte[]> payload;

  /**
   * @brief Open existing Data Accessor object and check descriptor file
   *
   * @param fileName Storage file
   * @param reverse type of Get/Set operations index - from head or from tail
   */
  storageAccessor(std::string fileName);

  void createDescriptor(const Descriptor descriptor);

  enum dataForm { noDescriptor, open } dataFileStatus;

  /**
   * @brief Reads data package from storage
   *
   * @param recordIndex location from beginging of the storage [unit: Records]
   * @return success status - true eq. success
   */
  bool read(const size_t recordIndex);

  /**
   * @brief Sends record to the storage
   *
   * @param recordIndex location from begining of the storage [unit: Records]
   * @return success status- true eq. success
   */
  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());

  /**
   * @brief Accessor method - get ref to descriptor
   *
   * @return Descriptor& reference to descriptor inner object
   */
  Descriptor& getDescriptor();

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
   * @brief Get storage filename
   *
   * @return const string filename
   */
  const std::string getStorageFilename();
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DACC_H_