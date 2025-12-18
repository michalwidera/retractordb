#include "rdb/facctxtsrc.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_uniqe
#include <ratio>

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
                                           const size_t sizeRec,               //
                                           const rdb::Descriptor &descriptor,  //
                                           bool loopToBeginningIfEOF)
    : filename(std::string(fileName)),
      descriptor(descriptor),
      sizeRec(sizeRec),
      readCount(0),
      loopToBeginningIfEOF_(loopToBeginningIfEOF) {
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(filename, std::ios::in);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  payload = std::make_unique<rdb::payload>(descriptor);
}

textSourceAccessorRO::~textSourceAccessorRO() { myFile.close(); }

auto textSourceAccessorRO::name() -> std::string & { return filename; }

ssize_t textSourceAccessorRO::read(uint8_t *ptrData, const size_t position) {
  assert(position == 0);
  assert(sizeRec != 0);

  if (!loopToBeginningIfEOF_) {
    if (myFile.eof()) {
      std::memset(reinterpret_cast<char *>(ptrData), 0, descriptor.getSizeInBytes());
      readCount++;

      return EXIT_SUCCESS;
    }

    // all assertions releated to failbit are after read operations
    // on the end of file should not block here
  }

  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  auto i = 0;
  for (auto item : descriptor) {
    if (item.rlen != 0) {
      if (item.rtype == rdb::STRING) {
        std::string var;
        auto strLen = item.rlen * item.rarray;
        char c;
        while (myFile.get(c) && c != '"');
        while (myFile.get(c) && c != '"') var += c;
        var.erase(remove(var.begin(), var.end(), '"'), var.end());
        var.resize(strLen);
        // SPDLOG_INFO("test nr:{} val:{}", i, var.c_str());
        payload->setItem(i, var);
      } else
        for (auto j = 0; j < item.rarray; j++) {
          if (item.rtype == rdb::INTEGER) {
            auto var = readFromFstream<int>(myFile, loopToBeginningIfEOF_);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::UINT) {
            auto var = readFromFstream<unsigned>(myFile, loopToBeginningIfEOF_);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::FLOAT) {
            auto var = readFromFstream<float>(myFile, loopToBeginningIfEOF_);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::DOUBLE) {
            auto var = readFromFstream<double>(myFile, loopToBeginningIfEOF_);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::BYTE) {  // This is different!
            auto var = readFromFstream<unsigned>(myFile, loopToBeginningIfEOF_);
            payload->setItem(i + j, static_cast<uint8_t>(var));
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

  std::memcpy(reinterpret_cast<char *>(ptrData), payload->get(), descriptor.getSizeInBytes());

  if (loopToBeginningIfEOF_) assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  readCount++;

  return EXIT_SUCCESS;
}

size_t textSourceAccessorRO::count() { return readCount; }

}  // namespace rdb