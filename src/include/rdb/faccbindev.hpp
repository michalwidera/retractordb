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
/// Obiekt klasy binaryDeviceRO powinien:
/// - odczytywać kolejne porcje surowych danych binarnych o długości wyznaczonej przez Descriptor,
///   Test: `BinaryDeviceROTest.read_exact_record_and_count`.
/// - używać Descriptor wyłącznie do określenia długości rekordu i rozmiaru wektora nullBitset,
///   Test: `BinaryDeviceROTest.binfile_retries_eintr_and_short_reads_without_interpreting_bytes`.
/// - składać rekord z krótkich odczytów i ponawiać wywołanie systemowe przerwane przez EINTR,
///   Test: `BinaryDeviceROTest.binfile_retries_eintr_and_short_reads_without_interpreting_bytes`, `BinaryDeviceROTest.device_fill_retries_eintr_and_short_reads`.
/// - implementować interfejs FileInterface, pozostając źródłem tylko do odczytu; metoda write(...) zawsze zwraca ENOTSUP,
///   Test: `BinaryDeviceROTest.name_and_write_contract`.
/// - obsługiwać wyłącznie sekwencyjny odczyt, w którym jedyną poprawną pozycją jest 0,
///   Test: `BinaryDeviceROTest.binfile_invalid_position_and_read_error_return_counted_zero_null_rows`.
/// - przy poprawnym odczycie ustawiać nullBitset na same wartości false,
///   Test: `BinaryDeviceROTest.read_exact_record_and_count`, `BinaryDeviceROTest.binfile_retries_eintr_and_short_reads_without_interpreting_bytes`.
/// - w przypadku błędu, niepoprawnej pozycji lub braku danych zwracać dane wyzerowane i nullBitset ustawiony na same wartości true,
///   Test: `BinaryDeviceROTest.read_fails_on_missing_file_with_null_metadata`, `BinaryDeviceROTest.binfile_invalid_position_and_read_error_return_counted_zero_null_rows`, `BinaryDeviceROTest.read_zero_fills_on_eof_when_loop_disabled`.
/// - po osiągnięciu końca strumienia próbować wrócić do początku, jeśli włączono loopToBeginningIfEOF,
///   Test: `BinaryDeviceROTest.read_loops_to_beginning_on_eof_when_enabled`.
/// - po osiągnięciu końca strumienia przy wyłączonym loopToBeginningIfEOF zwracać rekord oznaczony jako null,
///   Test: `BinaryDeviceROTest.read_zero_fills_on_eof_when_loop_disabled`.
/// - w tym samym przypadku zgłaszać wyczerpanie wejścia przez exhausted(), bo rekord all-null jest nieodróżnialny od danych,
///   Test: `BinaryDeviceROTest.read_zero_fills_on_eof_when_loop_disabled`, `BinaryDeviceROTest.device_eof_means_no_writer_and_with_oneshot_exhaustion_after_data`.
/// - zliczać wykonane odczyty i zwracać ich liczbę przez count(),
///   Test: `BinaryDeviceROTest.count_starts_at_zero_before_any_read`, `BinaryDeviceROTest.read_exact_record_and_count`, `BinaryDeviceROTest.binfile_invalid_position_and_read_error_return_counted_zero_null_rows`.
/// - obsługiwać dwa typy źródła deklarowanego (#346): BINFILE (plik zwykły) i DEVICE (urządzenie znakowe
///   albo FIFO); ścieżkę złego rodzaju odrzucać przez initializationError() - BINFILE sprawdza stat()
///   przed otwarciem, więc FIFO bez pisarza nie blokuje open(), a oba typy dodatkowo fstat() po otwarciu,
///   Test: `BinaryDeviceROTest.binfile_refuses_a_fifo_without_blocking`, `BinaryDeviceROTest.binfile_refuses_a_character_device_and_device_a_regular_file`. Kontrola strukturalna: sourceKindMismatch przed open i po fstat w faccbindev.cc.
/// - dla DEVICE (#347) nigdy nie czekać: otwierać z O_NONBLOCK, dopełniać rekord próbami nieblokującymi
///   (fill()) w prywatnym buforze, który przeżywa krótki odczyt i timeout, a w read() oddawać rekord
///   skompletowany albo all-null; czekanie z terminem należy do awaitRecords(), poza blokadami modelu.
///   Test: `BinaryDeviceROTest.device_opens_a_fifo_without_a_writer_without_blocking`, `BinaryDeviceROTest.device_keeps_a_short_read_until_the_record_completes`, `BinaryDeviceROTest.device_read_does_not_retry_after_the_device_phase`, `BinaryDeviceROTest.await_partial_record_hits_the_deadline_and_completes_later`.
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
  [[nodiscard]] Result<size_t> count() override;
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
/// @requirements-exempt deviceWait: Parametry oczekiwania na zrodlo DEVICE: wskaznik zrodla i termin.
struct deviceWait {
  binaryDeviceRO *source;
  std::chrono::steady_clock::time_point deadline;
};

/// Funkcja awaitRecords powinna:
/// - w fazie DEVICE jednego slotu wykonywać najpierw jedną próbę nieblokującą dla każdego źródła, a czekać tylko na źródła, którym brakuje danych i których termin jeszcze nie minął.
///   Test: `BinaryDeviceROTest.await_with_zero_timeout_does_not_wait`, `BinaryDeviceROTest.await_returns_when_a_late_writer_completes_the_record`.
/// - czekać na wszystkie oczekujące źródła jednym poll() naraz, tak aby łączne czekanie było maksimum terminów, a nie ich sumą.
///   Test: `BinaryDeviceROTest.await_of_two_sources_takes_the_longer_deadline_not_the_sum`.
/// - nie odnawiać terminu źródła: po EINTR i po fałszywym przebudzeniu wracać do czekania na ten sam termin.
///   Test: `BinaryDeviceROTest.await_deadline_survives_eintr`, `BinaryDeviceROTest.await_deadline_survives_eagain`.
/// - kończyć czekanie źródła od razu po danych, EOF albo błędzie, a po błędzie poll() innym niż EINTR kończyć czekanie wszystkich źródeł w tym slocie z ostrzeżeniem.
///   Test: `BinaryDeviceROTest.await_returns_when_a_late_writer_completes_the_record`, `BinaryDeviceROTest.await_ends_on_eof_without_waiting_for_the_deadline`, `BinaryDeviceROTest.device_read_error_does_not_exhaust_and_next_slot_recovers`, `BinaryDeviceROTest.poll_failure_ends_all_waits_and_clears_every_awaited_mark`.
/// - na końcu zdejmować markAwaited() ze wszystkich podanych źródeł.
///   Test: `BinaryDeviceROTest.device_destroyed_during_the_device_phase_is_fatal`, `BinaryDeviceROTest.poll_failure_ends_all_waits_and_clears_every_awaited_mark`. Kontrola znacznika przez destruktor działa tylko w Debug.
///
/// @note Faza DEVICE (#347). Wola ja watek wykonawczy BEZ blokad modelu; zrodla musza przezyc cale wywolanie
///       (niezmiennik pilnowany w Debug przez destruktor binaryDeviceRO).
void awaitRecords(std::span<const deviceWait> waits);
}  // namespace rdb
