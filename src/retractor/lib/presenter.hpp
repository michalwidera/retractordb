#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "qTree.hpp"  // for qTree, query, token
#include "rdb/error.hpp"

/// Obiekt klasy presenter powinien:
/// - działać na planie przekazanym przy konstrukcji, bez zmiany jego zawartości.
///   Test: `presenter.mode_precedence_and_read_only_plan_are_observable`. Ograniczenie: rewizja i wydruk planu sa asercjami; brak zapisu do pozostalych skladowych kontroluje przeglad metod presentera.
/// - w run odrzucać tags bez fields kodem invalid_argument, a następnie wybierać jeden tryb wydruku w kolejności: dot, csv, diagram, a bez nich wydruk planu skompilowanego.
///   Test: `presenter.tags_without_fields_returns_error`, `presenter.mode_precedence_and_read_only_plan_are_observable`.
/// - w trybie dot wypisywać graf Graphviz z węzłem na każdy strumień planu poza dyrektywami, z kolorem zależnym od rodzaju (BINFILE, TEXTFILE, DEVICE, strumień generowany, SELECT), interwałem w etykiecie i krawędziami zależności, a pola, programy strumieni, tagi i reguły dołączać według opcji.
///   Test: `presenter.dot_colors_dependencies_and_optional_details_are_printed`, `presenter.listing_contains_boundaries_timeouts_windows_and_both_rule_actions`, `it_dot_labels-dot`.
/// - w trybie csv wypisywać kolejno tabele strumieni, programów FROM, pól, programów pól i reguł, rozdzielane tabulatorami, z nagłówkiem każdej tabeli.
///   Test: `presenter.csv_tables_have_ordered_headers_and_tab_separated_rows`.
/// - w trybie diagram przyjmowac argument <typ siatki>[:<liczba cykli>], w ktorym typ siatki jest w calosci liczba 0 albo 1, a podana liczba cykli w calosci liczba calkowita >= 1; brak typu albo tekst niebedacy taka liczba odrzucac kodem invalid_argument.
///   Test: `presenter.diagram_empty_grid_type_returns_error`, `presenter.diagram_invalid_grid_type_returns_error`, `presenter.diagram_rejects_non_numeric_grid_type_and_cycle_count`.
/// - w trybie diagram plan bez strumieni, przed jakimkolwiek rachunkiem siatki, oraz niedodatnia liczbe krokow cyklu albo liczbe cykli przepelniajaca liczbe krokow zglaszac jako FATAL z kodem EPERM.
///   Test: `presenter.empty_diagram_is_fatal_eperm`. Nietestowalne: niedodatnia liczba krokow i przepelnienie liczby krokow to kontrola strukturalna sequenceDiagram w presenter.cpp; plan o dodatnich interwalach nie daje niedodatniej liczby krokow.
/// - w wydruku planu skompilowanego wypisywać dla każdego węzła nazwę, interwał (poza dyrektywami), ogon tail i origin tylko gdy są dodatnie, plik źródła i jawny TIMEOUT, a następnie tokeny programu FROM, pola z typem i programem, tabelę grup okien rekordowych oraz reguły z warunkiem i akcją DUMP lub SYSTEM.
///   Test: `presenter.listing_contains_boundaries_timeouts_windows_and_both_rule_actions`, `it_dot_labels-compile`.
/// - nie zmieniać wydruku planów bez ogona, origin, TIMEOUT i okien rekordowych przy dodawaniu takich informacji.
///   Test: `presenter.plain_listing_omits_optional_annotations_byte_for_byte`.
/// - zwracać kod interrupted z opisem na stderr dla wyjątku bibliotek zewnętrznych, a success po udanym wydruku.
///   Test: `presenter.external_option_type_error_returns_interrupted_and_diagnostic`, `presenter.default_mode_returns_success`.
struct presenter {
  explicit presenter(qTree &coreInstance) : coreInstance(coreInstance) {};
  presenter() = delete;

  int run(const boost::program_options::variables_map &vm);

 private:
  qTree &coreInstance;

  void graphiz(std::ostream &xout, const boost::program_options::variables_map &vm);
  void qFieldsProgram();
  void qFields();
  void qPrograms();
  void qSet();
  void qRules();
  void onlyCompileShowProgram();
  /// Errc::Config, gdy plan nie daje osi czasu (interwal zero, brak strumieni).
  [[nodiscard]] rdb::Result<> sequenceDiagram(int gridType, int cycleCount);
};
