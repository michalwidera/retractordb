#pragma once

#include <atomic>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>

enum class formatMode { RAW, GRAPHITE, INFLUXDB };

class qry {
  static void producer();

  int timeLimitCntQry{0};
  std::map<std::string, boost::property_tree::ptree> streamTable;

 public:
  formatMode outputFormatMode{formatMode::RAW};

  bool select(bool, const int, const std::string &);
  void dir();
  int hello();
  bool detailShow(const std::string &);
  virtual boost::property_tree::ptree netClient(const std::string &, const std::string &);
  virtual ~qry(){};
};
