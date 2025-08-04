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
K readFromFstream(std::fstream &myFile) {
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
textSourceAccessor<T>::textSourceAccessor(const std::string_view fileName,  //
                                          const size_t sizeRec,             //
                                          const rdb::Descriptor &descriptor)
    : filename(std::string(fileName)), descriptor(descriptor), sizeRec(sizeRec), readCount(0) {
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(filename, std::ios::in);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  payload = std::make_unique<rdb::payload>(descriptor);
}

template <typename T>
textSourceAccessor<T>::~textSourceAccessor() {
  myFile.close();
}

template <class T>
auto textSourceAccessor<T>::name() const -> const std::string & {
  return filename;
}

template <class T>
auto textSourceAccessor<T>::name() -> std::string & {
  return filename;
}

template <typename T>
ssize_t textSourceAccessor<T>::write(const T *ptrData, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <typename T>
ssize_t textSourceAccessor<T>::read(T *ptrData, const size_t position) {
  assert(position == 0);
  assert(sizeRec != 0);

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
            auto var = readFromFstream<int>(myFile);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::UINT) {
            auto var = readFromFstream<unsigned>(myFile);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::FLOAT) {
            auto var = readFromFstream<float>(myFile);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::DOUBLE) {
            auto var = readFromFstream<double>(myFile);
            payload->setItem(i + j, var);
          } else if (item.rtype == rdb::BYTE) {  // This is different!
            auto var = readFromFstream<unsigned>(myFile);
            payload->setItem(i + j, static_cast<uint8_t>(var));
          } else {
            SPDLOG_ERROR("Unsupported type in text data source: {}", item.rtype);
            assert(false && "read - Unsupported type in text data source");
            abort();
          }
        }

      // rdb::RATIONAL - deprecate ?
      i++;
    }
  }
  std::memcpy(reinterpret_cast<char *>(ptrData), payload->get(), descriptor.getSizeInBytes());
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  readCount++;

  return EXIT_SUCCESS;
}

template <class T>
size_t textSourceAccessor<T>::count() {
  return readCount;
}

template class textSourceAccessor<uint8_t>;

}  // namespace rdb