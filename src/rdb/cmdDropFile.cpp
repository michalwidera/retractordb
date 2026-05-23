#include "cmdDropFile.hpp"

#include <filesystem>
#include <iostream>

std::pair<std::string, std::vector<std::string>> DropFileCmd::usage() const {
  return {"dropfile [file1] [file2] ... }", {"remove listed file(s), end with }"}};
}

bool DropFileCmd::execute(CommandContext& ctx) {
  std::string object;
  do {
    object.clear();
    std::cin >> object;
    if (!ctx.storageParam.empty() && object.find(ctx.storageParam) == std::string::npos) {
      object = ctx.storageParam + "/" + object;
    }
    if (std::filesystem::exists(object)) std::filesystem::remove(object);
  } while (object.find("}") == std::string::npos);
  return true;
}
