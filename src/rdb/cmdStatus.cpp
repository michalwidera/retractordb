#include "cmdStatus.hpp"

#include <iostream>
#include <magic_enum/magic_enum.hpp>

std::pair<std::string, std::vector<std::string>> StatusCmd::usage() const {
  return {"status", {"show current payload status"}};
}

bool StatusCmd::execute(CommandContext& ctx) {
  std::cout << magic_enum::enum_name(ctx.payloadStatus) << "\n";
  return true;
}
