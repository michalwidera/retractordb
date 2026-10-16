#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "qTree.hpp"
#include "rdb/error.hpp"

/// @brief Wczytanie zestawu RQL z TEKSTU i sprzatanie artefaktow poprzedniego planu.
///
/// Obie czynnosci sa wspolne dla dwoch drog, ktorymi plan trafia do silnika: startu
/// z pliku (launcher) i przeladowania w locie komenda `reset` (executorsm). Rozjazd
/// miedzy nimi bylby rozjazdem SEMANTYKI planu - inaczej rozpoznany komentarz albo
/// nieskasowany plik `.desc` po starym strumieniu - wiec zrodlo jest jedno.

/// Wynik wczytania zestawu.
/// @requirements-exempt PlanSource: Wynik parsowania planu: status i lista instrukcji zrodlowych.
struct PlanSource {
  /// "OK" albo komunikat parsera dla pierwszej instrukcji, ktora sie nie powiodla.
  std::string status{"OK"};
  /// Pary (nazwa strumienia z zapisu, tresc instrukcji) w kolejnosci wystapienia.
  /// Zasila `processedLines` - diagnostyke `xqry -t` i sprzatanie artefaktow.
  std::vector<std::pair<std::string, std::string>> lines;
};

/// Funkcja parsePlanText powinna:
/// - dzielić tekst na instrukcje przez readLogicalLines i parsować je kolejno przez parserRQLString, z numerem pierwszego wiersza instrukcji i nazwą pliku sourceFile do logu błędów składni, dopisując węzły do plan.
///   Test: `PlanSource.statements_land_in_the_plan_and_in_the_line_list`, `PlanSource.syntax_error_names_the_file_line_and_keeps_earlier_statements`.
/// - dla każdej udanej instrukcji dopisywać do lines parę (nazwa strumienia z zapisu, treść instrukcji) w kolejności wystąpienia.
///   Test: `PlanSource.statements_land_in_the_plan_and_in_the_line_list`, `PlanSource.syntax_error_names_the_file_line_and_keeps_earlier_statements`.
/// - przy pierwszej nieudanej instrukcji przerywać i zwracać jej komunikat jako status, z lines obejmującym tylko instrukcje sprzed niej.
///   Test: `PlanSource.load_stops_at_the_first_bad_statement`, `PlanSource.syntax_error_names_the_file_line_and_keeps_earlier_statements`.
/// - dla tekstu pustego albo złożonego wyłącznie z komentarzy i pustych wierszy zwracać status "OK" i pustą listę instrukcji.
///   Test: `PlanSource.empty_text_is_an_empty_plan_not_an_error`, `PlanSource.comments_and_blank_lines_alone_are_an_empty_plan`.
///
/// @note Pusty plan nie jest tu bledem: dla uslugi to tryb bezczynny, dla `--onlycompile` blad - rozstrzyga
///       wolajacy. Funkcja jest wspolna dla startu z pliku (launcher) i przeladowania `reset` (executorsm),
///       bo rozjazd miedzy nimi bylby rozjazdem semantyki planu.
[[nodiscard]] PlanSource parsePlanText(qTree &plan, const std::string &text, std::string_view sourceFile = {});

/// Funkcja dropStalePlanArtifacts powinna:
/// - pomijać dyrektywy kompilatora i brać katalog z dyrektywy `:STORAGE` planu, bez katalogu domyślnego (pusty = katalog roboczy).
///   Test: `PlanSource.start_drops_whole_file_families_of_every_plan_node`, `PlanSource.start_drops_generator_instances_in_the_working_directory`.
/// - dla deklaracji kasować wyłącznie `<id>.desc` zapisany przed #346 z TYPE DEVICE, gdy plan deklaruje TYPE BINFILE, a REF i sloty się zgadzają; każdy inny `.desc` deklaracji, także przy błędzie jego sprawdzenia lub odczytu, zostawiać.
///   Test: `PlanSource.declared_descriptor_must_match_the_source_kind_and_path`, `PlanSource.unreadable_declaration_descriptor_does_not_throw`, `PlanSource.start_drops_generator_instances_in_the_working_directory`.
/// - dla pozostałych węzłów, także pośrednich i instancji generatora, kasować pełną rodzinę plików: `<id>.desc`, plik danych (nazwa z `FILE` albo id) z cieniem `.shadow`, `.meta` z cieniem oraz segmenty retencji `<nazwa>_segment_N` z cieniami.
///   Test: `PlanSource.start_drops_whole_file_families_of_every_plan_node`, `PlanSource.start_drops_generator_instances_in_the_working_directory`.
/// - przy dyrektywie `:ROTATION` zostawiać pliki węzłów plikowych i kasować tylko rodziny strumieni MEMORY (`VOLATILE` albo `STORAGE memory`).
///   Test: `PlanSource.rotation_drops_only_the_configuration_of_memory_streams`, `PlanSource.rotation_keeps_whole_file_families`.
/// - błąd usunięcia pliku logować ostrzeżeniem i iść dalej, a błąd odczytu katalogu kończyć tylko szukanie segmentów.
///   Test: `PlanSource.cleanup_errors_leave_files_but_do_not_stop_the_start` (pominiety pod root; tresci ostrzezenia nie sprawdza, bo Release wycina SPDLOG_WARN).
///
/// @note Katalog domyslny konfiguracji dopisuje do planu wolajacy. Przy `:ROTATION` poprzednie przebiegi
///       sa danymi, a `.desc` i `.meta` strumienia MEMORY to konfiguracja POPRZEDNIEGO przebiegu (magazyn
///       bierze TYPE i RETMEMORY z pliku). Do 2026-09-27 znikaly tylko `<id>`, `<id>.desc` i `<id>.meta`:
///       stare segmenty wracaly pod swieze metadane ("record count mismatch"), a pliki wezlow posrednich rosly.
void dropStalePlanArtifacts(const qTree &plan);

/// Funkcja checkKeptStores powinna:
/// - dla planu bez dyrektywy `:ROTATION` zwracać pustą odmowę.
///   Test: `PlanSource.rotation_refuses_kept_files_written_with_another_configuration`.
/// - badać tylko węzły wynikowe, z pominięciem deklaracji, dyrektyw i strumieni MEMORY, a przy niepustej liście nazw tylko węzły z listy.
///   Test: `PlanSource.kept_store_check_scope_and_error_classes`.
/// - pomijać węzeł bez pliku `<id>.desc` w katalogu planStorageDir(plan, defaultStorageDir).
///   Test: `PlanSource.kept_store_check_scope_and_error_classes`.
/// - odrzucać z klasą Plan `.desc`, który nie jest zwykłym plikiem ani dowiązaniem, a błąd lstat inny niż ENOENT zwracać z klasą Environment i errno.
///   Test: `PlanSource.kept_store_check_scope_and_error_classes`.
/// - zwracać odmowę wczytania `.desc`, gdy pliku nie da się wczytać.
///   Test: `PlanSource.rotation_refuses_kept_files_written_with_another_configuration`.
/// - porównywać konfigurację magazynu z pliku (TYPE z `.desc`, a bez niego STORAGE planu, oraz RETENTION) z planowaną i przy różnicy zwracać odmowę klasy Plan nazywającą strumień, plik i obie konfiguracje.
///   Test: `PlanSource.rotation_refuses_kept_files_written_with_another_configuration`, `PlanSource.kept_store_check_scope_and_error_classes`.
/// - zwracać pierwszą znalezioną odmowę, a pustą, gdy wszystkie zachowane magazyny zgadzają się z planem.
///   Test: `PlanSource.kept_store_check_scope_and_error_classes`, `PlanSource.rotation_refuses_kept_files_written_with_another_configuration`.
///
/// @note TYPE i RETENTION z zachowanego `.desc` wygrywaja w magazynie z planem, a zmiana pojemnosci przy
///       zachowanych segmentach przestawilaby adresowanie (groupFile liczy segment jako indeks / capacity).
///       Dlatego rozjazd jest odmowa, zanim cokolwiek zostanie zmienione (decyzja P5). Bez `:ROTATION`
///       start kasuje rodziny plikow.
[[nodiscard]] rdb::Refusal checkKeptStores(qTree &plan, std::string_view defaultStorageDir,
                                           const std::vector<std::string> &streamNames = {});

/// Funkcja checkDescriptorFiles powinna:
/// - pomijać dyrektywy; przy pustej liście nazw badać deklaracje zawsze, a węzły wynikowe tylko przy `:ROTATION` i poza strumieniami MEMORY; przy niepustej liście badać każdy węzeł z listy.
///   Test: `PlanSource.kept_select_descriptor_must_not_move_the_data_file`, `PlanSource.descriptor_check_can_be_limited_to_named_streams`, `PlanSource.adhoc_checks_retained_select_descriptors_and_only_the_new_streams`, `PlanSource.descriptor_file_check_scope_and_error_classes`.
/// - pomijać węzeł bez pliku `<id>.desc` w katalogu planStorageDir(plan, defaultStorageDir), a `.desc` niebędący zwykłym plikiem ani dowiązaniem, błąd lstat i błąd wczytania zgłaszać jak checkKeptStores.
///   Test: `PlanSource.descriptor_file_check_scope_and_error_classes`, `PlanSource.invalid_declared_descriptor_is_rejected_before_plan_activation`.
/// - odrzucać z klasą Plan TYPE z pliku spoza typów deklaracji (dla deklaracji) albo spoza typów zapisywalnych (dla węzła wynikowego).
///   Test: `PlanSource.adhoc_checks_retained_select_descriptors_and_only_the_new_streams`, `PlanSource.descriptor_file_check_scope_and_error_classes`.
/// - dla TYPE TEXTSOURCE odrzucać z klasą Plan pole o niezerowej długości typu innego niż BYTE, INTEGER, UINT, FLOAT, DOUBLE i STRING.
///   Test: `PlanSource.text_source_refuses_a_kept_rational_even_when_its_schema_fits_double`.
/// - odrzucać z klasą Plan rozjazd schematu, porównując deskryptor planu z deskryptorem pliku w kierunku plan == plik.
///   Test: `PlanSource.declared_descriptor_check_uses_the_runtime_direction`, `PlanSource.adhoc_checks_retained_select_descriptors_and_only_the_new_streams`.
/// - dla węzła wynikowego odrzucać z klasą Plan każde niepuste REF w pliku.
///   Test: `PlanSource.kept_select_descriptor_must_not_move_the_data_file`, `PlanSource.descriptor_file_check_scope_and_error_classes`.
/// - dla deklaracji przepuszczać tylko dawny TYPE DEVICE (sprzed #346) zwykłego pliku binarnego, gdy plan deklaruje TYPE BINFILE, a REF i sloty się zgadzają, a poza nim odrzucać z klasą Plan różnicę TYPE albo REF między plikiem a planem.
///   Test: `PlanSource.declared_descriptor_must_match_the_source_kind_and_path`, `PlanSource.descriptor_file_check_scope_and_error_classes`.
/// - zwracać pierwszą znalezioną odmowę, a pustą, gdy wszystkie pliki zgadzają się z planem.
///   Test: `PlanSource.descriptor_file_check_scope_and_error_classes`, `PlanSource.descriptor_check_can_be_limited_to_named_streams`.
///
/// @note Magazyn bierze TYPE i REF z pliku, nie z planu (#346): do tej zmiany plan wskazywal `w.txt`, a silnik
///       czytal `v.txt` sprzed zmiany. Przepuszczony dawny TYPE DEVICE zastepuje dropStalePlanArtifacts().
///       Ad-hoc przekazuje tylko nowo dodawane strumienie, ktorych deskryptory zostaja takze bez ROTATION i dla
///       MEMORY (#473). REF w `.desc` wyniku przenioslby zapis tam, gdzie plan nie pisze (#278).
///       Descriptor::operator== jest asymetryczny, stad kierunek jak w rdb::verifyDescriptorMatch.
[[nodiscard]] rdb::Refusal checkDescriptorFiles(qTree &plan, std::string_view defaultStorageDir,
                                                const std::vector<std::string> &streamNames = {});

/// Funkcja checkDeclaredSources powinna:
/// - badać tylko deklaracje, a przy niepustej liście nazw tylko deklaracje z listy.
///   Test: `PlanSource.declared_source_kind_is_checked_without_opening_the_path`, `PlanSource.declared_source_check_scope_classes_and_deprecated_hints`.
/// - dla TYPE TEXTSOURCE odrzucać z klasą Plan pole o niezerowej długości typu innego niż BYTE, INTEGER, UINT, FLOAT, DOUBLE i STRING.
///   Nietestowalne: gramatyka DECLARE dopuszcza tylko BYTE, STRING, UINT, INTEGER, FLOAT i DOUBLE, wiec plan z parsera nie niesie odrzucanego pola; te sama kontrole pol dla zachowanego `.desc` sprawdza `PlanSource.text_source_refuses_a_kept_rational_even_when_its_schema_fits_double`.
/// - sprawdzać rodzaj pliku pod ścieżką przez stat(), bez otwierania; brak pliku (ENOENT, ENOTDIR) przepuszczać, a inny błąd stat zwracać z klasą Environment i errno.
///   Test: `PlanSource.declared_source_kind_is_checked_without_opening_the_path`, `PlanSource.declared_source_check_scope_classes_and_deprecated_hints`.
/// - odrzucać z klasą Plan, z nazwą strumienia i ścieżką, BINFILE albo TEXTFILE, które nie wskazują zwykłego pliku, oraz DEVICE, które nie wskazuje urządzenia znakowego ani FIFO.
///   Test: `PlanSource.declared_source_kind_is_checked_without_opening_the_path`, `PlanSource.declared_source_check_scope_classes_and_deprecated_hints`.
/// - dla deklaracji w przestarzałej formie `FILE` dopisywać do odmowy słowo wybrane przez regułę, a gdy pod ścieżką leży urządzenie znakowe, FIFO albo zwykły plik - także pasujące słowo (DEVICE albo "BINFILE or TEXTFILE").
///   Test: `PlanSource.declared_source_kind_is_checked_without_opening_the_path`, `PlanSource.declared_source_check_scope_classes_and_deprecated_hints`.
///
/// @note stat() zamiast otwarcia, wiec FIFO bez pisarza niczego nie wiesza. Brak zrodla nadal daje NULL (#346).
///       Nie w `-c` - kompilacja nie musi biec na maszynie z danymi.
[[nodiscard]] rdb::Refusal checkDeclaredSources(const qTree &plan, const std::vector<std::string> &streamNames = {});

/// Funkcja checkStorageDirectory powinna:
/// - dla planu złożonego wyłącznie z dyrektyw oraz dla pustego katalogu (katalog roboczy) zwracać pustą odmowę.
///   Test: `PlanSource.storage_directory_refusal_precedes_all_storage_operations`, `PlanSource.storage_directory_check_edges`.
/// - sprawdzać katalog planStorageDir(plan, defaultStorageDir) bez końcowych separatorów (poza samym "/"), także gdy wszystkie strumienie są MEMORY, i nigdy go nie tworzyć.
///   Test: `PlanSource.storage_directory_refusal_precedes_all_storage_operations`, `PlanSource.storage_directory_check_edges`.
/// - dla nieistniejącego katalogu zwracać odmowę klasy Plan z ENOENT, ścieżką bezwzględną i podpowiedzią `mkdir -p`.
///   Test: `PlanSource.storage_directory_refusal_precedes_all_storage_operations`, `PlanSource.storage_directory_check_edges`.
/// - dla ścieżki, która nie jest katalogiem (także ENOTDIR ze stat), zwracać odmowę klasy Plan, a dla innego błędu stat odmowę klasy Environment, w obu przypadkach z errno.
///   Test: `PlanSource.storage_directory_refusal_precedes_all_storage_operations`, `PlanSource.storage_directory_check_edges`.
[[nodiscard]] rdb::Refusal checkStorageDirectory(const qTree &plan, std::string_view defaultStorageDir);

/// Funkcja checkPlanStorage powinna:
/// - wykonywać kolejno checkStorageDirectory, checkKeptStores, checkDescriptorFiles, checkDeclaredSources i checkOutputFilesOpenable z tymi samymi argumentami i zwracać pierwszą niepustą odmowę.
///   Test: `PlanSource.storage_directory_refusal_precedes_all_storage_operations`, `PlanSource.plan_storage_check_order_and_counter_left_to_the_caller`, `PlanSource.adhoc_checks_retained_select_descriptors_and_only_the_new_streams`.
/// - nie sprawdzać licznika rotacji, który wołający sprawdza osobno przez checkRotationCounter.
///   Test: `PlanSource.plan_storage_check_order_and_counter_left_to_the_caller`.
///
/// @note Wspolny preflight czterech drog (#473), przed kasowaniem plikow, importem i rezerwacja. Pusta lista
///       oznacza wymiane calego planu; lista nazw - nowe strumienie ad-hoc.
[[nodiscard]] rdb::Refusal checkPlanStorage(qTree &plan, std::string_view defaultStorageDir,
                                            const std::vector<std::string> &streamNames = {});

/// Wynik checkRotationCounter(): pusty `refusal` = plan bez `:ROTATION` albo licznik uzyteczny.
/// @requirements-exempt CounterCheck: Wynik sprawdzenia licznika rotacji; sprawdzenie realizuje funkcja modulu.
struct CounterCheck {
  bool environment{false};  ///< true: pliku nie da sie sprawdzic ani odczytac (K4); false: wadliwa tresc (K2)
  std::string refusal;
};

/// Funkcja checkRotationCounter powinna:
/// - dla planu bez dyrektywy `:ROTATION` ze ścieżką zwracać pusty wynik.
///   Test: `PlanSource.rotation_counter_preflight_is_read_only_and_preserves_error_class`.
/// - tylko odczytywać licznik przez PersistentCounter::read, bez rezerwowania numeru.
///   Test: `PlanSource.rotation_counter_preflight_is_read_only_and_preserves_error_class`.
/// - dla brakującego pliku i dla poprawnej wartości zwracać pusty wynik.
///   Test: `PlanSource.rotation_counter_preflight_is_read_only_and_preserves_error_class`.
/// - dla pliku, którego nie da się odczytać, zwracać environment = true i odmowę ze ścieżką i przyczyną (K4).
///   Test: `PlanSource.rotation_counter_preflight_is_read_only_and_preserves_error_class`.
/// - dla wadliwej treści zwracać environment = false i odmowę ze ścieżką, przyczyną i instrukcją naprawy albo usunięcia pliku (K2).
///   Test: `PlanSource.rotation_counter_preflight_is_read_only_and_preserves_error_class`.
///
/// @note #472: ta sama odmowa co checkKeptStores, zanim cokolwiek zostanie zmienione, bo konstruktor
///       PersistentCounter biegnie juz po punkcie bez powrotu. Start reaguje na obie przyczyny inaczej:
///       wadliwa tresc to odmowa planu, nieczytelny plik - awaria srodowiska z planem zachowanym.
///       Nie w `-c` - kompilacja nie musi biec na maszynie z danymi.
[[nodiscard]] CounterCheck checkRotationCounter(const qTree &plan);

/// Funkcja deprecatedFileWarnings powinna:
/// - zwracać po jednym ostrzeżeniu na deklarację w przestarzałej formie `DECLARE ... FILE` (#346), z numerem wiersza, nazwą strumienia, ścieżką i słowem wybranym przez zamrożoną regułę.
///   Test: `PlanSource.deprecated_file_warnings_follow_the_plan_lines`.
/// - porządkować ostrzeżenia według wiersza deklaracji w planie, nie według kolejności węzłów.
///   Test: `PlanSource.deprecated_file_warnings_follow_the_plan_lines`.
/// - przy niepustej liście nazw ograniczać się do strumieni z listy.
///   Test: `PlanSource.deprecated_file_warnings_follow_the_plan_lines`.
///
/// @note Kompilator sortuje wezly po interwale, stad sortowanie po wierszu. Wypisuje je tylko wolajacy z
///       --verbose: bez niego stare plany, testy i narzedzia pomiarowe nie zmieniaja wyjscia. Ad-hoc
///       przekazuje tylko nowo dodawane strumienie.
[[nodiscard]] std::vector<std::string> deprecatedFileWarnings(const qTree &plan,
                                                              const std::vector<std::string> &streamNames = {});

/// Funkcja deviceTimeoutWarnings powinna:
/// - zwracać po jednym ostrzeżeniu na źródło DEVICE, którego efektywny TIMEOUT (effectiveDeviceTimeout z configured, w trybie z zegarem) jest ściśle dłuższy od interwału strumienia, z wierszem, nazwą, terminem, jego pochodzeniem i interwałem.
///   Test: `PlanSource.device_timeout_warnings_use_the_effective_timeout`.
/// - porządkować ostrzeżenia według wiersza deklaracji.
///   Test: `PlanSource.device_timeout_warnings_follow_the_plan_lines`.
/// - przy niepustej liście nazw ograniczać się do strumieni z listy.
///   Test: `PlanSource.device_timeout_warnings_use_the_effective_timeout`.
///
/// @note #347: czekanie dluzsze od interwalu przekracza slot. @p configured pochodzi z `[sources] timeout_s`.
[[nodiscard]] std::vector<std::string> deviceTimeoutWarnings(const qTree &plan, std::optional<double> configured,
                                                             const std::vector<std::string> &streamNames = {});

/// Funkcja deviceTimeoutReport powinna:
/// - zwracać po jednym wpisie na każde źródło DEVICE z efektywnym terminem i jego pochodzeniem z effectiveDeviceTimeout ("RQL", "config", "default" albo "no-clock" przy noClock), w kolejności węzłów planu.
///   Test: `PlanSource.timeout_report_covers_sources_filters_and_no_clock`.
/// - przy niepustej liście nazw ograniczać się do strumieni z listy.
///   Test: `PlanSource.timeout_report_covers_sources_filters_and_no_clock`.
///
/// @note #347: wpis do dziennika przy starcie epoki i przy imporcie ad-hoc - operator widzi, z jakim terminem
///       naprawde czyta kazde zrodlo.
[[nodiscard]] std::vector<std::string> deviceTimeoutReport(const qTree &plan, std::optional<double> configured, bool noClock,
                                                           const std::vector<std::string> &streamNames = {});

/// Funkcja planStreamNames powinna:
/// - zwracać identyfikatory wszystkich węzłów planu poza dyrektywami kompilatora, w kolejności planu.
///   Test: `PlanSource.directives_do_not_claim_stream_names`, `PlanSource.path_helpers_follow_directives_and_normalize`.
///
/// @note To nazwy, ktore plan ROSCI na magistrali.
[[nodiscard]] std::vector<std::string> planStreamNames(const qTree &plan);

/// Funkcja planCounterPath powinna:
/// - zwracać absolutePathOf ścieżki z pierwszej dyrektywy `:ROTATION` o niepustej ścieżce, a pusty napis, gdy takiej nie ma.
///   Test: `PlanSource.path_helpers_follow_directives_and_normalize`, `PlanSource.directives_do_not_claim_stream_names`.
[[nodiscard]] std::string planCounterPath(const qTree &plan);

/// Funkcja planStorageDir powinna:
/// - zwracać ścieżkę z dyrektywy `:STORAGE` planu (przy kilku - z ostatniej), a bez niej defaultStorageDir.
///   Test: `PlanSource.default_storage_dir_yields_to_the_storage_directive`, `PlanSource.path_helpers_follow_directives_and_normalize`.
///
/// @note defaultStorageDir to domyslny katalog z konfiguracji. Pusty wynik znaczy "katalog roboczy".
[[nodiscard]] std::string planStorageDir(const qTree &plan, std::string_view defaultStorageDir);

/// Funkcja planStorePaths powinna:
/// - zwracać ścieżki plików danych węzłów wynikowych, znormalizowane przez absolutePathOf, w katalogu planStorageDir(plan, defaultStorageDir) i pod nazwą z `FILE` albo identyfikatorem węzła.
///   Test: `PlanSource.stores_follow_the_file_clause_and_the_storage_directive`, `PlanSource.default_storage_dir_yields_to_the_storage_directive`.
/// - pomijać dyrektywy, deklaracje i strumienie MEMORY (`VOLATILE` albo `STORAGE memory`).
///   Test: `PlanSource.stores_follow_the_file_clause_and_the_storage_directive`, `PlanSource.memory_streams_do_not_claim_a_store`.
/// - pomijać powtórzenia ścieżki, zachowując kolejność pierwszego wystąpienia.
///   Test: `PlanSource.aliased_streams_claim_one_store_once`.
/// - nie uwzględniać przestawienia pliku magazynu przez pole REF istniejącego `.desc`.
///   Test: `PlanSource.path_helpers_follow_directives_and_normalize`.
///
/// @param defaultStorageDir katalog uzywany, gdy plan nie niesie dyrektywy `:STORAGE` -- ta sama
///        wartosc, ktora launcher dokleja z konfiguracji (`[storage] dir`). Bez niej rezerwacja
///        wskazywalaby katalog roboczy, a plan pisalby gdzie indziej.
///
/// @note Nazwa strumienia chroni tylko deskryptor `<id>.desc`; plik danych bierze `qry.filename` przy klauzuli
///       `FILE`, wiec dwa plany o rozlacznych nazwach moga wskazywac jeden magazyn. Deklaracje sa zrodlami
///       TYLKO DO ODCZYTU (wiele serwerow czytajacych jeden plik jest poprawne - it_multiserver_uniqueness),
///       a `rdb::memoryFile` nie dotyka systemu plikow. Powtorzenie nie wnosi nic, bo roszczenie porownuje
///       wylacznie instancje. ZNANY LIMIT: REF (rdb::StoragePaths::relocateFromRef) z planu nie widac, a
///       czytanie deskryptorow przy roszczeniu byloby operacja dyskowa w sciezce atomowej i krotkiej.
[[nodiscard]] std::vector<std::string> planStorePaths(const qTree &plan, std::string_view defaultStorageDir);

/// Funkcja checkOutputFilesOpenable powinna:
/// - badać węzły wynikowe, z pominięciem dyrektyw, deklaracji i strumieni MEMORY, a przy niepustej liście nazw tylko węzły z listy.
///   Test: `PlanSource.output_preflight_scope_segments_shadows_and_permissions`.
/// - sprawdzać plik `<katalog>/<FILE albo id>`, przy TYPE DEFAULT i DIRECT z RETENTION jego segment `_segment_0`, a przy DEFAULT i POSIXSHD także cień `.shadow`.
///   Test: `PlanSource.output_preflight_scope_segments_shadows_and_permissions`.
/// - istniejący plik otwierać jak akcesor (O_RDWR, O_NOFOLLOW, O_NONBLOCK) i zamykać, odrzucając z klasą Plan plik, który nie jest zwykłym plikiem, a błąd fstat lub close zwracając z klasą Environment i errno.
///   Test: `PlanSource.output_preflight_scope_segments_shadows_and_permissions`; bledow fstat i close na otwartym deskryptorze nie da sie wywolac z testu - te dwie galezie zostaja bez testu.
/// - błąd otwarcia inny niż ENOENT zwracać z errno, z klasą Plan dla EISDIR i ENOTDIR, a Environment dla pozostałych.
///   Test: `PlanSource.output_preflight_distinguishes_plan_and_environment_without_creating_files`, `PlanSource.output_preflight_scope_segments_shadows_and_permissions`.
/// - nie tworzyć pliku: dla nieistniejącego odrzucać z klasą Plan brak katalogu nadrzędnego albo rodzica niebędącego katalogiem, a z klasą Environment inny błąd stat rodzica i brak prawa zapisu i przeszukania w nim.
///   Test: `PlanSource.output_preflight_distinguishes_plan_and_environment_without_creating_files`, `PlanSource.output_preflight_scope_segments_shadows_and_permissions`; rodzic niebedacy katalogiem i inny blad stat rodzica sa nieosiagalne bez wyscigu, bo open() zglasza wczesniej ENOTDIR albo EACCES.
/// - zwracać pierwszą znalezioną odmowę, a pustą, gdy każdy plik da się otworzyć albo utworzyć.
///   Test: `PlanSource.output_preflight_distinguishes_plan_and_environment_without_creating_files`, `PlanSource.output_preflight_scope_segments_shadows_and_permissions`.
///
/// @note Brak tworzenia pliku chroni nosnik embedded przed zapisem przy kazdym resecie. K2: wadliwa sciezka
///       lub rodzaj pliku; K4: system odmowil operacji (z zachowanym errno).
[[nodiscard]] rdb::Refusal checkOutputFilesOpenable(const qTree &plan, std::string_view defaultStorageDir,
                                                    const std::vector<std::string> &streamNames = {});

/// Funkcja absolutePathOf powinna:
/// - dla pustej ścieżki zwracać pusty napis.
///   Test: `PlanSource.path_helpers_follow_directives_and_normalize`.
/// - zwracać weakly_canonical(absolute(path)), a przy błędzie absolute samo path, przy błędzie weakly_canonical - ścieżkę bezwzględną.
///   Test: `PlanSource.path_helpers_follow_directives_and_normalize`.
///
/// @note Normalizuje sciezke publikowana w slocie magistrali. absolute() PRZED weakly_canonical(): plik
///       licznika przy pierwszym starcie jeszcze nie istnieje, a weakly_canonical nad nieistniejaca sciezka
///       wzgledna zwraca ja bez katalogu roboczego, o ktory w tej normalizacji chodzi.
[[nodiscard]] std::string absolutePathOf(const std::string &path);
