#pragma once

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>

#include "CRSMath.h"
#include "QStruct.h"
#include "lockManager.hpp"

typedef boost::property_tree::ptree ptree;

struct executorsm {
  int run(qTree &coreInstance, bool verbose, FlockServiceGuard &guard);

  enum : int { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;

  static void commandProcessorLoop();
  static ptree commandProcessor(ptree ptInval);
  static ptree collectStreamsParameters();

  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl);
  std::string printRowValue(const std::string &query_name);
};
