#pragma once

#include <boost/program_options.hpp>  // IWYU pragma: keep

#include "qTree.hpp"  // for qTree, query, token
#include "rdb/error.hpp"

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
  /// Errc::Config, gdy plan nie daje osi czasu (interwal zero, brak strumieni).
  [[nodiscard]] rdb::Result<> sequenceDiagram(int gridType, int cycleCount);
};
