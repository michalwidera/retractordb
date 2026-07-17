#include "uxSysTermTools.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>  //remove it with std::
#include <memory>
#include <string>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "fatalError.hpp"

namespace {
constexpr char kCarriageReturn = '\r';

// Priorytety syslog (man sd-daemon / RFC 5424) używane w prefiksie sd-daemon "<N>".
constexpr int kPrioCrit    = 2;  // LOG_CRIT
constexpr int kPrioErr     = 3;  // LOG_ERR
constexpr int kPrioWarning = 4;  // LOG_WARNING
constexpr int kPrioInfo    = 6;  // LOG_INFO
constexpr int kPrioDebug   = 7;  // LOG_DEBUG

// Flaga patternu '%*' dla trybu usługowego: prefiks priorytetu sd-daemon "<N>" na początku linii.
// journald odczytuje ten prefiks i klasyfikuje wagę komunikatu (man sd-daemon). Mapowanie poziomu
// spdlog -> priorytet syslog: critical=2, err=3, warn=4, info=6, debug/trace=7.
class SdPriorityFlag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg &msg, const std::tm & /*tm_time*/, spdlog::memory_buf_t &dest) override {
    int prio = kPrioInfo;  // domyślnie LOG_INFO
    switch (msg.level) {
      case spdlog::level::critical:
        prio = kPrioCrit;
        break;
      case spdlog::level::err:
        prio = kPrioErr;
        break;
      case spdlog::level::warn:
        prio = kPrioWarning;
        break;
      case spdlog::level::debug:
      case spdlog::level::trace:
        prio = kPrioDebug;
        break;
      default:
        prio = kPrioInfo;
        break;
    }
    const char c = static_cast<char>('0' + prio);
    dest.push_back(c);
  }

  [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override { return std::make_unique<SdPriorityFlag>(); }
};
}  // namespace

bool _kbhit(bool ignoreAnyKey) {
  if (ignoreAnyKey) return false;
  struct termios oldt = {};
  struct termios newt = {};
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
    return true;
  }
  return false;
}

int _getch() { return getchar(); }

void fixArgcv(int argc, char **argv) {
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
        if (argv[i][len - 1] == kCarriageReturn) argv[i][len - 1] = 0;
    }
}

// https://github.com/gabime/spdlog/wiki/2.-Creating-loggers#creating-loggers-with-multiple-sinks
std::string setupLoggerMain(const std::string &loggerFile, bool dual, bool service) {
  namespace fs = std::filesystem;
  fs::path tmp;

  const auto loggerFileSole = fs::path(loggerFile).filename();

  // Tryb usługi systemowej: logujemy na stderr (przechwytywany przez journald),
  // bez pliku w /tmp, bez własnego znacznika czasu i bez kodów ANSI; flush po każdej linii.
  if (service) {
    auto journal_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
    // Pattern z prefiksem priorytetu sd-daemon "<N>" (flaga '%*') na początku linii — journald
    // klasyfikuje wagę. Dalej zwięźle: poziom + treść, bez własnego znacznika czasu i bez ANSI.
    auto journal_formatter = std::make_unique<spdlog::pattern_formatter>();
    journal_formatter->add_flag<SdPriorityFlag>('*').set_pattern("<%*>[%L] %v");
    journal_sink->set_formatter(std::move(journal_formatter));
    journal_sink->set_level(spdlog::level::trace);

    auto journal_logger = std::make_shared<spdlog::logger>("log", journal_sink);
    spdlog::set_default_logger(journal_logger);
    // Poziom runtime = poziom kompilacji (Release: tylko błędy, Debug: pełna diagnostyka).
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    spdlog::flush_on(spdlog::level::trace);
    return "journald (stderr)";
  }

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
    auto filelog = spdlog::basic_logger_mt("log", tmp.string() + ".log");
    spdlog::set_default_logger(filelog);

    spdlog::set_pattern(common_log_pattern);
    spdlog::flush_on(spdlog::level::trace);
  }
  // Poziom runtime = poziom kompilacji (Release: tylko błędy trafiają do pliku logu na
  // urządzeniu pomiarowym, gdzie /tmp leży na karcie SD; Debug: pełna diagnostyka).
  spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
  return tmp.string() + ".log";
}
