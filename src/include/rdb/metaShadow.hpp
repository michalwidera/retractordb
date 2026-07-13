#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "descriptor.hpp"

namespace rdb {

/// @brief Klasa utrzymująca cień indeksu (.meta.shadow) — nadpisania wzorców null dla magazynów
///        posiadających plik cienia danych (.shadow, zob. posixBinaryFileWithShadow).
///
/// Obiekt klasy metaShadow powinien:
/// - przechowywać nadpisania wzorca null dla poszczególnych rekordów jako pary (indeks rekordu, wzorzec null),
///   nie modyfikując głównego indeksu, do którego cień się odnosi,
/// - podążać za plikiem cienia danych (.shadow): plik cienia indeksu (<metaFilePath>.shadow) jest aktualizowany
///   przy każdym nadpisaniu oraz scalany/usuwany razem z cieniem danych, aby uniknąć rozbieżności między nimi,
/// - przy wielokrotnych nadpisaniach tej samej pozycji traktować ostatni zapisany wpis jako obowiązujący stan rekordu,
/// - zapisywać i odtwarzać nadpisania z pliku tak, aby przetrwały ponowne uruchomienie programu,
/// - umożliwiać scalenie nadpisań (w kolejności zapisu) do głównego indeksu przez wywołującego oraz odrzucenie
///   ich bez scalania, mirrorując odpowiednio merge() i usunięcie pliku .shadow danych,
/// - udostępniać konwencję nazewnictwa pliku cienia (<metaFilePath>.shadow) jako metodę statyczną, aby inne
///   warstwy (np. storageShadow, storage) mogły się do niej odwołać bez duplikowania sufiksu,
/// - zarządzać własnymi zasobami w sposób bezpieczny i bez wycieków pamięci.
/// @note Klasa nie zna pojęcia całkowitej liczby rekordów głównego indeksu ani nie waliduje zakresu
///       recordIndex — to odpowiedzialność wywołującego (storageShadow).
/// @note Format wpisu na dysku jest zgodny z dawnym IndexRecord (flaga(1B) + size_t + size_t + bitset), aby
///       istniejące pliki .meta.shadow pozostały czytelne po tej refaktoryzacji.
class metaShadow {
 public:
  /// @brief Single override: the new null bit-set pattern for one absolute record index.
  struct ShadowOverride {
    size_t recordIndex{0};         ///< absolute position of the overridden record in the main index
    std::vector<bool> nullBitset;  ///< new null bit-set pattern for that record
    [[nodiscard]] std::vector<std::byte> serialize() const;
    static ShadowOverride deserialize(std::span<const std::byte> data);
  };

  /// @param descriptor   descriptor of the indexed data stream (determines entry size on disk)
  /// @param metaFilePath path of the main meta index file; the shadow file is `<metaFilePath>.shadow`
  explicit metaShadow(const Descriptor &descriptor, std::string metaFilePath);

  /// @brief (Re)load overrides from `<metaFilePath>.shadow`, replacing any in-memory state.
  void load();

  /// @brief Record a new override for @p recordIndex, both in memory and on disk.
  void appendOverride(size_t recordIndex, const std::vector<bool> &nullBitset);

  /// @brief Look up the most recent override for @p recordIndex.
  /// @return the overriding null bit-set, or std::nullopt if no override exists for this record
  [[nodiscard]] std::optional<std::vector<bool>> lookup(size_t recordIndex) const;

  /// @brief All overrides in arrival order (oldest first). Use this to merge into the main index.
  [[nodiscard]] const std::vector<ShadowOverride> &overrides() const noexcept { return overrides_; }

  /// @brief Discard all overrides and remove the shadow file.
  void discard();

  /// @brief Path of the shadow index file corresponding to @p metaFilePath, without instantiating a metaShadow.
  [[nodiscard]] static std::string shadowFilePathFor(std::string_view metaFilePath);

 private:
  std::string shadowFilePath_;             ///< path of the shadow index file (<metaFilePath>.shadow)
  const size_t entrySize_;                 ///< serialized size of one ShadowOverride on disk
  std::vector<ShadowOverride> overrides_;  ///< in-memory mirror of the shadow file, arrival order
};  // class metaShadow

}  // namespace rdb
