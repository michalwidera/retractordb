#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>  // make_unique
#include <string>

#include "cmdFieldAccess.hpp"
#include "cmdMeta.hpp"
#include "cmdOpen.hpp"
#include "config.h"
#include "rdb/descriptor.hpp"
#include "rdb/payload.hpp"
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
    if (cmd == "open") {
      if (!executeOpen(dacc, file, storageParam, storagePolicy, payloadStatus, colors)) continue;
    } else if (cmd == "help" || cmd == "h") {
      std::cout << colors.GREEN;
      std::cout << "exit|quit|q                     exit\n";
      std::cout << "quitdrop|qd                     exit & drop artifacts (data, .desc, .meta)\n";
      std::cout << "open file [schema]              open or create database with schema\n";
      std::cout << "                                example: .open test_db { INTEGER dane STRING name[3] }\n";
      std::cout << "storage [path]                  set storage path for database\n";
      std::cout << "policy [name]                   set storage policy\n";
      std::cout << "dropfile [file1] [file2] ... }  remove listed file(s), end with }\n";
      std::cout << "desc|descc                      show schema\n";
      std::cout << "read|rread [n]                  read record from database into payload\n";
      std::cout << "write [n]                       from payload send record to database\n";
      std::cout << "purge                           remove all records from database\n";
      std::cout << "append                          append payload to database\n";
      std::cout << "set [field][value]              set payload field value\n";
      std::cout << "setpos [position][number value] set payload field number value\n";
      std::cout << "getpos [position]               show payload field value\n";
      std::cout << "status                          show current payload status\n";
      std::cout << "rox                             remove on exit flip (data, .desc, .meta)\n";
      std::cout << "print|printt                    show payload\n";
      std::cout << "list|rlist [count]              print first records\n";
      std::cout << "input [[field][value]]          fill payload\n";
      std::cout << "hex|dec                         type of input/output of byte/number fields\n";
      std::cout << "size                            show database size in records\n";
      std::cout << "cap [value]                     set device stream backread capacity\n";
      std::cout << "dump                            show payload memory\n";
      std::cout << "meta                            show meta index (null patterns) for open db\n";
      std::cout << "metaraw                         show internal meta file structure\n";
      std::cout << "echo                            print message on terminal\n";
      std::cout << "system                          execute system command\n";
      std::cout << "#|rem [text]                    comment line\n";
      std::cout << "help|h                          show this help\n";
      std::cout << argv[0] << " - data accessing tool." << std::endl << std::endl;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << filelog << std::endl;
      std::cout << warranty << std::endl;
      std::cout << colors.RESET;
    } else if (cmd == "dropfile") {
      std::string object;
      do {
        object.clear();
        std::cin >> object;
        if (storageParam != "" && object.find(storageParam) == std::string::npos) {
          object = storageParam + "/" + object;
        }
        if (std::filesystem::exists(object)) {
          std::filesystem::remove(object);
        }
      } while (object.find("}") == std::string::npos);
    } else if (!dacc) {
      std::cout << colors.RED << "unconnected\n" << colors.RESET;
      continue;
    } else if (cmd == "desc") {
      std::cout << colors.YELLOW << dacc->descriptor << colors.RESET << std::endl;
      continue;
    } else if (cmd == "descc") {
      std::cout << colors.YELLOW << rdb::singleLineFormat << dacc->descriptor << colors.RESET << std::endl;
      continue;
    } else if (cmd == "read" || cmd == "rread") {
      int record;
      std::cin >> record;
      if (!dacc->isDeclared() && record >= dacc->getRecordsCount()) {
        std::cout << colors.RED << "record out of range - read command\n" << colors.RESET;
        continue;
      }
      if (dacc->isDeclared()) {
        dacc->bufferState = rdb::sourceState::flux;
      }
      auto returnStatus = (cmd == "read") ? dacc->read(record) : dacc->revRead(record);
      if (dacc->isDeclared()) {
        dacc->fire();
      }
      payloadStatus = returnStatus ? fetched : error;
    } else if (cmd == "system") {
      std::string systemCommand;
      std::getline(std::cin, systemCommand);
      int returnCode = std::system(systemCommand.c_str());
      if (returnCode != 0) {
        std::cout << colors.RED << "system command error: " << returnCode << "\n" << colors.RESET;
      }
    } else if (cmd == "set") {
      std::cin >> *(dacc->getPayload());
      payloadStatus = changed;
      continue;
    } else if (cmd == "setpos") {
      executeSetPos(*dacc, payloadStatus);
      continue;
    } else if (cmd == "getpos") {
      executeGetPos(*dacc);
    } else if (cmd == "cap") {
      int backCapacityValue;
      std::cin >> backCapacityValue;
      dacc->setCapacity(backCapacityValue);
    } else if (cmd == "rox") {
      rox = !rox;
      dacc->setDisposable(rox);
    } else if (cmd == "print") {
      std::cout << colors.ORANGE << *(dacc->getPayload()) << colors.RESET;
      continue;
    } else if (cmd == "printt") {
      std::cout << colors.ORANGE << rdb::singleLineFormat << *(dacc->getPayload()) << colors.RESET << std::endl;
      continue;
    } else if (cmd == "list" || cmd == "rlist") {
      int record{0};
      std::cin >> record;
      for (auto i = 0; i < record; i++) {
        if (i >= dacc->getRecordsCount()) {
          std::cout << colors.RED << "record out of range - list command\n" << colors.RESET;
          continue;
        }
        auto returnStatus = (cmd == "rlist") ? dacc->revRead(i) : dacc->read(i);
        payloadStatus     = returnStatus ? fetched : error;

        if (payloadStatus == error) {
          std::cout << colors.RED << "fetch error\n" << colors.RESET;
          continue;
        }
        std::cout << colors.ORANGE << rdb::singleLineFormat << *(dacc->getPayload()) << colors.RESET << std::endl;
      }
      continue;
    } else if (cmd == "input") {
      for (auto i : dacc->descriptor)
        std::cin >> *(dacc->getPayload());
      continue;
    } else if (cmd == "write") {
      size_t record{0};
      std::cin >> record;
      if (record >= dacc->getRecordsCount()) {
        std::cout << colors.RED << "record out of range - Check append command.\n" << colors.RESET;
        continue;
      }
      auto writeStatus = dacc->write(record);
      if (writeStatus == 0)
        payloadStatus = stored;
      else
        payloadStatus = error;
    } else if (cmd == "append") {
      dacc->write();
      payloadStatus = stored;
    } else if (cmd == "purge") {
      dacc->purge();
    } else if (cmd == "status") {
      switch (payloadStatus) {
        case (fetched):
          std::cout << "fetched\n";
          break;
        case (clean):
          std::cout << "clean\n";
          break;
        case (stored):
          std::cout << "stored\n";
          break;
        case (changed):
          std::cout << "changed\n";
          break;
      }
    } else if (cmd == "size") {
      std::cout << dacc->getRecordsCount() << " Record(s)\n";
      std::cout << dacc->descriptor.getSizeInBytes() << " Byte(s) per record.\n";
      continue;
    } else if (cmd == "hex") {
      dacc->getPayload()->setHex(true);
    } else if (cmd == "dec") {
      dacc->getPayload()->setHex(false);
    } else if (cmd == "dump") {
      auto payloadSpan = dacc->getPayload()->span();
      for (auto byte : payloadSpan) {
        std::cout << std::hex;
        std::cout << std::setfill('0');
        std::cout << std::setw(2);
        std::cout << static_cast<unsigned>(byte);
        std::cout << std::dec;
        std::cout << " ";
      }
      std::cout << "\n";
    } else if (cmd == "meta") {
      executeMeta(*dacc, file, storageParam, colors);
      continue;
    } else if (cmd == "metaraw") {
      executeMetaRaw(*dacc, file, storageParam, colors);
      continue;
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
