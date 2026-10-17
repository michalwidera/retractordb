#pragma once

#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {

/// @brief Implementacja binarnego magazynu rekordów opartego na deskryptorze pliku POSIX.
///
/// Obiekt klasy posixBinaryFile powinien:
/// - [faccposix.posix-binary-file.1~1] implementować interfejs FileInterface dla binarnego pliku rekordowego,
/// - [faccposix.posix-binary-file.2~1] umożliwiać dopisywanie rekordów oraz zapis i odczyt rekordu od wskazanej pozycji w pliku,
/// - [faccposix.posix-binary-file.3~1] traktować Descriptor jako źródło rozmiaru pojedynczego rekordu,
/// - [faccposix.posix-binary-file.4~1] ignorować parametr nullBitset przy zapisie i czyścić go przy odczycie; klasa nie przechowuje metadanych null,
/// - [faccposix.posix-binary-file.5~1] zwracać przez count() liczbę rekordów wynikającą z rozmiaru pliku danych,
/// - [faccposix.posix-binary-file.6~1] obsługiwać purge przez write(nullptr, ..., 0), opróżniając plik danych w miejscu (deskryptor pozostaje ważny),
/// - [faccposix.posix-binary-file.7~1] przez discard() usuwać plik i rezygnować z rotacji - dla właściciela, który porzuca plik celowo,
/// - [faccposix.posix-binary-file.8~1] przy tworzeniu obiektu odtwarzać spójny stan istniejącego pliku przez przycięcie niepełnego końca do wielokrotności rozmiaru rekordu,
/// - [faccposix.posix-binary-file.9~1] przy niszczeniu obiektu zamykać deskryptor pliku i podejmować próbę utrwalenia danych przez fsync,
/// - [faccposix.posix-binary-file.10~1] jeśli parametr percounter jest nieujemny, próbować po zamknięciu przemianować plik do postaci `<nazwa>.old<percounter>`.
///
/// @note Klasa nie zapewnia dodatkowej warstwy trwałości ani wersjonowania poza bezpośrednim zapisem do jednego pliku binarnego.

class posixBinaryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  /**
   * @brief Posix File Descriptor
   */
  int fd{-1};
  int percounter_;
  std::string initializationError_;
  int initializationErrno_{0};

 public:
  /// @param followFinalLink plik danych spod REF wolajacego moze byc dowiazaniem (#374, storageFile.hpp)
  posixBinaryFile(std::string_view fileName, const Descriptor &descriptor, int percounter = -1, bool followFinalLink = false);
  ~posixBinaryFile() override;

  // Kopia zamknelaby ten sam deskryptor dwa razy; akcesor zyje wylacznie w unique_ptr (R-01, #274).
  posixBinaryFile(const posixBinaryFile &)            = delete;
  posixBinaryFile &operator=(const posixBinaryFile &) = delete;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;

  auto name() -> std::string & override;
  [[nodiscard]] Result<size_t> count() override;
  [[nodiscard]] const std::string &initializationError() const override { return initializationError_; }
  [[nodiscard]] int initializationErrno() const override { return initializationErrno_; }

  /// @brief Usuwa plik i wylacza rotacje w destruktorze (groupFile: retencja i purge).
  ///        Obiekt nie nadaje sie potem do zapisu.
  void discard();
  void suppressRotation() { percounter_ = -1; }
};
}  // namespace rdb
