#include "cmdOpen.hpp"

#include <iostream>
#include <sstream>

#include "rdb/descriptor.hpp"

std::pair<std::string, std::vector<std::string>> OpenCmd::usage() const {
  return {"open file [schema]", {
    "open or create database with schema",
    "example: open test_db { INTEGER dane STRING name[3] }"
  }};
}

bool OpenCmd::execute(CommandContext& ctx) {
  std::cin >> ctx.file;
  if (ctx.file.find('{') != std::string::npos) {
    std::cout << ctx.colors.RED << "unrecognized or missing file:" << ctx.file << "\n" << ctx.colors.RESET;
    return false;
  }
  auto oldPos = ctx.file.find(".old");
  if (oldPos != std::string::npos) {
    ctx.dacc = std::make_unique<rdb::storage>(ctx.file.substr(0, oldPos), ctx.file, ctx.storageParam, ctx.storagePolicy);
  } else {
    ctx.dacc = std::make_unique<rdb::storage>(ctx.file, ctx.file, ctx.storageParam, ctx.storagePolicy);
  }
  if (ctx.dacc->descriptorFileExist()) {
    ctx.dacc->attachDescriptor();
  } else {
    std::string sschema;
    std::string object;
    do {
      std::cin >> object;
      sschema.append(object);
      sschema.append(" ");
    } while (object.find("}") == std::string::npos);
    std::stringstream scheamStringStream(sschema);
    rdb::Descriptor desc;
    scheamStringStream >> desc;
    ctx.dacc->attachDescriptor(&desc);
  }
  ctx.payloadStatus = clean;
  ctx.dacc->setDisposable(false);
  if (ctx.dacc->isDeclared()) ctx.dacc->setCapacity(1);
  return true;
}
