#include "rdb/facctxtsrc.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_unique
#include <optional>
#include <sstream>

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

std::optional<std::string> readTokenFromFstream(std::fstream &myFile, bool loopToBeginningIfEOF = true) {
  std::string token;

  auto readToken = [&]() -> bool { return static_cast<bool>(myFile >> token); };

  if (readToken()) return token;

  if (myFile.eof() && loopToBeginningIfEOF) {
    myFile.clear();
    myFile.seekg(0, std::ios::beg);
    if (readToken()) return token;
  }

  return std::nullopt;
}

bool isNullToken(const std::string &token) { return token == "NULL" || token == "Null" || token == "null"; }

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
  if ((myFile_.rdstate() & std::ifstream::failbit) != 0) {
    SPDLOG_WARN("Unable to open text source file: {}", filename_);
    myFile_.clear();
  }

  payload_ = std::make_unique<rdb::payload>(descriptor_);
}

textSourceRO::~textSourceRO() { myFile_.close(); }

auto textSourceRO::name() -> std::string & { return filename_; }

ssize_t textSourceRO::read(uint8_t *ptrData, const size_t position) {
  auto markAllNullAndZero = [&](ssize_t status) {
    payload_->setNullBitset(std::vector<bool>(descriptor_.size(), true));
    std::fill(payload_->span().begin(), payload_->span().end(), 0);
    if (ptrData != nullptr) {
      std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());
    }
    readCount_++;
    return status;
  };

  assert(position == 0);
  if (position != 0) return markAllNullAndZero(EXIT_FAILURE);

  assert(recordSize_ != 0);
  if (recordSize_ == 0) return markAllNullAndZero(EXIT_FAILURE);

  if (!myFile_.is_open()) return markAllNullAndZero(EXIT_FAILURE);

  if (!loopToBeginningIfEOF_) {
    if (myFile_.eof()) {
      return markAllNullAndZero(EXIT_SUCCESS);
    }
  }

  if (myFile_.fail()) return markAllNullAndZero(EXIT_FAILURE);

  auto i = 0;
  for (auto item : descriptor_) {
    if (item.rtype == rdb::NULLTYPE) {
      auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
      if (token.has_value() && !isNullToken(*token)) {
        SPDLOG_ERROR("Expected NULL token for NULL field, got: {}", *token);
        assert(false && "read - Expected NULL token for NULL field");
        abort();
      }
      payload_->setItem(i, std::nullopt);
      i++;
      continue;
    }

    if (item.rlen != 0) {
      if (item.rtype == rdb::STRING) {
        myFile_ >> std::ws;
        if (myFile_.peek() != '"') {
          auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
          if (!token.has_value() || isNullToken(*token)) {
            payload_->setItem(i, std::nullopt);
            i++;
            continue;
          }
        }

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
          auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
          if (!token.has_value()) {
            payload_->setItem(i + j, std::nullopt);
            continue;
          }
          if (isNullToken(*token)) {
            payload_->setItem(i + j, std::nullopt);
            continue;
          }

          std::istringstream tokenStream(*token);
          if (item.rtype == rdb::INTEGER) {
            int var{0};
            tokenStream >> var;
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::UINT) {
            unsigned var{0};
            tokenStream >> var;
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::FLOAT) {
            float var{0};
            tokenStream >> var;
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::DOUBLE) {
            double var{0};
            tokenStream >> var;
            payload_->setItem(i + j, var);
          } else if (item.rtype == rdb::BYTE) {  // This is different!
            unsigned var{0};
            tokenStream >> var;
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

const std::vector<bool> &textSourceRO::lastNullBitset() const { return payload_->getNullBitset(); }

}  // namespace rdb
