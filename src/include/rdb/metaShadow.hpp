#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "descriptor.hpp"
#include "error.hpp"

namespace rdb {

/// @brief Klasa utrzymująca cień indeksu (.meta.shadow) - nadpisania wzorców null dla magazynów
///        posiadających plik cienia danych (.shadow, zob. posixBinaryFileWithShadow).
///
/// Obiekt klasy metaShadow powinien:
/// - przechowywać nadpisania wzorca null dla poszczególnych rekordów jako pary (indeks rekordu, wzorzec null),
///   nie modyfikując głównego indeksu, do którego cień się odnosi,
/// - przez appendOverride() dopisywać nadpisanie do pamięci i podejmować próbę zapisu do pliku
///   <metaFilePath>.shadow, o ile jego ścieżka nie jest pusta; koordynację z cieniem danych pozostawiać wywołującemu,
/// - przy wielokrotnych nadpisaniach tej samej pozycji traktować ostatni zapisany wpis jako obowiązujący stan rekordu,
/// - przy udanym I/O zapisywać nadpisania do pliku i odtwarzać je przez load(), które zastępuje stan w pamięci,
/// - umożliwiać scalenie nadpisań (w kolejności zapisu) do głównego indeksu przez wywołującego oraz odrzucenie
///   ich bez scalania, mirrorując odpowiednio merge() i usunięcie pliku .shadow danych,
/// - udostępniać konwencję nazewnictwa pliku cienia (<metaFilePath>.shadow) jako metodę statyczną, aby inne
///   warstwy (np. storageShadow, storage) mogły się do niej odwołać bez duplikowania sufiksu,
/// - zwalniać własną pamięć przy niszczeniu obiektu.
/// @note appendOverride() zmienia pamięć przed próbą I/O i nie propaguje błędów otwarcia ani zapisu;
///       obecność nadpisania w lookup() nie potwierdza, że można je odtworzyć po restarcie.
/// @note Klasa nie zna pojęcia całkowitej liczby rekordów głównego indeksu ani nie waliduje zakresu
///       recordIndex - to odpowiedzialność wywołującego (storageShadow).
/// @note Format wpisu na dysku jest zgodny z dawnym IndexRecord (flaga(1B) + size_t + size_t + bitset), aby
///       istniejące pliki .meta.shadow pozostały czytelne po tej refaktoryzacji.
class metaShadow {
 public:
  /// @brief Single override: the new null bit-set pattern for one absolute record index.
  /// Obiekt klasy ShadowOverride powinien:
  /// - przechowywać bezwzględny indeks nadpisanego rekordu głównego indeksu i nowy wzorzec null tego rekordu, jeden bit na pole deskryptora.
  /// - przez serialize zapisywać bajt zarezerwowany równy 0, recordIndex i długość bitsetu jako size_t w porządku bajtów maszyny, a następnie bitset upakowany od najmłodszego bitu w ceil(n/8) bajtach z zerowymi bitami dopełnienia, w układzie zgodnym z wpisem IndexRecord.
  /// - przez deserialize odtwarzać wpis zapisany przez serialize, pomijając wartość bajtu zarezerwowanego.
  /// - zwracać z deserialize Errc::IO z kodem EIO, gdy ramka jest za krótka na nagłówek albo deklarowana długość bitsetu przekracza pozostałe bajty, porównując długość przed zaokrągleniem do bajtów.
  struct ShadowOverride {
    size_t recordIndex{0};         ///< absolute position of the overridden record in the main index
    std::vector<bool> nullBitset;  ///< new null bit-set pattern for that record
    [[nodiscard]] std::vector<std::byte> serialize() const;
    /// Blad IO dla uszkodzonego wpisu cienia - jak IndexRecord::deserialize.
    [[nodiscard]] static Result<ShadowOverride> deserialize(std::span<const std::byte> data);
  };

  /// @param descriptor   descriptor of the indexed data stream (determines entry size on disk)
  /// @param metaFilePath path of the main meta index file; the shadow file is `<metaFilePath>.shadow`
  explicit metaShadow(const Descriptor &descriptor, const std::string &metaFilePath);

  /// @brief (Re)load overrides from `<metaFilePath>.shadow`, replacing any in-memory state.
  void load();
  [[nodiscard]] const std::optional<Error> &initializationError() const { return initializationError_; }

  /// @brief Record a new override for @p recordIndex, both in memory and on disk.
  void appendOverride(size_t recordIndex, const std::vector<bool> &nullBitset);

  /// @brief Look up the most recent override for @p recordIndex.
  /// @return the overriding null bit-set, or std::nullopt if no override exists for this record
  [[nodiscard]] std::optional<std::vector<bool>> lookup(size_t recordIndex) const;

  /// @brief All overrides in arrival order (oldest first). Use this to merge into the main index.
  [[nodiscard]] const std::vector<ShadowOverride> &overrides() const noexcept { return overrides_; }

  /// @brief Discard all overrides and remove the shadow file.
  void discard();

  /// @brief Archiwizuje cien pod tym samym numerem co plik danych i glowny indeks.
  /// @return false po bledzie rotacji; szczegoly sa logowane na poziomie ERROR.
  [[nodiscard]] bool rotate(int percounter);

  /// @brief Sciezka aktywnego pliku cienia; pusta, gdy cien nie ma pliku.
  [[nodiscard]] const std::string &filePath() const noexcept { return shadowFilePath_; }

  /// @brief Path of the shadow index file corresponding to @p metaFilePath, without instantiating a metaShadow.
  [[nodiscard]] static std::string shadowFilePathFor(std::string_view metaFilePath);

 private:
  std::optional<Error> initializationError_;
  std::string shadowFilePath_;             ///< path of the shadow index file (<metaFilePath>.shadow)
  const size_t entrySize_;                 ///< serialized size of one ShadowOverride on disk
  std::vector<ShadowOverride> overrides_;  ///< in-memory mirror of the shadow file, arrival order
};  // class metaShadow

}  // namespace rdb
