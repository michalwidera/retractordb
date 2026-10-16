#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "descriptor.hpp"
#include "metaData.hpp"
#include "metaShadow.hpp"

namespace rdb {

/// @brief Wariant indeksu metadanych null dla magazynów posiadających plik cienia danych (.shadow).
///
/// Obiekt klasy storageShadow powinien:
/// - być wstrzykiwany do storage zamiast bazowego metaData, gdy accessor magazynu utrzymuje plik cienia
///   danych (FileInterface::hasShadow(), np. posixBinaryFileWithShadow), tak aby write/read w storage nie
///   zawierały żadnych rozgałęzień zależnych od obecności cienia; posiadanie pliku cienia jest niezależne od
///   posiadania metaindeksu - cień danych zachowuje oryginalną zarejestrowaną zawartość, metaindeks rejestruje
///   wartości null i przerwy w transmisji,
///   Test: `AccessorFactoryTest.meta_index_variant_selection`. Test: `xrdb.storage_rotates_data_and_null_indexes_on_shutdown`.
/// - kierować aktualizacje rekordów (onRecordModified()) do cienia indeksu (.meta.shadow, obiekt metaShadow),
///   nie modyfikując głównego pliku indeksu - główny indeks pozostaje spójny z głównym plikiem danych,
///   Test: `StorageShadowFixture.polymorphic_updates_preserve_main_bytes_and_merge_in_arrival_order`.
/// - przy odczycie (getNullBitset()) zwracać nadpisanie z cienia, jeśli istnieje; pozostałe rekordy prezentować
///   z głównego indeksu,
///   Test: `StorageShadowFixture.scenariusz_nadpisanie_w_cieniu`.
/// - przy tworzeniu obiektu wczytywać istniejące nadpisania z pliku cienia indeksu (persystencja po restarcie),
///   Test: `StorageShadowFixture.scenariusz_persystencja_i_merge`.
/// - udostępniać mergeShadow() scalające nadpisania (w kolejności zapisu - ostatnie nadpisanie pozycji wygrywa)
///   do głównego indeksu metadanych i usuwające .meta.shadow; scalenie danych pozostawiać wywołującemu,
///   Test: `StorageShadowFixture.polymorphic_updates_preserve_main_bytes_and_merge_in_arrival_order`, `StorageShadowFixture.scenariusz_persystencja_i_merge`.
/// - udostępniać discardShadow() odrzucające cień metadanych bez scalania; usunięcie cienia danych
///   pozostawiać wywołującemu,
///   Test: `StorageShadowFixture.scenariusz_discard_i_reset`.
/// - przy reset() (purge) czyscic glowny indeks i odrzucac cien,
///   Test: `StorageShadowFixture.scenariusz_discard_i_reset`.
/// - przy rotacji archiwizowac glowny indeks i jego cien pod tym samym numerem co dane,
///   Test: `xrdb.storage_rotates_data_and_null_indexes_on_shutdown`. Test: `StorageRotationTest.shadow_failure_does_not_discard_unarchived_overrides`.
/// - udostępniać ścieżkę pliku cienia indeksu (metaShadowFilePath()) na potrzeby porządkowania zasobów przez storage.
///   Test: `StorageShadowTest.metaShadowFilePath_dopisuje_sufiks_shadow`.
/// @note mergeShadow() i discardShadow() działają wyłącznie na metadanych; wywołujący odpowiada za
///       skoordynowanie ich z operacjami na cieniu danych; klasa nie wywołuje merge()/discard() akcesora.
/// @note Walidacja zakresu recordIndex w onRecordModified() odbywa się względem totalRecords() głównego indeksu.
/// @note Pozostałe wymagania (RLE, gap, persystencja głównego indeksu) dziedziczone z metaData.
class storageShadow : public metaData {
 public:
  /// @param descriptor   descriptor of the indexed data stream
  /// @param metaFilePath path of the main meta index file; the shadow index file is `<metaFilePath>.shadow`
  ///
  /// Loads any existing shadow overrides from disk.
  storageShadow(const Descriptor &descriptor, const std::string &metaFilePath);

  /// @brief Record the modification as a shadow override instead of rewriting the main index.
  /// @return Errc::Logic if recordIndex >= totalRecords()
  [[nodiscard]] Result<> onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitset) override;

  /// @brief Shadow override wins; otherwise the main index is consulted.
  std::vector<bool> getNullBitset(size_t recordIndex) const override;

  /// @brief Clear the main index and discard the shadow index.
  void reset() override;
  [[nodiscard]] const std::optional<Error> &initializationError() const override {
    return metaData::initializationError() ? metaData::initializationError() : shadow_.initializationError();
  }

  [[nodiscard]] Result<> rotate(int percounter, bool reopen = true) override;

  /// @brief Merge all shadow overrides into the main index and clear the shadow.
  ///
  /// Applies each recorded override to the main index (in arrival order, so
  /// the last override for a record wins) and then discards the shadow index,
  /// mirroring the data store's merge() of .shadow into the main file.
  [[nodiscard]] Result<> mergeShadow();

  /// @brief Discard all shadow overrides and remove the shadow index file.
  ///
  /// Mirrors deletion of the data .shadow file (purge / reset / rotation).
  void discardShadow();

  /// @brief Ścieżka pliku cienia indeksu metadanych odpowiadającego danemu plikowi indeksu.
  /// @param metaIndexFile ścieżka głównego pliku indeksu metadanych (<storageFile>.meta)
  [[nodiscard]] static std::string metaShadowFilePath(std::string_view metaIndexFile);

 private:
  metaShadow shadow_;  ///< cień indeksu (.meta.shadow); zob. metaShadow.hpp
};  // class storageShadow

}  // namespace rdb
