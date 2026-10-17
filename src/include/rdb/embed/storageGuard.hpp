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
///   Test: `embedStorageGuard.claim_marker_canonical_paths_and_artifact_boundaries`, `embedStorageGuardBoundary.current_directory_and_temporary_cleanup`.
/// - otwierać istniejący katalog z deskryptorem zamykanym przy exec (O_CLOEXEC) i przed sprawdzaniem lub tworzeniem znacznika uzyskiwać na tym deskryptorze nieblokującą wyłączną blokadę flock, obejmującą także inne otwarcia tego samego katalogu w tym samym procesie i przez aliasy ścieżki.
///   Test: `embedStorageGuardBoundary.cloexec_alias_busy_and_destruction_release`.
/// - zwracać Errc::Config z EBUSY, gdy wyłączna blokada katalogu jest już zajęta.
///   Test: `embedStorageGuardBoundary.cloexec_alias_busy_and_destruction_release`, `embedEnginePlan.directory_lock_covers_storage_directives_and_aliases`.
/// - po uzyskaniu blokady porównywać urządzenie i i-węzeł otwartego katalogu z obiektem wskazanym przez jego ścieżkę, odmawiając inicjalizacji jako Errc::Compile w razie rozbieżności.
///   Test: `embedStorageGuardBoundary.metadata_and_lock_failures_close_the_candidate_fd`.
///   Ograniczenie weryfikacji: rozbieznosc urzadzenia lub i-wezla wstrzyknieta po stronie testu; nie jest to czasowy wyscig z rename.
/// - przy claim włączonym i braku znacznika dopuszczać przejęcie wyłącznie pustego katalogu, odmawiając katalogu niepustego jako Errc::Compile, i tworzyć plik .retractordb-embedded w trybie wyłącznego utworzenia, bez podążania za dowiązaniem, z żądanymi uprawnieniami 0600 i treścią retractordb embedded storage v1 zakończoną znakiem nowej linii.
///   Test: `embedStorageGuard.claim_marker_canonical_paths_and_artifact_boundaries`.
///   Nietestowalne: flagi O_EXCL i O_NOFOLLOW podczas tworzenia znacznika sprawdzone strukturalnie w StorageGuard::initialize i StorageFd; nie wymuszono podmiany pliku miedzy lstat a open.
/// - przy claim wyłączonym i braku znacznika kończyć inicjalizację bez tworzenia znacznika i bez utrzymywania blokady katalogu.
///   Test: `embedStorageGuard.claim_marker_canonical_paths_and_artifact_boundaries`, `embedStorageGuardBoundary.current_directory_and_temporary_cleanup`.
/// - przy istniejącym znaczniku wymagać, sprawdzając go bez podążania za dowiązaniem, zwykłego pliku należącego do efektywnego użytkownika procesu, z dokładnie jednym dowiązaniem i dokładnie oczekiwaną treścią, czytaną przez otwarcie nieblokujące i bez podążania za dowiązaniem, zwracając Errc::Compile dla niezgodnego znacznika.
///   Test: `embedStorageGuard.invalid_marker_links_and_contents_are_refused_without_mutation`, `embedStorageGuardBoundary.foreign_owner_nonregular_and_trailing_marker_are_refused`.
///   Ograniczenie weryfikacji: UID wstrzykniety w lstat, bez rzeczywistej zmiany uzytkownika; O_NONBLOCK/O_NOFOLLOW odczytu sprawdzone strukturalnie w initialize i StorageFd.
/// - utrzymywać uzyskaną blokadę poprawnie oznaczonego katalogu przez czas życia strażnika oraz raportować przez ownsDirectory, czy strażnik posiada otwarty deskryptor chroniący katalog.
///   Test: `embedStorageGuard.claim_marker_canonical_paths_and_artifact_boundaries`, `embedStorageGuardBoundary.cloexec_alias_busy_and_destruction_release`.
/// - w checkPath rozwiązywać ścieżkę artefaktu, względną względem bieżącego katalogu procesu, i akceptować wyłącznie ścieżki ściśle wewnątrz chronionego katalogu, porównując elementy ścieżki i odrzucając sam katalog oraz ścieżkę znacznika własności jako Errc::Compile.
///   Test: `embedStorageGuard.claim_marker_canonical_paths_and_artifact_boundaries`, `embedStorageGuardBoundary.filesystem_failures_keep_path_and_errno`.
/// - przekazywać pozostałe błędy operacji systemu plików jako Errc::IO z komunikatem wskazującym ścieżkę i kodem systemowym błędu.
///   Test: `embedStorageGuardBoundary.filesystem_failures_keep_path_and_errno`, `embedStorageGuardBoundary.metadata_and_lock_failures_close_the_candidate_fd`.
///   Ograniczenie weryfikacji: reprezentatywne open/fstat/flock/resolve, nie wszystkie kombinacje odmow kazdego wywolania systemowego.
/// - zamykać posiadany deskryptor katalogu przy zniszczeniu oraz po nieudanej inicjalizacji, zwalniając blokadę bez usuwania katalogu i trwałego znacznika.
///   Test: `embedStorageGuardBoundary.cloexec_alias_busy_and_destruction_release`, `embedStorageGuardBoundary.metadata_and_lock_failures_close_the_candidate_fd`.
/// - uniemożliwiać kopiowanie strażnika, aby pojedynczy obiekt odpowiadał za zamknięcie posiadanego deskryptora.
///   Sprawdzenie kompilacji: static_assert !is_copy_constructible/assignable<StorageGuard> w test_embedEngine.cpp.
/// - nie tworzyć blokad instancji xretractor ani obiektów magistrali.
///   Nietestowalne: kontrola strukturalna StorageGuard::initialize, checkPath i destruktora w storageGuard.cc - operacje plikowe, bez magistrali i tworzenia blokad serwisu; embedding_boundary sprawdza dozwolony stan procesu.
/// - po nieudanym zapisie treści znacznika utworzonego w tym samym wywołaniu podejmować jego usunięcie przed zwróceniem Errc::IO, tak aby po udanym unlink katalog pozostał możliwy do przejęcia.
///   Test: `embedStorageGuard.failed_marker_write_leaves_directory_claimable`.
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
/// - odrzucać niepusty względny lockDir jako Errc::Config.
///   Test: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`.
/// - czytać wyłącznie wskazany plik konfiguracji, a bez configPath żadnego, i zwracać jego błąd oraz błędny klucz embedded.lenient_lock_dirs jako Errc::Config.
///   Test: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_bad_config_is_a_config_error_and_keeps_the_kernel`.
///   Nietestowalne: pominiecie loadAppConfig przy pustym configPath sprawdzone strukturalnie w offlinePolicy; test nie zmienia /etc.
/// - przyjmować katalog blokad z argumentu lockDir, a gdy jest pusty - z paths.lock_dir pliku konfiguracji; pusty wynik oznacza katalog tymczasowy systemu.
///   Test: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_config_supplies_the_lock_dir_and_the_argument_wins`.
/// - bez klucza embedded.lenient_lock_dirs uznawać za łagodny wyłącznie katalog tymczasowy systemu, a z kluczem wyłącznie katalog z listy; porównywać ścieżki po weakly_canonical, a gdy rozwiązanie pierwszej ścieżki zawiedzie - leksykalnie po normalizacji.
///   Test: `embedStorageGuardBoundary.offline_policy_resolves_configuration_and_rejects_relative_arguments`, pytest `test_storage.test_system_temp_is_lenient_until_the_key_says_otherwise`, `test_storage.test_explicit_lock_dir_is_strict_unless_listed_as_lenient`.
///
/// @note Biblioteka nie szuka konfiguracji sama (bez warstw /etc i XDG). Katalog tymczasowy jest brany tak
///       jak przez serwis przy pustym paths.lock_dir (#450).
[[nodiscard]] Result<OfflinePolicy> offlinePolicy(std::string_view lockDir, std::string_view configPath);

/// Funkcja checkOffline powinna:
/// - sprawdzać pliki blokad instancji xretractor (rodzina serviceLockInstance) w katalogu polityki albo, gdy jest pusty, w katalogu tymczasowym systemu, bez podążania za dowiązaniem i bez blokowania.
///   Test: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`, pytest `test_storage.test_held_service_lock_refuses_storage_and_engine`.
/// - zwracać Errc::Config z EBUSY, gdy którakolwiek blokada jest trzymana.
///   Test: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`.
/// - w trybie łagodnym przepuszczać niepewność (brak lub nieczytelny katalog, nieczytelny wpis, otwarty wpis niebędący według fstat plikiem zwykłym), a w trybie ścisłym zwracać ją jako Errc::IO z errno, a otwarty wpis niebędący plikiem zwykłym jako Errc::Compile; odmowę open z ENOENT pomijać w obu trybach.
///   Test: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`, pytest `test_storage.test_missing_lock_directory_preserves_errno_and_kernel`.
///   Ograniczenie weryfikacji: brak osobnego wymuszenia bledu directory_iterator::increment ani fstat pliku blokady; pominiecie ENOENT z open potwierdzone przegladem checkOffline.
/// - tylko obserwować blokady: współdzieloną nieblokującą próbę flock zwalniać od razu przez zamknięcie deskryptora, nie tworząc, nie zmieniając i nie usuwając plików blokad.
///   Test: `embedStorageGuardBoundary.offline_busy_and_uncertainty_checks_do_not_modify_locks`.
///
/// @note Katalog musi odpowiadac paths.lock_dir serwisu. Kontrola nie wyklucza pozniejszego startu xretractor.
///       Lagodnosc wynika z tego, ze domyslny katalog to zgadywanie, a we wspoldzielonym /tmp obcy wpis
///       blokowalby wszystkich.
[[nodiscard]] Result<> checkOffline(const OfflinePolicy &policy);

/// Funkcja temporaryStorageDirectory powinna:
/// - tworzyć nowy, unikalny katalog retractordb-engine-XXXXXX w katalogu tymczasowym systemu przez mkdtemp i zwracać jego ścieżkę.
///   Test: `embedStorageGuardBoundary.current_directory_and_temporary_cleanup`.
/// - błąd ustalenia katalogu tymczasowego albo utworzenia katalogu zwracać jako Errc::IO z errno.
///   Test: `embedStorageGuardBoundary.invalid_tmpdir_preserves_errno`, `embedStorageGuardBoundary.mkdtemp_failure_preserves_errno_without_creating_a_directory`.
[[nodiscard]] Result<std::string> temporaryStorageDirectory();

/// Funkcja removeTemporaryStorage powinna:
/// - usuwać katalog wraz z zawartością wyłącznie po uzyskaniu jego blokady StorageGuard z przejęciem, a katalog zajęty przez inny magazyn albo nieprzejmowalny pozostawiać.
///   Test: `embedEngine.temporary_directory_is_kept_while_another_guard_owns_it`, `embedStorageGuardBoundary.current_directory_and_temporary_cleanup`.
/// - nie zgłaszać błędów ani wyjątków.
///   Sprawdzenie kompilacji: static_assert noexcept(removeTemporaryStorage(...)) w test_embedStorageGuard.cpp.
///   Test: `embedStorageGuardBoundary.current_directory_and_temporary_cleanup`.
///   Ograniczenie weryfikacji: noexcept nie zapewnia przezycia hosta przy awarii alokacji w StorageGuard::create.
void removeTemporaryStorage(const std::string &directory) noexcept;

}  // namespace rdb::embed
