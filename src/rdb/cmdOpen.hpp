#pragma once
#include "ICommand.hpp"

class OpenCmd : public ICommand {
 public:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  [[nodiscard]] bool requiresConnection() const override { return false; }
  bool execute(CommandContext &ctx) override;
};
