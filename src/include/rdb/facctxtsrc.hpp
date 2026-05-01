#pragma once

#include <fstream>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "payload.hpp"

namespace rdb {

/// @brief Implementacja tekstowego źródła danych działającego wyłącznie w trybie odczytu sekwencyjnego.
///
/// Obiekt textSourceRO powinien:
/// - odczytywać kolejne rekordy z pliku tekstowego i interpretować je zgodnie z dostarczonym Descriptor,
/// - wspierać pola liczbowe, tekstowe oraz pola NULL zgodnie z zakresem obsługiwanym przez implementację,
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca EXIT_FAILURE,
/// - obsługiwać informację o wartościach null przez nullBitset powiązany z wewnętrznym obiektem payload,
/// - traktować brak tokenu lub token `null`/`NULL`/`Null` jako wartość null dla odpowiedniego pola,
/// - działać sekwencyjnie; jedyną poprawną pozycją odczytu jest 0, a inna pozycja skutkuje zwróceniem danych wyzerowanych i nullBitset ustawionego na same wartości true,
/// - po osiągnięciu końca pliku wracać do początku, jeśli włączono loopToBeginningIfEOF,
/// - po osiągnięciu końca pliku przy wyłączonym loopToBeginningIfEOF zwracać kolejne rekordy jako wyzerowane dane z nullBitset ustawionym na same wartości true,
/// - w przypadku błędu otwarcia pliku lub błędu odczytu zwracać dane wyzerowane z nullBitset ustawionym na same wartości true,
/// - zliczać wykonane odczyty i zwracać ich liczbę przez count().
///
/// @note Klasa nie obsługuje zapisu i nie zapewnia losowego dostępu do rekordów tekstowych.
/// @note Interpretacja błędnych tokenów numerycznych zależy od bieżącej implementacji parsera i nie stanowi pełnej walidacji wejścia.
class textSourceRO : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;

  Descriptor descriptor_;

  std::unique_ptr<rdb::payload> payload_;

  std::fstream myFile_;

  size_t readCount_ = 0;

  bool loopToBeginningIfEOF_ = true;

 public:
  textSourceRO(const std::string_view fileName,    //
               const rdb::Descriptor &descriptor,  //
               bool loopToBeginningIfEOF);

  ~textSourceRO() override;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override {
    return EXIT_FAILURE;
  }

  auto name() -> std::string & override;
  size_t count() override;

  const std::vector<bool> &lastNullBitset() const;
};
}  // namespace rdb
