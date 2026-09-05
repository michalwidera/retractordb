#pragma once

#include <atomic>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "bus.hpp"
#include "compiler.hpp"
#include "CRSMath.hpp"
#include "lockManager.hpp"

#include "appConfig.hpp"

using ptree  = boost::property_tree::ptree;
using vm_map = boost::program_options::variables_map;
struct executorsm {
  /// serverName pusta => tozsamosc historyczna (jeden serwer na maszyne, dotychczasowe nazwy
  /// obiektow IPC). Niepusta => wlasny, rozlaczny obszar IPC tej instancji.
  int run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm, const AppConfig &cfg,
          std::string_view serverName = {});

  enum : std::int8_t { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;
  static int cfgQueueBufferSeconds;
  static int cfgMinQueueElements;
  static int cfgRtPriority;
  // Set from IpcServer's onReady callback once all IPC resources are ready.
  // run() waits on this before publishLockInfo(), so PID appears in the lock file
  // only after IPC is fully initialized and xqry can connect safely.
  static std::atomic<bool> ipcReady;

  static ptree commandProcessor(const ptree &ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(const std::string &adHocQuery);
  static ptree attachAdHocRule(qTree &coreInstanceCopy, const std::string &streamName);

  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);
};
