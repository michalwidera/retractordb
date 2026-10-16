#pragma once
#include "ICommand.hpp"

class SetPosCmd : public ICommand {
 private:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};

class GetPosCmd : public ICommand {
 private:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};
