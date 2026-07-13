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
///   posiadania metaindeksu — cień danych zachowuje oryginalną zarejestrowaną zawartość, metaindeks rejestruje
///   wartości null i przerwy w transmisji,
/// - kierować aktualizacje rekordów (onRecordModified()) do cienia indeksu (.meta.shadow, obiekt metaShadow),
///   nie modyfikując głównego pliku indeksu — główny indeks pozostaje spójny z głównym plikiem danych,
/// - podążać za plikiem cienia danych (.shadow): cień indeksu jest aktualizowany przy każdej aktualizacji
///   rekordu oraz scalany/usuwany razem z cieniem danych, aby uniknąć rozbieżności między nimi,
/// - przy odczycie (getNullBitset()) zwracać nadpisanie z cienia, jeśli istnieje; pozostałe rekordy prezentować
///   z głównego indeksu,
/// - przy tworzeniu obiektu wczytywać istniejące nadpisania z pliku cienia indeksu (persystencja po restarcie),
/// - udostępniać mergeShadow() scalające nadpisania (w kolejności zapisu — ostatnie nadpisanie pozycji wygrywa)
///   do głównego indeksu i usuwające cień, lustrzanie do merge() pliku cienia danych,
/// - udostępniać discardShadow() odrzucające cień bez scalania, lustrzanie do usunięcia pliku .shadow danych,
/// - przy reset() (purge/rotacja w storage) czyścić główny indeks i odrzucać cień,
/// - udostępniać ścieżkę pliku cienia indeksu (metaShadowFilePath()) na potrzeby porządkowania zasobów przez storage.
/// @note Walidacja zakresu recordIndex w onRecordModified() odbywa się względem totalRecords() głównego indeksu.
/// @note Pozostałe wymagania (RLE, gap, persystencja głównego indeksu) dziedziczone z metaData.
class storageShadow : public metaData {
 public:
  /// @param descriptor   descriptor of the indexed data stream
  /// @param metaFilePath path of the main meta index file; the shadow index file is `<metaFilePath>.shadow`
  ///
  /// Loads any existing shadow overrides from disk.
  storageShadow(const Descriptor &descriptor, std::string metaFilePath);

  /// @brief Record the modification as a shadow override instead of rewriting the main index.
  /// @throws std::out_of_range if recordIndex >= totalRecords()
  void onRecordModified(size_t recordIndex, const std::vector<bool> &nullBitset) override;

  /// @brief Shadow override wins; otherwise the main index is consulted.
  std::vector<bool> getNullBitset(size_t recordIndex) const override;

  /// @brief Clear the main index and discard the shadow index.
  void reset() override;

  /// @brief Merge all shadow overrides into the main index and clear the shadow.
  ///
  /// Applies each recorded override to the main index (in arrival order, so
  /// the last override for a record wins) and then discards the shadow index,
  /// mirroring the data store's merge() of .shadow into the main file.
  void mergeShadow();

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
