#pragma once

#include <any>
#include <memory>  // std::unique_ptr
#include <span>

#include "descriptor.h"

namespace rdb {
/// @brief Definicja klasy implementującej dostęp do binarnego obrazu danych transportowanych w systemie.
///
/// Obiekt payload powinien:
/// - być tworzony na podstawie obiektu descriptor, który definiuje strukturę danych w payload.
/// - udostępniać metodę span() zwracającą std::span<uint8_t> do obszaru pamięci, który jest zarządzany przez payload.
/// - przechowywać dane w formacie binarnym, zgodnie z opisem w descriptorze.
/// - udostępniać metodę setItem(int position, std::any value) do ustawiania wartości w obszarze pamięci zgodnie z pozycją i typem określonymi w descriptor.
/// - udostępniać metodę getItem(int position) do pobierania wartości z obszaru pamięci zgodnie z pozycją i typem określonymi w descriptor.
/// - obsługiwać formatowanie danych w formacie szesnastkowym (hex).
/// - być kopiowalny i przypisywalny.
/// - być strumieniowalny do std::ostream i std::istream.
class payload {
  /// @brief Payload memory area
  std::unique_ptr<uint8_t[]> payloadData_;

  /// @brief Type of dumped or read numeric formats
  bool hexFormat_ = false;

  template <typename T>
  void setItemBy(const int position, std::any value);

  payload &operator=(const Descriptor &other);

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
  void setItem(const int position, std::any valueParam);

  /// @brief Direct getter
  /// @param position position according to descriptor
  /// @return address of beginning memory that contains data described by
  /// descriptor
  std::any getItem(const int position);

  /// @brief Set format input/output formater - default false
  /// @param hexFormat true if out/in in hex
  void setHex(bool hexFormat);

  friend std::istream &operator>>(std::istream &is, const payload &rhs);
  friend std::ostream &operator<<(std::ostream &os, const payload &rhs);

  payload &operator=(const payload &other);
  payload operator+(const payload &other);
};
}  // namespace rdb
