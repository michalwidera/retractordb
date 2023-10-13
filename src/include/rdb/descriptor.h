#ifndef STORAGE_RDB_INCLUDE_DESC_H_
#define STORAGE_RDB_INCLUDE_DESC_H_

#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "cmdID.h"
#include "fldType.h"

namespace rdb {

// https://developers.google.com/protocol-buffers/docs/overview#scalar
// https://doc.rust-lang.org/book/ch03-02-data-types.html

enum FieldColumn { rname = 0, rlen = 1, rarray = 2, rtype = 3 };

extern bool flatOutput;

//
// Creates ability to create descriptions of binary frames using types and arrays
//

class Descriptor : public std::vector<rField> {
  std::vector<std::pair<int, int>> convMap;
  std::map<std::pair<int, int>, int> convReMap;
  std::vector<int> offsetMap;
  int clen = 0;
  void updateConvMaps();

 public:
  bool dirtyMap{true};
  bool isEmpty() const;

  Descriptor(std::initializer_list<rField> l);
  Descriptor(const std::string &n, int l, int a, rdb::descFld t);

  Descriptor() = default;
  Descriptor(const Descriptor &init);

  void append(std::initializer_list<rField> l);

  Descriptor &operator+=(const Descriptor &rhs);
  Descriptor &operator=(const Descriptor &rhs);
  bool operator==(const Descriptor &rhs) const;

  Descriptor &createHash(const std::string &name, Descriptor lhs, Descriptor rhs);
  Descriptor &cleanRef();

  int getSizeInBytes() const;
  int position(const std::string &name);
  std::string fieldName(int fieldPosition);
  int len(const std::string &name);
  constexpr int len(const rdb::rField &field) const;
  int offsetBegArr(const std::string &name);
  int offset(int position);
  int arraySize(const std::string &name);
  std::string type(const std::string &name);
  int sizeFlat();

  std::pair<rdb::descFld, int> getMaxType();

  std::optional<std::pair<int, int>> convert(int position);
  std::optional<int> convert(std::pair<int, int> position);

  template <typename T>
  std::string toString(const std::string &name, T *ptr) {
    return std::string(reinterpret_cast<char *>(ptr + offsetBegArr(name)), len(name));
  }

  /**
   * @brief Reads data from binary package via tuple-data from inner container
   *
   * @param T Type that data should be converted (returned)
   * @param name name of given field
   * @param ptr pointer to beginning of package
   * @return auto Value from binary package that corresponds to field from
   * container
   */
  template <typename T, typename K>
  auto cast(const std::string &name, K *ptr) {
    return *(reinterpret_cast<T *>(ptr + offsetBegArr(name)));
  };

  // Operators that enables read and write Descriptor to file/screen i Human
  // Readable Form.

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

// http://www.gotw.ca/gotw/004.htm
const Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs);

std::ostream &flat(std::ostream &os);
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DESC_H_