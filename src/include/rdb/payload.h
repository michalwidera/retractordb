#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <any>
#include <memory>  // std::unique_ptr

#include "descriptor.h"

namespace rdb {
/// @brief This class define accessing method to payload (memory area)
class payload {
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Payload memory area
  std::unique_ptr<uint8_t[]> payloadData;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat;

 public:
  /// @brief Accessor to descriptor object
  /// @return Descriptor
  Descriptor getDescriptor() const;

  /// @brief Accessor to pointer to payload
  /// @return  T* pointer to payload
  uint8_t *get() const;

  /// @brief Constructor of payload object
  /// @param descriptor descriptor of payload area
  payload(Descriptor descriptor);

  /// @brief Copy constructor
  /// @param from destructor
  payload(const payload &other);

  /// @brief Default constructor is disallowed
  payload() = delete;

  /// @brief Direct setter
  /// @param position position according to descriptor
  /// @param valueParam value of given type according to descriptor that will be set
  /// in payload
  void setItem(int position, std::any valueParam);

  /// @brief Direct getter
  /// @param position position according to descriptor
  /// @return address of beginning memory that contains data described by
  /// descriptor
  std::any getItem(int position);

  /// @brief Set format input/output formater - default false
  /// @param hexFormat true if out/in in hex
  void setHex(bool hexFormat);

  template <typename T>
  void setItemBy(int position, std::any value);

  friend std::istream &operator>>(std::istream &is, const payload &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payload &rhs);

  payload &operator=(payload &other);
  payload &operator=(const payload &other);

  payload operator+(payload &other);
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_PAYLOADACC_H_