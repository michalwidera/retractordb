#pragma once

#include <atomic>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>

enum class formatMode { RAW, GRAPHITE, INFLUXDB };

class qry {
  static void consumer(qry *);
  static void producer(qry *);

 public:
  std::map<std::string, boost::property_tree::ptree> streamTable;

  int timeLimitCntQry{0};

  void decTimeLimit();

  formatMode outputFormatMode{formatMode::RAW};

  // Graphite embedded schema in format "path.to.data value timestamp"
  boost::property_tree::ptree schema;

  std::string sInputStream{""};

  bool select(bool, int, const std::string &);
  void dir();
  int hello();
  bool detailShow(const std::string &);
  boost::property_tree::ptree netClient(std::string, const std::string &);
};
