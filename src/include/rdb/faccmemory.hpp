#pragma once

#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "memoryStore.hpp"

namespace rdb {

/// @brief Implementacja magazynu rekordów przechowywanych wyłącznie w pamięci procesu.
///
/// Obiekt klasy memoryFile powinien:
/// - [faccmemory.memory-file.1~1] implementować interfejs FileInterface dla danych przechowywanych w pamięci,
/// - [faccmemory.memory-file.2~1] umożliwiać zapis, odczyt i nadpisywanie rekordów binarnych o rozmiarze wyznaczonym przez Descriptor,
/// - [faccmemory.memory-file.3~1] przechowywać rekordy, ich nullBitset i licznik zapisów w MemoryStore, indeksowane nazwą strumienia i współdzielone między ŻYWYMI instancjami o tej samej nazwie, KTÓRE DOSTAŁY TEN SAM Memory Store,
/// - [faccmemory.memory-file.4~1] kasować ten stan razem z ostatnią instancją, która go używa - strumień o tej samej nazwie w następnym planie zaczyna od zera,
/// - [faccmemory.memory-file.5~1] obsługiwać append przez position == std::numeric_limits<size_t>::max() oraz update przez wskazaną pozycję,
/// - [faccmemory.memory-file.6~1] traktować write(nullptr, ...) jako polecenie wyczyszczenia pamięci dla danego strumienia,
/// - [faccmemory.memory-file.7~1] zwracać przez count() logiczną liczbę rekordów (łączną liczbę zapisów append),
/// - [faccmemory.memory-file.8~1] przy ustawionej retencji przechowywać najwyżej wskazaną liczbę rekordów, zastępując najstarsze nowymi;
///   bez retencji nie ograniczać liczby rekordów,
/// - [faccmemory.memory-file.9~1] przy ustawionej retencji R odczyt i nadpisanie w miejscu rekordu spoza ostatnich R zapisów (starszego albo
///   jeszcze niezapisanego) odrzucać kodem ERANGE, nie sięgając do slotu zajętego przez inny rekord - tak jak magazyn
///   dyskowy odrzuca rekord z usuniętego segmentu.
///
/// @note Trwałość danych dotyczy wyłącznie czasu życia Memory Store (MemoryStore); klasa nie zapisuje stanu na dysk.
/// @note Stan rekordów, nullBitset i licznik zapisów są współdzielone po nazwie strumienia między żywymi instancjami -
///       w granicach JEDNEGO MemoryStore, nie całego procesu. Bez podanego Memory Store obowiązuje
///       MemoryStore::processDefault(), czyli zachowanie sprzed fazy 2 (#446, C2).

struct memoryFile : public FileInterface {
 private:
  std::string filename_;
  const ssize_t recordSize_;
  /// Rozmiar bufora kołowego; no_retention wyłącza ograniczenie liczby rekordów.
  /// Append wybiera slot writeCount % retentionSize_, a read i update slot location % retentionSize_ - tylko dla
  /// rekordu z okna ostatnich retentionSize_ zapisow.
  const size_t retentionSize_;

 public:
  enum : std::uint8_t { no_retention = 0 };  // Default retention size if not specified

 private:
  /// Węzeł TEGO strumienia w sklepie, wyszukany raz w konstruktorze i ważny do końca życia
  /// instancji. Powód i warunki ważności - patrz konstruktor w faccmemory.cc.
  memoryBucket *bucket_;

  /// Właściciel kubełka - patrz memoryStore.hpp. Destruktor ostatniej instancji kasuje w nim kubełek.
  MemoryStore &store_;

 public:
  memoryFile(std::string_view fileName, const Descriptor &descriptor, const std::pair<std::string, size_t> &retentionSize,
             MemoryStore &store = MemoryStore::processDefault());
  ~memoryFile() override;

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;

  auto name() -> std::string & override;
  [[nodiscard]] Result<size_t> count() override;

  /// Liczba kubełków w domyślnym Memory Store procesu. Tylko do testów jednostkowych - z zewnątrz nie ma innej drogi,
  /// żeby sprawdzić, że wymiana planu nie zostawia kubełków po strumieniach, których już nie ma.
  static size_t bucketCountForUnitTest();

  memoryFile()                                    = delete;
  memoryFile(const memoryFile &)                  = delete;
  const memoryFile &operator=(const memoryFile &) = delete;
};

}  // namespace rdb
