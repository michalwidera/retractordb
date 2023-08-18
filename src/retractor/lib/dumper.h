#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "QStruct.h"  // for query, token

struct dumper {
  dumper(qTree &coreInstance) : coreInstance(coreInstance){};
  qTree &coreInstance;

  int run(boost::program_options::variables_map &vm);

  void graphiz(std::ostream &xout, bool bShowFileds, bool bShowStreamProgs, bool bShowTags);
  void qFieldsProgram();
  void qFields();
  void qPrograms();
  void qSet();
  void rawTextFile();
};
