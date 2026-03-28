#pragma once

#include "fainterface.h"

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
  genericBinaryFile(const std::string_view fileName, const ssize_t recordSize, int percounter = -1);
  ~genericBinaryFile() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;

  genericBinaryFile()                                                   = delete;
  genericBinaryFile(const genericBinaryFile &)                  = delete;
  const genericBinaryFile &operator=(const genericBinaryFile &) = delete;
};

}  // namespace rdb
