#include "cmdFieldAccess.hpp"

#include <any>
#include <iostream>
#include <string>

void executeSetPos(rdb::storage& dacc, payloadStatusType& payloadStatus) {
  int position{0};
  std::cin >> position;
  auto fieldName = dacc.descriptor[position].rname;
  if (dacc.descriptor.fieldTypeName(fieldName) == "INTEGER") {
    int value{0};
    std::cin >> value;
    dacc.getPayload()->setItem(position, value);
  } else if (dacc.descriptor.fieldTypeName(fieldName) == "DOUBLE") {
    double value{0};
    std::cin >> value;
    dacc.getPayload()->setItem(position, value);
  } else if (dacc.descriptor.fieldTypeName(fieldName) == "BYTE") {
    uint8_t value{0};
    std::cin >> value;
    dacc.getPayload()->setItem(position, value);
  } else if (dacc.descriptor.fieldTypeName(fieldName) == "STRING") {
    std::string record{""};
    std::cin >> record;
    dacc.getPayload()->setItem(position, record);
  } else {
    std::cerr << "field not found\n";
  }
  payloadStatus = changed;
}

void executeGetPos(rdb::storage& dacc) {
  int position;
  std::cin >> position;
  auto fieldName = dacc.descriptor[position].rname;
  auto valueOpt  = dacc.getPayload()->getItem(position);
  if (!valueOpt.has_value()) {
    std::cout << fieldName << ": null" << std::endl;
    return;
  }
  std::any value = valueOpt.value();
  if (value.type() == typeid(std::monostate)) std::cout << "null" << std::endl;
  if (value.type() == typeid(std::string)) std::cout << std::any_cast<std::string>(value) << std::endl;
  if (value.type() == typeid(int)) std::cout << std::any_cast<int>(value) << std::endl;
  if (value.type() == typeid(uint8_t)) std::cout << std::any_cast<uint8_t>(value) << std::endl;
  if (value.type() == typeid(float)) std::cout << std::any_cast<float>(value) << std::endl;
  if (value.type() == typeid(double)) std::cout << std::any_cast<double>(value) << std::endl;
}
