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

/// Plik konfiguracji czytany tylko, gdy podany. Argument lockDir wygrywa nad paths.lock_dir;
/// lagodny jest katalog wymieniony w embedded.lenient_lock_dirs, a bez tego klucza katalog
/// tymczasowy systemu (TMPDIR, bez niej /tmp).
[[nodiscard]] Result<OfflinePolicy> offlinePolicy(std::string_view lockDir, std::string_view configPath);

/// Tylko obserwacja trzymanych blokad. Katalog musi odpowiadac paths.lock_dir serwisu.
/// Nie wyklucza pozniejszego startu xretractor.
[[nodiscard]] Result<> checkOffline(const OfflinePolicy &policy);
[[nodiscard]] Result<std::string> temporaryStorageDirectory();
/// Sprzatanie tymczasowego katalogu nie usuwa magazynu zajetego przez Storage.
void removeTemporaryStorage(const std::string &directory) noexcept;

}  // namespace rdb::embed
