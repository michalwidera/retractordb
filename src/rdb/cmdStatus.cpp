#include "cmdStatus.hpp"

#include <print>

#include <magic_enum/magic_enum.hpp>

std::pair<std::string, std::vector<std::string>> StatusCmd::usage() const { return {"status", {"show current payload status"}}; }

bool StatusCmd::execute(CommandContext &ctx) {
  std::print("{}\n", magic_enum::enum_name(ctx.payloadStatus));
  return true;
}
