#pragma once

#include <vector>

#include "descriptor.hpp"
#include "fainterface.hpp"

namespace rdb {

/// @brief Implementacja magazynu rekordów przechowywanych wyłącznie w pamięci procesu.
///
/// Obiekt memoryFile powinien:
/// - implementować interfejs FileInterface dla danych przechowywanych w pamięci,
/// - umożliwiać zapis, odczyt i nadpisywanie rekordów binarnych o rozmiarze wyznaczonym przez Descriptor,
/// - przechowywać dane rekordów w globalnej strukturze indeksowanej nazwą strumienia, współdzielonej między instancjami o tej samej nazwie,
/// - przechowywać nullBitset dla rekordów w osobnej globalnej strukturze współdzielonej między instancjami o tej samej nazwie,
/// - obsługiwać append przez position == std::numeric_limits<size_t>::max() oraz update przez wskazaną pozycję,
/// - traktować write(nullptr, ...) jako polecenie wyczyszczenia pamięci dla danego strumienia,
/// - zwracać przez count() logiczną liczbę rekordów (łączną liczbę zapisów append),
/// - opcjonalnie ograniczać liczbę przechowywanych rekordów przy dopisywaniu za pomocą kołowego bufora (ring buffer):
///   zapis nadpisuje najstarszy slot, odczyt używa location % retentionSize jako indeksu w buforze.
///
/// @note Trwałość danych dotyczy wyłącznie czasu życia procesu; klasa nie zapisuje stanu na dysk.
/// @note Stan rekordów, nullBitset i licznik zapisów są współdzielone globalnie po nazwie strumienia między instancjami.

struct memoryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  const size_t retentionSize_;               // Retention size for the records, if set to no_retention, no limit is applied
  enum : std::uint8_t { no_retention = 0 };  // Default retention size if not specified
 public:
  memoryFile(const std::string_view fileName, const Descriptor &descriptor, const std::pair<std::string, size_t> &retentionSize)
      : filename_(std::string(fileName)),                                //
        recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),  //
        retentionSize_(retentionSize.second) {};

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;

  auto name() -> std::string & override;
  size_t count() override;

  memoryFile()                                    = delete;
  memoryFile(const memoryFile &)                  = delete;
  const memoryFile &operator=(const memoryFile &) = delete;
};

}  // namespace rdb
