#include "cmdStorage.hpp"

#include <iostream>
#include <print>

#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

bool ReadCmd::execute(CommandContext &ctx) {
  int record;
  std::cin >> record;
  if (!ctx.dacc->isDeclared() && record >= ctx.dacc->getRecordsCount()) {
    std::print("{}record out of range - read command\n{}", ctx.colors.RED, ctx.colors.RESET);
    return false;
  }
  if (ctx.dacc->isDeclared()) ctx.dacc->bufferState = rdb::sourceState::flux;
  auto returnStatus = reverse_ ? ctx.dacc->revRead(record) : ctx.dacc->read(record);
  if (ctx.dacc->isDeclared()) ctx.dacc->fire();
  ctx.payloadStatus = returnStatus ? fetched : error;
  return true;
}

bool ListCmd::execute(CommandContext &ctx) {
  int record{0};
  std::cin >> record;
  for (int i = 0; i < record; i++) {
    if (static_cast<size_t>(i) >= ctx.dacc->getRecordsCount()) {
      std::print("{}record out of range - list command\n{}", ctx.colors.RED, ctx.colors.RESET);
      continue;
    }
    auto returnStatus = reverse_ ? ctx.dacc->revRead(i) : ctx.dacc->read(i);
    ctx.payloadStatus = returnStatus ? fetched : error;
    if (ctx.payloadStatus == error) {
      std::print("{}fetch error\n{}", ctx.colors.RED, ctx.colors.RESET);
      continue;
    }
    std::cout << ctx.colors.ORANGE << rdb::singleLineFormat << *(ctx.dacc->getPayload()) << ctx.colors.RESET << "\n";
  }
  return false;
}

std::pair<std::string, std::vector<std::string>> WriteCmd::usage() const {
  return {"write [n]", {"from payload send record to database"}};
}

bool WriteCmd::execute(CommandContext &ctx) {
  size_t record{0};
  std::cin >> record;
  if (record >= ctx.dacc->getRecordsCount()) {
    std::print("{}record out of range - Check append command.\n{}", ctx.colors.RED, ctx.colors.RESET);
    return false;
  }
  auto writeStatus  = ctx.dacc->write(record);
  ctx.payloadStatus = (!writeStatus) ? stored : error;
  return true;
}

std::pair<std::string, std::vector<std::string>> DumpCmd::usage() const { return {"dump", {"show payload memory"}}; }

bool DumpCmd::execute(CommandContext &ctx) {
  auto payloadSpan = ctx.dacc->getPayload()->span();
  for (auto byte : payloadSpan) {
    std::print("{:02x} ", static_cast<unsigned>(byte));
  }
  std::print("\n");
  return true;
}

std::pair<std::string, std::vector<std::string>> AppendCmd::usage() const { return {"append", {"append payload to database"}}; }

bool AppendCmd::execute(CommandContext &ctx) {
  ctx.dacc->write();
  ctx.payloadStatus = stored;
  return true;
}

std::pair<std::string, std::vector<std::string>> PurgeCmd::usage() const {
  return {"purge", {"remove all records from database"}};
}

bool PurgeCmd::execute(CommandContext &ctx) {
  ctx.dacc->purge();
  return true;
}

std::pair<std::string, std::vector<std::string>> SizeCmd::usage() const { return {"size", {"show database size in records"}}; }

bool SizeCmd::execute(CommandContext &ctx) {
  std::println("{} Record(s)", ctx.dacc->getRecordsCount());
  std::println("{} Byte(s) per record.", ctx.dacc->descriptor.getSizeInBytes());
  return false;
}

bool DescCmd::execute(CommandContext &ctx) {
  if (compact_)
    std::cout << ctx.colors.YELLOW << rdb::singleLineFormat << ctx.dacc->descriptor << ctx.colors.RESET << "\n";
  else
    std::cout << ctx.colors.YELLOW << ctx.dacc->descriptor << ctx.colors.RESET << "\n";
  return false;
}

std::pair<std::string, std::vector<std::string>> CapCmd::usage() const {
  return {"cap [value]", {"set device stream backread capacity"}};
}

bool CapCmd::execute(CommandContext &ctx) {
  int backCapacityValue{};
  std::cin >> backCapacityValue;
  ctx.dacc->setCapacity(backCapacityValue);
  return true;
}

std::pair<std::string, std::vector<std::string>> RoxCmd::usage() const {
  return {"rox", {"remove on exit flip (data, .desc, .meta)"}};
}

bool RoxCmd::execute(CommandContext &ctx) {
  ctx.rox = !ctx.rox;
  ctx.dacc->setDisposable(ctx.rox);
  return true;
}
