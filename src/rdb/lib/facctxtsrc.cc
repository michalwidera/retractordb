#include "rdb/facctxtsrc.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_unique
#include <optional>
#include <sstream>

namespace rdb {

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

template <typename T>
T parseAs(const std::string &token) {
  T var{0};
  std::istringstream(token) >> var;
  return var;
}

void parseAndSetNumericItem(rdb::payload &payload, int index, rdb::descFld rtype, const std::string &token) {
  switch (rtype) {
    case rdb::INTEGER:
      payload.setItem(index, parseAs<int>(token));
      break;
    case rdb::UINT:
      payload.setItem(index, parseAs<unsigned>(token));
      break;
    case rdb::FLOAT:
      payload.setItem(index, parseAs<float>(token));
      break;
    case rdb::DOUBLE:
      payload.setItem(index, parseAs<double>(token));
      break;
    case rdb::BYTE:
      payload.setItem(index, static_cast<uint8_t>(parseAs<unsigned>(token)));
      break;
    default:
      SPDLOG_ERROR("Unsupported type in text data source: {}", static_cast<int>(rtype));
      assert(false && "read - Unsupported type in text data source");
      abort();
  }
}

textSourceRO::textSourceRO(const std::string_view fileName,    //
                           const rdb::Descriptor &descriptor,  //
                           bool loopToBeginningIfEOF)
    : filename_(std::string(fileName)),
      descriptor_(descriptor),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
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

ssize_t textSourceRO::read(uint8_t *ptrData, const size_t position, std::vector<bool> &nullBitset) {
  auto markAllNullAndZero = [&](ssize_t status) {
    payload_->setNullBitset(std::vector<bool>(descriptor_.size(), true));
    std::fill(payload_->span().begin(), payload_->span().end(), 0);
    if (ptrData != nullptr) {
      std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());
    }
    nullBitset = payload_->getNullBitset();
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
      } else {
        for (auto j = 0; j < item.rarray; j++) {
          auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
          if (!token.has_value() || isNullToken(*token)) {
            payload_->setItem(i + j, std::nullopt);
            continue;
          }
          parseAndSetNumericItem(*payload_, i + j, item.rtype, *token);
        }
      }

      // rdb::RATIONAL - deprecate ?
      i++;
    }
  }

  std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());

  nullBitset = payload_->getNullBitset();
  readCount_++;

  return EXIT_SUCCESS;
}

size_t textSourceRO::count() { return readCount_; }

const std::vector<bool> &textSourceRO::lastNullBitset() const { return payload_->getNullBitset(); }

}  // namespace rdb
