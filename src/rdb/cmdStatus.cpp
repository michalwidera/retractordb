#include "cmdStatus.hpp"

#include <iostream>

std::pair<std::string, std::vector<std::string>> StatusCmd::usage() const {
  return {"status", {"show current payload status"}};
}

bool StatusCmd::execute(CommandContext& ctx) {
  switch (ctx.payloadStatus) {
    case fetched: std::cout << "fetched\n"; break;
    case clean:   std::cout << "clean\n";   break;
    case stored:  std::cout << "stored\n";  break;
    case changed: std::cout << "changed\n"; break;
    case error:   std::cout << "error\n";   break;
  }
  return true;
}
