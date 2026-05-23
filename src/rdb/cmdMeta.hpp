#pragma once
#include "ICommand.hpp"

class MetaCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class MetaRawCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};
