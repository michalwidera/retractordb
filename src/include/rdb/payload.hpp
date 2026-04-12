#pragma once

#include <any>
#include <memory>  // std::unique_ptr
#include <optional>
#include <span>
#include <vector>

#include "descriptor.hpp"

/// @brief Definicja klasy zarządzającej obszarem pamęci, który jest interpretowany zgodnie z opisem w Descriptorze.
///
/// Obiekt klasy payload powinien:
/// - zarządzać obszarem pamięci, który jest interpretowany zgodnie z opisem w Descriptorze.
/// - umożliwiać ustawianie i pobieranie wartości w obszarze pamięci zgodnie z opisem w Descriptorze.
/// - obsługiwać różne typy danych, takie jak liczby całkowite, zmiennoprzecinkowe, łańcuchy znaków itp., zgodnie z opisem w Descriptorze.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - obsługiwać formatowanie danych w formacie szesnastkowym (hex) dla typów numerycznych.
/// - udostępniać metody do ustawiania i pobierania wartości w obszarze pamięci, które są zgodne z opisem w Descriptorze.
/// - dostarczać metody do kopiowania danych między obiektami payload.
/// - uwzględniać obsługę wartości null dla pól, które mogą być przechowywać wartości null.
namespace rdb {
/// @brief This class define accessing method to payload (memory area)
/// This is accessor for payload memory area that supports applying descriptor type over memory area.
class payload {
  /// @brief Payload memory area
  std::unique_ptr<uint8_t[]> payloadData_;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat_ = false;

  template <typename T>
  void setItemBy(const int position, std::optional<std::any> value);

  payload &operator=(const Descriptor &other);

  std::vector<bool> nullBitset_;  // true if field at position i is null, false otherwise
 public:
  /// @brief Descriptor of managed payload area
  Descriptor descriptor;

  /// @brief Span accessor to payload (modern, bounds-aware)
  /// @return  std::span over the payload memory
  std::span<uint8_t> span() const;

  /// @brief Constructor of payload object
  /// @param descriptor descriptor of payload area
  explicit payload(const Descriptor &descriptor);

  /// @brief Copy constructor
  /// @param from destructor
  payload(const payload &other);

  /// @brief Default constructor is allowed - creates uninitialized object, ready to assign
  payload() = default;

  /// @brief Direct setter
  /// @param position position according to descriptor
  /// @param valueParam value of given type according to descriptor that will be set
  /// in payload
  void setItem(const int position, std::optional<std::any> valueParam);

  /// @brief Direct getter
  /// @param position position according to descriptor
  /// @return address of beginning memory that contains data described by
  /// descriptor
  std::optional<std::any> getItem(const int position);

  /// @brief Set format input/output formater - default false
  /// @param hexFormat true if out/in in hex
  void setHex(bool hexFormat);

  /// @brief Expose null metadata for external persistence (metaDataStream)
  const std::vector<bool> &getNullBitset() const;

  /// @brief Restore null metadata read from metaDataStream/text source
  void setNullBitset(const std::vector<bool> &nullBitset);

  friend std::istream &operator>>(std::istream &is, const payload &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payload &rhs);

  payload &operator=(const payload &other);
  payload operator+(const payload &other);
};
}  // namespace rdb
