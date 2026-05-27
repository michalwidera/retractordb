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

enum FieldColumn : std::uint8_t { rname = 0, rlen = 1, rarray = 2, rtype = 3 };
//
// Creates ability to create descriptions of binary frames using types and arrays
//

/// @brief Klasa opisująca układ pól rekordu oraz metadane konfiguracyjne związane z magazynem danych.
///
/// Obiekt Descriptor powinien:
/// - przechowywać definicje pól rekordu w postaci listy opisowej (rField),
/// - umożliwiać definiowanie pól o różnych typach, długościach i krotnościach tablicowych,
/// - udostępniać informacje o logicznej i fizycznej strukturze danych, takie jak nazwy pól, typy, rozmiary i offsety,
/// - wyznaczać rozmiar binarnej reprezentacji rekordu z pominięciem pól konfiguracyjnych,
/// - udostępniać odwzorowanie między pozycją w spłaszczonym widoku pól a indeksem pola oraz offsetem w rekordzie,
/// - umożliwiać dodawanie i łączenie opisów pól,
/// - umożliwiać odczyt i zapis tekstowej reprezentacji deskryptora przez operatory strumieniowe,
/// - udostępniać metadane konfiguracyjne zapisane w polach specjalnych, takie jak REF, TYPE, RETENTION i RETMEMORY,
/// - umożliwiać porównanie kompatybilności dwóch deskryptorów w zakresie pól danych przez operator==.
///
/// @note Descriptor dziedziczy po std::vector<rField>, więc zachowuje się jak kontener pól z dodatkowymi metodami pomocniczymi.
/// @note Operator== nie oznacza ścisłej równości wszystkich właściwości deskryptora; sprawdza zgodność pól danych z pominięciem pól konfiguracyjnych.

class Descriptor : public std::vector<rField> {
  std::vector<std::pair<int, int>> flatToDescriptorIndexMap_;
  std::vector<int> fieldByteOffsets_;
  int flattenedFieldCount_ = 0;
  bool fieldMappingsDirty_{true};
  void rebuildFieldMappings();

  static bool singleLineOutput_;

 public:
  static bool isSingleLineOutput() { return singleLineOutput_; }
  static void setSingleLineOutput(bool enabled) { singleLineOutput_ = enabled; }

  Descriptor(std::initializer_list<rField> fields);
  Descriptor(const std::string &fieldName, int length, int elementCount, rdb::descFld type);

  Descriptor()                         = default;
  Descriptor(const Descriptor &source) = default;

  void append(std::initializer_list<rField> fields);
  Descriptor &operator+=(const Descriptor &rhs);
  Descriptor &operator=(const Descriptor &rhs) = default;
  bool operator==(const Descriptor &rhs) const;
  void composeHashDescriptorFrom(const std::string &fieldNamePrefix, Descriptor lhs, Descriptor rhs);
  void removeConfigurationFields();
  [[nodiscard]] size_t getSizeInBytes() const;
  size_t fieldIndex(std::string_view fieldName);
  int fieldSize(std::string_view fieldName);
  [[nodiscard]] int fieldSize(const rdb::rField &field) const;
  size_t fieldByteOffset(std::string_view fieldName);
  int byteOffsetAtFlatIndex(int flatIndex);
  std::string_view fieldTypeName(std::string_view fieldName);
  int flatElementCount();
  std::vector<rField> dataFields();
  rdb::retention_t retention();
  std::pair<std::string, size_t> storagePolicy();
  std::pair<rdb::descFld, int> widestFieldType();
  std::optional<std::pair<int, int>> flatIndexToDescriptorPosition(int flatIndex);

  [[nodiscard]] bool hasField(const std::string_view fieldName) const {
    return std::any_of(begin(), end(), [fieldName](const auto &f) { return f.rname == fieldName; });
  }

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

// http://www.gotw.ca/gotw/004.htm
Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs);
std::ostream &singleLineFormat(std::ostream &os);
}  // namespace rdb
