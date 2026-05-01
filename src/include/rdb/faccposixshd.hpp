#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <memory>

namespace rdb {

/// @brief Implementacja binarnego magazynu POSIX z dodatkowym plikiem cienia dla operacji update.
///
/// Obiekt posixBinaryFileWithShadow powinien:
/// - realizować append do głównego pliku danych oraz update do osobnego pliku cienia,
/// - przechowywać w pliku cienia wyłącznie pary (pozycja, dane) opisujące zmiany istniejących rekordów,
/// - podczas odczytu zwracać najnowszą wartość rekordu, preferując dane z pliku cienia przed danymi z pliku głównego,
/// - przy wielokrotnych aktualizacjach tej samej pozycji traktować ostatni wpis w pliku cienia jako obowiązujący stan rekordu,
/// - ignorować parametr nullBitset przy zapisie i czyścić go przy odczycie; obsługa wartości null jest realizowana poza tą klasą,
/// - implementować interfejs FileInterface,
/// - udostępniać operację merge(), która scala zmiany z pliku cienia do pliku głównego i zeruje plik cienia,
/// - w operacji purge wywołanej przez write(nullptr, ..., 0) usuwać zarówno plik główny, jak i plik cienia,
/// - zwracać przez count() liczbę rekordów wynikającą wyłącznie z rozmiaru pliku głównego,
/// - przy tworzeniu obiektu odtwarzać stan z istniejącego pliku głównego i istniejącego pliku cienia, przycinając niepełne końcówki wpisów,
/// - zwalniać deskryptory plików przy niszczeniu obiektu i podejmować próbę utrwalenia zapisanych danych przez fsync.
///
/// @note Klasa nie utrzymuje osobnego indeksu zmian; wyszukiwanie aktualizacji w pliku cienia odbywa się przez przeszukiwanie wpisów od końca.
/// @note Zwykłe operacje update nie modyfikują głównego pliku danych; fizyczne scalenie zmian następuje dopiero po wywołaniu merge().

class posixBinaryFileWithShadow : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor for main file
   */
  int fd;
  /**
   * @brief Posix File Descriptor for shadow file
   * Shadow file stores (position, data) pairs for update operations.
   * Each shadow entry has size: sizeof(size_t) + recordSize_
   */
  int fd_shadow;
  int percounter_;

  std::string shadowName() const;
  ssize_t shadowFind(uint8_t *ptrData, size_t position);

 public:
  posixBinaryFileWithShadow(const std::string_view fileName, const Descriptor &descriptor, int percounter = -1);
  ~posixBinaryFileWithShadow() override;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;

  auto name() -> std::string & override;
  size_t count() override;

  /// @brief Scala plik cienia z głównym plikiem i usuwa plik cienia
  ssize_t merge();
};
}  // namespace rdb
