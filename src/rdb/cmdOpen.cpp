#include "cmdOpen.hpp"

#include <iostream>
#include <sstream>

#include "rdb/descriptor.hpp"

std::pair<std::string, std::vector<std::string>> OpenCmd::usage() const {
  return {"open file [schema]",
          {"open or create database with schema", "example: open test_db { INTEGER dane STRING name[3] }"}};
}

bool OpenCmd::execute(CommandContext &ctx) {
  std::cin >> ctx.file;
  if (ctx.file.contains('{')) {
    std::cout << ctx.colors.RED << "unrecognized or missing file:" << ctx.file << "\n" << ctx.colors.RESET;
    return false;
  }
  const auto oldPos = ctx.file.find(".old");
  const auto base   = (oldPos != std::string::npos) ? ctx.file.substr(0, oldPos) : ctx.file;
  ctx.dacc          = std::make_unique<rdb::storage>(base, ctx.file, ctx.storageParam, ctx.storagePolicy);

  if (ctx.dacc->descriptorFileExist()) {
    ctx.dacc->attachDescriptor();
  } else {
    std::string schema;
    std::string token;
    do {
      std::cin >> token;
      schema += token;
      schema += ' ';
    } while (!token.contains('}'));
    std::stringstream schemaStream(schema);
    rdb::Descriptor desc;
    schemaStream >> desc;
    ctx.dacc->attachDescriptor(&desc);
  }
  ctx.payloadStatus = clean;
  ctx.dacc->setDisposable(false);
  if (ctx.dacc->isDeclared()) ctx.dacc->setCapacity(1);
  return true;
}
