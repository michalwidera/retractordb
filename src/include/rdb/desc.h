#ifndef STORAGE_RDB_INCLUDE_DESC_H_
#define STORAGE_RDB_INCLUDE_DESC_H_

#include <initializer_list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "enumDecl.h"

namespace rdb {

// https://developers.google.com/protocol-buffers/docs/overview#scalar
// https://doc.rust-lang.org/book/ch03-02-data-types.html

typedef int fieldLen;
typedef std::string fieldName;

/**
 * @brief Tuple type - that defines field - Name, Len and Type.
 */
typedef std::tuple<fieldName, fieldLen, rdb::eType> rfield;

/**
 * @brief This enum helps write std::get<rlen>(i) instead std::get<1>(i)
 */
enum FieldColumn { rname = 0, rlen = 1, rtype = 2 };

/**
 * @brief Structure resposible for mapping types into binary struct
 */
class Descriptor : public std::vector<rfield> {
  /**
   * @brief This is map of field names. Each field name should be unique for
   * each descriptor If two or more tuples have same name this fieldNames object
   * is empty.
   *
   */
  std::map<std::string, int> fieldNames;

  /**
   * @brief Internal function that will check names consistency in object
   *
   * @return true - no double names found
   * @return false - found doubled names fields
   */
  bool UpdateNames();

 public:
  bool isDirty() const;

  /**
   * @brief Construct a new Descriptor object
   *
   * @param l initializer list - this enables create Descriptor as Descriptor
   * obj{field("A",10,STRING), field("B",10,STRING)};
   */
  Descriptor(std::initializer_list<rfield> l);

  /**
   * @brief Construct a new Descriptor object - only for STRING (object with
   * len)
   *
   * @param n Field name
   * @param l Field len
   * @param t Field type - STRING (Maybe tables in future)
   */
  Descriptor(fieldName n, fieldLen l, rdb::eType t);

  /**
   * @brief Construct a new Descriptor object - only for no objects with len
   *
   * @param n Field name
   * @param t Field type - Int, Byte ...
   */
  Descriptor(fieldName n, rdb::eType t);

  /**
   * @brief Construct a new Descriptor object - Default constructor
   */
  Descriptor() = default;

  /**
   * @brief This function will object.Append({field("A",10,STRING)});
   *
   * @param l Initializer list of fields in {}
   */
  void Append(std::initializer_list<rfield> l);

  /**
   * @brief This constructor helps chaining operators with descriptor.
   *
   * DescriptorObject | Descriptor("A",10,STRING) | Descriptor("B",BYTE)
   *
   * @param rhs Right Hand Side - it is reference to Right arguemnt of operator
   * @return Descriptor& This return reference to result
   */
  Descriptor &operator|(const Descriptor &rhs);
  /**
   * @brief This constructor define Descriptor = Descriptor operation
   *
   * As it is a vector ... it wouldn't be necessary - but there are uniqnees
   * name requirement - and if vectors of tuples stacked in container will have
   * same names - these name map will be zeroed and this signs and error
   *
   * @param rhs
   * @return Descriptor&
   */
  Descriptor &operator=(const Descriptor &rhs);

  /**
   * @brief Copy constructor a new Descriptor object based on another Descriptor
   * object.
   *
   * @param init
   */
  Descriptor(const Descriptor &init);

  /**
   * @brief Get the Size object - it scans all tuples and sums size in bytes of
   * these objects.
   *
   * @return uint size of package [unit: Bytes]
   */
  uint GetSize() const;

  /**
   * @brief Return position as index in vector of tuples of given field name
   *
   * @param name Field name
   * @return uint Position [unit: Index]
   */
  uint Position(std::string name);

  /**
   * @brief Finds in inner container given tuple by name and return this tuple
   * len
   *
   * @param name Field name
   * @return uint Field name [unit: Bytes]
   */
  uint Len(const std::string name);

  /**
   * @brief Counts over inner container and finds offset of given field name
   * from package begining
   *
   * @param name Field name
   * @return uint Filed offset [unit: Bytes]
   */
  uint Offset(const std::string name);

  /**
   * @brief Return type of given field
   *
   * @param name Field name
   * @return std::string Type as literal string
   */
  std::string Type(const std::string name);

  /**
   * @brief In case of string types this funcion will get binary representation
   * and convert it to string by accessing name
   *
   * @param name Name of searched field
   * @param ptr Pointer to std::byte table / begininng package
   * @return std::string Returned string from field.
   */
  template <typename T>
  std::string ToString(const std::string name, T *ptr) {
    return std::string(reinterpret_cast<char *>(ptr + Offset(name)), Len(name));
  }

  /**
   * @brief Reads data from binary package via tuple-data from inner container
   *
   * @tparam T Type that data should be converted (returned)
   * @param name name of given field
   * @param ptr pointer to begininng of package
   * @return auto Value from binary package that corresponds to field from
   * container
   */
  template <typename T, typename K>
  auto Cast(const std::string name, K *ptr) {
    return *(reinterpret_cast<T *>(ptr + Offset(name)));
  };

  // Operators that enables read and write Descriptor to file/scereen i Human
  // Readable Form.

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

}  // namespace rdb

#endif  // STORAGE_RDB_INCLUDE_DESC_H_