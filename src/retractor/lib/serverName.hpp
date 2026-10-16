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

/// Nazwa instancji dzialajacej jako USLUGA. Jedna, stala i znana z gory, bo usluga jest
/// adresatem polecen operatora (`xqry --server service --reset plan.rql`), a adresat, ktorego
/// nazwy trzeba najpierw poszukac na magistrali, nie nadaje sie do wpisania w skrypt. Serwerow
/// zwyklych moze dzialac wiele; usluga jest dokladnie jedna i pilnuje tego blokada tej nazwy
/// oraz kontrola trybu kService przy roszczeniu slotu.
inline constexpr const char *kServiceInstanceName = "service";

/// Funkcja generate powinna:
/// - zwracać nazwę "przymiotnik_nazwisko" (np. "nervous_hopper") wylosowaną z dwóch stałych list po 64 pozycje, poprawną według isValid.
/// - brać ziarno losowania z std::random_device, nie z zegara.
/// - nie gwarantować różnych nazw w kolejnych wywołaniach.
///
/// @note Ziarno z zegara dawaloby te sama nazwe serwerom startowanym przez skrypt w tej samej sekundzie.
///       O rozstrzyganiu kolizji decyduje wolajacy.
std::string generate();

/// Funkcja isValid powinna:
/// - zwracać true wyłącznie dla nazwy o długości 1..kMaxLength, której pierwszy znak to mała litera ASCII, a pozostałe to małe litery ASCII, cyfry, '_' albo '-'.
///
/// @note Taka nazwa nadaje sie na czlon nazwy obiektu IPC i pliku blokady.
bool isValid(std::string_view name);

/// Nazwa zmiennej srodowiskowej niosacej przestrzen nazw calego uruchomienia.
inline constexpr const char *kNamespaceEnv = "RDB_NAMESPACE";

/// Funkcja environmentNamespace powinna:
/// - zwracać surową wartość zmiennej środowiskowej kNamespaceEnv, bez sprawdzania, a pusty napis, gdy zmienna nie jest ustawiona.
///
/// @note Jedno pokretlo rozdziela KOMPLET zasobow globalnych maszyny: segment magistrali (bus::segmentName),
///       domyslna nazwe instancji xretractora i domyslny cel xqry - dla rownoczesnych testow integracyjnych
///       dzielacych nazwy strumieni. Wolajacy ocenia wartosc przez isValid i zatrzymuje program z
///       komunikatem: po cichu zignorowana przestrzen nazw cofnelaby uruchomienia na wspolne zasoby.
std::string environmentNamespace();

}  // namespace servername
