#pragma once

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>

#include "CRSMath.h"
#include "QStruct.h"
#include "compiler.h"
#include "lockManager.hpp"

typedef boost::property_tree::ptree ptree;

struct executorsm {
  int run(qTree &coreInstance, bool percount, bool verbose, FlockServiceGuard &guard, compiler &cm);

  enum : int { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;

  static void commandProcessorLoop();
  static ptree commandProcessor(ptree ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(std::string adHocQuery);

  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);
};
