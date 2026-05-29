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

void Formatter::initGnuplot(std::tuple<int, int, int> dim) {
  std::println("set term qt noraise");
  std::println("set style fill transparent solid 0.5");
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

  std::print("plot");
  int colIdx{0};
  for (const auto &v : schema.get_child("db.field")) {
    if (colIdx != 0) std::print(",");
    auto columnName = v.second.get<std::string>("");
    std::ranges::replace(columnName, '_', '-');
    std::print(" '-' u 1:2 t '[{}]' w lines lc rgb '{}'", columnName, colors_[colIdx % colors_.size()]);
    colIdx++;
  }
  std::print("\r\n");

  for (int i = 0; i < count; i++) {
    gnuplot_lines_[i].push_front(displayedValue(row, i, nullmap, formatMode::GNUPLOT));
    if (gnuplot_lines_[i].size() > static_cast<size_t>(std::get<0>(dim))) gnuplot_lines_[i].pop_back();
  }
  for (int i = 0; i < count; i++) {
    for (size_t j = 0; j < gnuplot_lines_[i].size(); j++)
      std::print("{} {}\r\n", j, gnuplot_lines_[i][j]);
    std::print("e\r\n");
  }
}

void Formatter::renderGraphite(const ptree &row, const std::string &nullmap, const std::string &input, const ptree &schema) {
  int i{0};
  for (const auto &v : schema.get_child("db.field")) {
    if (isNullAt(nullmap, i)) {
      ++i;
      continue;
    }
    std::println("{}.{} {} {}", input, v.second.get<std::string>(""), row.get(std::to_string(i++), ""),
                 (unsigned long long)time(nullptr));
  }
}

void Formatter::renderInfluxDB(const ptree &row, const std::string &nullmap, const std::string &input, const ptree &schema) {
  // https://docs.influxdata.com/influxdb/v1.5/write_protocols/line_protocol_tutorial/
  using namespace std::chrono;
  int i{0};
  bool firstValNoComma(true);
  std::stringstream line;
  line << input << " ";
  for (const auto &v : schema.get_child("db.field")) {
    if (isNullAt(nullmap, i)) {
      ++i;
      continue;
    }
    if (firstValNoComma)
      firstValNoComma = false;
    else
      line << ",";
    line << v.second.get<std::string>("") << "=" << row.get(std::to_string(i++), "");
  }
  if (!firstValNoComma) {
    line << " " << duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    std::println("{}", line.str());
  }
}
