#include "cmdOpen.hpp"

#include <iostream>
#include <print>
#include <sstream>

#include "rdb/descriptor.hpp"
#include "rdb/exceptions.hpp"

std::pair<std::string, std::vector<std::string>> OpenCmd::usage() const {
  return {"open file [schema]",
          {"open or create database with schema", "example: open test_db { INTEGER value STRING name[3] }"}};
}

bool OpenCmd::execute(CommandContext &ctx) {
  std::cin >> ctx.file;
  if (ctx.file.contains('{')) {
    std::print("{}unrecognized or missing file:{}\n{}", ctx.colors.RED, ctx.file, ctx.colors.RESET);
    return false;
  }
  const auto oldPos = ctx.file.find(".old");
  const auto base   = (oldPos != std::string::npos) ? ctx.file.substr(0, oldPos) : ctx.file;
  ctx.dacc          = std::make_unique<rdb::storage>(base, ctx.file, ctx.storageParam, ctx.storagePolicy);

  if (ctx.dacc->descriptorFileExist()) {
    // loadDescriptorFile rzuca od fazy 1 zamiast konczyc proces. xtrdb jest powloka
    // interaktywna: uszkodzony plik ma zostac zgloszony i zostawic operatora przy
    // prompcie, a nie wyrzucic go z narzedzia w srodku sesji.
    try {
      ctx.dacc->attachDescriptor();
    } catch (const rdb::Error &error) {
      std::print("{}{}{}\n", ctx.colors.RED, error.what(), ctx.colors.RESET);
      ctx.dacc.reset();
      return false;
    }
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
    // Schemat przychodzi wprost od operatora, wiec literowka jest tu przypadkiem
    // NORMALNYM, a nie awaryjnym. Do fazy 1 konczyla xtrdb przez exit(EPERM) w listenerze
    // parsera; teraz ekstraktor zapala failbit i komenda odmawia, zostawiajac powloke.
    if (schemaStream.fail()) {
      std::print("{}invalid schema:{}\n{}", ctx.colors.RED, schema, ctx.colors.RESET);
      // Magazyn powstal przed odczytem schematu i zostalby bez deskryptora - kazda
      // nastepna komenda widzialaby otwarta baze, ktorej nie da sie uzyc.
      ctx.dacc.reset();
      return false;
    }
    ctx.dacc->attachDescriptor(&desc);
  }
  ctx.payloadStatus = clean;
  ctx.dacc->setDisposable(false);
  if (ctx.dacc->isDeclared()) ctx.dacc->setCapacity(1);
  return true;
}
