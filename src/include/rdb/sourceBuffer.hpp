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

/// @brief Bufor bieżącego rekordu i historii dla źródeł deklarowanych (BINFILE/DEVICE/TEXTSOURCE) - wydzielony z klasy storage.
///
/// Obiekt klasy SourceBuffer powinien:
/// - po attach(descriptor) utrzymywać komorę (chamber) - payload bieżącego rekordu odczytanego ze źródła,
/// - pobierać bieżący rekord i wzorzec null przez null-aware read() akcesora w readCurrent(); jest to
///   jedyne miejsce fizycznego odczytu źródeł plikowych; dla DEVICE (#347) bajty zbiera wcześniej faza
///   DEVICE poza blokadami (rdb::awaitRecords), a read() oddaje gotowy rekord albo all-null,
/// - po EILSEQ z akcesora zwracać Errc::Config z opisem inputError(), pozostawiając payload wyjściowy
///   bez zmian; dla pozostałych statusów przekazywać rekord i nullBitset otrzymane z akcesora,
///   logując ostrzeżenie przy statusie różnym od EXIT_SUCCESS,
///   Test: `SourceBufferTest.input_content_failure_preserves_the_output_and_reports_the_cause`, `SourceBufferTest.read_failure_warning_preserves_the_null_row`.
/// - kopiować komorę do payloadu wyjściowego i do historii przez fire() - jedyne miejsce zasilania bufora
///   historii; pojemność zero jest naruszeniem niezmiennika zgłaszanym przez RDB_ASSERT/rdb::fatal,
///   Test: `SourceBufferDeathTest.fire_without_capacity_is_an_invariant_failure`.
/// - utrzymywać bufor historii o pojemności co najmniej 1 (setCapacity()) - spójność źródeł deklarowanych
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
  /// Errc::Config, gdy zrodlo zglosi blad tresci wejscia (EILSEQ, patrz FileInterface::inputError()).
  /// Kazdy inny blad odczytu daje - jak dotad - wiersz all-null i ostrzezenie w logu.
  [[nodiscard]] Result<> readCurrent(FileInterface &source, payload &out);

  /// @brief Copy the chamber to @p out and push it into history; capacity zero is a broken invariant (RDB_ASSERT).
  void fire(payload &out);

  /// @brief History record at @p index (0 = newest); requires index < size().
  [[nodiscard]] const payload &history(size_t index) const { return circularBuffer_[index]; }

 private:
  std::unique_ptr<payload> chamber_;
  boost::circular_buffer<payload> circularBuffer_{0};
};

}  // namespace rdb
