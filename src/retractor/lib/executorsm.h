#pragma once

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>

#include "CRSMath.h"

typedef boost::property_tree::ptree ptree;

struct executorsm {
  executorsm(qTree &coreInstance) : coreInstance(coreInstance){};

  int run(bool verbose, int iTimeLimitCntParam);

 private:
  static qTree *coreInstancePtr;
  qTree &coreInstance;

  static void commandProcessorLoop();
  static ptree commandProcessor(ptree ptInval);
  static ptree collectStreamsParameters();

  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl);
  std::string printRowValue(const std::string &query_name);
};
