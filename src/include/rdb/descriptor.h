#ifndef STORAGE_RDB_INCLUDE_DESC_H_
#define STORAGE_RDB_INCLUDE_DESC_H_

#include <initializer_list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "cmdID.h"
#include "fldType.h"

namespace rdb {

// https://developers.google.com/protocol-buffers/docs/overview#scalar
// https://doc.rust-lang.org/book/ch03-02-data-types.html

enum FieldColumn { rname = 0, rlen = 1, rtype = 2 };

constexpr int error_desc_location = -1;

extern bool flatOutput;

class Descriptor : public std::vector<rField> {
 public:
  bool isEmpty() const;

  Descriptor(std::initializer_list<rField> l);
  Descriptor(std::string n, int l, rdb::descFld t);

  Descriptor() = default;
  Descriptor(const Descriptor &init);

  void append(std::initializer_list<rField> l);

  Descriptor &operator|(const Descriptor &rhs);
  Descriptor &operator=(const Descriptor &rhs);
  bool operator==(const Descriptor &rhs);

  void createHash(const std::string name, Descriptor lhs, Descriptor rhs);
  void cleanRef();

  int getSizeInBytes() const;
  int position(std::string name);
  std::string fieldName(int fieldPosition);
  int len(const std::string name);
  int offset(const std::string name);
  int offset(int position);

  std::string type(const std::string name);

  std::pair<rdb::descFld, int> getMaxType();

  template <typename T>
  std::string toString(const std::string name, T *ptr) {
    return std::string(reinterpret_cast<char *>(ptr + offset(name)), len(name));
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
  auto cast(const std::string name, K *ptr) {
    return *(reinterpret_cast<T *>(ptr + offset(name)));
  };

  // Operators that enables read and write Descriptor to file/screen i Human
  // Readable Form.

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

std::ostream &flat(std::ostream &os);
}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DESC_H_