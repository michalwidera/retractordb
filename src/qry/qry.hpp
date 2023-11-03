#pragma once

#include <atomic>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <string>

class qry {
  static void consumer();
  static void producer();

 public:
  bool select(bool, int, const std::string &);
  void dir();
  int hello();
  void setmode(std::string const &);
  bool detailShow(const std::string &);
  boost::property_tree::ptree netClient(std::string, const std::string &);
};
