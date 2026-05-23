#pragma once
#include "ICommand.hpp"

class DropFileCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool requiresConnection() const override { return false; }
  bool execute(CommandContext &ctx) override;
};
