#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>

namespace rdb {
/**
 * @brief Object that implements storage interface via fstream
 *
 * File  as File system - type. This is underlying type that supports access to binary fields.
 * std::fstream is used as interface. :Rdb user does not need to use this object directly
 *
 * Type: GENERIC
 */
struct genericBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  int percounter_;

 public:
  genericBinaryFile(const std::string_view fileName, const Descriptor &descriptor, int percounter = -1);
  ~genericBinaryFile() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t write(const uint8_t *ptrData, const size_t position, const std::vector<bool> &nullBitset) override;
  ssize_t read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) override;

  auto name() -> std::string & override;
  size_t count() override;

 private:
  ssize_t readRaw(uint8_t *ptrData, const size_t position);
  ssize_t writeRaw(const uint8_t *ptrData, const size_t position);

  genericBinaryFile()                                           = delete;
  genericBinaryFile(const genericBinaryFile &)                  = delete;
  const genericBinaryFile &operator=(const genericBinaryFile &) = delete;
};

}  // namespace rdb
