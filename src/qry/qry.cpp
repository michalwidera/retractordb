#include "qry.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/system/system_error.hpp>

#include "constants.hpp"
#include "fatalError.hpp"
#include "formatters.hpp"
#include "ipc_transport.hpp"
#include "uxSysTermTools.hpp"

using namespace boost;
using boost::property_tree::ptree;

// Rozmiar bufora dla formatowania wyjścia kolumnowego w dir() (sprintf).
// Musi pomieścić jedną sformatowaną linię z nazwami wszystkich kolumn strumienia.
constexpr int kDirLineBufferSize = 1024;

qry::qry() : transport_(std::make_unique<IpcTransport>()), formatter_(std::make_unique<Formatter>()) {}
qry::~qry() = default;

ptree qry::netClient(const std::string &cmd, const std::string &arg) { return transport_->netClient(cmd, arg); }

bool qry::adhoc(const std::string &sAdhoc) {
  if (sAdhoc.empty()) {
    SPDLOG_ERROR("qry::adhoc: adhoc query string must not be empty");
    return false;
  }
  ptree pt = netClient("adhoc", sAdhoc);

  std::string rcv("fail.");
  for (auto &[first, second] : pt) {
    rcv = second.get<std::string>("");
  }

  if (rcv != "OK") {
    SPDLOG_ERROR("bad rcv: {}", rcv.c_str());
    return false;
  }
  return true;
}

bool qry::select(boost::program_options::variables_map &vm, const int iTimeLimit, const std::string &input,
                 std::tuple<int, int, int> gnuplotDim) {
  timeLimitCntQry = (iTimeLimit > 0) ? iTimeLimit + 1 : iTimeLimit;
  ptree pt        = netClient("get", "");

  const auto stream = pt.get_child("db.stream");
  const bool found  = std::ranges::any_of(stream, [input, this](const auto &node) {
    const ptree &v = node.second;
    bool ret       = (input == v.get<std::string>(""));
    if (ret) streamTable[input] = netClient("show", input);
    return ret;
  });

  if (!found) {
    SPDLOG_ERROR("not found: {}", input);
    return found;
  }

  std::jthread producer_thread([this] { transport_->producer(); });

  if (outputFormatMode == formatMode::GNUPLOT) Formatter::initGnuplot(gnuplotDim);

  ptree schema;
  if (outputFormatMode != formatMode::RAW) schema = netClient("detail", input);

  constexpr int serverTimeoutMs = 10000;
  int noDataCounter             = 0;

  ptree e_value;
  try {
    while (!transport_->done) {
      if (_kbhit(vm.contains("needctrlc"))) break;
      if (timeLimitCntQry == 1) {
        if (vm.contains("kill")) {
          netClient("kill", "");
          transport_->done = true;
        }
        break;
      }
      while (transport_->popQueue(e_value)) {
        const std::string streamN = e_value.get("stream", "");
        const std::string nullmap = e_value.get("nullmap", "");
        if (streamN == constants::Reserved_id_oob) {
          transport_->done = true;
          break;
        }
        for (auto &[w, k] : streamTable)
          if (w == streamN) {
            const int count = std::stoi(e_value.get("count", ""));
            if (outputFormatMode == formatMode::RAW)
              Formatter::renderRaw(e_value, count, nullmap, vm.contains("null"));
            else if (outputFormatMode == formatMode::GNUPLOT)
              formatter_->renderGnuplot(e_value, count, nullmap, input, schema, gnuplotDim);
            else if (outputFormatMode == formatMode::GRAPHITE)
              Formatter::renderGraphite(e_value, nullmap, input, schema);
            else if (outputFormatMode == formatMode::INFLUXDB)
              Formatter::renderInfluxDB(e_value, nullmap, input, schema);

            if (timeLimitCntQry > 1) --timeLimitCntQry;
            noDataCounter = 0;
          }
      }
      std::this_thread::sleep_for(ipc::kQueuePollInterval);
      if (++noDataCounter > serverTimeoutMs) {
        SPDLOG_WARN("No data received for {} ms, assuming server is dead.", serverTimeoutMs);
        transport_->done = true;
        break;
      }
    }
    while (transport_->popQueue(e_value) && !transport_->done)
      std::this_thread::sleep_for(ipc::kQueuePollInterval);

    if (timeLimitCntQry != 1 && !transport_->done) _getch();

  } catch (...) {
    SPDLOG_ERROR("General exception catched.");
  }

  transport_->done = true;
  return found;
}

int qry::hello() {
  ptree pt = netClient("hello", "");

  std::string rcv("fail.");
  for (auto &[first, second] : pt) {
    rcv = second.get<std::string>("");
  }
  if (rcv != "world") {
    SPDLOG_ERROR("bad rcv: {}", rcv.c_str());
    return system::errc::protocol_error;
  }
  return system::errc::success;
}

std::string qry::dirYaml() {
  std::stringstream retval;
  ptree pt = netClient("get", "");

  retval << "---\napiVersion: xqry/v1\n";
  retval << "streams:\n";
  for (const auto &v : pt.get_child("db.stream")) {
    auto location = v.second.get<std::string>("location");
    auto size     = v.second.get<std::string>("size");

    retval << "  - name: " << v.second.get<std::string>("") << "\n";
    retval << "    delta: " << v.second.get<std::string>("duration") << "\n";
    if (size != "-1") retval << "    size: " << size << "\n";
    retval << "    count: " << v.second.get<std::string>("count") << "\n";
    if (!location.empty()) retval << "    location: " << location << "\n";
  }

  return retval.str();
}

std::string qry::dir() {
  std::stringstream retval;
  ptree pt                       = netClient("get", "");
  std::vector<std::string> vcols = {"", "duration", "size", "count", "location", "cap"};
  std::stringstream ss;
  for (auto nName : vcols) {
    auto stream    = pt.get_child("db.stream");
    auto maxSizeIt = std::ranges::max_element(stream, [&nName](const auto &node1, const auto &node2) {
      const ptree &v1 = node1.second;
      const ptree &v2 = node2.second;
      return v1.get<std::string>(nName).length() < v2.get<std::string>(nName).length();
    });
    ss << "|%" << maxSizeIt->second.get<std::string>(nName).length() << "s";
  }
  ss << "|\n";

  std::array<char, static_cast<std::size_t>(kDirLineBufferSize)> buffer{};
  for (const auto &v : pt.get_child("db.stream")) {
    sprintf(buffer.data(), ss.str().c_str(), v.second.get<std::string>("").c_str(),
            v.second.get<std::string>("duration").c_str(), v.second.get<std::string>("size").c_str(),
            v.second.get<std::string>("count").c_str(), v.second.get<std::string>("location").c_str(),
            v.second.get<std::string>("cap").c_str());
    retval << buffer.data();
  }

  return retval.str();
}

static const std::string indent = "  ";

std::string qry::detailShow(const std::string &input) {
  std::stringstream retval;
  ptree pt = netClient("get", "");

  const auto streams = pt.get_child("db.stream");
  bool found         = std::ranges::any_of(streams, [&input](const auto &node) {
    const ptree &v = node.second;
    return input == v.get<std::string>("");
  });

  if (found) {
    ptree ptsh = netClient("detail", input);
    auto delta = ptsh.get_child("db.duration");
    auto query = ptsh.get_child("db.processed_line");
    auto id    = ptsh.get_child("db.stream");

    retval << "---\napiVersion: xqry/v1\n";
    retval << "stream:\n";
    retval << indent << "name: " << id.get_value<std::string>() << "\n";
    retval << indent << "delta: " << delta.get_value<std::string>() << "\n";
    retval << "query: " << query.get_value<std::string>() << "\n";
    retval << "fields:\n";
    for (const auto &v : ptsh.get_child("db.field")) {
      retval << indent << input << "." << v.second.get<std::string>("") << ":\n";
      retval << indent << indent << "type: " << ptsh.get<std::string>("db.field_type." + v.second.get<std::string>("")) << "\n";
    }
  } else
    SPDLOG_ERROR("not found");

  return retval.str();
}
