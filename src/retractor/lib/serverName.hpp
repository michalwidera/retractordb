#pragma once

#include <string>
#include <string_view>

/// @brief Nazwa instancji serwera: identyfikator, ktory wyroznia jeden proces xretractor
///        sposrod wielu na tej samej maszynie.
///
/// Nazwa jest czlonem nazw obiektow IPC (patrz ipc::names) oraz nazwy pliku blokady, wiec
/// musi byc bezpieczna w obu tych rolach: bez separatorow sciezki, bez spacji, bez kropki
/// (kropka rozdziela nazwe bazowa od czlonu serwera).
namespace servername {

/// Maksymalna dlugosc nazwy. Ograniczenie jest realne, nie ozdobne: nazwy obiektow POSIX
/// shm maja limit dlugosci, a nazwa serwera doklejana jest do nazwy bazowej i identyfikatora
/// klienta.
inline constexpr std::size_t kMaxLength = 32;

/// Losuje nazwe w stylu "przymiotnik_nazwisko" (jeden token, bez spacji), np. "nervous_hopper".
/// Kolejne wywolania moga dac te sama nazwe -- o rozstrzyganiu kolizji decyduje wolajacy.
std::string generate();

/// Czy nazwa nadaje sie na czlon nazwy obiektu IPC i pliku blokady:
/// pierwszy znak mala litera, dalej male litery, cyfry, '_' lub '-', dlugosc 1..kMaxLength.
bool isValid(std::string_view name);

/// Nazwa zmiennej srodowiskowej niosacej przestrzen nazw calego uruchomienia.
inline constexpr const char *kNamespaceEnv = "RDB_NAMESPACE";

/// Przestrzen nazw uruchomienia odczytana z kNamespaceEnv; pusta, gdy zmienna nie jest
/// ustawiona albo ma wartosc pusta.
///
/// Jedno pokretlo rozdziela KOMPLET zasobow globalnych dla maszyny: nazwe segmentu magistrali
/// (bus::segmentName), domyslna nazwe instancji xretractora i domyslny cel xqry. Powstalo dla
/// rownoczesnych testow integracyjnych, ktore dziela nazwy strumieni: bez wlasnej magistrali
/// druga taka instancja odpada na ClaimStatus::Conflict, bo rozlacznosc nazw jest wlasnoscia
/// maszyny, a nie katalogu roboczego.
///
/// Wartosc jest zwracana SUROWA, bez sprawdzania. Ocenia ja wolajacy przez isValid i ma
/// zatrzymac program z komunikatem: po cichu zignorowana przestrzen nazw cofnelaby rownolegle
/// uruchomienia na wspolne zasoby, czyli awaria bylaby widoczna dopiero jako kolizja gdzie
/// indziej.
std::string environmentNamespace();

}  // namespace servername
