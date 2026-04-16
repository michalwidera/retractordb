#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>
#include <vector>

namespace rdb {

/// @brief Implementacja binarnego magazynu rekordów opartego na deskryptorze pliku POSIX.
///
/// Obiekt posixBinaryFile powinien:
/// - implementować interfejs FileInterface dla binarnego pliku rekordowego,
/// - umożliwiać dopisywanie rekordów oraz zapis i odczyt rekordu od wskazanej pozycji w pliku,
/// - traktować Descriptor jako źródło rozmiaru pojedynczego rekordu,
/// - ignorować parametr nullBitset przy zapisie i czyścić go przy odczycie; klasa nie przechowuje metadanych null,
/// - zwracać przez count() liczbę rekordów wynikającą z rozmiaru pliku danych,
/// - obsługiwać purge przez write(nullptr, ..., 0), usuwając plik danych,
/// - przy tworzeniu obiektu odtwarzać spójny stan istniejącego pliku przez przycięcie niepełnego końca do wielokrotności rozmiaru rekordu,
/// - przy niszczeniu obiektu zamykać deskryptor pliku i podejmować próbę utrwalenia danych przez fsync,
/// - jeśli parametr percounter jest nieujemny, próbować po zamknięciu przemianować plik do postaci `<nazwa>.old<percounter>`.
///
/// @note Klasa nie zapewnia dodatkowej warstwy trwałości ani wersjonowania poza bezpośrednim zapisem do jednego pliku binarnego.

class posixBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd;
  int percounter_;

 public:
  posixBinaryFile(const std::string_view fileName, const Descriptor &descriptor, int percounter = -1);
  ~posixBinaryFile() override;

  using FileInterface::write;
  using FileInterface::read;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
