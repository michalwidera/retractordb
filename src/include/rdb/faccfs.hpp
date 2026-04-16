#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>

namespace rdb {
/**
 * @brief Implementacja prostego magazynu binarnego opartego na `std::fstream`.
 *
 * Obiekt `genericBinaryFile` powinien:
 * - realizować odczyt i zapis rekordów binarnych o długości wyznaczonej przez `Descriptor`,
 * - implementować interfejs `FileInterface`,
 * - obsługiwać dopisywanie danych przy `position == std::numeric_limits<size_t>::max()`,
 * - obsługiwać zapis i odczyt od wskazanej pozycji w pliku dla operacji update/read,
 * - ignorować parametr `nullBitset` przy zapisie i czyścić go przy odczycie,
 * - zwracać przez `count()` liczbę rekordów wynikającą z rozmiaru pliku,
 * - udostępniać niepolimorficzne przeciążenia bez parametru `nullBitset` przez `using FileInterface::write; using FileInterface::read;`,
 * - pełnić rolę wewnętrznej implementacji dostępu do danych binarnych, a nie publicznego typu wysokiego poziomu.
 *
 * @note Klasa nie utrzymuje osobnych metadanych null ani dodatkowego mechanizmu trwałości poza samym zapisem do pliku.
 * @note Obsługa purge przez `write(nullptr, ..., 0)` nie stanowi tu pełnego, ogólnego kontraktu i zależy od bieżącej implementacji.
 */
struct genericBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  int percounter_;

 public:
  genericBinaryFile(const std::string_view fileName, const Descriptor &descriptor, int percounter = -1);
  ~genericBinaryFile() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;

  auto name() -> std::string & override;
  size_t count() override;

 private:
  genericBinaryFile()                                           = delete;
  genericBinaryFile(const genericBinaryFile &)                  = delete;
  const genericBinaryFile &operator=(const genericBinaryFile &) = delete;
};

}  // namespace rdb
