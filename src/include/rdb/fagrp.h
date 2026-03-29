#pragma once

#include <memory>
#include <vector>

#include "faccposixshd.h"
#include "retention.h"

/// @brief klasa groupFile tworzy pliki z retencją
///
/// obiekty klasy groupFile powinny:
/// - tworzyć segmenty plików o określonej pojemności (retention_.capacity).
/// - przechowywać określoną liczbę segmentów (retention_.segments).
/// - usuwać najstarszy segment (i jego plik cienia) po przekroczeniu liczby segmentów.
/// - obsługiwać odczyt i zapis danych z/do odpowiednich segmentów na podstawie pozycji.
/// - umożliwiać tryb bez retencji, gdzie wszystkie dane są zapisywane do pojedynczego pliku.
/// - dostarczać usługi zgodne z interfejsem FileInterface.
/// - zapewniać, że operacje odczytu i zapisu są poprawnie kierowane do segmentów zgodnie z ustawieniami retencji.
/// - zarządzać stanem segmentów i ich rotacją w sposób spójny z polityką retencji.
/// - umożliwiać odczyt liczby zapisanych rekordów, uwzględniając usunięte segmenty.
/// - umożliwiać zapis danych w trybie aktualizacji (update-in-place) w ramach segmentu, jeśli pozycja jest określona.  
/// - zapewniać, że nazwa zwracana przez name() jest zgodna z aktualnym segmentem, jeśli retencja jest włączona, lub podstawową nazwą pliku, jeśli retencja jest wyłączona.
/// - w przypadku żądania zapisu danych z nullptr i position 0, usuwać wszystkie segmenty i ich pliki cienia, resetując stan grupy do początkowego.
namespace rdb {
class groupFile : public FileInterface {
  std::string filename_;
  std::string currentFilename_;
  const ssize_t recordSize_;

  retention_t retention_{0, 0};

  std::vector<std::unique_ptr<posixBinaryFileWithShadow>> vec_;

  size_t writeCount_     = 0;
  size_t currentSegment_ = 0;

  size_t removedSegments_ = 0;

  int percounter_;

 public:
  groupFile(const std::string_view fileName, const ssize_t recordSize, const retention_t &retention, int percounter);
  ~groupFile() override;

  ssize_t read(uint8_t *ptrData, const size_t position) override;
  ssize_t write(const uint8_t *ptrData, const size_t position = std::numeric_limits<size_t>::max()) override;
  ssize_t purge();

  auto name() -> std::string & override;
  size_t count() override;
};
}  // namespace rdb
