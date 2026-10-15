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
/// - obsługiwać pola danych BYTE, INTEGER, UINT, FLOAT, DOUBLE, STRING i NULLTYPE; dla BYTE, INTEGER i UINT
///   czytać dziesiętny zapis całkowity mieszczący się odpowiednio w uint8_t, int i unsigned int, z opcjonalnym
///   znakiem '+' przed cyfrą, a znakiem '-' tylko dla INTEGER; dla FLOAT i DOUBLE czytać przez operator>>
///   std::istringstream zapis dziesiętny lub wykładniczy mieszczący się w float albo double, odrzucając inf i nan;
///   token liczbowy musi być odczytany w całości, a niedomiar FLOAT/DOUBLE może dać zero,
/// - dla STRING czytać token rozdzielany białymi znakami albo tekst między podwójnymi cudzysłowami bez
///   interpretacji sekwencji escape; zapisywać najwyżej rlen * rarray bajtów i dopełniać krótszy tekst zerami,
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca ENOTSUP,
/// - obsługiwać informację o wartościach null przez nullBitset powiązany z wewnętrznym obiektem payload,
/// - traktować brak tokenu lub token `null`/`NULL`/`Null` jako wartość null dla odpowiedniego pola,
/// - działać sekwencyjnie; jedyną poprawną pozycją odczytu jest 0, a inna pozycja skutkuje zwróceniem danych wyzerowanych i nullBitset ustawionego na same wartości true,
/// - po osiągnięciu końca pliku wracać do początku, jeśli włączono loopToBeginningIfEOF,
/// - po osiągnięciu końca pliku przy wyłączonym loopToBeginningIfEOF zwracać kolejne rekordy jako wyzerowane dane z nullBitset ustawionym na same wartości true,
/// - w tym samym przypadku zgłaszać wyczerpanie wejścia przez exhausted(), bo rekord all-null jest nieodróżnialny od danych,
/// - przy błędzie otwarcia pliku lub błędzie I/O rozpoznanym przed parsowaniem rekordu zwracać dane
///   wyzerowane z nullBitset ustawionym na same wartości true,
/// - dla tokenu innego niż null/NULL/Null w polu NULLTYPE zwracać EILSEQ i opis przez inputError(),
///   bez publikowania częściowo zbudowanego rekordu do ptrData i nullBitset,
/// - zliczać odczyty zakończone przekazaniem rekordu, także all-null, i zwracać ich liczbę przez count();
///   odczyt przerwany przez EILSEQ nie zwiększa licznika,
/// - czytać wyłącznie plik zwykły (#346): ścieżkę innego rodzaju odrzucać przez initializationError(),
///   sprawdzając ją przez stat() przed otwarciem, żeby FIFO bez pisarza nie zablokowało open().
/// - traktować token liczbowy niepoprawny (nieodczytany w całości albo poza zakresem typu pola) jako null pola
///   i zgłaszać tylko pierwszy taki token źródła jednym ostrzeżeniem w logu (#471).
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
