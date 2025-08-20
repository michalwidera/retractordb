#include <fcntl.h>
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>  //remove it with std::
#include <string>

int _kbhit(void) {
  struct termios oldt;
  struct termios newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

int _getch() { return getchar(); }

void fixArgcv(int argc, char *argv[]) {
  // Clarification: When gcc has been upgraded to 9.x version some tests fails.
  // Bug appear when data are passing to program via script .sh
  // additional 13 (\r) character was append - this code normalize argv list.
  // C99: The parameters argc and argv and the strings pointed to by the argv
  // array shall be modifiable by the program, and retain their last-stored
  // values between program startup and program termination.
  if (argc > 0 && argv != nullptr)
    for (int i = 0; i < argc; ++i) {
      auto len = strlen(argv[i]);
      if (len > 0)
        if (argv[i][len - 1] == 13) argv[i][len - 1] = 0;
    }
}

// https://github.com/gabime/spdlog/wiki/2.-Creating-loggers#creating-loggers-with-multiple-sinks
std::string setupLoggerMain(const std::string &loggerFile, bool dual) {
  namespace fs = std::filesystem;
  fs::path tmp;

  const auto loggerFileSole = fs::path(loggerFile).filename();
  assert(!loggerFileSole.empty());

  // Functional description: system first checks if in current folder
  // there is temp folder - if found we stop looking and temp folder became log folder
  // then is there temp folder in home directory - if found we stop looking and this is log
  // next with tmp - the same process
  // if none of them have been found - we are trying to get system temp folder /tmp
  // if temporary folder is not found - we stop on assert failure.

  /*
  conflict with temp in rql
  for (const auto &tempName : std::vector<std::string>{"temp", "tmp"}) {
    const auto currentPath{fs::current_path().append(tempName)};
    if (fs::is_directory(currentPath) && tmp.empty()) tmp = currentPath;

    const auto homeDir{fs::path(getenv("HOME")).append(tempName)};
    if (fs::is_directory(homeDir) && tmp.empty()) tmp = homeDir;
  }
  */

  if (tmp.empty()) tmp = fs::temp_directory_path();

  assert(!tmp.empty());

  tmp.append(loggerFileSole.string());

  constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";

  if (dual) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();

    console_sink->set_pattern("%v%$");
    console_sink->set_level(spdlog::level::trace);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(tmp.string() + ".log", false /* truncate log */);

    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern(common_log_pattern);

    std::vector<spdlog::sink_ptr> sinks({console_sink, file_sink});

    auto combined_logger = std::make_shared<spdlog::logger>("mlog", sinks.begin(), sinks.end());
    spdlog::register_logger(combined_logger);
    spdlog::set_default_logger(spdlog::get("mlog"));

  } else {
    auto filelog = spdlog::basic_logger_mt("log", "/tmp/retractor-logs/" + tmp.string() + ".log");
    spdlog::set_default_logger(filelog);

    spdlog::set_pattern(common_log_pattern);
    spdlog::flush_on(spdlog::level::trace);
  }
  return tmp.string() + ".log";
}
