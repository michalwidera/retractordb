#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "ICommand.hpp"
#include "cmdDropFile.hpp"
#include "cmdFieldAccess.hpp"
#include "cmdMeta.hpp"
#include "cmdOpen.hpp"
#include "cmdPayload.hpp"
#include "cmdStatus.hpp"
#include "cmdStorage.hpp"
#include "config.h"
#include "rdb/storageacc.hpp"
#include "uxSysTermTools.hpp"
#include "xtrdbTypes.hpp"

int main(int argc, char* argv[]) {
  payloadStatusType payloadStatus{clean};

  Colors colors{
      .GREEN  = "\x1B[32m",
      .RED    = "\x1B[31m",
      .ORANGE = "\x1B[33m",
      .BLUE   = "\x1B[34m",
      .YELLOW = "\x1B[93m",
      .RESET  = "\033[0m",
      .BLINK  = "\x1b[5m",
  };

  const auto filelog    = setupLoggerMain(std::string(argv[0]), false);
  std::string storagePolicy = "DEFAULT";
  bool cliNoPrompt      = false;

  for (std::string_view arg : std::span(argv + 1, static_cast<size_t>(argc - 1))) {
    if (arg == "-h") {
      std::cout << argv[0] << " - data accessing tool.\n\n"
                << config_line << "\nLog: " << filelog << "\n"
                << warranty << "\n";
      spdlog::shutdown();
      return 0;
    }
    if (arg == "noprompt") cliNoPrompt = true;
  }

  if (cliNoPrompt) colors = {};

  {
    const auto lockPath = std::filesystem::temp_directory_path() / "xretractor_service.lock";
    const int fd        = open(lockPath.c_str(), O_RDONLY);
    if (fd != -1) {
      const bool running = (flock(fd, LOCK_SH | LOCK_NB) == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));
      close(fd);
      if (running) {
        std::cerr << "xretractor is running — stop it before using xtrdb.\n";
        spdlog::shutdown();
        return 1;
      }
    }
  }

  std::unique_ptr<rdb::storage> dacc;
  std::string file;
  std::string storageParam;
  bool rox                  = true;
  std::string_view prompt   = cliNoPrompt ? "" : ".";
  std::string_view ok       = cliNoPrompt ? "" : "ok\n";

  CommandContext ctx{dacc, payloadStatus, file, storageParam, storagePolicy, colors, rox};

  std::map<std::string, std::unique_ptr<ICommand>> commands;
  commands["append"]   = std::make_unique<AppendCmd>();
  commands["cap"]      = std::make_unique<CapCmd>();
  commands["dec"]      = std::make_unique<HexCmd>(false);
  commands["desc"]     = std::make_unique<DescCmd>(false);
  commands["descc"]    = std::make_unique<DescCmd>(true);
  commands["dropfile"] = std::make_unique<DropFileCmd>();
  commands["dump"]     = std::make_unique<DumpCmd>();
  commands["getpos"]   = std::make_unique<GetPosCmd>();
  commands["hex"]      = std::make_unique<HexCmd>(true);
  commands["input"]    = std::make_unique<InputCmd>();
  commands["list"]     = std::make_unique<ListCmd>(false);
  commands["meta"]     = std::make_unique<MetaCmd>();
  commands["metaraw"]  = std::make_unique<MetaRawCmd>();
  commands["open"]     = std::make_unique<OpenCmd>();
  commands["print"]    = std::make_unique<PrintCmd>(false);
  commands["printt"]   = std::make_unique<PrintCmd>(true);
  commands["purge"]    = std::make_unique<PurgeCmd>();
  commands["read"]     = std::make_unique<ReadCmd>(false);
  commands["rlist"]    = std::make_unique<ListCmd>(true);
  commands["rox"]      = std::make_unique<RoxCmd>();
  commands["rread"]    = std::make_unique<ReadCmd>(true);
  commands["set"]      = std::make_unique<SetCmd>();
  commands["setpos"]   = std::make_unique<SetPosCmd>();
  commands["size"]     = std::make_unique<SizeCmd>();
  commands["status"]   = std::make_unique<StatusCmd>();
  commands["write"]    = std::make_unique<WriteCmd>();

  std::string cmd;
  while (true) {
    if (cmd != "#") std::cout << prompt;
    std::cin >> cmd;
    if (cmd == "exit" || cmd == "quit" || cmd == "q") break;
    if (cmd == "#" || cmd == "rem") {
      // Two comment styles: # suppresses output entirely (used in test scripts),
      // rem behaves like a normal command (prints ok). Both consume the rest of the line.
      std::string rest;
      std::getline(std::cin, rest);
      if (cmd == "rem") std::cout << ok;
      continue;
    }
    if (cmd == "quitdrop" || cmd == "qd") {
      if (dacc) dacc->setDisposable(true);
      break;
    }
    if (cmd == "echo") {
      std::string rest;
      std::getline(std::cin, rest);
      std::cout << colors.BLINK << rest << "\n" << colors.RESET;
      continue;
    }
    if (cmd == "storage") { std::cin >> storageParam; continue; }
    if (cmd == "policy") {
      std::cin >> storagePolicy;
      std::ranges::transform(storagePolicy, storagePolicy.begin(), ::toupper);
      continue;
    }

    if (auto it = commands.find(cmd); it != commands.end()) {
      if (it->second->requiresConnection() && !dacc) {
        std::cout << colors.RED << "unconnected\n" << colors.RESET;
        continue;
      }
      if (!it->second->execute(ctx)) continue;
    } else if (cmd == "help" || cmd == "h") {
      std::cout << colors.GREEN;
      for (auto [c, d] : std::initializer_list<std::pair<std::string_view, std::string_view>>{
               {"exit|quit|q",    "exit"},
               {"quitdrop|qd",    "exit & drop artifacts (data, .desc, .meta)"},
               {"storage [path]", "set storage path for database"},
               {"policy [name]",  "set storage policy"},
               {"echo",           "print message on terminal"},
               {"system",         "execute system command"},
               {"#|rem [text]",   "comment line"},
               {"help|h",         "show this help"},
           })
        std::cout << std::left << std::setw(32) << c << d << "\n";
      for (auto& [name, cmdPtr] : commands) {
        auto [cmd, desc] = cmdPtr->usage();
        if (cmd.empty()) continue;
        std::cout << std::left << std::setw(32) << cmd;
        if (!desc.empty()) std::cout << desc[0];
        std::cout << "\n";
        for (size_t i = 1; i < desc.size(); ++i)
          std::cout << std::string(32, ' ') << desc[i] << "\n";
      }
      std::cout << argv[0] << " - data accessing tool.\n\n"
                << config_line << "\nLog: " << filelog << "\n"
                << warranty << "\n" << colors.RESET;
    } else if (!dacc) {
      std::cout << colors.RED << "unconnected\n" << colors.RESET;
      continue;
    } else if (cmd == "system") {
      std::string systemCommand;
      std::getline(std::cin, systemCommand);
      if (const int rc = std::system(systemCommand.c_str()); rc != 0)
        std::cout << colors.RED << "system command error: " << rc << "\n" << colors.RESET;
    } else {
      std::cout << colors.RED << "?\n" << colors.RESET;
      continue;
    }
    std::cout << ok;
  }
  dacc.reset();
  spdlog::shutdown();
  return 0;
}
