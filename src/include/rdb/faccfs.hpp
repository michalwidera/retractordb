#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>

namespace rdb {
/**
 * @brief Definicja klasy implementującej dostęp do pliku binarnego poprzez std::fstream
 *
 * Object genericBinaryFile powinien:
 * - umożliwiać odczyt i zapis danych binarnych poprzez interfejs std::fstream (GENERIC type)
 * - zapisywać każdą zarejestrowaną wartość do pliku, aby zapewnić trwałość danych
 * - implementować null-aware interfejs FileInterface: podstawowe metody wirtualne to `write(data, nullBitset, position)` i `read(data, nullBitset, position)`
 * - udostępniać niepolimorficzne przeciążenia bez parametru nullBitset via `using FileInterface::write; using FileInterface::read;`
 * - nie być używany bezpośrednio przez użytkownika rdb; jest typem wewnętrznym zapewniającym dostęp do pól binarnych
 *
 * Type: GENERIC
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
