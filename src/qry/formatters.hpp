#pragma once
#include <deque>
#include <string>
#include <tuple>
#include <vector>

#include <boost/property_tree/ptree.hpp>

enum class formatMode : std::uint8_t { RAW, GRAPHITE, INFLUXDB, GNUPLOT };

/// Obiekt klasy Formatter powinien:
/// - przez isNullAt uznawać wartość za null wyłącznie, gdy znak mapy null na tej pozycji jest równy '1', a przez isAllNull
///   zwracać prawdę tylko dla dodatniej liczby wartości, z których wszystkie są null.
/// - przez displayedValue zwracać wartość z wiersza albo dla null tekst NaN w trybie gnuplot i null w pozostałych trybach.
/// - przez renderRaw wypisywać wartości wiersza rozdzielone spacją i zakończone CRLF, pomijając wiersz złożony wyłącznie z null, gdy wołający o to prosi.
/// - przez initGnuplot wypisywać ustawienia terminala i zakresów osi, z osią x malejącą w trybie od prawej do lewej.
/// - przez renderGnuplot przechowywać dla każdej kolumny okno ostatnich wartości o szerokości z wymiaru x, najnowszą
///   na pozycji 0, i wypisywać polecenie plot z jedną krzywą na każdą wartość wiersza i jednym blokiem danych na krzywą.
///   Tytuły budować z pól db.field rozwiniętych po jednym elemencie na kolumnę według krotności
///   db.field_count.<pole> (domyślnie 1), jako pole[i] dla tablic, a przy krótszym schemacie używać col<i>;
///   w nazwach tytułów zamieniać podkreślenia na myślniki.
/// - przez renderGnuplotOhlc traktować cztery pierwsze wartości jako open, high, low i close, a dalsze jako próbki świecy, rysować świece z pominięciem niepełnych i próbki na wspólnej osi, a dla wiersza bez próbek jednorazowo zgłaszać na stderr niewłaściwy kształt strumienia i nic nie rysować.
/// - przez renderGraphite rozwijać pola db.field w kolumny według krotności db.field_count.<pole> (domyślnie 1)
///   i dla każdej wartości nieoznaczonej jako null, także pustego tekstu, wypisywać wiersz
///   <strumień>.<metryka> <wartość> <czas uniksowy>, z metryką pole.<i> dla elementów tablic i nazwą pola
///   dla skalarów; wartości wiersza poza kolumnami schematu pomijać.
/// - przez renderInfluxDB rozwijać pola db.field w kolumny według krotności db.field_count.<pole> (domyślnie 1)
///   i wypisywać jeden wiersz line protocol z wartościami tych kolumn nieoznaczonymi jako null, także pustymi
///   tekstami, jako pole=wartość, z kluczem pole_<i> dla elementów tablic i znacznikiem czasu w nanosekundach;
///   wartości wiersza poza kolumnami schematu pomijać, a gdy żadna kolumna nie ma wartości nieoznaczonej jako null,
///   nie wypisywać niczego.
class Formatter {
  std::vector<std::deque<std::string>> gnuplot_lines_;
  bool ohlcShapeReported_{false};
  static const std::vector<std::string> colors_;

  /// Jedna WARTOSC wiersza: pole, ktore ja niesie, jej pozycja w tym polu i krotnosc pola.
  /// Serwer splaszcza tablice (jedna wartosc na element, nie na pole), wiec sama lista pol
  /// ze schematu przestala opisywac wiersz -- kolumny sa jej rozwinieciem.
  /// @requirements-exempt Column: Opis kolumny odpowiedzi: pole, indeks elementu i szerokosc.
  struct Column {
    std::string field;
    int index;
    int width;
  };

  /// Splaszczone kolumny odpowiedzi 'detail'. Krotnosc pola przychodzi w
  /// db.field_count.<pole>; starszy serwer jej nie wysyla i wtedy pole liczy sie jako
  /// jednoelementowe -- klient nadal dziala, tylko nie rozwija tablic.
  static std::vector<Column> columns(const boost::property_tree::ptree &schema);

 public:
  static bool isNullAt(const std::string &nullmap, int index);
  static bool isAllNull(const std::string &nullmap, int count);
  static std::string displayedValue(const boost::property_tree::ptree &row, int index, const std::string &nullmap,
                                    formatMode mode);

  static void initGnuplot(std::tuple<int, int, int> dim, bool rightToLeft);
  static void renderRaw(const boost::property_tree::ptree &row, int count, const std::string &nullmap, bool skipNull);
  void renderGnuplot(const boost::property_tree::ptree &row, int count, const std::string &nullmap, const std::string &input,
                     const boost::property_tree::ptree &schema, std::tuple<int, int, int> dim);
  /// Tryb swiecowy (--gnuplot-ohlc): wiersz to open, high, low, close i dalej probki, ktore
  /// swieca obejmuje, w kolejnosci naplywu. Rysuje swiece nad jej probkami na wspolnej osi.
  void renderGnuplotOhlc(const boost::property_tree::ptree &row, int count, const std::string &nullmap, const std::string &input,
                         std::tuple<int, int, int> dim);
  static void renderGraphite(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
  static void renderInfluxDB(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
};
