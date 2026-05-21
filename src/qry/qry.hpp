#pragma once
#include <map>
#include <memory>
#include <string>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "formatters.hpp"

class IpcTransport;

class qry {
  int timeLimitCntQry{0};
  std::map<std::string, boost::property_tree::ptree> streamTable;
  std::unique_ptr<IpcTransport> transport_;
  std::unique_ptr<Formatter> formatter_;

 public:
  formatMode outputFormatMode{formatMode::RAW};

  qry();
  bool select(boost::program_options::variables_map &vm, const int, const std::string &, std::tuple<int, int, int>);
  bool adhoc(const std::string &);
  std::string dir();
  std::string dirYaml();
  int hello();
  std::string detailShow(const std::string &);
  virtual boost::property_tree::ptree netClient(const std::string &, const std::string &);
  virtual ~qry();
};
