#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <any>
#include <cstddef>  // std::byte
#include <memory>   // std::unique_ptr

#include "descriptor.h"

namespace rdb {
/// @brief This class define accessing method to payload (memory area)
class payloadAccessor {
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Payload memory area
  std::unique_ptr<std::byte[]> payload;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat;

 public:
  /// @brief Accessor to descriptor object
  /// @return Descriptor
  Descriptor getDescriptor() const;

  /// @brief Accessor to pointer to payload
  /// @return  T* pointer to payload
  std::byte *getPayloadPtr() const;

  /// @brief Constructor of payloadAccessor object
  /// @param descriptor descriptor of payload area
  payloadAccessor(Descriptor descriptor);

  /// @brief Default constructor is dissalowed
  payloadAccessor() = delete;

  /// @brief Dirrect setter
  /// @param position position according to descriptor
  /// @param value value of given type according to desciptor that will be set
  /// in payload
  void setItem(int position, std::any value);

  /// @brief Dirrect getter
  /// @param position position according to descriptor
  /// @return address of begining memory that contains data descibed by
  /// descriptor
  std::any getItem(int position);

  /// @brief Set format input/output formater - default false
  /// @param hexFormat true if out/in in hex
  void setHex(bool hexFormat);

  friend std::istream &operator>>(std::istream &is, const payloadAccessor &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payloadAccessor &rhs);
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_PAYLOADACC_H_