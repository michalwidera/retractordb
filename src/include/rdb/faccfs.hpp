#pragma once

#include <memory>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {
/**
 * @brief Implementacja prostego magazynu binarnego otwierającego plik osobno przy każdej operacji.
 *
 * Obiekt klasy genericBinaryFile powinien:
 * - realizować odczyt i zapis rekordów binarnych o długości wyznaczonej przez `Descriptor`,
 *   Test: `FaccfsTest.write_and_read_single_record`, `FaccfsTest.single_byte_record_size`, `FaccfsTest.large_record_size`.
 * - implementować interfejs `FileInterface`,
 *   Test: `AccessorFactoryTest.maps_type_to_accessor_class`.
 * - obsługiwać dopisywanie danych przy `position == std::numeric_limits<size_t>::max()`,
 *   Test: `FaccfsTest.append_multiple_and_read_back`.
 * - obsługiwać zapis i odczyt od wskazanej pozycji w pliku dla operacji update/read,
 *   Test: `FaccfsTest.update_in_place_modifies_correct_record`.
 * - ignorować parametr `nullBitset` przy zapisie i czyścić go przy odczycie,
 *   Test: `FaccfsTest.read_clears_null_bitset`, `FaccfsTest.write_with_null_bitset_append`, `FaccfsTest.write_with_null_bitset_update`.
 * - zwracać przez `count()` liczbę rekordów wynikającą z rozmiaru pliku,
 *   Test: `FaccfsTest.count_returns_correct_number_of_records`, `FaccfsTest.count_after_update_does_not_change`.
 * - udostępniać niepolimorficzne przeciążenia bez parametru `nullBitset` przez `using FileInterface::write; using FileInterface::read;`.
 *   Test: `FaccfsTest.write_and_read_single_record`.
 *
 * @note Klasa nie utrzymuje osobnych metadanych null ani dodatkowego mechanizmu trwałości poza samym zapisem do pliku.
 * @note Purge przez `write(nullptr, ..., 0)` opróżnia plik (plik pozostaje, ma 0 bajtów).
 * @note Do #374 klasa używała `std::fstream`, który nie przyjmuje O_NOFOLLOW; plik otwiera teraz
 *       openStorageFile() (storageFile.hpp), a dowiązanie pod nazwą pliku daje EIO.
 */
struct genericBinaryFile : public FileInterface {
 private:
  std::string filename_;
  const ssize_t recordSize_;
  int percounter_;
  bool followFinalLink_;

 public:
  /// @param followFinalLink plik danych spod REF wolajacego moze byc dowiazaniem (#374, storageFile.hpp)
  genericBinaryFile(std::string_view fileName, const Descriptor &descriptor, int percounter = -1, bool followFinalLink = false);
  ~genericBinaryFile() override;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;

  auto name() -> std::string & override;
  [[nodiscard]] Result<size_t> count() override;

  genericBinaryFile()                                           = delete;
  genericBinaryFile(const genericBinaryFile &)                  = delete;
  const genericBinaryFile &operator=(const genericBinaryFile &) = delete;
};

}  // namespace rdb
