#pragma once

#include <algorithm>
#include <initializer_list>
#include <iosfwd>
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
/// Obiekt klasy Descriptor powinien:
/// - przechowywać definicje pól rekordu w postaci listy opisowej (rField),
///   Test: `descriptor.print_and_basic_accessors`.
/// - umożliwiać definiowanie pól o różnych typach, długościach i krotnościach tablicowych,
///   Test: `Descriptor.NamedLookupsAreConstAndPreserveArrayLayout`.
/// - udostępniać informacje o logicznej i fizycznej strukturze danych, takie jak nazwy pól, typy, rozmiary i offsety,
///   Test: `Descriptor.NamedLookupsAreConstAndPreserveArrayLayout`.
/// - wyznaczać rozmiar binarnej reprezentacji rekordu z pominięciem pól konfiguracyjnych,
///   Test: `descriptor.print_and_basic_accessors`, `descriptor.clean_ref_and_flat_fields`.
/// - udostępniać odwzorowanie między pozycją w spłaszczonym widoku pól a indeksem pola oraz offsetem w rekordzie,
///   Test: `descriptor.offset_and_convert_for_string_and_arrays`, `descriptor.position_conversion_case_1`, `descriptor.position_conversion_case_2`.
/// - umożliwiać dodawanie i łączenie opisów pól,
///   Test: `descriptor.append_refreshes_cached_layout`, `descriptor.concatenation_refreshes_cached_layout`.
/// - umożliwiać odczyt i zapis tekstowej reprezentacji deskryptora przez operatory strumieniowe,
///   Test: `descriptor.read_from_stream`, `descriptor.print_and_basic_accessors`.
/// - udostępniać metadane konfiguracyjne zapisane w polach specjalnych, takie jak REF, TYPE, RETENTION i RETMEMORY,
///   Test: `descriptor.retention_and_policy_defaults`, `descriptor.retention_and_policy_values`.
/// - umożliwiać porównanie kompatybilności dwóch deskryptorów w zakresie pól danych przez operator==.
///   Test: `descriptor.compare`, `descriptor.compare_ignores_configuration_fields_without_out_of_bounds_access`.
///
/// @note Pola są dostępne tylko do odczytu; zmiany układu przechodzą przez metody unieważniające cache.
/// @note Operator== nie oznacza ścisłej równości wszystkich właściwości deskryptora; sprawdza zgodność pól danych z pominięciem pól konfiguracyjnych.

class Descriptor {
  std::vector<rField> fields_;
  // Cache mapowan pol jest mutable: metody odczytu (getItem w payload dziala na
  // const Descriptorze) musza moc leniwie przebudowac mapowania bez kopiowania
  // calego deskryptora. Przebudowa jest idempotentna; obiekt jest logicznie const.
  mutable std::vector<std::pair<int, int>> flatToDescriptorIndexMap_;
  mutable std::vector<int> fieldByteOffsets_;
  mutable int flattenedFieldCount_ = 0;
  mutable size_t dataSizeBytes_    = 0;
  mutable bool fieldMappingsDirty_{true};
  // Jedyna droga modyfikacji ukladu poza konstrukcja i przeniesieniem/kopia
  // calego deskryptora wraz z jego cache. Uniewaznienie poprzedza mutacje.
  std::vector<rField> &mutableFields() noexcept {
    fieldMappingsDirty_ = true;
    return fields_;
  }
  void rebuildFieldMappings() const;
  /// Cache po oddaniu zawartosci (zrodlo przeniesienia) - wraca do stanu z konstruktora
  /// domyslnego, zeby liczniki nie opisywaly ukladu, ktorego juz tu nie ma.
  void dropFieldMappings() noexcept {
    flatToDescriptorIndexMap_.clear();
    fieldByteOffsets_.clear();
    flattenedFieldCount_ = 0;
    dataSizeBytes_       = 0;
    fieldMappingsDirty_  = true;
  }
  // Zimna sciezka byteOffsetAtFlatIndex (poza TU, uzywa FatalError -> nie wciaga
  // fmt do tego szeroko-includowanego naglowka). Hot-path jest inline nizej.
  [[noreturn]] void flatIndexOutOfRange(int flatIndex) const;

 public:
  [[nodiscard]] size_t size() const noexcept { return fields_.size(); }
  [[nodiscard]] bool empty() const noexcept { return fields_.empty(); }
  [[nodiscard]] const rField &operator[](size_t index) const noexcept { return fields_[index]; }
  [[nodiscard]] std::vector<rField>::const_iterator begin() const noexcept { return fields_.cbegin(); }
  [[nodiscard]] std::vector<rField>::const_iterator end() const noexcept { return fields_.cend(); }
  [[nodiscard]] std::vector<rField>::const_iterator cbegin() const noexcept { return begin(); }
  [[nodiscard]] std::vector<rField>::const_iterator cend() const noexcept { return end(); }

  Descriptor(std::initializer_list<rField> fields);
  Descriptor(const std::string &fieldName, int length, int elementCount, rdb::descFld type);

  Descriptor()                         = default;
  Descriptor(const Descriptor &source) = default;

  // Przenoszenie musi byc zadeklarowane JAWNIE i nie moze byc `= default`.
  //
  // Jawnie, bo deklaracja konstruktora kopiujacego wyzej blokuje niejawne operacje
  // przenoszenia: bez tych dwoch std::move(Descriptor) jest gleboka kopia wektora pol
  // i obu wektorow cache.
  //
  // Nie `= default`, bo wersja domyslna wektory cache PRZENOSI, a liczniki i flage
  // fieldMappingsDirty_ KOPIUJE. Zrodlo zostawaloby z pustym wektorem pol i cache'em
  // udajacym aktualny, wiec byteOffsetAtFlatIndex przechodzilby kontrole zakresu
  // (flattenedFieldCount_ z poprzedniego ukladu) i indeksowal pusty fieldByteOffsets_.
  // Dlatego cel bierze cache razem z polami, a zrodlo wraca do stanu poczatkowego
  // przez dropFieldMappings().
  //
  // Cache jest przenoszony, a nie porzucany, bo to on jest wiekszoscia kosztu kopii:
  // porzucony wymusza odbudowe przy pierwszym dostepie do celu (dwie alokacje plus
  // przejscie po polach), czyli zabiera przenoszeniu prawie caly zysk - zmierzone na
  // 25 polach: 101 ns z odbudowa wobec 113 ns pelnej kopii.
  Descriptor(Descriptor &&other) noexcept
      : fields_(std::move(other.fields_)),
        flatToDescriptorIndexMap_(std::move(other.flatToDescriptorIndexMap_)),
        fieldByteOffsets_(std::move(other.fieldByteOffsets_)),
        flattenedFieldCount_(other.flattenedFieldCount_),
        dataSizeBytes_(other.dataSizeBytes_),
        fieldMappingsDirty_(other.fieldMappingsDirty_) {
    other.dropFieldMappings();
  }
  Descriptor &operator=(Descriptor &&other) noexcept {
    if (this == &other) return *this;
    flatToDescriptorIndexMap_ = std::move(other.flatToDescriptorIndexMap_);
    fieldByteOffsets_         = std::move(other.fieldByteOffsets_);
    flattenedFieldCount_      = other.flattenedFieldCount_;
    dataSizeBytes_            = other.dataSizeBytes_;
    fieldMappingsDirty_       = other.fieldMappingsDirty_;
    other.dropFieldMappings();
    fields_ = std::move(other.fields_);
    return *this;
  }

  void append(std::initializer_list<rField> fields);
  Descriptor &operator+=(const Descriptor &rhs);
  Descriptor &operator=(const Descriptor &rhs) = default;
  bool operator==(const Descriptor &rhs) const;
  void composeHashDescriptorFrom(const std::string &fieldNamePrefix, Descriptor lhs, Descriptor rhs);
  void removeConfigurationFields();
  [[nodiscard]] size_t getSizeInBytes() const;
  size_t fieldIndex(std::string_view fieldName) const;
  int fieldSize(std::string_view fieldName) const;
  [[nodiscard]] int fieldSize(const rdb::rField &field) const;
  size_t fieldByteOffset(std::string_view fieldName) const;
  // Hot-path (P2, speed_improvement): dirty-check inline, zero wywolan cross-TU
  // gdy cache aktualny (getItem/setItem wolaja to per dostep, ~11% instrukcji
  // processRows przed inline). Ciezka przebudowa i zimny blad pozostaja poza TU.
  [[nodiscard]] int byteOffsetAtFlatIndex(int flatIndex) const {
    if (fieldMappingsDirty_) rebuildFieldMappings();
    if (flatIndex < 0 || flatIndex >= flattenedFieldCount_) flatIndexOutOfRange(flatIndex);
    return fieldByteOffsets_[flatIndex];
  }
  std::string_view fieldTypeName(std::string_view fieldName) const;
  [[nodiscard]] int flatElementCount() const {
    if (fieldMappingsDirty_) rebuildFieldMappings();
    return flattenedFieldCount_;
  }
  std::vector<rField> dataFields();
  rdb::retention_t retention();
  std::pair<std::string, size_t> storagePolicy();
  std::pair<rdb::descFld, int> widestFieldType();
  [[nodiscard]] std::optional<std::pair<int, int>> flatIndexToDescriptorPosition(int flatIndex) const {
    if (fieldMappingsDirty_) rebuildFieldMappings();
    if (flatIndex < 0 || flatIndex >= flattenedFieldCount_) return {};
    return flatToDescriptorIndexMap_[flatIndex];
  }

  [[nodiscard]] bool hasField(const std::string_view fieldName) const {
    return std::ranges::any_of(*this, [fieldName](const auto &f) { return f.rname == fieldName; });
  }

  friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
  friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
};

/// Funkcja operator+ powinna:
/// - zwracać nowy deskryptor z polami lhs, a po nich polami rhs, bez zmiany argumentów.
///   Test: `descriptor.concatenation_refreshes_cached_layout`.
///
/// @note Wzorzec: http://www.gotw.ca/gotw/004.htm
Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs);

/// Funkcja singleLineFormat powinna:
/// - jako manipulator włączać format jednowierszowy deskryptora i payloadu wyłącznie dla podanego strumienia i zwracać ten strumień.
///   Test: `descriptor.flat_output_resets_after_stream`.
///
/// @note Stan nalezy do strumienia (slot std::ios_base::xalloc()), nie do procesu: do fazy 2 (#446, C2) byla to
///       flaga calego procesu, o ktora scigaly sie watki. Jednorazowosc zostaje: operator<< zdejmuje flage po
///       wypisaniu, wiec `os << rdb::singleLineFormat << x` dotyczy tego jednego wypisu.
std::ostream &singleLineFormat(std::ostream &os);

/// Funkcja isSingleLineOutput powinna:
/// - zwracać, czy dla podanego strumienia jest włączony format jednowierszowy.
///   Test: `descriptor.single_line_mode_does_not_leak_between_streams`.
[[nodiscard]] bool isSingleLineOutput(std::ostream &os);

/// Funkcja setSingleLineOutput powinna:
/// - włączać albo wyłączać format jednowierszowy wyłącznie dla podanego strumienia.
///   Test: `descriptor.single_line_mode_does_not_leak_between_streams`.
void setSingleLineOutput(std::ostream &os, bool enabled);
}  // namespace rdb
