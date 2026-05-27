#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <utility>

#include "token.hpp"

struct rule {
  std::string name;
  std::list<token> condition;

  enum actionType : std::uint8_t { UNKNOWN_ACTION, DUMP, SYSTEM } action{UNKNOWN_ACTION};

  std::pair<long int, long int> dumpRange{std::make_pair(0, 0)};
  size_t dump_retention{0};

  std::string systemCommand;

  rule(std::string name, std::list<token> condition) : name(std::move(name)), condition(std::move(condition)) {}
};
