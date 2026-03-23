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
      myFile >> var;
    }
  } else {
    assert((myFile.rdstate() & std::ifstream::failbit) == 0);
    myFile >> var;
    if (myFile.eof() && loopToBeginningIfEOF) {
      myFile.clear();
      myFile.seekg(0, std::ios::beg);
      myFile >> var;
    }
  }
  return var;
}

textSourceAccessorRO::textSourceAccessorRO(const std::string_view fileName,    //
                                           const ssize_t recordSize,              //
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

textSourceAccessorRO::~textSourceAccessorRO() { myFile_.close(); }

auto textSourceAccessorRO::name() -> std::string & { return filename_; }

ssize_t textSourceAccessorRO::read(uint8_t *ptrData, const size_t position) {
  assert(position == 0);
  assert(recordSize_ != 0);

  if (!loopToBeginningIfEOF_) {
    if (myFile_.eof()) {
      std::memset(ptrData, 0, descriptor_.getSizeInBytes());
      readCount_++;

      return EXIT_SUCCESS;
    }

    // all assertions releated to failbit are after read operations
    // on the end of file should not block here
  }

  assert((myFile_.rdstate() & std::ifstream::failbit) == 0);

  auto i = 0;
  for (auto item : descriptor_) {
    if (item.rlen != 0) {
      if (item.rtype == rdb::STRING) {
        std::string var;
        auto strLen = item.rlen * item.rarray;
        char c;
        while (myFile_.get(c) && c != '"')
          ;
        while (myFile_.get(c) && c != '"')
          var += c;
        var.erase(remove(var.begin(), var.end(), '"'), var.end());
        var.resize(strLen);
        // SPDLOG_INFO("test nr:{} val:{}", i, var.c_str());
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

  if (loopToBeginningIfEOF_) assert((myFile_.rdstate() & std::ifstream::failbit) == 0);

  readCount_++;

  return EXIT_SUCCESS;
}

size_t textSourceAccessorRO::count() { return readCount_; }

}  // namespace rdb
