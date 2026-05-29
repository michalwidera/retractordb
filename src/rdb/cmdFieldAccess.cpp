#include "cmdFieldAccess.hpp"

#include <any>
#include <iostream>

std::pair<std::string, std::vector<std::string>> SetPosCmd::usage() const {
  return {"setpos [position][number value]", {"set payload field number value"}};
}

bool SetPosCmd::execute(CommandContext &ctx) {
  int position{};
  std::cin >> position;
  const auto fieldName = ctx.dacc->descriptor[position].rname;
  const auto typeName  = ctx.dacc->descriptor.fieldTypeName(fieldName);
  if (typeName == "INTEGER") {
    int value{};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (typeName == "DOUBLE") {
    double value{};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (typeName == "BYTE") {
    uint8_t value{};
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else if (typeName == "STRING") {
    std::string value;
    std::cin >> value;
    ctx.dacc->getPayload()->setItem(position, value);
  } else {
    std::cerr << "field not found\n";
  }
  ctx.payloadStatus = changed;
  return false;
}

std::pair<std::string, std::vector<std::string>> GetPosCmd::usage() const {
  return {"getpos [position]", {"show payload field value"}};
}

bool GetPosCmd::execute(CommandContext &ctx) {
  int position;
  std::cin >> position;
  const auto fieldName = ctx.dacc->descriptor[position].rname;
  const auto valueOpt  = ctx.dacc->getPayload()->getItem(position);
  if (!valueOpt.has_value()) {
    std::cout << fieldName << ": null\n";
    return false;
  }
  const std::any &value = *valueOpt;
  if (value.type() == typeid(std::monostate))
    std::cout << "null\n";
  else if (value.type() == typeid(std::string))
    std::cout << std::any_cast<std::string>(value) << "\n";
  else if (value.type() == typeid(int))
    std::cout << std::any_cast<int>(value) << "\n";
  else if (value.type() == typeid(uint8_t))
    std::cout << static_cast<unsigned int>(std::any_cast<uint8_t>(value)) << "\n";
  else if (value.type() == typeid(float))
    std::cout << std::any_cast<float>(value) << "\n";
  else if (value.type() == typeid(double))
    std::cout << std::any_cast<double>(value) << "\n";
  return true;
}
