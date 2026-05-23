#pragma once
#include "ICommand.hpp"

class ReadCmd : public ICommand {
  bool reverse_;

 public:
  explicit ReadCmd(bool reverse) : reverse_(reverse) {}
  // rread is an alias — suppress duplicate help entry
  std::pair<std::string, std::vector<std::string>> usage() const override {
    if (reverse_) return {};
    return {"read|rread [n]", {"read record from database into payload"}};
  }
  bool execute(CommandContext& ctx) override;
};

class ListCmd : public ICommand {
  bool reverse_;

 public:
  explicit ListCmd(bool reverse) : reverse_(reverse) {}
  // rlist is an alias — suppress duplicate help entry
  std::pair<std::string, std::vector<std::string>> usage() const override {
    if (reverse_) return {};
    return {"list|rlist [count]", {"print first records"}};
  }
  bool execute(CommandContext& ctx) override;
};

class WriteCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class DumpCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class AppendCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class PurgeCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class SizeCmd : public ICommand {
 public:
  std::pair<std::string, std::vector<std::string>> usage() const override;
  bool execute(CommandContext& ctx) override;
};

class DescCmd : public ICommand {
  bool compact_;

 public:
  explicit DescCmd(bool compact) : compact_(compact) {}
  // descc is an alias — suppress duplicate help entry
  std::pair<std::string, std::vector<std::string>> usage() const override {
    if (compact_) return {};
    return {"desc|descc", {"show schema (descc: compact single-line)"}};
  }
  bool execute(CommandContext& ctx) override;
};
