#pragma once

#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {

/// @brief Implementacja binarnego źródła danych działającego wyłącznie w trybie odczytu sekwencyjnego.
///
/// Obiekt binaryDeviceRO powinien:
/// - odczytywać kolejne porcje surowych danych binarnych o długości wyznaczonej przez Descriptor,
/// - używać Descriptor wyłącznie do określenia długości rekordu i rozmiaru wektora nullBitset,
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca EXIT_FAILURE,
/// - obsługiwać wyłącznie sekwencyjny odczyt, w którym jedyną poprawną pozycją jest 0,
/// - przy poprawnym odczycie ustawiać nullBitset na same wartości false,
/// - w przypadku błędu, niepoprawnej pozycji lub braku danych zwracać dane wyzerowane i nullBitset ustawiony na same wartości true,
/// - po osiągnięciu końca strumienia próbować wrócić do początku, jeśli włączono loopToBeginningIfEOF,
/// - po osiągnięciu końca strumienia przy wyłączonym loopToBeginningIfEOF zwracać rekord oznaczony jako null,
/// - zliczać wykonane odczyty i zwracać ich liczbę przez count().
///
/// @note Klasa nie interpretuje semantyki pól opisanych w Descriptor; przekazuje jedynie surowe bajty do bufora wyjściowego.
/// @note Mechanizm powrotu do początku zakłada źródło wspierające lseek; dla urządzeń nieseekowalnych zachowanie zależy od systemowego deskryptora.
class binaryDeviceRO : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  Descriptor descriptor_;
  /**
   * @brief Posix File Descriptor
   */
  int fd_;

  size_t cnt_ = 0;

  bool loopToBeginningIfEOF_ = true;
  std::vector<bool> lastNullBitset_;

 public:
  explicit binaryDeviceRO(const std::string_view fileName,    //
                          const rdb::Descriptor &descriptor,  //
                          bool loopToBeginningIfEOF);
  ~binaryDeviceRO() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override {
    return EXIT_FAILURE;
  };

  auto name() -> std::string & override;
  size_t count() override;

  const std::vector<bool> &lastNullBitset() const;
};
}  // namespace rdb
