#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>  // make_unique
#include <string>

#include "rdb/descriptor.h"
#include "rdb/faccfs.h"
#include "rdb/faccposix.h"
#include "rdb/payload.h"
#include "rdb/storageacc.h"

std::string GREEN = "\x1B[32m";
std::string RED = "\x1B[31m";
std::string ORANGE = "\x1B[33m";
std::string BLUE = "\x1B[34m";
std::string YELLOW = "\x1B[93m";
std::string RESET = "\033[0m";

constexpr auto common_log_pattern = "%C%m%d %T.%e %^%s:%# [%L] %v%$";

/*
 * This code creates xtrdb executable file.
 */

enum payloadStatusType { fetched, clean, stored, changed, error };

payloadStatusType payloadStatus(clean);

int main(int argc, char* argv[]) {
  auto filelog = spdlog::basic_logger_mt("log", std::string(argv[0]) + ".log");
  spdlog::set_default_logger(filelog);
  spdlog::set_pattern(common_log_pattern);
  spdlog::flush_on(spdlog::level::trace);

  std::unique_ptr<rdb::storageAccessor> dacc;
  std::string file;
  bool reverse = false;
  bool rox = true;
  bool mono = false;
  std::string cmd;
  std::string wasteComment;
  do {
    if (cmd != "#") std::cout << ".";
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
      if (cmd == "rem") std::cout << "ok\n";
      continue;
    }
    if (cmd == "mono") {
      GREEN = "";
      RED = "";
      ORANGE = "";
      BLUE = "";
      YELLOW = "";
      RESET = "";
      std::cout << "ok\n";
      continue;
    }
    if (cmd == "quitdrop" || cmd == "qd") {
      if (dacc) dacc->setRemoveOnExit(true);
      break;
    }
    if (cmd == "open" || cmd == "ropen" || cmd == "openx" || cmd == "ropenx") {
      std::cin >> file;
      if (cmd == "open" || cmd == "ropen")
        dacc = std::make_unique<rdb::storageAccessor>(file, file);
      else {
        std::string descFile;
        std::cin >> descFile;
        dacc = std::make_unique<rdb::storageAccessor>(descFile, file);
      }
      if (dacc->peekDescriptor()) {
        dacc->attachDescriptor();  // we are sure here that descriptor file exist
      } else {
        //
        // no descriptor found - need to create
        //
        std::string sschema;
        std::string object;
        do {
          std::cin >> object;
          sschema.append(object);
          sschema.append(" ");
        } while (object.find("}") == std::string::npos);
        std::stringstream scheamStringStream(sschema);
        rdb::Descriptor desc;
        scheamStringStream >> desc;
        dacc->attachDescriptor(&desc);
      }
      dacc->setReverse(cmd == "ropen" || cmd == "ropenx");
      payloadStatus = clean;
      dacc->setRemoveOnExit(false);
    } else if (cmd == "help" || cmd == "h") {
      std::cout << GREEN;
      std::cout << "exit|quit|q \t\t\t exit\n";
      std::cout << "quitdrop|qd \t\t\t exit & drop artifacts\n";
      std::cout << "open|ropen file [schema] \t open or create database with schema (r-reverse iterator)\n";
      std::cout << "openx|ropenx desc file [schema] \n";
      std::cout << "\t\t\t\t example: .open test_db { INTEGER dane STRING name[3] } \n";
      std::cout << "desc|descc \t\t\t show schema\n";
      std::cout << "read [n] \t\t\t read record from database into payload\n";
      std::cout << "write [n] \t\t\t send record to database from payload\n";
      std::cout << "append \t\t\t\t append payload to database\n";
      std::cout << "set [field][value] \t\t set payload field value\n";
      std::cout << "setpos [position][number value]\t set payload field number value\n";
      std::cout << "status \t\t\t\t show status of payload\n";
      std::cout << "flip \t\t\t\t flip reverse iterator\n";
      std::cout << "rox \t\t\t\t remove on exit flip\n";
      std::cout << "print|printt \t\t\t show payload\n";
      std::cout << "list|rlist [value] \t\t print value records\n";
      std::cout << "input [[field][value]] \t\t fill payload\n";
      std::cout << "hex|dec \t\t\t type of input/output of byte/number fields\n";
      std::cout << "size \t\t\t\t show database size in records\n";
      std::cout << "dump \t\t\t\t show payload memory\n";
      std::cout << "mono \t\t\t\t no color mode\n";
      std::cout << "fetch [amount] \t\t\t fetch and print amount of data from database\n";
      std::cout << RESET;
    } else if (cmd == "dropfile") {
      std::string object;
      do {
        object.clear();
        std::cin >> object;
        if (std::filesystem::exists(object)) {
          std::filesystem::remove(object);
        }
      } while (object.find("}") == std::string::npos);
    } else if (!dacc) {
      std::cout << RED << "unconnected\n" << RESET;
      continue;
    } else if (cmd == "desc") {
      std::cout << YELLOW << dacc->getDescriptor() << RESET << std::endl;
      continue;
    } else if (cmd == "descc") {
      std::cout << YELLOW << rdb::flat << dacc->getDescriptor() << RESET << std::endl;
      continue;
    } else if (cmd == "read") {
      int record;
      std::cin >> record;
      if (record >= dacc->getRecordsCount()) {
        std::cout << RED << "record out of range\n" << RESET;
        continue;
      }
      auto returnStatus = dacc->read(record);
      if (returnStatus)
        payloadStatus = fetched;
      else
        payloadStatus = error;
    } else if (cmd == "set") {
      std::cin >> *(dacc->getPayload());
      payloadStatus = changed;
      continue;
    } else if (cmd == "setpos") {
      int position;
      std::cin >> position;
      auto fieldName = dacc->getDescriptor().fieldName(position);
      if (dacc->getDescriptor().type(fieldName) == "INTEGER") {
        int value;
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldName) == "DOUBLE") {
        double value;
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldName) == "BYTE") {
        uint8_t value;
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldName) == "STRING") {
        std::string record;
        std::cin >> record;
        dacc->getPayload()->setItem(position, record);
      } else
        std::cerr << "field not found\n";
      payloadStatus = changed;
      continue;
    } else if (cmd == "getpos") {
      int position;
      std::cin >> position;
      auto fieldName = dacc->getDescriptor().fieldName(position);
      std::any value = dacc->getPayload()->getItem(position);
      if (value.type() == typeid(std::string)) {
        std::cout << std::any_cast<std::string>(value) << std::endl;
      }
      if (value.type() == typeid(int)) {
        std::cout << std::any_cast<int>(value) << std::endl;
      }
      if (value.type() == typeid(uint8_t)) {
        std::cout << std::any_cast<uint8_t>(value) << std::endl;
      }
      if (value.type() == typeid(float)) {
        std::cout << std::any_cast<float>(value) << std::endl;
      }
      if (value.type() == typeid(double)) {
        std::cout << std::any_cast<double>(value) << std::endl;
      }
    } else if (cmd == "flip") {
      reverse = !reverse;
      dacc->setReverse(reverse);
    } else if (cmd == "rox") {
      rox = !rox;
      dacc->setRemoveOnExit(rox);
    } else if (cmd == "fetch") {
      size_t wantFetchRecords;
      std::cin >> wantFetchRecords;
      wantFetchRecords = std::min(wantFetchRecords, dacc->getRecordsCount());
      for (auto i = 0; i < wantFetchRecords; i++) {
        // read part
        auto returnStatus = dacc->read(i);
        if (returnStatus)
          payloadStatus = fetched;
        else
          payloadStatus = error;
        // print part

        if (returnStatus) std::cout << ORANGE << *(dacc->getPayload()) << RESET;
      }
      continue;
    } else if (cmd == "print") {
      std::cout << ORANGE << *(dacc->getPayload()) << RESET;
      continue;
    } else if (cmd == "printt") {
      std::cout << ORANGE << rdb::flat << *(dacc->getPayload()) << RESET << std::endl;
      continue;
    } else if (cmd == "list" || cmd == "rlist") {
      int record;
      std::cin >> record;
      for (auto i = 0 ; i < record ; i++ ) {
        if ( i >= dacc->getRecordsCount()) {
          std::cout << RED << "record out of range\n" << RESET;
          continue;
        }
        auto returnStatus = (cmd == "list") ? dacc->read(i) : dacc->read(dacc->getRecordsCount() - i - 1) ;
        if (returnStatus)
          payloadStatus = fetched;
        else
          payloadStatus = error;

        if (payloadStatus == error) {
          std::cout << RED << "fetch error\n" << RESET;
          continue;
        }
        std::cout << ORANGE << rdb::flat << *(dacc->getPayload()) << RESET << std::endl;
      }
      continue;
    } else if (cmd == "input") {
      for (auto i : dacc->getDescriptor()) std::cin >> *(dacc->getPayload());
      continue;
    } else if (cmd == "write") {
      size_t record;
      std::cin >> record;
      if (record >= dacc->getRecordsCount()) {
        std::cout << RED << "record out of range - Check append command.\n" << RESET;
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
      std::cout << dacc->getDescriptor().getSizeInBytes() << " Byte(s) per record.\n";
      continue;
    } else if (cmd == "hex") {
      dacc->getPayload()->setHex(true);
    } else if (cmd == "dec") {
      dacc->getPayload()->setHex(false);
    } else if (cmd == "dump") {
      auto* ptr = reinterpret_cast<uint8_t*>(dacc->getPayload()->get());
      for (auto i = 0; i < dacc->getDescriptor().getSizeInBytes(); i++) {
        std::cout << std::hex;
        std::cout << std::setfill('0');
        std::cout << std::setw(2);
        std::cout << static_cast<unsigned>(*(ptr + i));
        std::cout << std::dec;
        std::cout << " ";
      }
      std::cout << "\n";
    } else {
      std::cout << RED << "?\n" << RESET;
      continue;
    }
    std::cout << "ok\n";
  } while (true);
  // use '$xxd datafile-11' to check
  return 0;
}
