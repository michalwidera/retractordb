#pragma once
#include "ICommand.hpp"

class MetaCmd : public ICommand {
 private:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};

class MetaRawCmd : public ICommand {
 private:
  [[nodiscard]] std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};
