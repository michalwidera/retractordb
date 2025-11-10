#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>

enum class formatMode { RAW, GRAPHITE, INFLUXDB, GNUPLOT };

class qry {
  static void producer();

  int timeLimitCntQry{0};
  std::map<std::string, boost::property_tree::ptree> streamTable;

 public:
  formatMode outputFormatMode{formatMode::RAW};

  bool select(boost::program_options::variables_map &vm, const int, const std::string &, std::pair<int, int>);
  bool adhoc(const std::string &);
  std::string dir();
  std::string dirYaml();
  int hello();
  std::string detailShow(const std::string &);
  virtual boost::property_tree::ptree netClient(const std::string &, const std::string &);
  virtual ~qry() = default;
};
