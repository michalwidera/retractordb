#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "QStruct.h"  // for query, token

struct dumper {
  explicit dumper(qTree &coreInstance) : coreInstance(coreInstance){};
  dumper() = delete;

  int run(boost::program_options::variables_map &vm);

 private:
  qTree &coreInstance;

  void graphiz(std::ostream &xout, bool bShowFileds, bool bShowStreamProgs, bool bShowTags);
  void qFieldsProgram();
  void qFields();
  void qPrograms();
  void qSet();
  void rawTextFile();
};
