#pragma once
#include <map>
#include <memory>
#include <string>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "formatters.hpp"

class IpcTransport;

inline constexpr int kDefaultServerNoDataTimeoutMs{10'000};
inline constexpr int kDefaultClientResponseMaxFails{10};

class qry {
  int elemLimitCnt{0};
  int serverNoDataTimeoutMs_{kDefaultServerNoDataTimeoutMs};
  std::map<std::string, boost::property_tree::ptree> streamTable;
  std::unique_ptr<IpcTransport> transport_;
  std::unique_ptr<Formatter> formatter_;

 public:
  formatMode outputFormatMode{formatMode::RAW};
  bool gnuplotRightToLeft{false};

  explicit qry(int serverNoDataTimeoutMs  = kDefaultServerNoDataTimeoutMs,
               int clientResponseMaxFails = kDefaultClientResponseMaxFails);
  bool select(boost::program_options::variables_map &vm, int /*iElemLimit*/, const std::string & /*input*/,
              std::tuple<int, int, int> /*gnuplotDim*/, bool /*gnuplotRightToLeft*/ = false);
  bool adhoc(const std::string & /*sAdhoc*/);
  std::string dir();
  std::string dirYaml();
  int hello();
  std::string detailShow(const std::string & /*input*/);
  virtual boost::property_tree::ptree netClient(const std::string & /*cmd*/, const std::string & /*arg*/);
  virtual ~qry();
};
