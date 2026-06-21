#pragma once

#include <atomic>
#include <cstdint>
#include <set>
#include <string>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "compiler.hpp"
#include "CRSMath.hpp"
#include "lockManager.hpp"

#include "appConfig.hpp"

using ptree  = boost::property_tree::ptree;
using vm_map = boost::program_options::variables_map;
struct executorsm {
  int run(qTree &coreInstance, FlockServiceGuard &guard, compiler &cm, vm_map &vm, const AppConfig &cfg);

  enum : std::int8_t { inifitie_loop = 0, stop_now = 1, waitForXqry = -1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;
  static int cfgQueueBufferSeconds;
  static int cfgMinQueueElements;
  static int cfgRtPriority;
  // Set by commandProcessorLoop once all IPC resources are ready.
  // run() waits on this before acquireLock(), so the lock file appears
  // only after IPC is fully initialized and xqry can connect safely.
  static std::atomic<bool> ipcReady;

  static void commandProcessorLoop();
  static ptree commandProcessor(const ptree &ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(const std::string &adHocQuery);
  static void boradcastOutOfBussiness();

  void boradcast(const std::set<std::string> &inSet);
  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);
};
