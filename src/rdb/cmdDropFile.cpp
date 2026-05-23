#include "cmdDropFile.hpp"

#include <filesystem>
#include <iostream>

std::pair<std::string, std::vector<std::string>> DropFileCmd::usage() const {
  return {"dropfile [file1] [file2] ... }", {"remove listed file(s), end with }"}};
}

bool DropFileCmd::execute(CommandContext &ctx) {
  std::string object;
  do {
    std::cin >> object;
    if (!ctx.storageParam.empty() && !object.contains(ctx.storageParam))
      object = (std::filesystem::path(ctx.storageParam) / object).string();
    std::filesystem::remove(object);
  } while (!object.contains('}'));
  return true;
}
