#pragma once
#include "ICommand.hpp"

class DropFileCmd : public ICommand {
 public:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  [[nodiscard]] bool requiresConnection() const override { return false; }
  bool execute(CommandContext &ctx) override;
};
