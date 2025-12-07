#pragma once

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>

#include "CRSMath.h"
#include "QStruct.h"
#include "compiler.h"
#include "lockManager.hpp"

typedef boost::property_tree::ptree ptree;
typedef boost::program_options::variables_map vm_map;
struct executorsm {
  int run(qTree &coreInstance, FlockServiceGuard &guard, compiler &cm, vm_map &vm);

  enum : int { inifitie_loop = 0, stop_now = 1, waitForXqry = -1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;

  static void commandProcessorLoop();
  static ptree commandProcessor(ptree ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(std::string adHocQuery);
  void boradcast(const std::set<std::string> &inSet);
  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);
};
