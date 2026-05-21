#pragma once
#include <deque>
#include <string>
#include <tuple>
#include <vector>

#include <boost/property_tree/ptree.hpp>

enum class formatMode { RAW, GRAPHITE, INFLUXDB, GNUPLOT };

class Formatter {
  std::vector<std::deque<std::string>> gnuplot_lines_;
  static const std::vector<std::string> colors_;

 public:
  static bool isNullAt(const std::string &nullmap, int index);
  static bool isAllNull(const std::string &nullmap, int count);
  static std::string displayedValue(const boost::property_tree::ptree &row, int index, const std::string &nullmap,
                                    formatMode mode);

  static void initGnuplot(std::tuple<int, int, int> dim);
  static void renderRaw(const boost::property_tree::ptree &row, int count, const std::string &nullmap, bool skipNull);
  void renderGnuplot(const boost::property_tree::ptree &row, int count, const std::string &nullmap, const std::string &input,
                     const boost::property_tree::ptree &schema, std::tuple<int, int, int> dim);
  static void renderGraphite(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
  static void renderInfluxDB(const boost::property_tree::ptree &row, const std::string &nullmap, const std::string &input,
                             const boost::property_tree::ptree &schema);
};
