#pragma once

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "cmdID.hpp"
#include "fldType.hpp"
#include "retention.hpp"
namespace rdb {

// https://developers.google.com/protocol-buffers/docs/overview#scalar
// https://doc.rust-lang.org/book/ch03-02-data-types.html

enum FieldColumn { rname = 0, rlen = 1, rarray = 2, rtype = 3 };
//
// Creates ability to create descriptions of binary frames using types and arrays
//

/// @brief Definicja klasy implementującej strukturę opisu danych
///
/// Obiekt Descriptor powinien:
/// - zapisywać i odczytywać pliki z opisem struktury danych
/// - umożliwiać definiowanie różnych typów danych, takich jak liczby całkowite, zmiennoprzecinkowe, łańcuchy znaków itp.
/// - umożliwiać definiowanie tablic o różnych rozmiarach i typach danych
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci
/// - udostępniać informacje o strukturze danych, takie jak nazwy pól, typy danych, rozmiary itp.
/// - udostępniać informacje o fizycznej reprezentacji danych, takie jak rozmiar w bajtach, offsety itp.
/// - dostarczać metod dodawnia i porównywania opisów danych

class Descriptor : public std::vector<rField> {
  std::vector<std::pair<int, int>> convMap_;
  std::vector<int> offsetMap_;
  int clen_ = 0;
  bool dirtyMap{true};
  /// @brief Rebuild cached flattened field mappings and byte offsets when descriptor layout changes
  /// @details Updates convMap_ with pairs of descriptor field index and array element index,
  /// offsetMap_ with byte offsets for flattened positions, and clen_ with flattened field count.
  /// Configuration fields are skipped and the cache is recomputed only when dirtyMap is set.
  void updateConvMaps();

  static bool flatOutput_;

 public:
  /// @brief Get current flat output formatting flag
  /// @return true when stream output should be written in a single line
  static bool getFlat() { return flatOutput_; }

  /// @brief Set flat output formatting flag
  /// @param val true enables single-line output mode
  static void setFlat(bool val) { flatOutput_ = val; }

  /// @brief Construct descriptor from initializer list of fields
  /// @param l list of fields appended in order
  Descriptor(std::initializer_list<rField> l);

  /// @brief Construct descriptor containing a single field
  /// @param name field name
  /// @param length size of a single element in bytes
  /// @param arrayCount number of elements in the field array
  /// @param type field type identifier
  Descriptor(const std::string &name, int length, int arrayCount, rdb::descFld type);

  /// @brief Default constructor
  Descriptor() = default;

  /// @brief Copy constructor
  /// @param init descriptor used as source
  Descriptor(const Descriptor &init) = default;

  /// @brief Append fields at the end of descriptor
  /// @param l list of fields to append
  void append(std::initializer_list<rField> l);

  /// @brief Append all fields from another descriptor
  /// @param rhs descriptor to append
  /// @return reference to current descriptor
  Descriptor &operator+=(const Descriptor &rhs);

  /// @brief Copy assignment operator
  /// @param rhs descriptor used as source
  /// @return reference to current descriptor
  Descriptor &operator=(const Descriptor &rhs) = default;

  /// @brief Compare compatibility of two descriptors
  /// @param rhs descriptor to compare with
  /// @return true when field layout is compatible
  bool operator==(const Descriptor &rhs) const;

  /// @brief Create descriptor for hash output from two inputs
  /// @param name prefix used for generated field names
  /// @param lhs left input descriptor
  /// @param rhs right input descriptor
  void composeHashDescriptorFrom(const std::string &name, Descriptor lhs, Descriptor rhs);

  /// @brief Remove configuration fields like REF, TYPE and retention metadata
  void removeConfigurationFields();

  /// @brief Calculate full binary size described by all non-configuration fields
  /// @return size in bytes
  size_t getSizeInBytes() const;

  /// @brief Find field position by name
  /// @param name field name
  /// @return index of the field inside descriptor
  size_t position(const std::string_view name);

  /// @brief Get total byte length of a field selected by name
  /// @param name field name
  /// @return field size in bytes including array expansion
  int fieldSize(const std::string_view name);

  /// @brief Get total byte length of a field
  /// @param field field definition
  /// @return field size in bytes including array expansion
  constexpr int fieldSize(const rdb::rField &field) const;

  /// @brief Compute byte offset of the beginning of named field
  /// @param name field name
  /// @return byte offset from the start of payload
  size_t offsetBegArr(const std::string_view name);

  /// @brief Compute byte offset for a flattened field position
  /// @param position flattened field index
  /// @return byte offset from the start of payload
  int offset(int position);

  /// @brief Get textual type name for a field
  /// @param name field name
  /// @return enum name of the field type
  std::string_view type(const std::string_view name);

  /// @brief Get number of fields in flattened view
  /// @return count of logical flattened fields
  int flatElementCount();

  /// @brief Get non-configuration fields without flattening arrays
  /// @return vector with data-carrying fields only
  std::vector<rField> fieldsFlat();

  /// @brief Read retention configuration from descriptor
  /// @return retention settings or default no-retention value
  rdb::retention_t retention();

  /// @brief Read storage policy encoded by TYPE and RETMEMORY fields
  /// @return pair of policy name and configured size
  std::pair<std::string, size_t> policy();

  /// @brief Find widest type and size used by non-configuration fields
  /// @return pair of maximal type and its byte size
  std::pair<rdb::descFld, int> getMaxType();

  /// @brief Convert flattened field position to descriptor field index and array offset
  /// @param position flattened field index
  /// @return pair of descriptor index and array element offset
  std::optional<std::pair<int, int>> convert(int position);

  /// @brief Check whether descriptor contains field with given name
  /// @param name field name
  /// @return true when field exists
  bool hasField(const std::string_view name) const {
    return std::any_of(begin(), end(), [name](const auto &f) { return f.rname == name; });
  }

  // Operators that enables read and write Descriptor to file/screen i Human
  // Readable Form.

  /// @brief Write descriptor in textual form to stream
  /// @param os output stream
  /// @param rhs descriptor to write
  /// @return output stream reference
  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);

  /// @brief Read descriptor from textual form stored in stream
  /// @param is input stream
  /// @param rhs descriptor to fill
  /// @return input stream reference
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

// http://www.gotw.ca/gotw/004.htm
/// @brief Create descriptor by concatenating fields from two descriptors
/// @param lhs left descriptor
/// @param rhs right descriptor
/// @return concatenated descriptor
Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs);

/// @brief Enable flat textual formatting for the next descriptor or payload stream output
/// @param os output stream
/// @return output stream reference
std::ostream &flat(std::ostream &os);
}  // namespace rdb
