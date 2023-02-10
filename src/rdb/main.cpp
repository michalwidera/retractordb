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
#include "spdlog/sinks/basic_file_sink.h"  // support for basic file logging
#include "spdlog/spdlog.h"

#define GREEN "\x1B[32m"
#define RED "\x1B[31m"
#define ORANGE "\x1B[33m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[93m"
#define RESET "\033[0m"

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
  std::unique_ptr<rdb::payload> payloadAcc;
  std::string file;
  bool reverse = false;
  bool rox = true;
  std::string cmd;
  do {
    std::cout << ".";
    std::cin >> cmd;
    if (cmd == "exit" || cmd == "quit" || cmd == "q") break;
    if (cmd == "quitdrop" || cmd == "qd") {
      if (dacc) dacc->setRemoveOnExit(true);
      break;
    }
    if (cmd == "open" || cmd == "ropen") {
      std::cin >> file;
      dacc = std::make_unique<rdb::storageAccessor>(file);
      if (dacc->peekDescriptor()) {
        dacc->attachDescriptor();  // we are sure here that decriptor file exist
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
      dacc->attachStorage();
      dacc->setReverse(cmd == "ropen");
      payloadAcc = std::make_unique<rdb::payload>(dacc->getDescriptor());
      dacc->attachPayload(*payloadAcc);
      payloadStatus = clean;
      dacc->setRemoveOnExit(false);
    } else if (cmd == "help" || cmd == "h") {
      std::cout << GREEN;
      std::cout << "exit|quit|q \t\t\t exit\n";
      std::cout << "quitdrop|qd \t\t\t exit & drop artifacts\n";
      std::cout << "open|ropen file [schema] \t open or create database with schema (r-reverse iterator)\n";
      std::cout << "\t\t\t\t example: .open test_db { INTEGER dane STRING name[3] } \n";
      std::cout << "desc \t\t\t\t show schema\n";
      std::cout << "read [n] \t\t\t read record from database into payload\n";
      std::cout << "write [n] \t\t\t send record to database from payload\n";
      std::cout << "append \t\t\t\t append payload to database\n";
      std::cout << "set [field][value] \t\t set payload field value\n";
      std::cout << "setpos [position][number value]\t set payload field number value\n";
      std::cout << "status \t\t\t\t show status of payload\n";
      std::cout << "flip \t\t\t\t flip reverse iterator\n";
      std::cout << "rox \t\t\t\t remove on exit flip\n";
      std::cout << "print \t\t\t\t show payload\n";
      std::cout << "hex|dec \t\t\t type of input/output of byte/number fields\n";
      std::cout << "size \t\t\t\t show database size in records\n";
      std::cout << "dump \t\t\t\t show payload memory\n";
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
      std::cout << RED "unconnected\n" RESET;
      continue;
    } else if (cmd == "desc") {
      std::cout << YELLOW << dacc->getDescriptor() << RESET << std::endl;
      continue;
    } else if (cmd == "read") {
      int record;
      std::cin >> record;
      if (record >= dacc->getRecordsCount()) {
        std::cout << RED "record out of range\n" RESET;
        continue;
      }
      auto returnStatus = dacc->read(record);
      if (returnStatus != 0)
        payloadStatus = error;
      else
        payloadStatus = fetched;
    } else if (cmd == "set") {
      std::cin >> *payloadAcc;
      payloadStatus = changed;
      continue;
    } else if (cmd == "setpos") {
      int position;
      std::cin >> position;
      auto fieldname = dacc->getDescriptor().fieldName(position);
      if (dacc->getDescriptor().type(fieldname) == "INTEGER") {
        int value;
        std::cin >> value;
        payloadAcc->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldname) == "DOUBLE") {
        double value;
        std::cin >> value;
        payloadAcc->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldname) == "BYTE") {
        unsigned char value;
        std::cin >> value;
        payloadAcc->setItem(position, value);
      } else if (dacc->getDescriptor().type(fieldname) == "STRING") {
        std::string record;
        std::cin >> record;
        payloadAcc->setItem(position, record);
      } else
        std::cerr << "field not found\n";
      payloadStatus = changed;
      continue;
    } else if (cmd == "getpos") {
      int position;
      std::cin >> position;
      auto fieldname = dacc->getDescriptor().fieldName(position);
      std::any value = payloadAcc->getItem(position);
      if (value.type() == typeid(std::string)) {
        std::cout << std::any_cast<std::string>(value) << std::endl;
      }
      if (value.type() == typeid(int)) {
        std::cout << std::any_cast<int>(value) << std::endl;
      }
      if (value.type() == typeid(unsigned char)) {
        std::cout << std::any_cast<unsigned char>(value) << std::endl;
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
    } else if (cmd == "print") {
      std::cout << *payloadAcc << std::endl;
      continue;
    } else if (cmd == "write") {
      size_t record;
      std::cin >> record;
      if (record >= dacc->getRecordsCount()) {
        std::cout << RED "record out of range - Check append command.\n" RESET;
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
      payloadAcc->setHex(true);
    } else if (cmd == "dec") {
      payloadAcc->setHex(false);
    } else if (cmd == "dump") {
      auto* ptr = reinterpret_cast<unsigned char*>(payloadAcc->get());
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
      std::cout << RED "?\n" RESET;
      continue;
    }
    std::cout << "ok\n";
  } while (true);
  // use '$xxd datafile-11' to check
  return 0;
}
