#pragma once

#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "faccposix.hpp"
#include "faccposixshd.hpp"
#include "retention.hpp"

/// @brief Klasa groupFile udostępnia magazyn rekordów podzielony na segmenty zgodnie z polityką retencji.
///
/// Obiekt klasy groupFile powinien:
/// - tworzyć i utrzymywać segmenty o pojemności określonej przez retention_.capacity,
/// - ograniczać liczbę utrzymywanych segmentów do retention_.segments i usuwać najstarszy segment po przekroczeniu limitu,
/// - umożliwiać tryb bez retencji, w którym wszystkie rekordy są przechowywane w pojedynczym pliku,
/// - realizować operacje read i write zgodnie z interfejsem FileInterface, kierując je do właściwego segmentu,
/// - traktować parametr position jako logiczną pozycję rekordu w groupFile; pozycja jest mapowana na segment i offset w segmencie,
/// - propagować parametr nullBitset do segmentu T bez własnego przechowywania informacji o wartościach null,
/// - obsługiwać dopisywanie rekordów oraz aktualizację istniejącego rekordu w odpowiednim segmencie,
/// - zwracać przez count() logiczną liczbę rekordów uwzględniającą także rekordy znajdujące się wcześniej w usuniętych segmentach,
/// - zwracać przez name() nazwę bieżącego segmentu, gdy retencja jest aktywna, albo nazwę bazową pliku w trybie bez retencji,
/// - raportować przez hasShadow() mechanizm cienia swoich segmentów T (grupa nie dodaje własnego pliku cienia),
/// - obsługiwać purge wywołane przez write(nullptr, ..., 0), resetując stan obiektu i odtwarzając pusty segment gotowy do dalszego zapisu,
/// - przy tworzeniu obiektu odtwarzać stan grupy na podstawie istniejących plików segmentów, wybierając spójny końcowy fragment sekwencji i rekonstruując removedSegments_.
///
/// @note Klasa zarządza segmentacją i retencją, ale nie interpretuje zawartości rekordów ani semantyki null.
/// @note Pozycje wskazujące rekordy należące do już usuniętych segmentów są traktowane jako niedostępne do odczytu i aktualizacji.

namespace rdb {

template <typename T = posixBinaryFileWithShadow>
class groupFile : public FileInterface {
  std::string filename_;
  std::string currentFilename_;
  Descriptor descriptor_;
  const ssize_t recordSize_;

  retention_t retention_{.segments = 0, .capacity = 0};

  std::vector<std::unique_ptr<T>> vec_;

  size_t writeCount_     = 0;
  size_t currentSegment_ = 0;

  size_t removedSegments_ = 0;

  int percounter_;

 public:
  groupFile(std::string_view fileName, const Descriptor &descriptor, const retention_t &retention, int percounter);
  ~groupFile() override;

  using FileInterface::read;
  using FileInterface::write;
  // Wirtualne skladowe instancjuje JAWNIE `template class groupFile<...>` na koncu fagrp.cc,
  // wiec nie zaleza od tego, czy dany kompilator instancjuje je niejawnie - a o to pyta
  // portability-template-virtual-member-function przy `extern template` nizej.
  // NOLINTBEGIN(portability-template-virtual-member-function)
  ssize_t write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, size_t position) override;
  ssize_t read(uint8_t *ptrData, std::vector<bool> &nullBitset, size_t position) override;
  ssize_t purge();

  auto name() -> std::string & override;
  size_t count() override;
  [[nodiscard]] bool hasShadow() const override { return !vec_.empty() && vec_.front()->hasShadow(); }
  // NOLINTEND(portability-template-virtual-member-function)
};

/// DEKLARACJA instancjacji, nie definicja - definicja stoi na koncu fagrp.cc, za
/// cialami metod.
///
/// Wczesniej bylo tu `template class groupFile<...>;`, czyli DEFINICJA instancjacji
/// w miejscu, w ktorym widac same deklaracje skladowych. Standard nie kaze tego
/// zglaszac, wiec kompilator po prostu nie emituje cial, ktorych nie widzi, i czy
/// symbole powstana, zalezy od tego, KIEDY dany kompilator wykonuje instancjacje.
/// GCC odklada ja do konca jednostki, wiec w fagrp.cc trafiala juz na definicje
/// nizej i symbole powstawaly. Clang instancjuje w miejscu zapisu - a tam nie ma
/// jeszcze czego instancjowac - wiec konstruktor groupFile nie powstawal w ogole
/// i jedenascie celow nie linkowalo sie na ten jeden brakujacy symbol.
///
/// `extern template` mowi wprost to, co i tak jest prawda: tej instancjacji nie
/// rob tutaj, ona jest gdzie indziej. Poza korzysciami z poprawnosci oszczedza tez
/// kazdej jednostce prob instancjacji.
extern template class groupFile<posixBinaryFileWithShadow>;
extern template class groupFile<posixBinaryFile>;
}  // namespace rdb
