#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <any>
#include <cstddef>  // std::byte

#include "desc.h"

namespace rdb {
/// @brief This class define accessing method to payload (memory area)
class payLoadAccessor {
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Pointer to payload
  std::byte *ptr;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat;

 public:
  /// @brief Accessor to descriptor object
  /// @return Descriptor
  Descriptor getDescriptor() const;

  /// @brief Accessor to pointer to payload
  /// @return  T* pointer to payload
  std::byte *getPayloadPtr() const;

  /// @brief Constructor of payLoadAccessor object
  /// @param descriptor descriptor of payload area
  /// @param ptr pointer to payload
  /// @param hexFormat type of default stored data
  payLoadAccessor(Descriptor descriptor, std::byte *ptr, bool hexFormat = false);

  /// @brief Default constructor is dissalowed
  payLoadAccessor() = delete;

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

  friend std::istream &operator>>(std::istream &is, const payLoadAccessor &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payLoadAccessor &rhs);
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_PAYLOADACC_H_