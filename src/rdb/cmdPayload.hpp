#pragma once
#include "ICommand.hpp"

class SetCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};

class PrintCmd : public ICommand {
  bool tabular_;

 public:
  explicit PrintCmd(bool tabular) : tabular_(tabular) {}
  // printt is an alias — suppress duplicate help entry
  std::pair<std::string, std::vector<std::string>> usage() const override {
    if (tabular_) return {};
    return {"print|printt", {"show payload (printt: single-line tabular)"}};
  }
  bool execute(CommandContext &ctx) override;
};

class InputCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext &ctx) override;
};

class HexCmd : public ICommand {
  bool hex_;

 public:
  explicit HexCmd(bool hex) : hex_(hex) {}
  // dec is an alias — suppress duplicate help entry
  std::pair<std::string, std::vector<std::string>> usage() const override {
    if (!hex_) return {};
    return {"hex|dec", {"type of input/output of byte/number fields"}};
  }
  bool execute(CommandContext &ctx) override;
};
