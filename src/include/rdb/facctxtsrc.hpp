#pragma once

#include <cerrno>
#include <fstream>
#include <memory>
#include <string>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "payload.hpp"

namespace rdb {

/// @brief Implementacja tekstowego źródła danych działającego wyłącznie w trybie odczytu sekwencyjnego.
///
/// Obiekt klasy textSourceRO powinien:
/// - odczytywać kolejne rekordy z pliku tekstowego i interpretować je zgodnie z dostarczonym Descriptor,
///   Test: `TextSourceROTest.test_read_multiple_integers`, `TextSourceROTest.test_read_multi_field`.
/// - obsługiwać pola danych BYTE, INTEGER, UINT, FLOAT, DOUBLE, STRING i NULLTYPE; dla BYTE, INTEGER i UINT
///   czytać dziesiętny zapis całkowity mieszczący się odpowiednio w uint8_t, int i unsigned int, z opcjonalnym
///   znakiem '+' przed cyfrą, a znakiem '-' tylko dla INTEGER; dla FLOAT i DOUBLE czytać przez operator>>
///   std::istringstream zapis dziesiętny lub wykładniczy mieszczący się w float albo double, odrzucając inf i nan;
///   token liczbowy musi być odczytany w całości, a niedomiar FLOAT/DOUBLE może dać zero,
///   Test: `TextSourceROTest.test_read_single_integer`, `TextSourceROTest.test_read_uint`, `TextSourceROTest.test_read_byte`, `TextSourceROTest.test_read_float`, `TextSourceROTest.test_read_double`, `TextSourceROTest.test_read_nulltype_array_keeps_next_field_aligned`, `TextSourceROTest.invalid_numeric_token_reads_as_null`, `TextSourceROTest.signed_numeric_tokens_still_parse`, `TextSourceROTest.floating_point_underflow_is_zero_with_clear_null_bits`.
/// - dla STRING czytać token rozdzielany białymi znakami albo tekst między podwójnymi cudzysłowami bez
///   interpretacji sekwencji escape; zapisywać najwyżej rlen * rarray bajtów i dopełniać krótszy tekst zerami,
///   Test: `TextSourceROTest.test_read_unquoted_string_and_int_stay_aligned`, `TextSourceROTest.test_read_string_unquoted_truncated_to_field`, `TextSourceROTest.quoted_strings_keep_backslashes_and_zero_pad_the_field`.
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca ENOTSUP,
///   Test: `TextSourceROTest.test_write_returns_failure`.
/// - obsługiwać informację o wartościach null przez nullBitset powiązany z wewnętrznym obiektem payload,
///   Test: `TextSourceROTest.test_last_null_bitset_for_mixed_record`, `TextSourceROTest.test_read_integer_array_null_element_nulls_whole_field`.
/// - traktować brak tokenu lub token `null`/`NULL`/`Null` jako wartość null dla odpowiedniego pola,
///   Test: `TextSourceROTest.null_spellings_and_missing_tokens_preserve_the_final_record`.
/// - działać sekwencyjnie; jedyną poprawną pozycją odczytu jest 0, a inna pozycja skutkuje zwróceniem danych wyzerowanych i nullBitset ustawionego na same wartości true,
///   Test: `TextSourceROTest.invalid_position_does_not_consume_input_and_counts_the_null_row`.
/// - po osiągnięciu końca pliku wracać do początku, jeśli włączono loopToBeginningIfEOF,
///   Test: `TextSourceROTest.test_loop_to_beginning`, `TextSourceROTest.test_read_string_loop_keeps_first_record`, `TextSourceROTest.test_read_string_and_int_loop_stays_aligned`.
/// - po osiągnięciu końca pliku przy wyłączonym loopToBeginningIfEOF zwracać kolejne rekordy jako wyzerowane dane z nullBitset ustawionym na same wartości true,
///   Test: `TextSourceROTest.test_no_loop_eof_zeros`, `TextSourceROTest.test_read_string_no_loop_eof_zero_padded`, `TextSourceROTest.null_spellings_and_missing_tokens_preserve_the_final_record`.
/// - w tym samym przypadku zgłaszać wyczerpanie wejścia przez exhausted(), bo rekord all-null jest nieodróżnialny od danych,
///   Test: `TextSourceROTest.null_spellings_and_missing_tokens_preserve_the_final_record`.
/// - przy nieotwartym pliku zwracać EBADF, a przy failbit ustawionym przed parsowaniem albo - przy włączonym
///   loopToBeginningIfEOF - po złożeniu rekordu (plik nie dał rekordu nawet po powrocie na początek, np. pusty) EIO;
///   w obu przypadkach zwracać wyzerowane dane i nullBitset ustawiony na same wartości true,
///   Test: `TextSourceROTest.test_read_missing_file_returns_null_row`, `TextSourceROTest.empty_looped_file_reports_eio_on_every_read`.
/// - dla tokenu innego niż null/NULL/Null w polu NULLTYPE zwracać EILSEQ i opis przez inputError(),
///   bez publikowania częściowo zbudowanego rekordu do ptrData i nullBitset,
///   Test: `TextSourceROTest.nulltype_content_error_preserves_output_and_does_not_count`.
/// - zliczać odczyty zakończone przekazaniem rekordu, także all-null, i zwracać ich liczbę przez count();
///   odczyt przerwany przez EILSEQ nie zwiększa licznika,
///   Test: `TextSourceROTest.test_count_increments`, `TextSourceROTest.nulltype_content_error_preserves_output_and_does_not_count`, `TextSourceROTest.invalid_position_does_not_consume_input_and_counts_the_null_row`.
/// - czytać wyłącznie plik zwykły (#346): ścieżkę innego rodzaju odrzucać przez initializationError(),
///   sprawdzając ją przez stat() przed otwarciem, żeby FIFO bez pisarza nie zablokowało open().
///   Test: `TextSourceROTest.textfile_refuses_a_fifo_without_blocking`, `TextSourceROTest.textfile_refuses_a_character_device`.
/// - traktować token liczbowy niepoprawny (nieodczytany w całości albo poza zakresem typu pola) jako null pola
///   i zgłaszać tylko pierwszy taki token źródła jednym ostrzeżeniem w logu (#471).
///   Test: `TextSourceROTest.invalid_numeric_token_reads_as_null`, `TextSourceROTest.invalid_array_element_nulls_whole_field`, `TextSourceROTest.invalid_token_warned_once_across_wrap`.
///
/// @note Klasa nie obsługuje zapisu i nie zapewnia losowego dostępu do rekordów tekstowych.
class textSourceRO : public FileInterface {
  std::string filename_;
  std::string initializationError_;
  const ssize_t recordSize_;

  Descriptor descriptor_;

  std::unique_ptr<rdb::payload> payload_;

  std::fstream myFile_;

  size_t readCount_ = 0;

  bool loopToBeginningIfEOF_ = true;

  /// Wejście wyczerpane: ustawiane wyłącznie przy wyłączonym zawijaniu, bo przy włączonym
  /// koniec pliku jest tylko powrotem na jego początek, a nie końcem danych.
  bool exhausted_ = false;
  std::string inputError_;  ///< opis bledu tresci wejscia z ostatniego read() zwracajacego EILSEQ

  /// Pierwszy niepoprawny token liczbowy został już zgłoszony - kolejne, także po zawinięciu pliku, nie są.
  bool badTokenReported_ = false;

 public:
  textSourceRO(std::string_view fileName,          //
               const rdb::Descriptor &descriptor,  //
               bool loopToBeginningIfEOF);

  ~textSourceRO() override;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override { return ENOTSUP; }

  auto name() -> std::string & override;
  [[nodiscard]] Result<size_t> count() override;
  [[nodiscard]] bool exhausted() const override { return exhausted_; }
  [[nodiscard]] const std::string &initializationError() const override { return initializationError_; }
  [[nodiscard]] const std::string &inputError() const override { return inputError_; }

  const std::vector<bool> &lastNullBitset() const;
};
}  // namespace rdb
