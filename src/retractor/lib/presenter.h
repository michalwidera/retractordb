#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "qTree.h"  // for qTree, query, token

struct presenter {
  explicit presenter(qTree &coreInstance) : coreInstance(coreInstance) {};
  presenter() = delete;

  int run(const boost::program_options::variables_map &vm);

 private:
  qTree &coreInstance;

  void graphiz(std::ostream &xout, const boost::program_options::variables_map &vm);
  void qFieldsProgram();
  void qFields();
  void qPrograms();
  void qSet();
  void qRules();
  void onlyCompileShowProgram();
  void sequenceDiagram(int gridCount, int cycleCount);
};
