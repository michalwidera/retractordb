#pragma once

#include <cstdint>
#include <memory>

#include <boost/circular_buffer.hpp>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "payload.hpp"

namespace rdb {

/// @brief Stan bufora źródła deklarowanego: empty → flux (odczyt fizyczny dozwolony) → armed (rekord pobrany).
enum class sourceState : std::uint8_t { empty, flux, armed };

/// @brief Bufor bieżącego rekordu i historii dla źródeł deklarowanych (DEVICE/TEXTSOURCE) — wydzielony z klasy storage.
///
/// Obiekt klasy SourceBuffer powinien:
/// - po attach(descriptor) utrzymywać komorę (chamber) — payload bieżącego rekordu odczytanego ze źródła,
/// - realizować jedyne miejsce fizycznego odczytu ze źródła deklarowanego (readCurrent()): rekord i jego
///   wzorzec null pochodzą z null-aware read() akcesora — źródła deklarowane przy błędzie same zwracają
///   dane wyzerowane z wzorcem all-null, co jest logowane jako ostrzeżenie,
/// - kopiować komorę do payloadu wyjściowego i do historii przez fire() — jedyne miejsce zasilania bufora
///   historii; pojemność zero kończy się przez FatalError,
/// - utrzymywać bufor historii o pojemności co najmniej 1 (setCapacity()) — spójność źródeł deklarowanych
///   wymaga zachowania przynajmniej bieżącego rekordu,
/// - udostępniać rekordy historii przez history(index) dla index < size(); kontrola zakresu względem
///   pojemności i rozmiaru należy do wywołującego (storage), bo to on decyduje o wartościach zastępczych.
class SourceBuffer {
 public:
  /// @brief Create the chamber payload for the given record layout.
  void attach(const Descriptor &descriptor);

  /// @brief Set history capacity; a declared source must keep at least one record.
  void setCapacity(int capacity);

  [[nodiscard]] size_t capacity() const { return circularBuffer_.capacity(); }
  [[nodiscard]] size_t size() const { return circularBuffer_.size(); }

  /// @brief The only place where data is physically read from a declared source; fills @p out with the record.
  void readCurrent(FileInterface &source, payload &out);

  /// @brief Copy the chamber to @p out and push it into history; FatalError when capacity is zero.
  void fire(payload &out);

  /// @brief History record at @p index (0 = newest); requires index < size().
  [[nodiscard]] const payload &history(size_t index) const { return circularBuffer_[index]; }

 private:
  std::unique_ptr<payload> chamber_;
  boost::circular_buffer<payload> circularBuffer_{0};
};

}  // namespace rdb
