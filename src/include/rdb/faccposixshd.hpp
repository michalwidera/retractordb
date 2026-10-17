#pragma once

#include <memory>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {

/// @brief Implementacja binarnego magazynu POSIX z dodatkowym plikiem cienia dla operacji update.
///
/// Obiekt klasy posixBinaryFileWithShadow powinien:
/// - [faccposixshd.posix-binary-file-with-shadow.1~1] realizować append do głównego pliku danych oraz update do osobnego pliku cienia,
/// - [faccposixshd.posix-binary-file-with-shadow.2~1] dzięki plikowi cienia zachowywać oryginalną, zarejestrowaną zawartość danych: aktualizacje nie nadpisują
///   rekordów w pliku głównym aż do jawnego wywołania merge(),
/// - [faccposixshd.posix-binary-file-with-shadow.3~1] przechowywać w pliku cienia pary (offset w bajtach, dane) dla operacji update,
/// - [faccposixshd.posix-binary-file-with-shadow.4~1] interpretować position w read() i update jako offset w bajtach; dopisywanie wybiera pozycja
///   std::numeric_limits<size_t>::max(),
/// - [faccposixshd.posix-binary-file-with-shadow.5~1] podczas odczytu zwracać najnowszą wartość rekordu, preferując dane z pliku cienia przed danymi z pliku głównego,
/// - [faccposixshd.posix-binary-file-with-shadow.6~1] przy wielokrotnych aktualizacjach tej samej pozycji traktować ostatni wpis w pliku cienia jako obowiązujący stan rekordu,
/// - [faccposixshd.posix-binary-file-with-shadow.7~1] ignorować parametr nullBitset przy zapisie i czyścić go przy odczycie; obsługa wartości null jest realizowana poza tą klasą,
/// - [faccposixshd.posix-binary-file-with-shadow.8~1] implementować interfejs FileInterface,
/// - [faccposixshd.posix-binary-file-with-shadow.9~1] udostępniać operację merge(), która scala zmiany z pliku cienia do pliku głównego i zeruje plik cienia,
/// - [faccposixshd.posix-binary-file-with-shadow.10~1] w operacji purge wywołanej przez write(nullptr, ..., 0) opróżniać w miejscu zarówno plik główny, jak i plik cienia,
/// - [faccposixshd.posix-binary-file-with-shadow.11~1] przez discard() usuwać oba pliki i rezygnować z rotacji - dla właściciela, który porzuca plik celowo,
/// - [faccposixshd.posix-binary-file-with-shadow.12~1] zwracać przez count() liczbę rekordów wynikającą wyłącznie z rozmiaru pliku głównego,
/// - [faccposixshd.posix-binary-file-with-shadow.13~1] raportować przez hasShadow() posiadanie pliku cienia danych, aby storage dobrał wariant indeksu metadanych,
/// - [faccposixshd.posix-binary-file-with-shadow.14~1] przy tworzeniu obiektu odtwarzać stan z istniejącego pliku głównego i istniejącego pliku cienia, przycinając niepełne końcówki wpisów,
/// - [faccposixshd.posix-binary-file-with-shadow.15~1] zwalniać deskryptory plików przy niszczeniu obiektu i podejmować próbę utrwalenia zapisanych danych przez fsync.
///
/// @note Klasa nie utrzymuje osobnego indeksu zmian; wyszukiwanie aktualizacji w pliku cienia odbywa się przez przeszukiwanie wpisów od końca.
/// @note Zwykłe operacje update nie modyfikują głównego pliku danych; fizyczne scalenie zmian następuje dopiero po wywołaniu merge().
/// @note Update nie sprawdza, czy pozycja mieści się w pliku głównym; kontrola zakresu należy do wywołującego.

class posixBinaryFileWithShadow : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor for main file
   */
  int fd{-1};
  /**
   * @brief Posix File Descriptor for shadow file
   * Shadow file stores (position, data) pairs for update operations.
   * Each shadow entry has size: sizeof(size_t) + recordSize_
   */
  int fd_shadow{-1};
  int percounter_;
  std::string initializationError_;
  int initializationErrno_{0};

  [[nodiscard]] std::string shadowName() const;
  ssize_t shadowFind(uint8_t *ptrData, size_t position) const;

 public:
  /// @param followFinalLink plik danych spod REF wolajacego moze byc dowiazaniem; cien `.shadow` nigdy (#374)
  posixBinaryFileWithShadow(std::string_view fileName, const Descriptor &descriptor, int percounter = -1,
                            bool followFinalLink = false);
  ~posixBinaryFileWithShadow() override;

  // Kopia zamknelaby te same deskryptory dwa razy; akcesor zyje wylacznie w unique_ptr (R-01, #274).
  posixBinaryFileWithShadow(const posixBinaryFileWithShadow &)            = delete;
  posixBinaryFileWithShadow &operator=(const posixBinaryFileWithShadow &) = delete;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;

  auto name() -> std::string & override;
  [[nodiscard]] Result<size_t> count() override;
  [[nodiscard]] const std::string &initializationError() const override { return initializationError_; }
  [[nodiscard]] int initializationErrno() const override { return initializationErrno_; }
  [[nodiscard]] bool hasShadow() const override { return true; }

  /// @brief Scala plik cienia z głównym plikiem i usuwa plik cienia
  ssize_t merge();

  /// @brief Usuwa oba pliki i wylacza rotacje w destruktorze (groupFile: retencja i purge).
  ///        Obiekt nie nadaje sie potem do zapisu.
  void discard();
  void suppressRotation() { percounter_ = -1; }
};
}  // namespace rdb
