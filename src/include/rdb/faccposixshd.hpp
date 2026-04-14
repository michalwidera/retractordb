#pragma once

#include "fainterface.hpp"

namespace rdb {

/// @brief Definicja klasy implementującej dostęp do pliku binarnego
///
/// Obiekt posixBinaryFileWithShadow powinien:
/// - umożliwiać odczyt i zapis danych do pliku binarnego
/// - zapisywać każdą zarejestrowaną wartość do pliku, aby zapewnić trwałość danych
/// - zapisywać zmiany uprzednio zarejestrowanych danych do pliku cienia (shadow file) zamiast aktualizacji głównego pliku, aby uniemożliwić fizyczną modyfikację uprzednio zarejestrowanych danych.
/// - przechowywać w pliku cienia wyłącznie pary (pozycja, dane) dla operacji update, aby plik cienia był kompaktowy i znacznie mniejszy od głównego pliku danych.
/// - udostępniać dane, jeśli istnieją z pliku cienia jako aktualny stan danych, umożliwiając odczyt i dalsze aktualizacje bez modyfikacji głównego pliku.
/// - w przypadku wielokrotnych aktualizacji tej samej pozycji, zwracać dane z ostatniego zapisu (przeszukiwanie pliku cienia od końca).
/// - implementować interfejs FileInterface, aby umożliwić integrację z innymi komponentami systemu
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu danych
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci
/// - być w stanie obsłużyć duże ilości danych, zapewniając płynne działanie systemu
/// - zapewnić mechanizm integracji pliku cienia z głównym plikiem (merge), umożliwiając scalanie zmian z pliku cienia do głównego pliku na żądanie.
/// - zapewnić mechanizm zarządzania plikiem cienia, umożliwiając usuwanie pliku cienia po scaleniu zmian z głównym plikiem (truncate po merge).
/// - w operacji truncate (write z nullptr i position 0) czyścić zarówno główny plik jak i plik cienia.
/// - zwracać liczbę rekordów wyłącznie na podstawie głównego pliku (count), niezależnie od zawartości pliku cienia.
/// - przed zakończeniem życia obiektu, dane powinny być bezpiecznie zapisane w pliku, a zasoby systemowe powinny być zwolnione
/// - po ponownym utworzeniu obiektu, powinien odtworzyć stan z pliku, jeśli plik już istnieje, aby zapewnić ciągłość danych między uruchomieniami programu

// TODO: Wymaganie 12 mówi o "usuwaniu pliku cienia" po merge, jednak implementacja merge()
//       wywołuje ::ftruncate(fd_shadow, 0), co zeruje plik, ale nie usuwa go z systemu plików.
//       Należy ujednolicić: albo zmienić wymaganie na "zerowanie pliku cienia" (truncate do 0),
//       albo zastąpić ftruncate wywołaniem ::unlink() + ::close() + ponownym ::open() z O_CREAT|O_TRUNC.

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
  posixBinaryFileWithShadow(const std::string_view fileName, const ssize_t recordSize, int percounter = -1);
  ~posixBinaryFileWithShadow() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;

  auto name() -> std::string & override;
  size_t count() override;

  /// @brief Scala plik cienia z głównym plikiem i usuwa plik cienia
  ssize_t merge();
};
}  // namespace rdb
