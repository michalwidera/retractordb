#pragma once

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "cmdID.hpp"
#include "fldType.hpp"
#include "retention.h"
namespace rdb {

// https://developers.google.com/protocol-buffers/docs/overview#scalar
// https://doc.rust-lang.org/book/ch03-02-data-types.html

enum FieldColumn { rname = 0, rlen = 1, rarray = 2, rtype = 3 };
//
// Creates ability to create descriptions of binary frames using types and arrays
//

class Descriptor : public std::vector<rField> {
  std::vector<std::pair<int, int>> convMap_;
  std::vector<int> offsetMap_;
  int clen_ = 0;
  bool dirtyMap{true};
  void updateConvMaps();

  static bool flatOutput_;

 public:
  static bool getFlat() { return flatOutput_; }
  static void setFlat(bool val) { flatOutput_ = val; }
  Descriptor(std::initializer_list<rField> l);
  Descriptor(const std::string &name, int length, int arrayCount, rdb::descFld type);

  Descriptor() = default;
  Descriptor(const Descriptor &init) = default;

  void append(std::initializer_list<rField> l);

  Descriptor &operator+=(const Descriptor &rhs);
  Descriptor &operator=(const Descriptor &rhs) = default;
  bool operator==(const Descriptor &rhs) const;

  Descriptor &createHash(const std::string &name, Descriptor lhs, Descriptor rhs);
  Descriptor &cleanRef();

  size_t getSizeInBytes() const;
  size_t position(const std::string_view name);
  int len(const std::string_view name);
  constexpr int len(const rdb::rField &field) const;
  size_t offsetBegArr(const std::string_view name);
  int offset(int position);
  std::string_view type(const std::string_view name);
  int sizeFlat();
  std::vector<rField> fieldsFlat();

  rdb::retention_t retention();
  std::pair<std::string, size_t> policy();

  std::pair<rdb::descFld, int> getMaxType();

  std::optional<std::pair<int, int>> convert(int position);

  bool hasField(const std::string_view name) const {
    return std::any_of(begin(), end(), [name](const auto &f) { return f.rname == name; });
  }

  // Operators that enables read and write Descriptor to file/screen i Human
  // Readable Form.

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

// http://www.gotw.ca/gotw/004.htm
Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs);

std::ostream &flat(std::ostream &os);
}  // namespace rdb
