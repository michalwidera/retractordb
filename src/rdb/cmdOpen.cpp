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
  if (!(std::cin >> ctx.file) || ctx.file.contains('{')) {
    std::print("{}unrecognized or missing file:{}\n{}", ctx.colors.RED, ctx.file, ctx.colors.RESET);
    return false;
  }
  const auto oldPos = ctx.file.find(".old");
  const auto base   = (oldPos != std::string::npos) ? ctx.file.substr(0, oldPos) : ctx.file;
  ctx.dacc          = std::make_unique<rdb::storage>(base, ctx.file, ctx.storageParam, ctx.storagePolicy);
  std::string openError;

  if (ctx.dacc->descriptorFileExist()) {
    // Od fazy 1 attachDescriptor() nie konczy procesu: odmowe zwraca napisem (brak albo
    // uszkodzenie .desc, nieudane otwarcie magazynu), a pozostale bledy rzuca. xtrdb jest
    // powloka interaktywna: jedno i drugie ma zostac zgloszone i zostawic operatora przy
    // prompcie, a nie wyrzucic go z narzedzia w srodku sesji.
    try {
      openError = ctx.dacc->attachDescriptor();
    } catch (const rdb::Error &error) {
      openError = error.what();
    }
  } else {
    // Bez `.desc` schemat jest obowiazkowy i stoi w klamrach. Pierwszy znak sprawdzamy podgladem, bez
    // konsumowania, wiec nastepne polecenie skryptu zostaje poleceniem. Do 2026-09-27 petla brala
    // tokeny az do `}` bez kontroli strumienia: polecenia szly do schematu, a na koncu wejscia `>>`
    // nie zmienialo `token` i ostatni token doklejal sie bez konca (#334, 8 GB w 2 min).
    std::cin >> std::ws;
    if (std::cin.peek() != '{') {
      std::print("{}open: no descriptor file for '{}' and no schema - use: open {} {{ <schema> }}\n{}", ctx.colors.RED, ctx.file,
                 ctx.file, ctx.colors.RESET);
      ctx.dacc.reset();
      return false;
    }
    std::string schema;
    std::string token;
    while (!token.contains('}')) {
      if (!(std::cin >> token)) {
        std::print("{}open: schema for '{}' is not closed with '}}'\n{}", ctx.colors.RED, ctx.file, ctx.colors.RESET);
        ctx.dacc.reset();
        return false;
      }
      schema += token;
      schema += ' ';
    }
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
    try {
      openError = ctx.dacc->attachDescriptor(&desc);
    } catch (const rdb::Error &error) {
      openError = error.what();
    }
  }
  if (!openError.empty()) {
    std::print("{}open: {}\n{}", ctx.colors.RED, openError, ctx.colors.RESET);
    ctx.dacc.reset();
    return false;
  }
  ctx.payloadStatus = clean;
  ctx.dacc->setDisposable(false);
  if (ctx.dacc->isDeclared()) ctx.dacc->setCapacity(1);
  return true;
}
