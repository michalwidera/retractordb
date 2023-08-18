#pragma once
#include <map>
#include <string>

#include "QStruct.h"  // for query, token

struct compiler {
  explicit compiler(qTree &coreInstance) : coreInstance(coreInstance){};
  compiler() = delete;

  std::string intervalCounter();
  std::string simplifyLProgram();
  std::string prepareFields();
  std::string replicateIDX();
  std::string convertReferences();
  std::string convertRemotes();
  std::string applyConstraints();
  std::map<std::string, int> countBuffersCapacity();

 private:
  qTree &coreInstance;
  std::list<field> combine(const std::string &sName1, const std::string &sName2, token &cmd_token);
  std::string generateStreamName(const std::string &sName1, const std::string &sName2, command_id cmd);
};
