#pragma once

#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"
#include "memoryStore.hpp"

namespace rdb {

/// @brief Implementacja magazynu rekordów przechowywanych wyłącznie w pamięci procesu.
///
/// Obiekt klasy memoryFile powinien:
/// - implementować interfejs FileInterface dla danych przechowywanych w pamięci,
/// - umożliwiać zapis, odczyt i nadpisywanie rekordów binarnych o rozmiarze wyznaczonym przez Descriptor,
/// - przechowywać rekordy, ich nullBitset i licznik zapisów w MemoryStore, indeksowane nazwą strumienia i współdzielone między ŻYWYMI instancjami o tej samej nazwie, KTÓRE DOSTAŁY TEN SAM Memory Store,
/// - kasować ten stan razem z ostatnią instancją, która go używa - strumień o tej samej nazwie w następnym planie zaczyna od zera,
/// - obsługiwać append przez position == std::numeric_limits<size_t>::max() oraz update przez wskazaną pozycję,
/// - traktować write(nullptr, ...) jako polecenie wyczyszczenia pamięci dla danego strumienia,
/// - zwracać przez count() logiczną liczbę rekordów (łączną liczbę zapisów append),
/// - opcjonalnie ograniczać liczbę przechowywanych rekordów przy dopisywaniu za pomocą kołowego bufora (ring buffer):
///   zapis nadpisuje najstarszy slot, odczyt używa location % retentionSize jako indeksu w buforze.
///
/// @note Trwałość danych dotyczy wyłącznie czasu życia Memory Store (MemoryStore); klasa nie zapisuje stanu na dysk.
/// @note Stan rekordów, nullBitset i licznik zapisów są współdzielone po nazwie strumienia między żywymi instancjami -
///       w granicach JEDNEGO MemoryStore, nie całego procesu. Bez podanego Memory Store obowiązuje
///       MemoryStore::processDefault(), czyli zachowanie sprzed fazy 2 (#446, C2).

struct memoryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  const size_t retentionSize_;               // Retention size for the records, if set to no_retention, no limit is applied
  enum : std::uint8_t { no_retention = 0 };  // Default retention size if not specified

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
