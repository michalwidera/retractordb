#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "rdb/error.hpp"

namespace rdb::embed {

/// Wlasnosc katalogu osadzonego jest trwala; zajecie rozstrzyga flock na i-wezle katalogu.
/// Nie tworzy blokad instancji xretractor ani obiektow magistrali.
/// Obiekt klasy StorageGuard powinien:
/// - tworzyć strażnika przez fabrykę zwracającą Result i udostępniać bezwzględną ścieżkę katalogu po rozwiązaniu istniejących dowiązań, przyjmując katalog bieżący dla pustego argumentu directory.
/// - otwierać istniejący katalog z deskryptorem zamykanym przy exec (O_CLOEXEC) i przed sprawdzaniem lub tworzeniem znacznika uzyskiwać na tym deskryptorze nieblokującą wyłączną blokadę flock, obejmującą także inne otwarcia tego samego katalogu w tym samym procesie i przez aliasy ścieżki.
/// - zwracać Errc::Config z EBUSY, gdy wyłączna blokada katalogu jest już zajęta.
/// - po uzyskaniu blokady porównywać urządzenie i i-węzeł otwartego katalogu z obiektem wskazanym przez jego ścieżkę, odmawiając inicjalizacji jako Errc::Compile w razie rozbieżności.
/// - przy claim włączonym i braku znacznika dopuszczać przejęcie wyłącznie pustego katalogu, odmawiając katalogu niepustego jako Errc::Compile, i tworzyć plik .retractordb-embedded w trybie wyłącznego utworzenia, bez podążania za dowiązaniem, z żądanymi uprawnieniami 0600 i treścią retractordb embedded storage v1 zakończoną znakiem nowej linii.
/// - przy claim wyłączonym i braku znacznika kończyć inicjalizację bez tworzenia znacznika i bez utrzymywania blokady katalogu.
/// - przy istniejącym znaczniku wymagać, sprawdzając go bez podążania za dowiązaniem, zwykłego pliku należącego do efektywnego użytkownika procesu, z dokładnie jednym dowiązaniem i dokładnie oczekiwaną treścią, czytaną przez otwarcie nieblokujące i bez podążania za dowiązaniem, zwracając Errc::Compile dla niezgodnego znacznika.
/// - utrzymywać uzyskaną blokadę poprawnie oznaczonego katalogu przez czas życia strażnika oraz raportować przez ownsDirectory, czy strażnik posiada otwarty deskryptor chroniący katalog.
/// - w checkPath rozwiązywać ścieżkę artefaktu, względną względem bieżącego katalogu procesu, i akceptować wyłącznie ścieżki ściśle wewnątrz chronionego katalogu, porównując elementy ścieżki i odrzucając sam katalog oraz ścieżkę znacznika własności jako Errc::Compile.
/// - przekazywać pozostałe błędy operacji systemu plików jako Errc::IO z komunikatem wskazującym ścieżkę i kodem systemowym błędu.
/// - zamykać posiadany deskryptor katalogu przy zniszczeniu oraz po nieudanej inicjalizacji, zwalniając blokadę bez usuwania katalogu i trwałego znacznika.
/// - uniemożliwiać kopiowanie strażnika, aby pojedynczy obiekt odpowiadał za zamknięcie posiadanego deskryptora.
/// - nie tworzyć blokad instancji xretractor ani obiektów magistrali.
/// - po nieudanym zapisie treści znacznika utworzonego w tym samym wywołaniu usuwać ten znacznik przed zwróceniem Errc::IO, aby katalog pozostał możliwy do przejęcia.
class StorageGuard {
 public:
  [[nodiscard]] static Result<std::unique_ptr<StorageGuard>> create(std::string_view directory, bool claim);
  ~StorageGuard();
  StorageGuard(const StorageGuard &)            = delete;
  StorageGuard &operator=(const StorageGuard &) = delete;

  [[nodiscard]] const std::string &directory() const noexcept { return directory_; }
  [[nodiscard]] bool ownsDirectory() const noexcept { return fd_ >= 0; }
  /// Plik artefaktu musi zostac wewnatrz katalogu, takze po rozwiazaniu dowiazan.
  [[nodiscard]] Result<> checkPath(const std::string &path) const;

 private:
  StorageGuard() = default;
  [[nodiscard]] Result<> initialize(std::string_view directory, bool claim);
  std::string directory_;
  int fd_{-1};
};

/// Kontrola blokad serwisu po rozstrzygnieciu argumentu i pliku konfiguracji (#450).
/// @requirements-exempt OfflinePolicy: Parametry kontroli dostepu offline; kontrole wykonuje checkOffline.
struct OfflinePolicy {
  std::string lockDir;  ///< pusty => TMPDIR
  /// Jak xtrdb: odmowe daje tylko trzymany flock; wpis nieczytelny i brak katalogu nie.
  bool lenient{true};
};

/// Funkcja offlinePolicy powinna:
/// - odrzucać niepusty względny lockDir jako Errc::Config.
/// - czytać wyłącznie wskazany plik konfiguracji, a bez configPath żadnego, i zwracać jego błąd oraz błędny klucz embedded.lenient_lock_dirs jako Errc::Config.
/// - przyjmować katalog blokad z argumentu lockDir, a gdy jest pusty - z paths.lock_dir pliku konfiguracji; pusty wynik oznacza katalog tymczasowy systemu.
/// - bez klucza embedded.lenient_lock_dirs uznawać za łagodny wyłącznie katalog tymczasowy systemu, a z kluczem wyłącznie katalog wymieniony na jego liście, porównując katalogi jako te same obiekty systemu plików.
///
/// @note Biblioteka nie szuka konfiguracji sama (bez warstw /etc i XDG). Katalog tymczasowy jest brany tak
///       jak przez serwis przy pustym paths.lock_dir (#450).
[[nodiscard]] Result<OfflinePolicy> offlinePolicy(std::string_view lockDir, std::string_view configPath);

/// Funkcja checkOffline powinna:
/// - sprawdzać pliki blokad instancji xretractor (rodzina serviceLockInstance) w katalogu polityki albo, gdy jest pusty, w katalogu tymczasowym systemu, bez podążania za dowiązaniem i bez blokowania.
/// - zwracać Errc::Config z EBUSY, gdy którakolwiek blokada jest trzymana.
/// - w trybie łagodnym przepuszczać niepewność (brak lub nieczytelny katalog, nieczytelny wpis, wpis niebędący plikiem zwykłym), a w trybie ścisłym zwracać ją jako Errc::IO z errno, a wpis niebędący plikiem zwykłym jako Errc::Compile.
/// - tylko obserwować blokady, nie zakładając własnej.
///
/// @note Katalog musi odpowiadac paths.lock_dir serwisu. Kontrola nie wyklucza pozniejszego startu xretractor.
///       Lagodnosc wynika z tego, ze domyslny katalog to zgadywanie, a we wspoldzielonym /tmp obcy wpis
///       blokowalby wszystkich.
[[nodiscard]] Result<> checkOffline(const OfflinePolicy &policy);

/// Funkcja temporaryStorageDirectory powinna:
/// - tworzyć nowy, unikalny katalog retractordb-engine-XXXXXX w katalogu tymczasowym systemu przez mkdtemp i zwracać jego ścieżkę.
/// - błąd ustalenia katalogu tymczasowego albo utworzenia katalogu zwracać jako Errc::IO z errno.
[[nodiscard]] Result<std::string> temporaryStorageDirectory();

/// Funkcja removeTemporaryStorage powinna:
/// - usuwać katalog wraz z zawartością wyłącznie po uzyskaniu jego blokady StorageGuard z przejęciem, a katalog zajęty przez inny magazyn albo nieprzejmowalny pozostawiać.
/// - nie zgłaszać błędów ani wyjątków.
void removeTemporaryStorage(const std::string &directory) noexcept;

}  // namespace rdb::embed
