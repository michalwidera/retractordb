#pragma once

#include <cstdint>  // uint8_t
#include <limits>

#include "descriptor.h"

namespace rdb {
/// @brief This class define accessing method to payload (memory area)
/// This is accessor for payload memory area that supports applying descriptor type over memory area.

class metaDataStream {
 public:
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Constructor of payload object
  /// @param descriptor descriptor of payload area
  explicit metaDataStream(const Descriptor &descriptor) : descriptor(descriptor) {}
};
}  // namespace rdb