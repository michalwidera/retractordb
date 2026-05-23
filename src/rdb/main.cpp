#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>  // make_unique
#include <string>

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

using namespace boost;

/*
 * This code creates xtrdb executable file.
 */

int main(int argc, char *argv[]) {
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

  const auto filelog = setupLoggerMain(std::string(argv[0]), false);

  std::string storagePolicy = "DEFAULT";

  bool cliNoPrompt = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0) {
      std::cout << argv[0] << " - data accessing tool." << std::endl << std::endl;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << filelog << std::endl;
      std::cout << warranty << std::endl;
      spdlog::shutdown();
      return system::errc::success;
    }
    if (strcmp(argv[i], "noprompt") == 0) {
      cliNoPrompt = true;
    }
  }

  if (cliNoPrompt) colors = {};

  {
    const auto lockPath = std::filesystem::temp_directory_path() / "xretractor_service.lock";
    int fd              = open(lockPath.c_str(), O_RDONLY);
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
  std::string storageParam = "";  // storage path parameter
  bool rox                 = true;
  std::string prompt       = cliNoPrompt ? "" : ".";
  std::string ok           = cliNoPrompt ? "" : "ok\n";

  CommandContext ctx{dacc, payloadStatus, file, storageParam, storagePolicy, colors};

  std::map<std::string, std::unique_ptr<ICommand>> commands;
  commands["append"]   = std::make_unique<AppendCmd>();
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
  commands["rread"]    = std::make_unique<ReadCmd>(true);
  commands["set"]      = std::make_unique<SetCmd>();
  commands["setpos"]   = std::make_unique<SetPosCmd>();
  commands["size"]     = std::make_unique<SizeCmd>();
  commands["status"]   = std::make_unique<StatusCmd>();
  commands["write"]    = std::make_unique<WriteCmd>();

  std::string cmd;
  std::string wasteComment;
  do {
    if (cmd != "#") std::cout << prompt;
    std::cin >> cmd;
    if (cmd == "exit" || cmd == "quit" || cmd == "q") break;
    if (cmd == "#" || cmd == "rem") {
      // This dual type of comment is required due fact that this tool is used on
      // pattern matching in unit/regression tests
      // rem - command act as normal xtrdb command - resulting ok and command prompt .
      // # - command act as dump comment - nothing reports - even . or ok
      // this # is used on commentary in test scripts, this rem is used when we want
      // turn of some functionality and do not change the output pattern is script
      // btw - I'm either surprised by two kinds of comments ...
      std::getline(std::cin, wasteComment);
      if (cmd == "rem") std::cout << ok;
      continue;
    }
    if (cmd == "quitdrop" || cmd == "qd") {
      if (dacc) dacc->setDisposable(true);
      break;
    }
    if (cmd == "echo") {
      std::getline(std::cin, wasteComment);
      std::cout << colors.BLINK << wasteComment << std::endl << colors.RESET;
      continue;
    }
    if (cmd == "storage") {
      std::cin >> storageParam;
      continue;
    }
    if (cmd == "policy") {
      std::cin >> storagePolicy;
      std::transform(storagePolicy.begin(), storagePolicy.end(), storagePolicy.begin(), ::toupper);
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
      std::cout << "exit|quit|q                     exit\n";
      std::cout << "quitdrop|qd                     exit & drop artifacts (data, .desc, .meta)\n";
      std::cout << "storage [path]                  set storage path for database\n";
      std::cout << "policy [name]                   set storage policy\n";
      std::cout << "rox                             remove on exit flip (data, .desc, .meta)\n";
      std::cout << "cap [value]                     set device stream backread capacity\n";
      std::cout << "echo                            print message on terminal\n";
      std::cout << "system                          execute system command\n";
      std::cout << "#|rem [text]                    comment line\n";
      std::cout << "help|h                          show this help\n";
      for (auto& [name, cmdPtr] : commands) {
        auto [cmd, desc] = cmdPtr->usage();
        if (cmd.empty()) continue;
        std::cout << std::left << std::setw(32) << cmd;
        if (!desc.empty()) std::cout << desc[0];
        std::cout << "\n";
        for (size_t i = 1; i < desc.size(); ++i)
          std::cout << std::string(32, ' ') << desc[i] << "\n";
      }
      std::cout << argv[0] << " - data accessing tool." << std::endl << std::endl;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << filelog << std::endl;
      std::cout << warranty << std::endl;
      std::cout << colors.RESET;
    } else if (!dacc) {
      std::cout << colors.RED << "unconnected\n" << colors.RESET;
      continue;
    } else if (cmd == "system") {
      std::string systemCommand;
      std::getline(std::cin, systemCommand);
      int returnCode = std::system(systemCommand.c_str());
      if (returnCode != 0) {
        std::cout << colors.RED << "system command error: " << returnCode << "\n" << colors.RESET;
      }
    } else if (cmd == "cap") {
      int backCapacityValue;
      std::cin >> backCapacityValue;
      dacc->setCapacity(backCapacityValue);
    } else if (cmd == "rox") {
      rox = !rox;
      dacc->setDisposable(rox);
    } else {
      std::cout << colors.RED << "?\n" << colors.RESET;
      continue;
    }
    std::cout << ok;
  } while (true);
  dacc.reset();
  spdlog::shutdown();
  return 0;
}
