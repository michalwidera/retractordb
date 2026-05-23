#include "cmdFieldAccess.hpp"

#include <any>
#include <iostream>
#include <string>

std::pair<std::string, std::vector<std::string>> SetPosCmd::usage() const {
  return {"setpos [position][number value]", {"set payload field number value"}};
}

bool SetPosCmd::execute(CommandContext& ctx) {
  int position{0};
  std::cin >> position;
  auto fieldName = ctx.dacc->descriptor[position].rname;
  if (ctx.dacc->descriptor.fieldTypeName(fieldName) == "INTEGER") {
    int value{0};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (ctx.dacc->descriptor.fieldTypeName(fieldName) == "DOUBLE") {
    double value{0};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (ctx.dacc->descriptor.fieldTypeName(fieldName) == "BYTE") {
    uint8_t value{0};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (ctx.dacc->descriptor.fieldTypeName(fieldName) == "STRING") {
    std::string record{""};
    std::cin >> record;
    ctx.dacc->getPayload()->setItem(position, record);
  } else {
    std::cerr << "field not found\n";
  }
  ctx.payloadStatus = changed;
  return false;
}

std::pair<std::string, std::vector<std::string>> GetPosCmd::usage() const {
  return {"getpos [position]", {"show payload field value"}};
}

bool GetPosCmd::execute(CommandContext& ctx) {
  int position;
  std::cin >> position;
  auto fieldName = ctx.dacc->descriptor[position].rname;
  auto valueOpt  = ctx.dacc->getPayload()->getItem(position);
  if (!valueOpt.has_value()) {
    std::cout << fieldName << ": null" << std::endl;
    return false;
  }
  std::any value = valueOpt.value();
  if (value.type() == typeid(std::monostate)) std::cout << "null" << std::endl;
  if (value.type() == typeid(std::string)) std::cout << std::any_cast<std::string>(value) << std::endl;
  if (value.type() == typeid(int)) std::cout << std::any_cast<int>(value) << std::endl;
  if (value.type() == typeid(uint8_t)) std::cout << std::any_cast<uint8_t>(value) << std::endl;
  if (value.type() == typeid(float)) std::cout << std::any_cast<float>(value) << std::endl;
  if (value.type() == typeid(double)) std::cout << std::any_cast<double>(value) << std::endl;
  return true;
}
