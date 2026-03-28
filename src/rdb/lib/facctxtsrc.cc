#include "rdb/facctxtsrc.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_unique

namespace rdb {

template <typename K>
K readFromFstream(std::fstream &myFile, bool loopToBeginningIfEOF = true) {
  K var{0};
  if (myFile.eof()) {
    if (loopToBeginningIfEOF) {
      myFile.clear();
      myFile.seekg(0, std::ios::beg);
      if (!(myFile >> var)) return K{0};
    }
  } else {
    myFile >> var;
    if (myFile.eof() && loopToBeginningIfEOF) {
      myFile.clear();
      myFile.seekg(0, std::ios::beg);
      if (!(myFile >> var)) return K{0};
    }
  }
  return var;
}

textSourceRO::textSourceRO(const std::string_view fileName,    //
                                           const ssize_t recordSize,           //
                                           const rdb::Descriptor &descriptor,  //
                                           bool loopToBeginningIfEOF)
    : filename_(std::string(fileName)),
      descriptor_(descriptor),
      recordSize_(recordSize),
      readCount_(0),
      loopToBeginningIfEOF_(loopToBeginningIfEOF) {
  myFile_.rdbuf()->pubsetbuf(nullptr, 0);
  myFile_.open(filename_, std::ios::in);
  assert((myFile_.rdstate() & std::ifstream::failbit) == 0);

  payload_ = std::make_unique<rdb::payload>(descriptor_);
}

textSourceRO::~textSourceRO() { myFile_.close(); }

auto textSourceRO::name() -> std::string & { return filename_; }

ssize_t textSourceRO::read(uint8_t *ptrData, const size_t position) {
  assert(position == 0);
  if (position != 0) return EXIT_FAILURE;

  assert(recordSize_ != 0);
  if (recordSize_ == 0) return EXIT_FAILURE;

  if (!myFile_.is_open()) return EXIT_FAILURE;

  if (!loopToBeginningIfEOF_) {
    if (myFile_.eof()) {
      std::memset(ptrData, 0, descriptor_.getSizeInBytes());
      readCount_++;

      return EXIT_SUCCESS;
    }
  }

  if (myFile_.fail()) return EXIT_FAILURE;

  auto i = 0;
  for (auto item : descriptor_) {
    if (item.rlen != 0) {
      if (item.rtype == rdb::STRING) {
        std::string var;
        auto strLen = item.rlen * item.rarray;
        char c;
        bool found = false;
        while (myFile_.get(c) && c != '"')
          ;
        if (myFile_.eof() && loopToBeginningIfEOF_) {
          myFile_.clear();
          myFile_.seekg(0, std::ios::beg);
          while (myFile_.get(c) && c != '"')
            ;
        }
        if (!myFile_.eof()) {
          while (myFile_.get(c) && c != '"')
            var += c;
          found = true;
        }
        if (!found && loopToBeginningIfEOF_) {
          myFile_.clear();
          myFile_.seekg(0, std::ios::beg);
          while (myFile_.get(c) && c != '"')
            ;
          while (myFile_.get(c) && c != '"')
            var += c;
        }
        var.erase(remove(var.begin(), var.end(), '"'), var.end());
        var.resize(strLen);
        payload_->setItem(i, var);
      } else
        for (auto j = 0; j < item.rarray; j++) {
          if (item.rtype == rdb::INTEGER) {
            auto var = readFromFstream<int>(myFile_, loopToBeginningIfEOF_);
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::UINT) {
            auto var = readFromFstream<unsigned>(myFile_, loopToBeginningIfEOF_);
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::FLOAT) {
            auto var = readFromFstream<float>(myFile_, loopToBeginningIfEOF_);
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::DOUBLE) {
            auto var = readFromFstream<double>(myFile_, loopToBeginningIfEOF_);
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::BYTE) {  // This is different!
            auto var = readFromFstream<unsigned>(myFile_, loopToBeginningIfEOF_);
            payload_->setItem(i + j, static_cast<uint8_t>(var));
          } else {
            SPDLOG_ERROR("Unsupported type in text data source: {}", static_cast<int>(item.rtype));
            assert(false && "read - Unsupported type in text data source");
            abort();
          }
        }

      // rdb::RATIONAL - deprecate ?
      i++;
    }
  }

  std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());

  readCount_++;

  return EXIT_SUCCESS;
}

size_t textSourceRO::count() { return readCount_; }

}  // namespace rdb
