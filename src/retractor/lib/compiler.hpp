#pragma once

#include <vector>

#include "qTree.hpp"  // for qTree, query, token

struct compiler {
  explicit compiler(qTree &coreInstance) : coreInstance(coreInstance) {};
  compiler() = delete;

  std::string compile();
  std::vector<std::string> importFrom(qTree &source);

 private:
  qTree &coreInstance;
  std::list<field> buildOutputSchema(const std::string &sName1, const std::string &sName2, token &cmd_token);
  std::string composeStreamName(const std::string &sName1, const std::string &sName2, command_id cmd);
  void resolveTokenReferences(std::list<token> &lProgram, query &q);

  // compile chain steps
  std::string resolveStreamIntervals();
  std::string extractIntermediateStreams();
  std::string expandSchemaWildcards();
  std::string expandIndexWildcards();
  std::string resolveFieldReferences();
  std::string localizeFieldOffsets();
  std::string validateConstraints();
  std::map<std::string, int> computeRequiredCapacities();
  std::string applyCapacitiesToStreams(const std::map<std::string, int> &capMap);
  std::string deduplicateSubstrats();
};
