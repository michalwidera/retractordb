#include "cmdPayload.hpp"

#include <iostream>

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"

std::pair<std::string, std::vector<std::string>> SetCmd::usage() const {
  return {"set [field][value]", {"set payload field value"}};
}

bool SetCmd::execute(CommandContext& ctx) {
  std::cin >> *(ctx.dacc->getPayload());
  ctx.payloadStatus = changed;
  return false;
}

bool PrintCmd::execute(CommandContext& ctx) {
  if (tabular_)
    std::cout << ctx.colors.ORANGE << rdb::singleLineFormat << *(ctx.dacc->getPayload()) << ctx.colors.RESET << "\n";
  else
    std::cout << ctx.colors.ORANGE << *(ctx.dacc->getPayload()) << ctx.colors.RESET;
  return false;
}

std::pair<std::string, std::vector<std::string>> InputCmd::usage() const {
  return {"input [[field][value]]", {"fill payload"}};
}

bool InputCmd::execute(CommandContext& ctx) {
  for ([[maybe_unused]] const auto& _ : ctx.dacc->descriptor)
    std::cin >> *(ctx.dacc->getPayload());
  return false;
}

bool HexCmd::execute(CommandContext& ctx) {
  ctx.dacc->getPayload()->setHex(hex_);
  return true;
}
