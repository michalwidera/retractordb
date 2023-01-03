#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/basic_file_sink.h"  // support for basic file logging

constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";

int main(int argc, char *argv[]) {
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}