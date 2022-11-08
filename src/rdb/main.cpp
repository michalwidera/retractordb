#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "rdb/desc.h"
#include "rdb/dsacc.h"
#include "rdb/faccfs.h"
#include "rdb/faccposix.h"
#include "rdb/payloadacc.h"

#define GREEN "\x1B[32m"
#define RED "\x1B[31m"
#define ORANGE "\x1B[33m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[93m"
#define RESET "\033[0m"

enum payloadStatusType { fetched, clean, stored, changed };

payloadStatusType payloadStatus(clean);

int main(int argc, char* argv[]) {
  std::unique_ptr<rdb::DataStorageAccessor<std::byte>> uPtr_dacc;
  std::unique_ptr<std::byte[]> uPtr_payload;
  std::string file;
  bool reverse = false;
  bool rox = true;
  bool hexFormat = false;
  std::string cmd;
  do {
    std::cout << ".";
    std::cin >> cmd;
    if (cmd == "exit" || cmd == "quit" || cmd == "q") break;
    if (cmd == "quitdrop" || cmd == "qd") {
      if (uPtr_dacc) uPtr_dacc->setRemoveOnExit(true);
      break;
    }
    if (cmd == "open" || cmd == "ropen") {
      std::cin >> file;
      if (!std::filesystem::exists(file)) {
        std::cout << RED "File does not exist\n" RESET;
        continue;
      }
      uPtr_dacc.reset(new rdb::DataStorageAccessor(file, cmd == "ropen"));
      if (uPtr_dacc->getDescriptor().GetSize() == 0) {
        std::cout << RED "File exist, description file missing (.desc)\n" RESET;
        continue;
      }
      uPtr_payload.reset(new std::byte[uPtr_dacc->getDescriptor().GetSize()]);
      memset(uPtr_payload.get(), 0, uPtr_dacc->getDescriptor().GetSize());
      payloadStatus = clean;
      uPtr_dacc->setRemoveOnExit(false);
    } else if (cmd == "create" || cmd == "rcreate") {
      std::cin >> file;
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
      uPtr_dacc.reset(
          new rdb::DataStorageAccessor(desc, file, cmd == "rcreate"));
      uPtr_payload.reset(new std::byte[uPtr_dacc->getDescriptor().GetSize()]);
      memset(uPtr_payload.get(), 0, uPtr_dacc->getDescriptor().GetSize());
      payloadStatus = clean;
      uPtr_dacc->setRemoveOnExit(false);
    } else if (cmd == "help" || cmd == "h") {
      std::cout << GREEN;
      std::cout << "exit|quit|q \t\t\t exit\n";
      std::cout << "quitdrop|qd \t\t\t exit & drop artifacts\n";
      std::cout << "open|ropen [file] \t\t open database - create connection "
                   "(r-reverse iterator)\n";
      std::cout << "create|rcreate [file][schema] \t create database with "
                   "schema (r-reverse iterator)\n";
      std::cout << "\t\t\t\t example: .create test_db { INTEGER dane STRING "
                   "name[3] } \n";
      std::cout << "desc \t\t\t\t show schema\n";
      std::cout << "read [n] \t\t\t read record from database into payload\n";
      std::cout << "write [n] \t\t\t send record to database from payload\n";
      std::cout << "append \t\t\t\t append payload to database\n";
      std::cout << "set [field][value] \t\t set payload field value\n";
      std::cout << "setpos [position][number value] \t\t set payload field "
                   "number value\n";
      std::cout << "status \t\t\t\t show status of payload\n";
      std::cout << "flip \t\t\t\t flip reverse iterator\n";
      std::cout << "rox \t\t\t\t remove on exit flip\n";
      std::cout << "print \t\t\t\t show payload\n";
      std::cout
          << "hex|dec \t\t\t type of input/output of byte/number fields\n";
      std::cout << "size \t\t\t\t show database size in records\n";
      std::cout << "dump \t\t\t\t show payload memory\n";
      std::cout << RESET;
    } else if (!uPtr_dacc) {
      std::cout << RED "unconnected\n" RESET;
      continue;
    } else if (cmd == "desc") {
      std::cout << YELLOW << uPtr_dacc->getDescriptor() << RESET << std::endl;
      continue;
    } else if (cmd == "read") {
      int record;
      std::cin >> record;
      if (record >= uPtr_dacc->getRecordsCount()) {
        std::cout << RED "record out of range\n" RESET;
        continue;
      }
      uPtr_dacc->Get(uPtr_payload.get(), record);
      payloadStatus = fetched;
    } else if (cmd == "set") {
      rdb::payLoadAccessor payload(uPtr_dacc->getDescriptor(),
                                   uPtr_payload.get(), hexFormat);
      std::cin >> payload;
      payloadStatus = changed;
      continue;
    } else if (cmd == "setpos") {
      rdb::payLoadAccessor payload(uPtr_dacc->getDescriptor(),
                                   uPtr_payload.get(), hexFormat);
      int position;
      std::cin >> position;
      auto fieldname = uPtr_dacc->getDescriptor().FieldName(position);
      if (uPtr_dacc->getDescriptor().Type(fieldname) == "INTEGER") {
        int value;
        std::cin >> value;
        payload.setPayloadField(position, value);
      } else if (uPtr_dacc->getDescriptor().Type(fieldname) == "DOUBLE") {
        double value;
        std::cin >> value;
        payload.setPayloadField(position, value);
      } else if (uPtr_dacc->getDescriptor().Type(fieldname) == "STRING") {
        std::string record;
        std::cin >> record;
        payload.setPayloadField(position, record);
      } else
        std::cerr << "field not found\n";
      payloadStatus = changed;
      continue;
    } else if (cmd == "getpos") {
      rdb::payLoadAccessor payload(uPtr_dacc->getDescriptor(),
                                   uPtr_payload.get(), hexFormat);
      int position;
      std::cin >> position;
      auto fieldname = uPtr_dacc->getDescriptor().FieldName(position);
      std::any value = payload.getPayloadField(position);
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
      uPtr_dacc->setReverse(reverse);
    } else if (cmd == "rox") {
      rox = !rox;
      uPtr_dacc->setRemoveOnExit(rox);
    } else if (cmd == "print") {
      rdb::payLoadAccessor payload(uPtr_dacc->getDescriptor(),
                                   uPtr_payload.get(), hexFormat);
      std::cout << payload << std::endl;
      continue;
    } else if (cmd == "write") {
      size_t record;
      std::cin >> record;
      if (record >= uPtr_dacc->getRecordsCount()) {
        std::cout << RED "record out of range - Check append command.\n" RESET;
        continue;
      }
      uPtr_dacc->Put(uPtr_payload.get(), record);
      payloadStatus = stored;
    } else if (cmd == "append") {
      uPtr_dacc->Put(uPtr_payload.get());
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
      std::cout << uPtr_dacc->getRecordsCount() << " Record(s)\n";
      std::cout << uPtr_dacc->getDescriptor().GetSize()
                << " Byte(s) per record.\n";
      continue;
    } else if (cmd == "hex")
      hexFormat = true;
    else if (cmd == "dec")
      hexFormat = false;
    else if (cmd == "dump") {
      auto* ptr = reinterpret_cast<unsigned char*>(uPtr_payload.get());
      for (auto i = 0; i < uPtr_dacc->getDescriptor().GetSize(); i++) {
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
