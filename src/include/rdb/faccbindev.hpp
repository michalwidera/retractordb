#pragma once

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {

/// @brief Implementacja binarnego źródła danych działającego wyłącznie w trybie odczytu sekwencyjnego.
///
/// Obiekt binaryDeviceRO powinien:
/// - odczytywać kolejne porcje surowych danych binarnych o długości wyznaczonej przez Descriptor,
/// - używać Descriptor wyłącznie do określenia długości rekordu i rozmiaru wektora nullBitset,
/// - składać rekord z krótkich odczytów i ponawiać wywołanie systemowe przerwane przez EINTR,
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca ENOTSUP,
/// - obsługiwać wyłącznie sekwencyjny odczyt, w którym jedyną poprawną pozycją jest 0,
/// - przy poprawnym odczycie ustawiać nullBitset na same wartości false,
/// - w przypadku błędu, niepoprawnej pozycji lub braku danych zwracać dane wyzerowane i nullBitset ustawiony na same wartości true,
/// - po osiągnięciu końca strumienia próbować wrócić do początku, jeśli włączono loopToBeginningIfEOF,
/// - po osiągnięciu końca strumienia przy wyłączonym loopToBeginningIfEOF zwracać rekord oznaczony jako null,
/// - w tym samym przypadku zgłaszać wyczerpanie wejścia przez exhausted(), bo rekord all-null jest nieodróżnialny od danych,
/// - zliczać wykonane odczyty i zwracać ich liczbę przez count(),
/// - obsługiwać dwa typy źródła deklarowanego (#346): BINFILE (plik zwykły) i DEVICE (urządzenie znakowe
///   albo FIFO); ścieżkę złego rodzaju odrzucać przez initializationError() - BINFILE sprawdza stat()
///   przed otwarciem, więc FIFO bez pisarza nie blokuje open(), a oba typy dodatkowo fstat() po otwarciu,
/// - dla DEVICE (#347) nigdy nie czekać: otwierać z O_NONBLOCK, dopełniać rekord próbami nieblokującymi
///   (fill()) w prywatnym buforze, który przeżywa krótki odczyt i timeout, a w read() oddawać rekord
///   skompletowany albo all-null; czekanie z terminem należy do awaitRecords(), poza blokadami modelu.
///
/// EOF dla DEVICE to `read() == 0` i nic innego - POLLHUP służy wyłącznie budzeniu, co znosi różnicę
/// Linux/Darwin w poll() na FIFO bez pisarza. Bez ONESHOT EOF znaczy "w tej chwili nie ma pisarza":
/// takt dostaje all-null, źródło zostaje otwarte, a ponowne podłączenie pisarza wznawia dane. Z ONESHOT
/// wyczerpaniem jest pierwszy EOF PO otrzymaniu co najmniej jednego bajtu - EOF przed pierwszymi danymi
/// to pisarz, który jeszcze się nie podłączył, inaczej start na FIFO byłby wyścigiem z pisarzem. Niepełny
/// rekord w chwili EOF jest odrzucany: granica rekordu zginęła razem z pisarzem, a następny pisarz
/// zaczyna od nowego rekordu. Błąd read() inny niż EAGAIN/EINTR daje all-null bez wyczerpania.
///
/// @note Klasa nie interpretuje semantyki pól opisanych w Descriptor; przekazuje jedynie surowe bajty do bufora wyjściowego.
/// @note Powrót do początku (lseek) dotyczy wyłącznie BINFILE; DEVICE nigdy nie przewija.
/// @note O_NONBLOCK i poll() nie chronią przed sterownikiem, który blokuje wewnątrz read() mimo flagi;
///       takie urządzenie wymaga izolacji (#348).
class binaryDeviceRO : public FileInterface {
  enum class readOutcome : std::uint8_t { complete, endOfFile, error };

 public:
  /// Wynik jednej próby nieblokującej dopełnienia rekordu DEVICE.
  enum class fillResult : std::uint8_t { complete, wouldBlock, endOfFile, error };

 private:
  /// Stan łącza DEVICE - ostrzeżenie pada przy zmianie stanu, nie w każdym takcie.
  enum class linkState : std::uint8_t { unknown, data, noWriter, failed };

  std::string filename_;
  std::string storageType_;
  std::string initializationError_;
  const ssize_t recordSize_;
  Descriptor descriptor_;
  /**
   * @brief Posix File Descriptor
   */
  int fd_ = -1;

  size_t cnt_ = 0;

  bool loopToBeginningIfEOF_ = true;
  std::vector<bool> lastNullBitset_;

  /// Wejście wyczerpane: ustawiane wyłącznie przy wyłączonym zawijaniu, bo przy włączonym
  /// koniec strumienia jest tylko powrotem na jego początek, a nie końcem danych.
  bool exhausted_ = false;

  /// Źródło żywe (#347): odczyt nieblokujący, bez przewijania.
  const bool isDevice_;
  /// Prywatny bufor rekordu DEVICE i liczba bajtów już w nim zebranych.
  std::vector<uint8_t> pending_;
  ssize_t filled_ = 0;
  /// Rekord kompletny, czeka na read().
  bool complete_ = false;
  /// Faza DEVICE próbowała już w tym slocie - read() pod blokadą nie robi wtedy żadnego syscalla.
  bool attempted_ = false;
  /// Choć jeden bajt od otwarcia - warunek wyczerpania z ONESHOT.
  bool sawData_   = false;
  linkState link_ = linkState::unknown;
  /// Źródło należy do migawki trwającej fazy DEVICE (awaitRecords). Atomowe, bo niezmiennik
  /// sprawdza destruktor, który - gdyby niezmiennik złamać - biegłby w innym wątku.
  std::atomic<bool> awaited_{false};

  /// @brief Wypełnia cały rekord, sklejając krótkie odczyty i ponawiając wywołanie przerwane przez EINTR.
  readOutcome readExact(uint8_t *ptrData) const;

  void noteLink(linkState state, int error = 0);

 public:
  explicit binaryDeviceRO(std::string_view fileName,          //
                          const rdb::Descriptor &descriptor,  //
                          bool loopToBeginningIfEOF,          //
                          std::string_view storageType);
  ~binaryDeviceRO() override;

  // Kopia zamknelaby ten sam deskryptor dwa razy; akcesor zyje wylacznie w unique_ptr (R-01, #274).
  binaryDeviceRO(const binaryDeviceRO &)            = delete;
  binaryDeviceRO &operator=(const binaryDeviceRO &) = delete;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override { return ENOTSUP; };

  auto name() -> std::string & override;
  size_t count() override;
  [[nodiscard]] bool exhausted() const override { return exhausted_; }
  [[nodiscard]] const std::string &initializationError() const override { return initializationError_; }

  [[nodiscard]] const std::vector<bool> &lastNullBitset() const;

  /// @brief Jedna próba nieblokująca dopełnienia rekordu DEVICE; czyta najwyżej do granicy rekordu.
  ///
  /// Kompletny rekord zostaje w prywatnym buforze do najbliższego read(). Zaznacza, że faza DEVICE
  /// próbowała w tym slocie, więc read() nie powtórzy wywołania systemowego pod blokadą modelu.
  fillResult fill();

  /// Deskryptor do poll() w awaitRecords(); ujemny, gdy otwarcie się nie udało.
  [[nodiscard]] int pollDescriptor() const { return fd_; }

  /// Oznaczenie źródła jako części migawki fazy DEVICE (patrz awaitRecords()).
  void markAwaited(bool value) { awaited_.store(value, std::memory_order_relaxed); }
};

/// Jedno źródło fazy DEVICE i jego termin, liczony od początku należnego slotu.
struct deviceWait {
  binaryDeviceRO *source;
  std::chrono::steady_clock::time_point deadline;
};

/// @brief Faza DEVICE jednego slotu (#347): najpierw jedna próba nieblokująca dla każdego źródła,
/// potem jedno poll() na wszystkie, które czekają, aż do ich terminów.
///
/// Łączne czekanie to maksimum terminów, nie ich suma. Termin się nie odnawia: EINTR z poll() i EAGAIN
/// po fałszywym przebudzeniu wracają do czekania na ten sam deadline. EOF i błąd kończą czekanie danego
/// źródła od razu. Woła ją wątek wykonawczy BEZ blokad modelu; źródła muszą przeżyć całe wywołanie
/// (niezmiennik pilnowany w Debug przez destruktor binaryDeviceRO). Na końcu zdejmuje markAwaited().
void awaitRecords(std::span<const deviceWait> waits);
}  // namespace rdb
