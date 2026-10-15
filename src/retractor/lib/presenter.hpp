#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "qTree.hpp"  // for qTree, query, token
#include "rdb/error.hpp"

/// Obiekt klasy presenter powinien:
/// - działać na planie przekazanym przy konstrukcji, bez zmiany jego zawartości.
/// - w run odrzucać tags bez fields kodem invalid_argument, a następnie wybierać jeden tryb wydruku w kolejności: dot, csv, diagram, a bez nich wydruk planu skompilowanego.
/// - w trybie dot wypisywać graf Graphviz z węzłem na każdy strumień planu poza dyrektywami, z kolorem zależnym od rodzaju (BINFILE, TEXTFILE, DEVICE, strumień generowany, SELECT), interwałem w etykiecie i krawędziami zależności, a pola, programy strumieni, tagi i reguły dołączać według opcji.
/// - w trybie csv wypisywać kolejno tabele strumieni, programów FROM, pól, programów pól i reguł, rozdzielane tabulatorami, z nagłówkiem każdej tabeli.
/// - w trybie diagram przyjmować argument <typ siatki>[:<liczba cykli>] z typem 0 lub 1, odrzucać brak lub niepoprawny typ kodem invalid_argument, a plan bez osi czasu zgłaszać jako FATAL z kodem EPERM.
/// - w wydruku planu skompilowanego wypisywać dla każdego węzła nazwę, interwał (poza dyrektywami), ogon tail i origin tylko gdy są dodatnie, plik źródła i jawny TIMEOUT, a następnie tokeny programu FROM, pola z typem i programem, tabelę grup okien rekordowych oraz reguły z warunkiem i akcją DUMP lub SYSTEM.
/// - nie zmieniać wydruku planów bez ogona, origin, TIMEOUT i okien rekordowych przy dodawaniu takich informacji.
/// - zwracać kod interrupted z opisem na stderr dla wyjątku bibliotek zewnętrznych, a success po udanym wydruku.
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
