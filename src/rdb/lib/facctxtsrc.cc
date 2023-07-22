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
constexpr const int kOpenBaseFlags = O_CLOEXEC;

template <typename K>
K readFromFstream(std::fstream& myFile) {
  K var;
  myFile >> var;
  if (myFile.eof()) {
    myFile.clear();
    myFile.seekg(0, std::ios::beg);
    myFile >> var;
  };
  return var;
}

template <typename T>
textSourceAccessor<T>::textSourceAccessor(std::string fileName) : fileNameStr(fileName) {
  myFile.rdbuf()->pubsetbuf(0, 0);
  myFile.open(fileName, std::ios::in);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
}

template <typename T>
textSourceAccessor<T>::~textSourceAccessor() {
  myFile.close();
}

template <class T>
std::string textSourceAccessor<T>::fileName() {
  return fileNameStr;
}

template <typename T>
ssize_t textSourceAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <typename T>
ssize_t textSourceAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  assert(position == 0);

  // myFile.seekg(position);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  // This (+1) look's like error (need investigate)
  // During test posix interface have properly read size data
  // but fstream get read instead 12 bytes only 11
  // Therefore +1 appears.
  // Last byte was omitted.
  // Look's like some inconsistency is here.

  auto i = 0;
  for (auto item : descriptor) {
    if (std::get<rlen>(item) != 0) {
      if (std::get<rtype>(item) == rdb::STRING) {
        std::string var;
        auto strLen = std::get<rlen>(item) * std::get<rarray>(item);
        char c;
        while (myFile.get(c) && c != '"')
          ;
        while (myFile.get(c) && c != '"') var += c;
        var.erase(remove(var.begin(), var.end(), '"'), var.end());
        var.resize(strLen);
        SPDLOG_INFO("test nr:{} val:{}", i, var.c_str());
        payload->setItem(i, var);
      } else
        for (auto j = 0; j < std::get<rarray>(item); j++) {
          if (std::get<rtype>(item) == rdb::INTEGER) {
            auto var = readFromFstream<int>(myFile);
            payload->setItem(i + j, var);
          } else if (std::get<rtype>(item) == rdb::UINT) {
            auto var = readFromFstream<unsigned>(myFile);
            payload->setItem(i + j, var);
          } else if (std::get<rtype>(item) == rdb::FLOAT) {
            auto var = readFromFstream<float>(myFile);
            payload->setItem(i + j, var);
          } else if (std::get<rtype>(item) == rdb::DOUBLE) {
            auto var = readFromFstream<double>(myFile);
            payload->setItem(i + j, var);
          } else if (std::get<rtype>(item) == rdb::BYTE) {  // This is different!
            auto var = readFromFstream<unsigned>(myFile);
            payload->setItem(i + j, static_cast<uint8_t>(var));
          } else {
            SPDLOG_ERROR("Unsupported type in text data source: {}", std::get<rtype>(item));
            abort();
          }
        }

      // rdb::RATIONAL - deprecate ?
      i++;
    }
  }
  std::memcpy(reinterpret_cast<char*>(ptrData), payload->get(), descriptor.getSizeInBytes());
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  return EXIT_SUCCESS;
}

template <typename T>
ssize_t textSourceAccessor<T>::fctrl(void* ptrData, const size_t size) {
  descriptor = *(reinterpret_cast<rdb::Descriptor*>(ptrData));
  payload = std::make_unique<rdb::payload>(descriptor);
  return EXIT_SUCCESS;
}

template class textSourceAccessor<uint8_t>;
template class textSourceAccessor<char>;

}  // namespace rdb