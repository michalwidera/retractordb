#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>

#include "QStruct.h"  // for query, token

struct compiler {
  explicit compiler(qTree &coreInstance) : coreInstance(coreInstance){};
  compiler() = delete;

  // run compile chain
  std::string run();
  std::vector<std::string> mergeCore(qTree &coreInstanceSrc);

 private:
  qTree &coreInstance;
  std::list<field> combine(const std::string &sName1, const std::string &sName2, token &cmd_token);
  std::string generateStreamName(const std::string &sName1, const std::string &sName2, command_id cmd);
  void ftokenfix(std::list<token> &lProgram, query &q);

  // chain functions
  std::string intervalCounter();
  std::string simplifyLProgram();
  std::string prepareFields();
  std::string replicateIDX();
  std::string convertReferences();
  std::string convertRemotes();
  std::string applyConstraints();
  std::map<std::string, int> countBuffersCapacity();
  std::string fillSubstractsMemSize(const std::map<std::string, int> &capMap);
};
