#include "formatters.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <print>
#include <sstream>
#include <utility>

using boost::property_tree::ptree;

const std::vector<std::string> Formatter::colors_ = {"red",   "blue", "green",  "orange", "purple",
                                                     "brown", "pink", "yellow", "cyan",   "magenta"};

bool Formatter::isNullAt(const std::string &nullmap, int index) {
  return index >= 0 && std::cmp_less(index, nullmap.size()) && nullmap[static_cast<std::size_t>(index)] == '1';
}

bool Formatter::isAllNull(const std::string &nullmap, int count) {
  if (count <= 0) return false;
  for (int i = 0; i < count; i++) {
    if (!isNullAt(nullmap, i)) return false;
  }
  return true;
}

std::string Formatter::displayedValue(const ptree &row, int index, const std::string &nullmap, formatMode mode) {
  if (!isNullAt(nullmap, index)) return row.get(std::to_string(index), "");
  return mode == formatMode::GNUPLOT ? "NaN" : "null";
}

std::vector<Formatter::Column> Formatter::columns(const ptree &schema) {
  std::vector<Column> ret;
  const auto fields = schema.get_child_optional("db.field");
  if (!fields) return ret;
  for (const auto &v : *fields) {
    const auto name = v.second.get_value<std::string>();
    const int width = std::max(1, schema.get<int>("db.field_count." + name, 1));
    for (int index = 0; index < width; ++index)
      ret.push_back({name, index, width});
  }
  return ret;
}

void Formatter::initGnuplot(std::tuple<int, int, int> dim, bool rightToLeft) {
  std::println("set term qt noraise");
  std::println("set style fill transparent solid 0.5");
  if (rightToLeft)
    std::println("set xrange [{}:0]", std::get<0>(dim));
  else
    std::println("set xrange [0:{}]", std::get<0>(dim));
  std::println("set yrange [{}:{}]", std::get<1>(dim), std::get<2>(dim));
  std::println("set ticslevel 0");
  std::println("set hidden3d");
  std::println("set view 60,30");
}

void Formatter::renderRaw(const ptree &row, int count, const std::string &nullmap, bool skipNull) {
  if (skipNull && isAllNull(nullmap, count)) return;
  for (int i = 0; i < count; i++)
    std::print("{} ", displayedValue(row, i, nullmap, formatMode::RAW));
  std::print("\r\n");
}

void Formatter::renderGnuplot(const ptree &row, int count, const std::string &nullmap, const std::string &input,
                              const ptree &schema, std::tuple<int, int, int> dim) {
  if (std::cmp_less(gnuplot_lines_.size(), count)) gnuplot_lines_.resize(static_cast<std::size_t>(count));

  const auto window = static_cast<size_t>(std::get<0>(dim));

  for (int i = 0; i < count; i++) {
    gnuplot_lines_[i].push_front(displayedValue(row, i, nullmap, formatMode::GNUPLOT));
    if (gnuplot_lines_[i].size() > window) gnuplot_lines_[i].pop_back();
  }

  // Liczba krzywych w poleceniu `plot` MUSI rownac sie liczbie blokow danych ponizej --
  // gnuplot czyta dokladnie jeden blok na kazde '-'. Wiersz niesie jedna wartosc na ELEMENT,
  // wiec lista pol schematu przestala go opisywac: dla INTEGER[3] szly trzy bloki przy jednej
  // zadeklarowanej krzywej i gnuplot dlawil sie reszta. Tytuly biora sie teraz ze
  // splaszczonych kolumn, a gdy schemat jest od wiersza krotszy (starszy serwer, bez
  // db.field_count), brakujace dostaja nazwe zastepcza -- polecenie ma zostac poprawne.
  const auto schemaColumns = columns(schema);
  std::print("plot");
  for (int i = 0; i < count; i++) {
    if (i != 0) std::print(",");
    std::string columnName = "col" + std::to_string(i);
    if (std::cmp_less(i, schemaColumns.size())) {
      const auto &column = schemaColumns[static_cast<std::size_t>(i)];
      columnName         = column.width > 1 ? column.field + "[" + std::to_string(column.index) + "]" : column.field;
    }
    std::ranges::replace(columnName, '_', '-');
    std::print(" '-' u 1:2 t '[{}]' w lines lc rgb '{}'", columnName, colors_[static_cast<std::size_t>(i) % colors_.size()]);
  }
  std::print("\r\n");

  for (int i = 0; i < count; i++) {
    for (size_t j = 0; j < gnuplot_lines_[i].size(); j++)
      std::print("{} {}\r\n", j, gnuplot_lines_[i][j]);
    std::print("e\r\n");
  }
}

void Formatter::renderGraphite(const ptree &row, const std::string &nullmap, const std::string &input, const ptree &schema) {
  int i{0};
  for (const auto &column : columns(schema)) {
    if (!isNullAt(nullmap, i)) {
      // Element tablicy dostaje wlasny czlon sciezki: w graphite kropka JEST separatorem
      // hierarchii, wiec numbers.v.0 jest tam naturalna nazwa metryki. Pole jednoelementowe
      // zostaje bez sufiksu -- inaczej kazdy istniejacy strumien skalarny zmienilby nazwy
      // metryk, a nie o to w tej poprawce chodzi.
      const std::string metric = column.width > 1 ? column.field + "." + std::to_string(column.index) : column.field;
      std::println("{}.{} {} {}", input, metric, row.get(std::to_string(i), ""), (unsigned long long)time(nullptr));
    }
    ++i;
  }
}

void Formatter::renderInfluxDB(const ptree &row, const std::string &nullmap, const std::string &input, const ptree &schema) {
  // https://docs.influxdata.com/influxdb/v1.5/write_protocols/line_protocol_tutorial/
  using namespace std::chrono;
  int i{0};
  bool firstValNoComma(true);
  std::stringstream line;
  line << input << " ";
  for (const auto &column : columns(schema)) {
    if (!isNullAt(nullmap, i)) {
      if (firstValNoComma)
        firstValNoComma = false;
      else
        line << ",";
      // Podkreslenie, nie kropka i nie nawias: w line protocol kropka rozdziela pomiar od
      // klucza pola, a nawiasy w kluczu wymagaja escapowania. v_0 przechodzi bez obrobki.
      const std::string key = column.width > 1 ? column.field + "_" + std::to_string(column.index) : column.field;
      line << key << "=" << row.get(std::to_string(i), "");
    }
    ++i;
  }
  if (!firstValNoComma) {
    line << " " << duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    std::println("{}", line.str());
  }
}
