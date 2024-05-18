#pragma once

#include <boost/property_tree/ptree.hpp>
#include <map>
#include <set>
#include <string>

#include "CRSMath.h"
#include "QStruct.h"

typedef boost::property_tree::ptree ptree;

struct executorsm {
  explicit executorsm(qTree &coreInstance) : coreInstance(coreInstance){};
  executorsm() = delete;

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
