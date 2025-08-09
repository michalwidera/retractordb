#ifndef STORAGE_RDB_INCLUDE_FAINTERFACE_H_
#define STORAGE_RDB_INCLUDE_FAINTERFACE_H_

#include <cstdint>  // uint8_t
#include <limits>
#include <string>

namespace rdb {
/**
 * @brief This is interface for accessor interface.
 *
 * File Accessor Interface.
 * This is used as pattern for: faccposix, facposixprm and faccfs types.
 * All these types need to support this interface
 *
 * There is only 3 operations over file/object/blob aka storage
 * append data at the end of storage
 * Read data from the storage
 * Update data in the middle of storage
 * This three has been covered by following interface
 * read ::= read(data, position)
 * append :== write(data, position == max_possible_value )
 * update :== write(data, position)
 *
 * @tparam T Type of stored data - uint8_t or char
 */
struct FileAccessorInterface {
  /**
   * @brief Reads from storage amount of bytes into memory pointed by ptrData
   * from position in storage
   *
   * @param ptrData pointer to data in memory where data will be fetched from storage
   * @param size size of data that will be transferred
   * @param position position from the beginning of file [unit: Bytes]
   * @return status of operation - 0/EXIT_SUCCESS success
   */
  virtual ssize_t read(uint8_t *ptrData, const size_t position) = 0;

  /**
   * @brief Updates or appends data in the storage
   *
   * @param ptrData pointer to table of bytes in memory that will be updated in storage
   * @param size  size of data to be updated
   * @param position position from the beginning of file [unit: Bytes]. If max possible value - works as append.
   * @return status of operation - 0/EXIT_SUCCESS success
   */
  virtual ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) = 0;

  // following: https://stackoverflow.com/questions/51615363/how-to-write-c-getters-and-setters
  virtual auto name() const -> const std::string & = 0;
  virtual auto name() -> std::string             & = 0;

  /**
   * @brief data count in storage
   *
   * @return number of records in storage
   */
  virtual size_t count() = 0;

  virtual ~FileAccessorInterface() = default;
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_FAINTERFACE_H_