#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "qTree.hpp"  // for qTree, query, token
#include "rdb/error.hpp"

/// Obiekt klasy presenter powinien:
/// - [presenter.presenter.1~1] działać na planie przekazanym przy konstrukcji, bez zmiany jego zawartości.
///   Ograniczenie: rewizja i wydruk planu sa asercjami; brak zapisu do pozostalych skladowych kontroluje przeglad metod presentera.
/// - [presenter.presenter.2~1] w run odrzucać tags bez fields kodem invalid_argument, a następnie wybierać jeden tryb wydruku w kolejności: dot, csv, diagram, a bez nich wydruk planu skompilowanego.
/// - [presenter.presenter.3~1] w trybie dot wypisywać graf Graphviz z węzłem na każdy strumień planu poza dyrektywami, z kolorem zależnym od rodzaju (BINFILE, TEXTFILE, DEVICE, strumień generowany, SELECT), interwałem w etykiecie i krawędziami zależności, a pola, programy strumieni, tagi i reguły dołączać według opcji.
/// - [presenter.presenter.4~1] w trybie csv wypisywać kolejno tabele strumieni, programów FROM, pól, programów pól i reguł, rozdzielane tabulatorami, z nagłówkiem każdej tabeli.
/// - [presenter.presenter.5~1] w trybie diagram przyjmowac argument <typ siatki>[:<liczba cykli>], w ktorym typ siatki jest w calosci liczba 0 albo 1, a podana liczba cykli w calosci liczba calkowita >= 1; brak typu albo tekst niebedacy taka liczba odrzucac kodem invalid_argument.
/// - [presenter.presenter.6~1] w trybie diagram plan bez strumieni, przed jakimkolwiek rachunkiem siatki, oraz niedodatnia liczbe krokow cyklu albo liczbe cykli przepelniajaca liczbe krokow zglaszac jako FATAL z kodem EPERM.
///   Nietestowalne: niedodatnia liczba krokow i przepelnienie liczby krokow to kontrola strukturalna sequenceDiagram w presenter.cpp; plan o dodatnich interwalach nie daje niedodatniej liczby krokow.
/// - [presenter.presenter.7~1] w wydruku planu skompilowanego wypisywać dla każdego węzła nazwę, interwał (poza dyrektywami), ogon tail i origin tylko gdy są dodatnie, plik źródła i jawny TIMEOUT, a następnie tokeny programu FROM, pola z typem i programem, tabelę grup okien rekordowych oraz reguły z warunkiem i akcją DUMP lub SYSTEM.
/// - [presenter.presenter.8~1] nie zmieniać wydruku planów bez ogona, origin, TIMEOUT i okien rekordowych przy dodawaniu takich informacji.
/// - [presenter.presenter.9~1] zwracać kod interrupted z opisem na stderr dla wyjątku bibliotek zewnętrznych, a success po udanym wydruku.
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
