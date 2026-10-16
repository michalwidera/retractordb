#pragma once

#include <string>

/// Funkcja showStorageMap powinna:
/// - dla brakującego `<baseName>.desc` wypisywać na stderr "Descriptor not found: <plik>" i kończyć bez wydruku mapy.
/// - dla deskryptora, którego nie da się sparsować, wypisywać na stderr "Descriptor does not parse: <plik>" i kończyć bez przerywania procesu.
/// - w pozostałych przypadkach wypisywać na stdout ramkę szerokości 64 znaków z mapą trzech kolumn ([shadow], [binary data], [meta index]) i sekcjami DESCRIPTOR, DATA, META, SHADOW oraz, gdy istnieją, ROTATED FILES.
/// - w sekcji DESCRIPTOR podawać pola danych z typem, nazwą, krotnością i rozmiarem oraz rozmiar rekordu.
/// - w sekcji DATA podawać plik `<baseName>` z liczbą rekordów, a przy segmentach RETENTION (`<baseName>_segment_N`) sumy rekordów i bajtów pliku bieżącego i segmentów, politykę retencji z limitem rekordów i bajtów albo "unbounded" oraz zakres rekordów każdego segmentu.
/// - w sekcji META podawać `<baseName>.meta` z liczbą segmentów i rekordów (bez luk), paskiem segmentów i legendą, a w sekcji SHADOW `<baseName>.shadow` z liczbą aktualizacji.
/// - oznaczać brakujący plik danych, `.meta` albo `.shadow` dopiskiem "(missing)".
/// - w sekcji ROTATED FILES wypisywać pliki `<nazwa bazowa>*.old<N>` z katalogu bazy, malejąco po N, z rozmiarem w bajtach.
/// - nie zmieniać żadnego pliku magazynu.
///
/// @note Narzedzie OGLADA pliki magazynu, takze uszkodzone, wiec bledny deskryptor nie konczy procesu (do fazy 1
///       konczyl go FatalError). Przyklad wydruku stoi na poczatku xtrdbStorageMap.cpp.
void showStorageMap(const std::string &baseName);
