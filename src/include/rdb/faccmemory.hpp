#pragma once

#include "descriptor.hpp"
#include "fainterface.hpp"

#include <vector>

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
/// - zwracać przez count() logiczną liczbę rekordów, z uwzględnieniem rekordów usuniętych wcześniej z powodu retencji,
/// - opcjonalnie ograniczać liczbę przechowywanych rekordów przy dopisywaniu za pomocą prostego mechanizmu retencji usuwającego najstarsze dane.
///
/// @note Trwałość danych dotyczy wyłącznie czasu życia procesu; klasa nie zapisuje stanu na dysk.
/// @note Stan rekordów i nullBitset jest współdzielony globalnie po nazwie strumienia, natomiast licznik usuniętych rekordów jest utrzymywany w instancji.

struct memoryFile : public FileInterface {
  std::string filename_;
  const ssize_t recordSize_;
  const size_t retentionSize_;  // Retention size for the records, if set to no_retention, no limit is applied
  int removed_count_ = 0;       // Count of removed records, used for retention management
  enum { no_retention = 0 };    // Default retention size if not specified
 public:
  memoryFile(const std::string_view fileName, const Descriptor &descriptor, std::pair<std::string, size_t> retentionSize)
      : filename_(std::string(fileName)),          //
        recordSize_(descriptor.getSizeInBytes()),  //
        retentionSize_(retentionSize.second) {};

  using FileInterface::read;
  using FileInterface::write;
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) override;

  auto name() -> std::string & override;
  size_t count() override;

 private:
  memoryFile()                                    = delete;
  memoryFile(const memoryFile &)                  = delete;
  const memoryFile &operator=(const memoryFile &) = delete;
};

}  // namespace rdb
