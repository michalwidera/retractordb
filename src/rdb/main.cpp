#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>  // make_unique
#include <string>

#include "config.h"
#include "rdb/descriptor.hpp"
#include "rdb/faccfs.hpp"
#include "rdb/faccposix.hpp"
#include "rdb/metaDataStream.hpp"
#include "rdb/payload.hpp"
#include "rdb/storageacc.hpp"
#include "uxSysTermTools.hpp"

using namespace boost;

/*
 * This code creates xtrdb executable file.
 */

enum payloadStatusType { fetched, clean, stored, changed, error };

int main(int argc, char *argv[]) {
  payloadStatusType payloadStatus{clean};

  std::string GREEN  = "\x1B[32m";
  std::string RED    = "\x1B[31m";
  std::string ORANGE = "\x1B[33m";
  std::string BLUE   = "\x1B[34m";
  std::string YELLOW = "\x1B[93m";
  std::string RESET  = "\033[0m";
  std::string BLINK  = "\x1b[5m";

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

  if (cliNoPrompt) {
    std::string empty = "";
    GREEN             = empty;
    RED               = empty;
    ORANGE            = empty;
    BLUE              = empty;
    YELLOW            = empty;
    RESET             = empty;
    BLINK             = empty;
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
      std::cout << BLINK << wasteComment << std::endl << RESET;
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
      std::cin >> file;
      if (file.find('{') != std::string::npos) {
        std::cout << RED << "unrecognized or missing file:" << file << "\n" << RESET;
        continue;
      }
      auto oldPos = file.find(".old");
      if (oldPos != std::string::npos) {
        dacc = std::make_unique<rdb::storage>(file.substr(0, oldPos), file, storageParam, storagePolicy);
      } else
        dacc = std::make_unique<rdb::storage>(file, file, storageParam, storagePolicy);
      assert(dacc != nullptr);
      if (dacc->descriptorFileExist()) {
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
      payloadStatus = clean;
      dacc->setDisposable(false);
      if (dacc->isDeclared()) dacc->setCapacity(1);
    } else if (cmd == "help" || cmd == "h") {
      std::cout << GREEN;
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
      // std::cout << "Usage: " << argv[0] << " [option]" << std::endl << std::endl;
      // std::cout << desc;
      std::cout << config_line << std::endl;
      std::cout << "Log: " << filelog << std::endl;
      std::cout << warranty << std::endl;

      std::cout << RESET;
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
      std::cout << RED << "unconnected\n" << RESET;
      continue;
    } else if (cmd == "desc") {
      std::cout << YELLOW << dacc->descriptor << RESET << std::endl;
      continue;
    } else if (cmd == "descc") {
      std::cout << YELLOW << rdb::flat << dacc->descriptor << RESET << std::endl;
      continue;
    } else if (cmd == "read" || cmd == "rread") {
      int record;
      std::cin >> record;
      if (!dacc->isDeclared() && record >= dacc->getRecordsCount()) {
        std::cout << RED << "record out of range - read command\n" << RESET;
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
        std::cout << RED << "system command error: " << returnCode << "\n" << RESET;
      }
    } else if (cmd == "set") {
      std::cin >> *(dacc->getPayload());
      payloadStatus = changed;
      continue;
    } else if (cmd == "setpos") {
      int position{0};
      std::cin >> position;
      auto fieldName = dacc->descriptor[position].rname;
      if (dacc->descriptor.type(fieldName) == "INTEGER") {
        int value{0};
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->descriptor.type(fieldName) == "DOUBLE") {
        double value{0};
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->descriptor.type(fieldName) == "BYTE") {
        uint8_t value{0};
        std::cin >> value;
        dacc->getPayload()->setItem(position, value);
      } else if (dacc->descriptor.type(fieldName) == "STRING") {
        std::string record{""};
        std::cin >> record;
        dacc->getPayload()->setItem(position, record);
      } else
        std::cerr << "field not found\n";
      payloadStatus = changed;
      continue;
    } else if (cmd == "getpos") {
      int position;
      std::cin >> position;
      auto fieldName = dacc->descriptor[position].rname;
      auto valueOpt  = dacc->getPayload()->getItem(position);
      if (!valueOpt.has_value()) {
        std::cout << fieldName << ": null" << std::endl;
        continue;
      }
      std::any value = valueOpt.value();
      if (value.type() == typeid(std::monostate)) {
        std::cout << "null" << std::endl;
      }
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
    } else if (cmd == "cap") {
      int backCapacityValue;
      std::cin >> backCapacityValue;
      dacc->setCapacity(backCapacityValue);
    } else if (cmd == "rox") {
      rox = !rox;
      dacc->setDisposable(rox);
    } else if (cmd == "print") {
      std::cout << ORANGE << *(dacc->getPayload()) << RESET;
      continue;
    } else if (cmd == "printt") {
      std::cout << ORANGE << rdb::flat << *(dacc->getPayload()) << RESET << std::endl;
      continue;
    } else if (cmd == "list" || cmd == "rlist") {
      int record{0};
      std::cin >> record;
      for (auto i = 0; i < record; i++) {
        if (i >= dacc->getRecordsCount()) {
          std::cout << RED << "record out of range - list command\n" << RESET;
          continue;
        }
        auto returnStatus = (cmd == "rlist") ? dacc->revRead(i) : dacc->read(i);
        payloadStatus     = returnStatus ? fetched : error;

        if (payloadStatus == error) {
          std::cout << RED << "fetch error\n" << RESET;
          continue;
        }
        std::cout << ORANGE << rdb::flat << *(dacc->getPayload()) << RESET << std::endl;
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
      const std::string metaFilePath =
          (storageParam.empty() ? std::filesystem::path(file) : std::filesystem::path(storageParam) / file).string() +
          ".meta";
      if (!std::filesystem::exists(metaFilePath)) {
        std::cout << RED << "meta file not found: " << metaFilePath << "\n" << RESET;
        continue;
      }
      rdb::metaDataStream metaView(dacc->descriptor, metaFilePath);
      auto committed       = metaView.entries();
      const auto &cur      = metaView.pendingEntry();
      const size_t nFields = dacc->descriptor.size();

      std::cout << BLUE << "meta: " << metaFilePath << "\n";
      std::cout << "interval: " << metaView.getSamplingInterval() << "  ";
      std::cout << "total records: " << metaView.totalRecords() << "\n" << RESET;

      size_t segIdx = 0;
      for (const auto &entry : committed) {
        std::cout << YELLOW << "[" << segIdx++ << "] ";
        if (entry.isGap) {
          std::cout << "gap\n";
        } else {
          std::cout << "[";
          for (size_t fi = 0; fi < nFields; ++fi) {
            if (fi > 0) std::cout << " ";
            std::cout << dacc->descriptor[fi].rname << ":";
            std::cout << (fi < entry.nullBitset.size() && entry.nullBitset[fi] ? "null" : "ok");
          }
          std::cout << "] runs:" << entry.recordCount << "\n";
        }
        std::cout << RESET;
      }
      if (cur.recordCount > 0) {
        std::cout << YELLOW << "[cur] [";
        for (size_t fi = 0; fi < nFields; ++fi) {
          if (fi > 0) std::cout << " ";
          std::cout << dacc->descriptor[fi].rname << ":";
          std::cout << (fi < cur.nullBitset.size() && cur.nullBitset[fi] ? "null" : "ok");
        }
        std::cout << "] runs:" << cur.recordCount << "\n" << RESET;
      }
      std::cout << BLUE << committed.size() + (cur.recordCount > 0 ? 1 : 0) << " segment(s)\n" << RESET;
      continue;
    } else if (cmd == "metaraw") {
      const std::string metaFilePath =
          (storageParam.empty() ? std::filesystem::path(file) : std::filesystem::path(storageParam) / file).string() +
          ".meta";
      if (!std::filesystem::exists(metaFilePath)) {
        std::cout << RED << "meta file not found: " << metaFilePath << "\n" << RESET;
        continue;
      }

      constexpr size_t headerSize = sizeof(int64_t) + sizeof(int32_t) + sizeof(int32_t);
      const size_t bitsetBytes    = (dacc->descriptor.size() + 7) / 8;
      const size_t entrySize      = sizeof(size_t) + sizeof(uint8_t) + sizeof(size_t) + bitsetBytes;

      std::ifstream in(metaFilePath, std::ios::binary);
      if (!in.is_open()) {
        std::cout << RED << "cannot open meta file: " << metaFilePath << "\n" << RESET;
        continue;
      }

      int64_t creationTimeNs = 0;
      int32_t rNum = 0, rDen = 1;
      in.read(reinterpret_cast<char *>(&creationTimeNs), sizeof(creationTimeNs));
      in.read(reinterpret_cast<char *>(&rNum), sizeof(rNum));
      in.read(reinterpret_cast<char *>(&rDen), sizeof(rDen));

      std::cout << BLUE << "meta raw: " << metaFilePath << "\n" << RESET;
      std::cout << "header size: " << headerSize << "\n";
      std::cout << "entry size: " << entrySize << "\n";
      std::cout << "sampling interval: " << rNum << "/" << rDen << "\n";

      size_t entryIdx = 0;
      while (true) {
        std::vector<std::byte> raw(entrySize);
        in.read(reinterpret_cast<char *>(raw.data()), static_cast<std::streamsize>(entrySize));
        const auto bytesRead = in.gcount();
        if (bytesRead == 0) break;
        if (static_cast<size_t>(bytesRead) != entrySize) {
          std::cout << RED << "truncated entry at index: " << entryIdx << "\n" << RESET;
          break;
        }

        auto rec = rdb::metaDataStream::IndexRecord::deserialize(std::span<const std::byte>(raw));
        std::cout << "entry[" << entryIdx << "] ";
        std::cout << "count:" << rec.recordCount << " ";
        std::cout << "flags:" << (rec.isGap ? 1 : 0) << " ";
        std::cout << "bitsetSize:" << rec.nullBitset.size() << " ";
        std::cout << "bitsetHex:";
        for (size_t bi = 0; bi < bitsetBytes; ++bi) {
          uint8_t byteVal = 0;
          for (size_t bit = 0; bit < 8; ++bit) {
            const size_t fieldIdx = bi * 8 + bit;
            if (fieldIdx < rec.nullBitset.size() && rec.nullBitset[fieldIdx]) byteVal |= static_cast<uint8_t>(1u << bit);
          }
          std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byteVal) << std::dec;
        }
        std::cout << "\n";
        ++entryIdx;
      }
      std::cout << "entries: " << entryIdx << "\n";
      continue;
    } else {
      std::cout << RED << "?\n" << RESET;
      continue;
    }
    std::cout << ok;
  } while (true);
  dacc.reset();
  spdlog::shutdown();
  return 0;
}
