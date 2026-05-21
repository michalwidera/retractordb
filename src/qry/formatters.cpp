#include "formatters.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>

using boost::property_tree::ptree;

const std::vector<std::string> Formatter::colors_ = {"red",   "blue", "green",  "orange", "purple",
                                                     "brown", "pink", "yellow", "cyan",   "magenta"};

bool Formatter::isNullAt(const std::string &nullmap, int index) {
  return index >= 0 && index < static_cast<int>(nullmap.size()) && nullmap[index] == '1';
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
  std::cout << "set term qt noraise\n";
  std::cout << "set style fill transparent solid 0.5\n";
  std::cout << "set xrange [0:" << std::get<0>(dim) << "]\n";
  std::cout << "set yrange [" << std::get<1>(dim) << ":" << std::get<2>(dim) << "]\n";
  std::cout << "set ticslevel 0\n";
  std::cout << "set hidden3d\n";
  std::cout << "set view 60,30\n";
}

void Formatter::renderRaw(const ptree &row, int count, const std::string &nullmap, bool skipNull) {
  if (skipNull && isAllNull(nullmap, count)) return;
  for (int i = 0; i < count; i++)
    printf("%s ", displayedValue(row, i, nullmap, formatMode::RAW).c_str());
  printf("\r\n");
}

void Formatter::renderGnuplot(const ptree &row, int count, const std::string &nullmap, const std::string &input,
                              const ptree &schema, std::tuple<int, int, int> dim) {
  if (static_cast<int>(gnuplot_lines_.size()) < count) gnuplot_lines_.resize(count);

  std::cout << "plot";
  int colIdx{0};
  for (const auto &v : schema.get_child("db.field")) {
    if (colIdx != 0) std::cout << ",";
    auto columnName = v.second.get<std::string>("");
    std::replace(columnName.begin(), columnName.end(), '_', '-');
    std::cout << " '-' u 1:2 t '[" << columnName << "]' w lines lc rgb '" << colors_[colIdx % colors_.size()] << "'";
    colIdx++;
  }
  std::cout << "\r\n";

  for (int i = 0; i < count; i++) {
    gnuplot_lines_[i].push_front(displayedValue(row, i, nullmap, formatMode::GNUPLOT));
    if (gnuplot_lines_[i].size() > static_cast<size_t>(std::get<0>(dim))) gnuplot_lines_[i].pop_back();
  }
  for (int i = 0; i < count; i++) {
    for (size_t j = 0; j < gnuplot_lines_[i].size(); j++)
      printf("%zu %s\r\n", j, gnuplot_lines_[i][j].c_str());
    printf("e\r\n");
  }
}

void Formatter::renderGraphite(const ptree &row, const std::string &nullmap, const std::string &input, const ptree &schema) {
  int i{0};
  for (const auto &v : schema.get_child("db.field")) {
    if (isNullAt(nullmap, i)) {
      ++i;
      continue;
    }
    printf("%s.%s %s %llu\n", input.c_str(), v.second.get<std::string>("").c_str(), row.get(std::to_string(i++), "").c_str(),
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
    printf("%s\n", line.str().c_str());
  }
}
