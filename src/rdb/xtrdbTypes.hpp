#pragma once
#include <string>

enum payloadStatusType { fetched, clean, stored, changed, error };

struct Colors {
  std::string GREEN;
  std::string RED;
  std::string ORANGE;
  std::string BLUE;
  std::string YELLOW;
  std::string RESET;
  std::string BLINK;
};
