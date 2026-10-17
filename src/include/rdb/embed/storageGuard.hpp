#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "rdb/error.hpp"

namespace rdb::embed {

/// Wlasnosc katalogu osadzonego jest trwala; zajecie rozstrzyga flock na i-wezle katalogu.
/// Nie tworzy blokad instancji xretractor ani obiektow magistrali.
/// Obiekt klasy StorageGuard powinien:
/// - [storageguard.storage-guard.1~1] tworzyć strażnika przez fabrykę zwracającą Result i udostępniać bezwzględną ścieżkę katalogu po rozwiązaniu istniejących dowiązań, przyjmując katalog bieżący dla pustego argumentu directory.
/// - [storageguard.storage-guard.2~1] otwierać istniejący katalog z deskryptorem zamykanym przy exec (O_CLOEXEC) i przed sprawdzaniem lub tworzeniem znacznika uzyskiwać na tym deskryptorze nieblokującą wyłączną blokadę flock, obejmującą także inne otwarcia tego samego katalogu w tym samym procesie i przez aliasy ścieżki.
/// - [storageguard.storage-guard.3~1] zwracać Errc::Config z EBUSY, gdy wyłączna blokada katalogu jest już zajęta.
/// - [storageguard.storage-guard.4~1] po uzyskaniu blokady porównywać urządzenie i i-węzeł otwartego katalogu z obiektem wskazanym przez jego ścieżkę, odmawiając inicjalizacji jako Errc::Compile w razie rozbieżności.
///   Ograniczenie weryfikacji: rozbieznosc urzadzenia lub i-wezla wstrzyknieta po stronie testu; nie jest to czasowy wyscig z rename.
/// - [storageguard.storage-guard.5~1] przy claim włączonym i braku znacznika dopuszczać przejęcie wyłącznie pustego katalogu, odmawiając katalogu niepustego jako Errc::Compile, i tworzyć plik .retractordb-embedded w trybie wyłącznego utworzenia, bez podążania za dowiązaniem, z żądanymi uprawnieniami 0600 i treścią retractordb embedded storage v1 zakończoną znakiem nowej linii.
///   Nietestowalne: flagi O_EXCL i O_NOFOLLOW podczas tworzenia znacznika sprawdzone strukturalnie w StorageGuard::initialize i StorageFd; nie wymuszono podmiany pliku miedzy lstat a open.
/// - [storageguard.storage-guard.6~1] przy claim wyłączonym i braku znacznika kończyć inicjalizację bez tworzenia znacznika i bez utrzymywania blokady katalogu.
/// - [storageguard.storage-guard.7~1] przy istniejącym znaczniku wymagać, sprawdzając go bez podążania za dowiązaniem, zwykłego pliku należącego do efektywnego użytkownika procesu, z dokładnie jednym dowiązaniem i dokładnie oczekiwaną treścią, czytaną przez otwarcie nieblokujące i bez podążania za dowiązaniem, zwracając Errc::Compile dla niezgodnego znacznika.
///   Ograniczenie weryfikacji: UID wstrzykniety w lstat, bez rzeczywistej zmiany uzytkownika; O_NONBLOCK/O_NOFOLLOW odczytu sprawdzone strukturalnie w initialize i StorageFd.
/// - [storageguard.storage-guard.8~1] utrzymywać uzyskaną blokadę poprawnie oznaczonego katalogu przez czas życia strażnika oraz raportować przez ownsDirectory, czy strażnik posiada otwarty deskryptor chroniący katalog.
/// - [storageguard.storage-guard.9~1] w checkPath rozwiązywać ścieżkę artefaktu, względną względem bieżącego katalogu procesu, i akceptować wyłącznie ścieżki ściśle wewnątrz chronionego katalogu, porównując elementy ścieżki i odrzucając sam katalog oraz ścieżkę znacznika własności jako Errc::Compile.
/// - [storageguard.storage-guard.10~1] przekazywać pozostałe błędy operacji systemu plików jako Errc::IO z komunikatem wskazującym ścieżkę i kodem systemowym błędu.
///   Ograniczenie weryfikacji: reprezentatywne open/fstat/flock/resolve, nie wszystkie kombinacje odmow kazdego wywolania systemowego.
/// - [storageguard.storage-guard.11~1] zamykać posiadany deskryptor katalogu przy zniszczeniu oraz po nieudanej inicjalizacji, zwalniając blokadę bez usuwania katalogu i trwałego znacznika.
/// - [storageguard.storage-guard.12~1] uniemożliwiać kopiowanie strażnika, aby pojedynczy obiekt odpowiadał za zamknięcie posiadanego deskryptora.
///   Sprawdzenie kompilacji: static_assert !is_copy_constructible/assignable<StorageGuard> w test_embedEngine.cpp.
/// - [storageguard.storage-guard.13~1] nie tworzyć blokad instancji xretractor ani obiektów magistrali.
///   Nietestowalne: kontrola strukturalna StorageGuard::initialize, checkPath i destruktora w storageGuard.cc - operacje plikowe, bez magistrali i tworzenia blokad serwisu; embedding_boundary sprawdza dozwolony stan procesu.
/// - [storageguard.storage-guard.14~1] po nieudanym zapisie treści znacznika utworzonego w tym samym wywołaniu podejmować jego usunięcie przed zwróceniem Errc::IO, tak aby po udanym unlink katalog pozostał możliwy do przejęcia.
///   Ograniczenie weryfikacji: initialize ignoruje blad samego unlink; nie gwarantuje usuniecia znacznika, gdy system plikow odmawia tej operacji.
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
/// - [storageguard.offline-policy.1~1] odrzucać niepusty względny lockDir jako Errc::Config.
/// - [storageguard.offline-policy.2~1] czytać wyłącznie wskazany plik konfiguracji, a bez configPath żadnego, i zwracać jego błąd oraz błędny klucz embedded.lenient_lock_dirs jako Errc::Config.
///   Zakres testow: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_bad_config_is_a_config_error_and_keeps_the_kernel`.
///   Nietestowalne: pominiecie loadAppConfig przy pustym configPath sprawdzone strukturalnie w offlinePolicy; test nie zmienia /etc.
/// - [storageguard.offline-policy.3~1] przyjmować katalog blokad z argumentu lockDir, a gdy jest pusty - z paths.lock_dir pliku konfiguracji; pusty wynik oznacza katalog tymczasowy systemu.
///   Zakres testow: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_config_supplies_the_lock_dir_and_the_argument_wins`.
/// - [storageguard.offline-policy.4~1] bez klucza embedded.lenient_lock_dirs uznawać za łagodny wyłącznie katalog tymczasowy systemu, a z kluczem wyłącznie katalog z listy; porównywać ścieżki po weakly_canonical, a gdy rozwiązanie pierwszej ścieżki zawiedzie - leksykalnie po normalizacji.
///   Zakres testow: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_system_temp_is_lenient_until_the_key_says_otherwise`, `test_storage.test_explicit_lock_dir_is_strict_unless_listed_as_lenient`.
///
/// @note Biblioteka nie szuka konfiguracji sama (bez warstw /etc i XDG). Katalog tymczasowy jest brany tak
///       jak przez serwis przy pustym paths.lock_dir (#450).
[[nodiscard]] Result<OfflinePolicy> offlinePolicy(std::string_view lockDir, std::string_view configPath);

/// Funkcja checkOffline powinna:
/// - [storageguard.check-offline.1~1] sprawdzać pliki blokad instancji xretractor (rodzina serviceLockInstance) w katalogu polityki albo, gdy jest pusty, w katalogu tymczasowym systemu, bez podążania za dowiązaniem i bez blokowania.
///   Zakres testow: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`, pytest `test_storage.test_held_service_lock_refuses_storage_and_engine`.
/// - [storageguard.check-offline.2~1] zwracać Errc::Config z EBUSY, gdy którakolwiek blokada jest trzymana.
/// - [storageguard.check-offline.3~1] w trybie łagodnym przepuszczać niepewność (brak lub nieczytelny katalog, nieczytelny wpis, otwarty wpis niebędący według fstat plikiem zwykłym), a w trybie ścisłym zwracać ją jako Errc::IO z errno, a otwarty wpis niebędący plikiem zwykłym jako Errc::Compile; odmowę open z ENOENT pomijać w obu trybach.
///   Zakres testow: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`, pytest `test_storage.test_missing_lock_directory_preserves_errno_and_kernel`.
///   Ograniczenie weryfikacji: brak osobnego wymuszenia bledu directory_iterator::increment ani fstat pliku blokady; pominiecie ENOENT z open potwierdzone przegladem checkOffline.
/// - [storageguard.check-offline.4~1] tylko obserwować blokady: współdzieloną nieblokującą próbę flock zwalniać od razu przez zamknięcie deskryptora, nie tworząc, nie zmieniając i nie usuwając plików blokad.
///
/// @note Katalog musi odpowiadac paths.lock_dir serwisu. Kontrola nie wyklucza pozniejszego startu xretractor.
///       Lagodnosc wynika z tego, ze domyslny katalog to zgadywanie, a we wspoldzielonym /tmp obcy wpis
///       blokowalby wszystkich.
[[nodiscard]] Result<> checkOffline(const OfflinePolicy &policy);

/// Funkcja temporaryStorageDirectory powinna:
/// - [storageguard.temporary-storage-directory.1~1] tworzyć nowy, unikalny katalog retractordb-engine-XXXXXX w katalogu tymczasowym systemu przez mkdtemp i zwracać jego ścieżkę.
/// - [storageguard.temporary-storage-directory.2~1] błąd ustalenia katalogu tymczasowego albo utworzenia katalogu zwracać jako Errc::IO z errno.
[[nodiscard]] Result<std::string> temporaryStorageDirectory();

/// Funkcja removeTemporaryStorage powinna:
/// - [storageguard.remove-temporary-storage.1~1] usuwać katalog wraz z zawartością wyłącznie po uzyskaniu jego blokady StorageGuard z przejęciem, a katalog zajęty przez inny magazyn albo nieprzejmowalny pozostawiać.
/// - [storageguard.remove-temporary-storage.2~1] nie zgłaszać błędów ani wyjątków.
///   Sprawdzenie kompilacji: static_assert noexcept(removeTemporaryStorage(...)) w test_embedStorageGuard.cpp.
///   Ograniczenie weryfikacji: noexcept nie zapewnia przezycia hosta przy awarii alokacji w StorageGuard::create.
void removeTemporaryStorage(const std::string &directory) noexcept;

}  // namespace rdb::embed
