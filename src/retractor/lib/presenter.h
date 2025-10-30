#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "QStruct.h"  // for query, token

struct presenter {
  explicit presenter(qTree &coreInstance) : coreInstance(coreInstance){};
  presenter() = delete;

  int run(boost::program_options::variables_map &vm);

 private:
  qTree &coreInstance;

  void graphiz(std::ostream &xout, bool bShowFileds, bool bShowStreamProgs, bool bShowTags, bool bShowRules, bool bTransparent);
  void qFieldsProgram();
  void qFields();
  void qPrograms();
  void qSet();
  void qRules();
  void onlyCompileShowProgram();
  void sequenceDiagram(std::ostream &out, int count_str);
};
