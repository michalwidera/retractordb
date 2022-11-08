#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <any>
#include <cstddef>  // std::byte

#include "desc.h"

namespace rdb {
/**
 * @brief This class define accessing method to payload (memory area)
 *
 * @tparam T Type of stored data - std::byte or char
 */
template <typename T>
class payLoadAccessor {
  /**
   * Descriptor of managed payload area
   */
  Descriptor descriptor;

  /**
   * Pointer to payload
   */
  T *ptr;

  /**
   * Type of dumped or read numeric formats
   */
  bool hexFormat;

 public:
  /**
   * @brief Accessor to descriptor object
   *
   * @return Descriptor
   */
  Descriptor getDescriptor() const;

  /**
   * @brief Accessor to pointer to payload
   *
   * @return T* pointer to payload
   */
  T *getPayloadPtr() const;

  /**
   * Constructor of payLoadAccessor object
   *
   * @param descriptor descriptor of payload area
   * @param ptr pointer to payload
   * @param hexFormat type of default stored data
   */
  payLoadAccessor(Descriptor descriptor, T *ptr, bool hexFormat = false);

  /**
   * Default constructor is dissalowed
   */
  payLoadAccessor() = delete;

  /**
   * Dirrect setter
   *
   * @param position position according to descriptor
   * @param value value of given type according to desciptor that will be set in
   * payload
   */
  void setPayloadField(int position, std::any value);

  /**
   * Dirrect getter
   *
   * @param position position according to descriptor
   * @return address of begining memory that contains data descibed by
   * descriptor
   */
  std::any getPayloadField(int position);

  template <typename K>
  friend std::istream &operator>>(std::istream &is,
                                  const payLoadAccessor<K> &rhs);

  template <typename K>
  friend std::ostream &operator<<(std::ostream &os,
                                  const payLoadAccessor<K> &rhs);
};
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_PAYLOADACC_H_