#pragma once
#include <string_view>

enum payloadStatusType { fetched, clean, stored, changed, error };

struct Colors {
  std::string_view GREEN;
  std::string_view RED;
  std::string_view ORANGE;
  std::string_view BLUE;
  std::string_view YELLOW;
  std::string_view RESET;
  std::string_view BLINK;
};
